// Offline run of the plugin's acoustics code (src/red4ext/Audio/Acoustics) over an Autopilot
// telemetry file, to check it against tools/acoustics/flyby.py, the reference. Probes the space round
// the car 15 times a second, as flyby.py does, and writes the same features.csv:
//   t,speed,floor,ceiling,left,right,enclosure,mean_free_path
// then prints the probe's time per call.
//
//   acoustics <navmap.bin> <telemetry.csv> <features.csv> [start s] [seconds, 0: all]
//
// Build: cmake --build build --target acoustics (not part of the default build).
// Compare: python tools/acoustics/compare_features.py <flyby features.csv> <this features.csv>

#include "Audio/Acoustics/Probe.hpp"
#include "Navigation/NavMap.hpp"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
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

} // namespace

int main(int argc, char **argv) {
  if (argc < 4) {
    std::fprintf(stderr, "acoustics <navmap.bin> <telemetry.csv> <features.csv> [start s] [seconds]\n");
    return 2;
  }
  // the shared spdlog build has no default logger (the plugin installs its own)
  spdlog::set_default_logger(spdlog::stdout_logger_mt("acoustics"));
  double start = argc > 4 ? std::atof(argv[4]) : 0.0;
  double seconds = argc > 5 ? std::atof(argv[5]) : 0.0;
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
  size_t i = 0, count = 0;
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
    ++count;
    std::fprintf(out, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", tp, r.speed, f.floor, f.ceiling, f.left, f.right,
                 f.enclosure, f.meanFreePath);
  }
  std::fclose(out);
  std::printf("%zu probes, %.3f ms a probe -> %s\n", count, 1000.0 * spent / double(count), argv[3]);
  return 0;
}
