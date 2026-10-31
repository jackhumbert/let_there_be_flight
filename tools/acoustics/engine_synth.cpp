// Offline run of the plugin's engine synth (src/red4ext/Audio/EngineSynth, docs/design/engine-synth.md)
// over a run's thruster parameters (the stems' <base>_params.csv): one synth a thruster, summed
// unspatialized as the stems' dry track sums the FMOD events, on the stems' timeline (<base>.json),
// so the two compare sample for sample.
//
//   engine_synth <stems base> <out prefix> [--voices] [--tune name=dB,...]
//
// Writes <prefix>.wav (stereo float, the stems' format); --voices also <prefix>_<voice>.wav (mono
// float), each voice's part for tools/acoustics/engine_fit.py. --tune adds dB to a voice's level by
// name (surge, lift, ..., master) and sets source (dB), sine and detune (st).
//
// Build: cmake --build build --target engine_synth (not part of the default build).

#include "Audio/EngineSynth/Synth.hpp"

#include <chrono>
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
      else if (c < 7) continue;
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

} // namespace

int main(int argc, char **argv) {
  if (argc < 3) {
    std::fprintf(stderr, "engine_synth <stems base> <out prefix> [--voices] [--tune name=dB,...]\n");
    return 2;
  }
  const std::string base = argv[1], prefix = argv[2];
  bool voices = false;
  EngineSynth::Tuning tuning;
  for (int a = 3; a < argc; ++a) {
    if (std::strcmp(argv[a], "--voices") == 0) {
      voices = true;
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
