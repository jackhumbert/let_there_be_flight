// The plugin's acoustics through FMOD itself, without the game (docs/design/acoustics.md, step 3):
// loads the mod's banks, attaches the DSPs with Acoustics::Attach as FlightAudio does, plays a run's
// recording into bus:/Engine's channel group (where the thruster events mix) and, as it plays in real
// time, feeds the car from the run's telemetry with Acoustics::SetCar / SetListener. FMOD's real-time
// WAV writer records the mix. Between --off a,b (s) the echoes are switched off, to hear and check the
// fade and the restart. bus:/Engine has a limiter (-12 dB) after the DSP: a recording mixed loud
// drives it, so --volume (0.05) plays it under it to check the echoes alone.
//
//   acoustics_fmod <navmap.bin> <telemetry.csv> <take.wav> <audio-at s> <out.wav> [--seconds s]
//                  [--off a,b] [--volume v]
//
// Prints Acoustics::State() every 5 s. Build: cmake --build build --target acoustics_fmod (not
// part of the default build); it needs fmod.dll and fmodstudio.dll beside it (copied by the build).
// Compare: tools/acoustics/compare_render.py against the acoustics target's render of the same run.

#include "Audio/Acoustics/Acoustics.hpp"
#include "Navigation/NavMap.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_studio.hpp>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

struct Row {
  double t;
  Acoustics::Vec3 pos, forward, up;
};

bool ReadTelemetry(const char *aPath, std::vector<Row> &aRows) {
  std::ifstream in(aPath);
  std::string line;
  if (!std::getline(in, line)) {
    return false;
  }
  std::unordered_map<std::string, size_t> col;
  {
    std::stringstream ss(line);
    std::string cell;
    for (size_t i = 0; std::getline(ss, cell, ','); ++i) {
      col[cell] = i;
    }
  }
  while (std::getline(in, line)) {
    std::vector<double> c;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ',')) {
      c.push_back(std::strtod(cell.c_str(), nullptr));
    }
    if (c.size() < col.size()) {
      continue;
    }
    auto f = [&](const char *name) { return float(c[col[name]]); };
    aRows.push_back({c[col["t"]], {f("x"), f("y"), f("z")}, {f("fwd_x"), f("fwd_y"), f("fwd_z")}, {f("up_x"), f("up_y"), f("up_z")}});
  }
  if (aRows.empty()) {
    return false;
  }
  double t0 = aRows.front().t;
  for (auto &r : aRows) {
    r.t -= t0;
  }
  return true;
}

bool Ok(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    std::fprintf(stderr, "%s: FMOD error %d - %s\n", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 6) {
    std::fprintf(stderr, "acoustics_fmod <navmap.bin> <telemetry.csv> <take.wav> <audio-at s> <out.wav> [--seconds s] [--off a,b]\n");
    return 2;
  }
  spdlog::set_default_logger(spdlog::stdout_logger_mt("acoustics_fmod"));
  double seconds = 0, offFrom = -1, offTo = -1;
  float volume = 1.0f;
  for (int a = 6; a + 1 < argc; a += 2) {
    std::string flag = argv[a];
    if (flag == "--seconds") {
      seconds = std::atof(argv[a + 1]);
    } else if (flag == "--off") {
      std::sscanf(argv[a + 1], "%lf,%lf", &offFrom, &offTo);
    } else if (flag == "--volume") {
      volume = float(std::atof(argv[a + 1]));
    }
  }
  if (!Navigation::NavMap::Get().Load(argv[1])) {
    return 1;
  }
  std::vector<Row> rows;
  if (!ReadTelemetry(argv[2], rows)) {
    std::fprintf(stderr, "%s: no telemetry\n", argv[2]);
    return 1;
  }
  const double audioAt = std::atof(argv[4]);
  const double end = seconds > 0 ? std::min(seconds, rows.back().t) : rows.back().t;

  FMOD::Studio::System *studio = nullptr;
  FMOD::System *core = nullptr;
  if (!Ok(FMOD::Studio::System::create(&studio), "create") || !Ok(studio->getCoreSystem(&core), "getCoreSystem")) {
    return 1;
  }
  core->setSoftwareFormat(48000, FMOD_SPEAKERMODE_STEREO, 0);
  core->setOutput(FMOD_OUTPUTTYPE_WAVWRITER);
  std::string out = argv[5];
  if (!Ok(studio->initialize(64, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, out.data()), "initialize")) {
    return 1;
  }
  // the mod's banks, as FlightAudio::Load
  const auto banks = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "src" / "fmod_studio" / "Build" / "Desktop";
  FMOD::Studio::Bank *bank = nullptr, *strings = nullptr;
  if (!Ok(studio->loadBankFile((banks / "base_sounds.bank").string().c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank), "bank") ||
      !Ok(studio->loadBankFile((banks / "base_sounds.strings.bank").string().c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &strings), "strings bank")) {
    return 1;
  }
  if (!Acoustics::Attach(studio, {"bus:/Engine", "bus:/Wind"})) {
    return 1;
  }
  FMOD::Studio::Bus *engine = nullptr;
  FMOD::ChannelGroup *group = nullptr;
  if (!Ok(studio->getBus("bus:/Engine", &engine), "getBus") || !Ok(engine->getChannelGroup(&group), "getChannelGroup")) {
    return 1;
  }
  FMOD::Sound *sound = nullptr;
  FMOD::Channel *channel = nullptr;
  if (!Ok(core->createSound(argv[3], FMOD_CREATESTREAM, nullptr, &sound), "createSound") ||
      !Ok(core->playSound(sound, group, true, &channel), "playSound")) {
    return 1;
  }
  channel->setPosition(unsigned(audioAt * 1000), FMOD_TIMEUNIT_MS);
  channel->setVolume(volume);
  channel->setPaused(false);

  auto lastState = std::chrono::steady_clock::now();
  size_t i = 0;
  for (;;) {
    unsigned ms = 0;
    bool playing = false;
    channel->isPlaying(&playing);
    channel->getPosition(&ms, FMOD_TIMEUNIT_MS);
    const double t = ms / 1000.0 - audioAt;
    if (!playing || t >= end) {
      break;
    }
    while (i + 1 < rows.size() && rows[i + 1].t <= t) {
      ++i;
    }
    const auto &r = rows[i];
    const bool off = t >= offFrom && t < offTo;
    Acoustics::SetCar(r.pos, r.forward, r.up, off ? 0.0f : 1.0f, 0.0f);
    Acoustics::SetListener(r.pos);
    studio->update();
    if (std::chrono::steady_clock::now() - lastState > std::chrono::seconds(5)) {
      lastState = std::chrono::steady_clock::now();
      std::printf("t %.1f: %s\n", t, Acoustics::State().c_str());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  std::printf("end: %s\n", Acoustics::State().c_str());
  channel->stop();
  // let the tail ring out into the file
  for (int k = 0; k < 100; ++k) {
    studio->update();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  Acoustics::Detach();
  sound->release();
  strings->unload();
  bank->unload();
  studio->release();
  std::printf("-> %s\n", out.c_str());
  return 0;
}
