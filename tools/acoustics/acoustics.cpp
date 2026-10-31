// Offline run of the plugin's acoustics code (src/red4ext/Audio/Acoustics) over an Autopilot
// telemetry file, to check it against tools/acoustics/flyby.py, the reference. Probes the space round
// the car 15 times a second, as flyby.py does, and writes the same features.csv:
//   t,speed,floor,ceiling,left,right,enclosure,mean_free_path
// then prints the probe's time per call. With --audio (the run's recording) and --audio-at (s into it
// of the telemetry's start; flyby.py prints it), renders it through Reflections too:
// <prefix>_dry.wav and <prefix>_wet.wav, the parts' levels and the cost.
//
//   acoustics <navmap.bin> <telemetry.csv> <features.csv> [--start s] [--seconds s, 0: all]
//             [--audio take.wav --audio-at s --out prefix] [--listener back,up] [--tune a=1,b=2]
//
// --tune sets Acoustics::Tuning fields by name.
//
// Build: cmake --build build --target acoustics (not part of the default build).
// Compare: python tools/acoustics/compare_features.py <flyby features.csv> <this features.csv>
//          python tools/acoustics/compare_render.py <flyby out dir> <prefix>

#include "Audio/Acoustics/Probe.hpp"
#include "Audio/Acoustics/Reflections.hpp"
#include "Navigation/NavMap.hpp"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr double kProbeHz = 15.0;

struct Row {
  double t;
  float speed;
  Acoustics::Vec3 pos, forward, up;
};

std::vector<std::string> Split(const std::string &aLine) {
  std::vector<std::string> out;
  std::stringstream ss(aLine);
  std::string cell;
  while (std::getline(ss, cell, ',')) {
    out.push_back(cell);
  }
  return out;
}

// the rows from start s for seconds s (0: all), t from 0 there, as flyby.py's telemetry()
bool ReadTelemetry(const char *aPath, double aStart, double aSeconds, std::vector<Row> &aRows) {
  std::ifstream in(aPath);
  std::string line;
  if (!std::getline(in, line)) {
    return false;
  }
  std::unordered_map<std::string, size_t> col;
  auto header = Split(line);
  for (size_t i = 0; i < header.size(); ++i) {
    col[header[i]] = i;
  }
  for (const char *name : {"t", "speed", "x", "y", "z", "fwd_x", "fwd_y", "fwd_z", "up_x", "up_y", "up_z"}) {
    if (!col.contains(name)) {
      spdlog::error("{}: no column {}", aPath, name);
      return false;
    }
  }
  std::vector<Row> all;
  while (std::getline(in, line)) {
    auto c = Split(line);
    if (c.size() < header.size()) {
      continue;
    }
    auto f = [&](const char *name) { return std::strtod(c[col[name]].c_str(), nullptr); };
    all.push_back({f("t"), float(f("speed")), {float(f("x")), float(f("y")), float(f("z"))},
                   {float(f("fwd_x")), float(f("fwd_y")), float(f("fwd_z"))},
                   {float(f("up_x")), float(f("up_y")), float(f("up_z"))}});
  }
  if (all.empty()) {
    return false;
  }
  double t0 = all.front().t + aStart;
  for (auto &r : all) {
    if (r.t >= t0 && (aSeconds <= 0 || r.t < t0 + aSeconds)) {
      r.t -= t0;
      aRows.push_back(r);
    }
  }
  return !aRows.empty();
}

// Tuning fields by name, "a=1,b=2"
bool SetTuning(Acoustics::Tuning &aTuning, const char *aText) {
  using T = Acoustics::Tuning;
  const std::pair<const char *, float T::*> fields[] = {
      {"wallReflectivity", &T::wallReflectivity}, {"groundReflectivity", &T::groundReflectivity},
      {"gain", &T::gain}, {"nearDistance", &T::nearDistance}, {"range", &T::range},
      {"flutterFeedback", &T::flutterFeedback}, {"flutterNear", &T::flutterNear}, {"flutterFar", &T::flutterFar},
      {"wallAbsorption", &T::wallAbsorption}, {"rt60Min", &T::rt60Min}, {"rt60Max", &T::rt60Max}, {"wet", &T::wet},
      {"maxDelayRate", &T::maxDelayRate}, {"minDelay", &T::minDelay}, {"pan", &T::pan},
      {"panOpposite", &T::panOpposite}, {"scatter", &T::scatter}, {"jumpSeconds", &T::jumpSeconds},
      {"fadeSeconds", &T::fadeSeconds}, {"wetFrom", &T::wetFrom}};
  std::stringstream ss(aText);
  std::string item;
  while (std::getline(ss, item, ',')) {
    auto eq = item.find('=');
    bool found = false;
    for (const auto &[name, member] : fields) {
      if (eq != std::string::npos && item.substr(0, eq) == name) {
        aTuning.*member = float(std::atof(item.c_str() + eq + 1));
        found = true;
      }
    }
    if (!found) {
      std::fprintf(stderr, "no Tuning field %s\n", item.c_str());
      return false;
    }
  }
  return true;
}

struct Wav {
  uint32_t rate = 0;
  int channels = 0;
  std::vector<float> samples; // interleaved
};

// 16 bit PCM or 32 bit float, plain or extensible
bool ReadWav(const char *aPath, Wav &aWav) {
  std::ifstream in(aPath, std::ios::binary);
  char riff[12];
  if (!in.read(riff, 12) || std::memcmp(riff, "RIFF", 4) || std::memcmp(riff + 8, "WAVE", 4)) {
    return false;
  }
  uint16_t format = 0, bits = 0;
  for (char id[4]; in.read(id, 4);) {
    uint32_t size;
    in.read(reinterpret_cast<char *>(&size), 4);
    std::vector<char> chunk(size + (size & 1));
    in.read(chunk.data(), chunk.size());
    if (!std::memcmp(id, "fmt ", 4)) {
      std::memcpy(&format, chunk.data(), 2);
      uint16_t channels;
      std::memcpy(&channels, chunk.data() + 2, 2);
      std::memcpy(&aWav.rate, chunk.data() + 4, 4);
      std::memcpy(&bits, chunk.data() + 14, 2);
      aWav.channels = channels;
      if (format == 0xFFFE && size >= 26) {
        std::memcpy(&format, chunk.data() + 24, 2); // the sub format's first two bytes
      }
    } else if (!std::memcmp(id, "data", 4)) {
      if (format == 1 && bits == 16) {
        aWav.samples.resize(size / 2);
        for (size_t k = 0; k < aWav.samples.size(); ++k) {
          int16_t v;
          std::memcpy(&v, chunk.data() + 2 * k, 2);
          aWav.samples[k] = float(v) / 32768.0f;
        }
      } else if (format == 3 && bits == 32) {
        aWav.samples.resize(size / 4);
        std::memcpy(aWav.samples.data(), chunk.data(), size);
      } else {
        return false;
      }
      return aWav.channels > 0;
    }
  }
  return false;
}

bool WriteWav(const std::string &aPath, const float *aSamples, size_t aFrames, int aChannels, uint32_t aRate) {
  FILE *f = std::fopen(aPath.c_str(), "wb");
  if (!f) {
    return false;
  }
  const uint32_t data = uint32_t(aFrames * aChannels * 4), riff = 36 + data, fmtSize = 16;
  const uint16_t format = 3, channels = uint16_t(aChannels), align = uint16_t(aChannels * 4), bits = 32;
  const uint32_t bytesPerSecond = aRate * align;
  std::fwrite("RIFF", 1, 4, f);
  std::fwrite(&riff, 4, 1, f);
  std::fwrite("WAVEfmt ", 1, 8, f);
  std::fwrite(&fmtSize, 4, 1, f);
  std::fwrite(&format, 2, 1, f);
  std::fwrite(&channels, 2, 1, f);
  std::fwrite(&aRate, 4, 1, f);
  std::fwrite(&bytesPerSecond, 4, 1, f);
  std::fwrite(&align, 2, 1, f);
  std::fwrite(&bits, 2, 1, f);
  std::fwrite("data", 1, 4, f);
  std::fwrite(&data, 4, 1, f);
  std::fwrite(aSamples, 4, aFrames * aChannels, f);
  std::fclose(f);
  return true;
}

// The recording from aAt s through the echoes, each probe's Params handed over one probe interval
// early (so they are reached at the probe's time, as flyby.py interpolates between probes). Writes
// <prefix>_dry.wav and <prefix>_wet.wav (32 bit float) and prints each part's level against the dry
// sound, as flyby.py does, and the cost.
int Render(const char *aAudio, double aAt, const std::vector<Acoustics::Params> &aParams,
           const Acoustics::Tuning &aTuning, const std::string &aPrefix) {
  Wav wav;
  if (!ReadWav(aAudio, wav)) {
    spdlog::error("{}: not a 16 bit or float wav", aAudio);
    return 1;
  }
  const int ch = wav.channels;
  const size_t total = wav.samples.size() / ch;
  const size_t i0 = size_t(std::llround(aAt * wav.rate));
  const int ramp = int(std::lround(wav.rate / kProbeHz));
  size_t n = size_t(double(aParams.size() - 1) / kProbeHz * wav.rate);
  if (i0 >= total) {
    spdlog::error("the recording ends before {:.2f} s", aAt);
    return 1;
  }
  n = std::min(n, total - i0);
  const float *dry = wav.samples.data() + i0 * ch;
  std::vector<float> wet(n * ch);
  Acoustics::Reflections reflections(float(wav.rate), ramp, aTuning);
  constexpr uint32_t kBlock = 1024; // FMOD's mixer block
  double spent = 0;
  for (size_t s = 0; s < n;) {
    const size_t k = s / ramp;
    if (s % ramp == 0) {
      if (k == 0) {
        reflections.SetTargets(aParams[0]);
      }
      if (k + 1 < aParams.size()) {
        reflections.SetTargets(aParams[k + 1]);
      }
    }
    const uint32_t frames = uint32_t(std::min<size_t>({kBlock, n - s, (k + 1) * ramp - s}));
    auto t0 = std::chrono::steady_clock::now();
    reflections.Process(dry + s * ch, wet.data() + s * ch, frames, ch);
    spent += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    s += frames;
  }
  WriteWav(aPrefix + "_dry.wav", dry, n, ch, wav.rate);
  WriteWav(aPrefix + "_wet.wav", wet.data(), n, ch, wav.rate);
  const auto &e = reflections.Parts();
  auto db = [&](double part) { return 10.0 * std::log10(part / e.dry + 1e-18); };
  std::printf("level re dry: floor %.1f dB, ceiling %.1f dB, walls %.1f dB, reverb %.1f dB\n", db(e.floor),
              db(e.ceiling), db(e.walls), db(e.reverb));
  const double seconds = double(n) / wav.rate;
  std::printf("%.1f s rendered in %.1f ms: %.0f ns a frame, %.2f%% of one core in real time -> %s_wet.wav\n",
              seconds, 1000 * spent, 1e9 * spent / double(n), 100 * spent / seconds, aPrefix.c_str());
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 4) {
    std::fprintf(stderr, "acoustics <navmap.bin> <telemetry.csv> <features.csv> [--start s] [--seconds s] "
                         "[--audio take.wav --audio-at s --out prefix] [--listener back,up]\n");
    return 2;
  }
  // the shared spdlog build has no default logger (the plugin installs its own)
  spdlog::set_default_logger(spdlog::stdout_logger_mt("acoustics"));
  double start = 0, seconds = 0, audioAt = 0;
  float back = 0, up = 0;
  const char *audio = nullptr, *prefix = nullptr;
  Acoustics::Tuning tuning;
  for (int a = 4; a + 1 < argc; a += 2) {
    std::string flag = argv[a];
    if (flag == "--start") {
      start = std::atof(argv[a + 1]);
    } else if (flag == "--seconds") {
      seconds = std::atof(argv[a + 1]);
    } else if (flag == "--audio") {
      audio = argv[a + 1];
    } else if (flag == "--audio-at") {
      audioAt = std::atof(argv[a + 1]);
    } else if (flag == "--out") {
      prefix = argv[a + 1];
    } else if (flag == "--listener") {
      std::sscanf(argv[a + 1], "%f,%f", &back, &up);
    } else if (flag == "--tune") {
      if (!SetTuning(tuning, argv[a + 1])) {
        return 2;
      }
    } else {
      std::fprintf(stderr, "unknown %s\n", argv[a]);
      return 2;
    }
  }
  auto &map = Navigation::NavMap::Get();
  if (!map.Load(argv[1])) {
    return 1;
  }
  std::vector<Row> rows;
  if (!ReadTelemetry(argv[2], start, seconds, rows)) {
    spdlog::error("{}: no telemetry", argv[2]);
    return 1;
  }
  FILE *out = std::fopen(argv[3], "w");
  if (!out) {
    spdlog::error("{}: cannot write", argv[3]);
    return 1;
  }
  std::fprintf(out, "t,speed,floor,ceiling,left,right,enclosure,mean_free_path\n");
  Acoustics::Probe probe(map);
  std::vector<Acoustics::Params> params; // one a probe
  Acoustics::Vec3 last{};
  size_t i = 0;
  double spent = 0;
  for (int k = 0;; ++k) {
    double tp = k * (1.0 / kProbeHz);
    if (tp >= rows.back().t) {
      break;
    }
    while (rows[i].t < tp) {
      ++i;
    }
    const Row &r = rows[i];
    auto t0 = std::chrono::steady_clock::now();
    auto f = probe(r.pos, r.forward, r.up);
    spent += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::fprintf(out, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", tp, r.speed, f.floor, f.ceiling, f.left, f.right,
                 f.enclosure, f.meanFreePath);
    Acoustics::Vec3 listener{r.pos.x - back * r.forward.x, r.pos.y - back * r.forward.y,
                             r.pos.z - back * r.forward.z + up};
    // the car's velocity from probe to probe, as the plugin's worker takes it
    Acoustics::Vec3 velocity{};
    if (k > 0) {
      const float hz = float(kProbeHz);
      velocity = {(r.pos.x - last.x) * hz, (r.pos.y - last.y) * hz, (r.pos.z - last.z) * hz};
    }
    last = r.pos;
    params.push_back(Acoustics::Targets(f, r.pos, listener, tuning, velocity));
  }
  std::fclose(out);
  std::printf("%zu probes, %.3f ms a probe -> %s\n", params.size(), 1000.0 * spent / double(params.size()), argv[3]);
  if (audio && prefix && !params.empty()) {
    return Render(audio, audioAt + start, params, tuning, prefix);
  }
  return 0;
}
