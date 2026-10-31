// Offline run of the plugin's engine synth (src/red4ext/Audio/EngineSynth, docs/design/engine-synth.md)
// over a run's thruster parameters (the stems' <base>_params.csv): one synth a thruster, summed
// unspatialized as the stems' dry track sums the FMOD events, on the stems' timeline (<base>.json),
// so the two compare sample for sample.
//
//   engine_synth <stems base> <out prefix> [--voices] [--tune name=dB,...] [--character text] [--sound name]
//                [--acoustics navmap.bin [--air]]
//
// Writes <prefix>.wav (stereo float, the stems' format); --voices also <prefix>_<voice>.wav (mono
// float), each voice's part for tools/acoustics/engine_fit.py. --tune adds dB to a voice's level by
// name (surge, lift, ..., master) and sets source (dB), sine and detune (st). --character keeps or drops
// the FMOD event's effects (EngineSynth::ParseCharacter: fmod, clean, cone=0,...). --sound picks the
// source (EngineSynth::ParseSound: drone, bullroarer, fan).
//
// --acoustics also writes <prefix>_wet.wav: the sum through the plugin's echoes (Reflections) along the
// run's own path, the car's place and axes from its four thrusters' logged positions, probed in the
// nav map 20 times a second (no live rays), heard from the car; --air with air absorption.
//
// Build: cmake --build build --target engine_synth (not part of the default build).

#include "Audio/Acoustics/Probe.hpp"
#include "Audio/Acoustics/Reflections.hpp"
#include "Audio/EngineSynth/Synth.hpp"
#include "Navigation/NavMap.hpp"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Row {
  double wall;
  std::string id;
  float pitch;
  EngineSynth::Inputs inputs;
  Acoustics::Vec3 position;
};

bool ReadParams(const std::string &aPath, std::vector<Row> &aRows) {
  std::ifstream in(aPath);
  std::string line;
  if (!std::getline(in, line)) {
    return false;
  }
  std::vector<std::string> names;
  {
    std::stringstream ss(line);
    std::string name;
    while (std::getline(ss, name, ',')) {
      names.push_back(name);
    }
  }
  while (std::getline(in, line)) {
    std::stringstream ss(line);
    std::string cell;
    Row r{};
    // wall, id, start pitch, volume, x, y, z by place; then the parameters by name
    for (size_t c = 0; c < names.size() && std::getline(ss, cell, ','); ++c) {
      const float v = std::strtof(cell.c_str(), nullptr);
      auto &i = r.inputs;
      const std::string &n = names[c];
      if (c == 0) r.wall = std::strtod(cell.c_str(), nullptr);
      else if (c == 1) r.id = cell;
      else if (c == 2) r.pitch = v;
      else if (c == 3) i.volume = v;
      else if (c == 4) r.position.x = v;
      else if (c == 5) r.position.y = v;
      else if (c == 6) r.position.z = v;
      else if (n == "speed") i.speed = v;
      else if (n == "surge") i.surge = v;
      else if (n == "lift") i.lift = v;
      else if (n == "pitch") i.pitch = v;
      else if (n == "roll") i.roll = v;
      else if (n == "sway") i.sway = v;
      else if (n == "yaw") i.yaw = v;
      else if (n == "brake") i.brake = v;
      else if (n == "damage") i.damage = v;
      else if (n == "inside") i.inside = v;
      else if (n == "scrape") i.scrape = v;
      else if (n == "water") i.water = v;
      else if (n == "cone") i.cone = v;
    }
    aRows.push_back(std::move(r));
  }
  return !aRows.empty();
}

double ReadJsonNumber(const std::string &aText, const char *aKey) {
  auto at = aText.find(std::string("\"") + aKey + "\":");
  return at == std::string::npos ? 0.0 : std::strtod(aText.c_str() + at + std::strlen(aKey) + 3, nullptr);
}

void WriteWav(const std::string &aPath, const std::vector<float> &aSamples, int aChannels, int aRate) {
  FILE *f = std::fopen(aPath.c_str(), "wb");
  if (!f) {
    std::fprintf(stderr, "cannot write %s\n", aPath.c_str());
    return;
  }
  const uint32_t data = uint32_t(aSamples.size() * 4), riff = 36 + data, fmtSize = 16, rate = uint32_t(aRate);
  const uint16_t format = 3, channels = uint16_t(aChannels), align = uint16_t(4 * aChannels), bits = 32;
  const uint32_t byteRate = rate * align;
  std::fwrite("RIFF", 1, 4, f);
  std::fwrite(&riff, 4, 1, f);
  std::fwrite("WAVEfmt ", 1, 8, f);
  std::fwrite(&fmtSize, 4, 1, f);
  std::fwrite(&format, 2, 1, f);
  std::fwrite(&channels, 2, 1, f);
  std::fwrite(&rate, 4, 1, f);
  std::fwrite(&byteRate, 4, 1, f);
  std::fwrite(&align, 2, 1, f);
  std::fwrite(&bits, 2, 1, f);
  std::fwrite("data", 1, 4, f);
  std::fwrite(&data, 4, 1, f);
  std::fwrite(aSamples.data(), 4, aSamples.size(), f);
  std::fclose(f);
}

// The car from its four thrusters (ids ending in FL, FR, BL, BR with a number after): the middle, forward
// from the back pair to the front, right from the left pair to the right
bool CarPose(const std::map<std::string, Acoustics::Vec3> &aAt, Acoustics::Vec3 &aPos, Acoustics::Vec3 &aForward,
             Acoustics::Vec3 &aUp) {
  Acoustics::Vec3 corner[4]{};
  int found = 0;
  const char *names[4] = {"FL", "FR", "BL", "BR"};
  for (const auto &[id, p] : aAt) {
    for (int k = 0; k < 4; ++k) {
      if (id.find(std::string("vehicle") + names[k]) == 0) {
        corner[k] = p;
        found |= 1 << k;
      }
    }
  }
  if (found != 15) {
    return false;
  }
  auto mid = [](Acoustics::Vec3 a, Acoustics::Vec3 b) { return Acoustics::Vec3{(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2}; };
  auto sub = [](Acoustics::Vec3 a, Acoustics::Vec3 b) { return Acoustics::Vec3{a.x - b.x, a.y - b.y, a.z - b.z}; };
  auto norm = [](Acoustics::Vec3 v) {
    const float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return l > 1e-6f ? Acoustics::Vec3{v.x / l, v.y / l, v.z / l} : Acoustics::Vec3{0, 1, 0};
  };
  const auto front = mid(corner[0], corner[1]), back = mid(corner[2], corner[3]);
  const auto right = norm(sub(mid(corner[1], corner[3]), mid(corner[0], corner[2])));
  aPos = mid(front, back);
  aForward = norm(sub(front, back));
  // up = right x forward, as the probe takes right = forward x up
  aUp = norm({right.y * aForward.z - right.z * aForward.y, right.z * aForward.x - right.x * aForward.z,
              right.x * aForward.y - right.y * aForward.x});
  return true;
}

// aOut (stereo, the dry sum) through Reflections along the run's path, in place
bool Echoes(const char *aNavmap, bool aAir, const std::vector<Row> &aRows, double aStart, int aRate,
            std::vector<float> &aOut) {
  // the nav map logs through spdlog's default logger
  spdlog::set_default_logger(spdlog::stderr_logger_mt("engine_synth"));
  auto &map = Navigation::NavMap::Get();
  if (!map.Load(aNavmap)) {
    std::fprintf(stderr, "%s: cannot load\n", aNavmap);
    return false;
  }
  Acoustics::Probe probe(map);
  Acoustics::Tuning tuning;
  tuning.airAbsorption = aAir;
  const int interval = aRate / 20;
  Acoustics::Reflections reflections(float(aRate), interval, tuning);
  const size_t frames = aOut.size() / 2;
  std::vector<float> dry(size_t(interval) * 2);
  std::map<std::string, Acoustics::Vec3> at;
  size_t next = 0;
  Acoustics::Vec3 last{}, velocity{};
  bool haveLast = false, started = false;
  for (size_t f = 0; f < frames; f += interval) {
    const uint32_t n = uint32_t(std::min<size_t>(interval, frames - f));
    const double now = aStart + double(f) / aRate;
    for (; next < aRows.size() && aRows[next].wall <= now; ++next) {
      at[aRows[next].id] = aRows[next].position;
    }
    Acoustics::Vec3 pos, forward, up;
    if (CarPose(at, pos, forward, up)) {
      if (haveLast) {
        const Acoustics::Vec3 v{(pos.x - last.x) * 20, (pos.y - last.y) * 20, (pos.z - last.z) * 20};
        velocity = {velocity.x + 0.5f * (v.x - velocity.x), velocity.y + 0.5f * (v.y - velocity.y),
                    velocity.z + 0.5f * (v.z - velocity.z)};
      }
      last = pos;
      haveLast = true;
      reflections.SetTargets(Acoustics::Targets(probe(pos, forward, up), pos, pos, tuning, velocity));
      started = true;
    }
    if (!started) {
      continue;
    }
    std::copy(aOut.begin() + 2 * f, aOut.begin() + 2 * (f + n), dry.begin());
    reflections.Process(dry.data(), aOut.data() + 2 * f, n, 2);
  }
  const auto &e = reflections.Parts();
  auto rel = [&](double x) { return 10 * std::log10(x / std::max(e.dry, 1e-30) + 1e-30); };
  std::printf("echoes: floor %.1f, walls %.1f, scatter %.1f, reverb %.1f dB against the dry sound%s\n", rel(e.floor),
              rel(e.walls), rel(e.scatter), rel(e.reverb), aAir ? ", air absorption" : "");
  return true;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 3) {
    std::fprintf(stderr, "engine_synth <stems base> <out prefix> [--voices] [--tune name=dB,...] [--character text] [--sound name] "
                         "[--acoustics navmap.bin [--air]]\n");
    return 2;
  }
  const std::string base = argv[1], prefix = argv[2];
  bool voices = false, air = false;
  const char *navmap = nullptr;
  EngineSynth::Tuning tuning;
  for (int a = 3; a < argc; ++a) {
    if (std::strcmp(argv[a], "--voices") == 0) {
      voices = true;
    } else if (std::strcmp(argv[a], "--air") == 0) {
      air = true;
    } else if (std::strcmp(argv[a], "--acoustics") == 0 && a + 1 < argc) {
      navmap = argv[++a];
    } else if (std::strcmp(argv[a], "--character") == 0 && a + 1 < argc) {
      if (!EngineSynth::ParseCharacter(argv[++a], tuning.character)) {
        std::fprintf(stderr, "--character %s: unknown\n", argv[a]);
        return 2;
      }
    } else if (std::strcmp(argv[a], "--sound") == 0 && a + 1 < argc) {
      if (!EngineSynth::ParseSound(argv[++a], tuning.sound)) {
        std::fprintf(stderr, "--sound %s: unknown\n", argv[a]);
        return 2;
      }
    } else if (std::strcmp(argv[a], "--tune") == 0 && a + 1 < argc) {
      std::stringstream ss(argv[++a]);
      for (std::string item; std::getline(ss, item, ',');) {
        const auto eq = item.find('=');
        const std::string name = item.substr(0, eq);
        const float db = std::strtof(item.c_str() + eq + 1, nullptr);
        if (name == "master") {
          tuning.master = db;
        } else if (name == "source") {
          tuning.source = db;
        } else if (name == "sine") {
          tuning.sine = db;
        } else if (name == "detune") {
          tuning.detune = db;
        }
        for (int v = 0; v < EngineSynth::kVoices; ++v) {
          if (name == EngineSynth::kVoiceNames[v]) {
            tuning.level[v] = db;
          }
        }
      }
    }
  }
  std::vector<Row> rows;
  if (!ReadParams(base + "_params.csv", rows)) {
    std::fprintf(stderr, "%s_params.csv: no rows\n", base.c_str());
    return 1;
  }
  std::ifstream js(base + ".json");
  const std::string json((std::istreambuf_iterator<char>(js)), std::istreambuf_iterator<char>());
  const int rate = int(ReadJsonNumber(json, "rate"));
  const double start = ReadJsonNumber(json, "startWall");
  if (rate <= 0 || start <= 0) {
    std::fprintf(stderr, "%s.json: no rate or start\n", base.c_str());
    return 1;
  }
  const double end = rows.back().wall + 0.5;
  const size_t frames = size_t(std::max(0.0, (end - start) * rate));
  std::vector<float> out(frames * 2, 0.0f), voiceOut(voices ? frames * EngineSynth::kVoices : 0, 0.0f);

  // one synth a thruster, fed its rows as their wall times pass
  struct Thruster {
    std::unique_ptr<EngineSynth::Synth> synth;
    double last = -1;
  };
  std::map<std::string, Thruster> thrusters;
  size_t next = 0;
  constexpr uint32_t kBlock = 256;
  std::vector<float> block(kBlock), blockVoices(kBlock * EngineSynth::kVoices);
  const auto t0 = std::chrono::steady_clock::now();
  uint64_t rendered = 0;
  for (size_t f = 0; f < frames; f += kBlock) {
    const uint32_t n = uint32_t(std::min<size_t>(kBlock, frames - f));
    const double now = start + double(f) / rate;
    for (; next < rows.size() && rows[next].wall <= now; ++next) {
      const Row &r = rows[next];
      auto &t = thrusters[r.id];
      if (!t.synth) {
        t.synth = std::make_unique<EngineSynth::Synth>(float(rate), r.pitch, uint32_t(std::hash<std::string>()(r.id)),
                                                       tuning);
      }
      t.synth->Set(r.inputs);
      t.last = r.wall;
    }
    for (auto &[id, t] : thrusters) {
      // a thruster not updated for a quarter second has stopped
      if (!t.synth || now - t.last > 0.25) {
        continue;
      }
      t.synth->Render(block.data(), n, voices ? blockVoices.data() : nullptr);
      rendered += n;
      for (uint32_t s = 0; s < n; ++s) {
        out[2 * (f + s)] += block[s];
        out[2 * (f + s) + 1] += block[s];
        if (voices) {
          for (int v = 0; v < EngineSynth::kVoices; ++v) {
            voiceOut[(f + s) * EngineSynth::kVoices + v] += blockVoices[s * EngineSynth::kVoices + v];
          }
        }
      }
    }
  }
  const double spent = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  WriteWav(prefix + ".wav", out, 2, rate);
  if (navmap) {
    if (!Echoes(navmap, air, rows, start, rate, out)) {
      return 1;
    }
    WriteWav(prefix + "_wet.wav", out, 2, rate);
  }
  if (voices) {
    std::vector<float> one(frames);
    for (int v = 0; v < EngineSynth::kVoices; ++v) {
      for (size_t s = 0; s < frames; ++s) {
        one[s] = voiceOut[s * EngineSynth::kVoices + v];
      }
      WriteWav(prefix + "_" + EngineSynth::kVoiceNames[v] + ".wav", one, 1, rate);
    }
  }
  std::printf("%zu rows, %zu thrusters, %.1f s -> %s.wav; %.0f ns a thruster frame, %.2f%% of a core a thruster\n",
              rows.size(), thrusters.size(), double(frames) / rate, prefix.c_str(),
              rendered ? 1e9 * spent / double(rendered) : 0.0, rendered ? 100.0 * spent * rate / double(rendered) : 0.0);
  return 0;
}
