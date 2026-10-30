// Offline route planning on navmap.bin with the plugin's own planner (src/red4ext/Navigation), for
// tuning and plotting routes without the game. Plans start -> goal twice, the plain route and the
// racing line, and writes next to the given prefix:
//   <prefix>_route.csv    x,y,z: the plain route
//   <prefix>_line.csv     x,y,z: the racing line (a rejected one too, to see where it is blocked)
//   <prefix>_profile.csv  d,x,y,z,v,limit,why,clearance: the line every 2 m, with the speed the car
//                         flies it at (Navigation/Vehicle.hpp), what limits it there and the
//                         distance to the nearest blocked space (up to 10 m)
// and prints one line "metrics {json}" for the tuner (tools/navplan/tune.py).
//
//   navplan <navmap.bin> <sx> <sy> <sz> <gx> <gy> <gz> <out prefix>
//
// Environment:
//   NAVPLAN_RACE=1          the racing line as the Autopilot's Race plans it (FlightModeAutopilot
//                           race route fields and FlightNavMap::PlanRaceAsync); off: PlanParams defaults
//   NAVPLAN_PARAMS=a=1,b=2  PlanParams fields by name (the table below), applied after the preset
//   NAVPLAN_VEHICLE=a=1,..  the car (VehicleSpec fields by name; tools/navplan/vehicles.json names cars)
//   NAVPLAN_REPEAT=1        plan the line again: what a later plan costs once its doorways are known
//   NAVPLAN_DOORWAY_COUNT=x0,y0,x1,y1  work out the doorways over that area instead, and save them
//
// Build: cmake --build build --target navplan (not part of the default build).

#include "Navigation/NavMap.hpp"
#include "Navigation/Planner.hpp"
#include "Navigation/Vehicle.hpp"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace {
using Navigation::PlanParams;

// PlanParams fields settable by name (NAVPLAN_PARAMS)
struct Field {
  const char *name;
  std::variant<float PlanParams::*, int PlanParams::*, bool PlanParams::*> member;
};
#define F(n) Field{#n, &PlanParams::n}
const Field kFields[] = {
    F(preferredHeight), F(climbCost), F(climbScale), F(underCost), F(heuristicWeight), F(turnCost), F(bearingCost),
    F(minPassage), F(startRadius), F(startLift), F(doorways), F(doorwaysFrom), F(maxGradient), F(legClearance),
    F(lineSpacing), F(lineSide), F(lineClearance), F(lineFloorClearance), F(lineFloorEnds), F(lineHeightIterations),
    F(lineHeightWavelength), F(lineHeightFree), F(lineHeightScale), F(lineOverEnds), F(lineOverReach),
    F(lineOverThick), F(lineOverRise), F(lineOverGain), F(lineOverWindow), F(lineCeilingClearance), F(linePush),
    F(lineSpread), F(lineCorridorIterations), F(lineCorridorMax), F(lineCorridorMargin), F(lineCurvatureRate),
    F(lineTurnRounds), F(lineTurnWeightMax), F(lineStep), F(lineVerticalWeight), F(directMargin), F(maxCruise),
};
#undef F

bool SetParams(PlanParams &aParams, const std::string &aText) {
  std::stringstream items(aText);
  std::string item;
  while (std::getline(items, item, ',')) {
    if (item.empty()) {
      continue;
    }
    auto eq = item.find('=');
    std::string key = item.substr(0, eq);
    const Field *field = nullptr;
    for (const auto &f : kFields) {
      if (key == f.name) {
        field = &f;
      }
    }
    if (eq == std::string::npos || !field) {
      std::fprintf(stderr, "NAVPLAN_PARAMS: unknown or malformed '%s'\n", item.c_str());
      return false;
    }
    float value = std::strtof(item.c_str() + eq + 1, nullptr);
    std::visit(
        [&](auto aMember) {
          using T = std::remove_reference_t<decltype(aParams.*aMember)>;
          aParams.*aMember = static_cast<T>(std::is_same_v<T, bool> ? value != 0.0f : value);
        },
        field->member);
  }
  return true;
}

bool Write(const std::string &aPath, const std::vector<Navigation::Vec3> &aPoints) {
  FILE *f = std::fopen(aPath.c_str(), "w");
  if (!f) {
    return false;
  }
  std::fprintf(f, "x,y,z\n");
  for (const auto &p : aPoints) {
    std::fprintf(f, "%.2f,%.2f,%.2f\n", p.x, p.y, p.z);
  }
  std::fclose(f);
  return true;
}

// Distance (m, up to 10) from a point to the nearest blocked space, in 8 directions across and
// straight up and down (the map already holds the car's size)
float Clearance(const Navigation::NavMap &aMap, float aX, float aY, float aZ) {
  static const float radii[] = {1, 2, 3, 4, 5, 6, 8, 10};
  float best = 10.0f;
  for (int32_t d = 0; d < 10; ++d) {
    float dx = 0, dy = 0, dz = 0;
    if (d < 8) {
      dx = std::cos(d * 0.785398f);
      dy = std::sin(d * 0.785398f);
    } else {
      dz = d == 8 ? 1.0f : -1.0f;
    }
    for (float r : radii) {
      if (r >= best) {
        break;
      }
      if (!aMap.IsFree(aX + dx * r, aY + dy * r, aZ + dz * r)) {
        best = r;
        break;
      }
    }
  }
  return best;
}

float Dist(const Navigation::Vec3 &aA, const Navigation::Vec3 &aB) {
  return std::sqrt((aA.x - aB.x) * (aA.x - aB.x) + (aA.y - aB.y) * (aA.y - aB.y) + (aA.z - aB.z) * (aA.z - aB.z));
}

const char *Why(Navigation::SpeedProfile::Limit aLimit) {
  switch (aLimit) {
  case Navigation::SpeedProfile::Limit::Turn:
    return "turn";
  case Navigation::SpeedProfile::Limit::Crest:
    return "crest";
  case Navigation::SpeedProfile::Limit::PullUp:
    return "pull-up";
  default:
    return "";
  }
}
} // namespace

int main(int argc, char **argv) {
  if (argc != 9) {
    std::fprintf(stderr, "usage: navplan <navmap.bin> <sx> <sy> <sz> <gx> <gy> <gz> <out prefix>\n");
    return 2;
  }
  // the shared spdlog build has no default logger (the plugin installs its own)
  spdlog::set_default_logger(spdlog::stdout_logger_mt("navplan"));
  auto &map = Navigation::NavMap::Get();
  if (!map.Load(argv[1])) {
    std::fprintf(stderr, "could not load %s\n", argv[1]);
    return 1;
  }
  const bool race = std::getenv("NAVPLAN_RACE") && std::atoi(std::getenv("NAVPLAN_RACE")) != 0;
  if (const char *area = std::getenv("NAVPLAN_DOORWAY_COUNT")) {
    // x0,y0,x1,y1: work out the doorways over the area with the Race's costs, and count them
    float x0, y0, x1, y1;
    if (std::sscanf(area, "%f,%f,%f,%f", &x0, &y0, &x1, &y1) == 4) {
      PlanParams params;
      Navigation::RacePreset(params);
      auto n = Navigation::WorkOutDoorways(map, params, x0, y0, x1, y1);
      std::printf("doorways: %u blocks, %u doors, %u links, %.0f ms, %zu bytes\n", n.blocks, n.doors, n.links, n.milliseconds,
                  n.saved.size());
      // the raw set to the out prefix (tools/heightmap/doorways.py packs it into navmap.bin)
      if (FILE *f = std::fopen((std::string(argv[8]) + ".doorways").c_str(), "wb")) {
        std::fwrite(n.saved.data(), 1, n.saved.size(), f);
        std::fclose(f);
      }
      return 0;
    }
  }
  Navigation::Vec3 start{std::strtof(argv[2], nullptr), std::strtof(argv[3], nullptr), std::strtof(argv[4], nullptr)};
  Navigation::Vec3 goal{std::strtof(argv[5], nullptr), std::strtof(argv[6], nullptr), std::strtof(argv[7], nullptr)};
  std::string prefix = argv[8];

  PlanParams params;
  auto route = Navigation::Plan(map, start, goal, params);
  std::printf("%s\n", route.info.c_str());
  for (size_t i = 0; i + 1 < route.points.size(); ++i) {
    const auto &a = route.points[i], &b = route.points[i + 1];
    float clear = map.SegmentClear(a.x, a.y, a.z, b.x, b.y, b.z);
    if (clear < 1.0f) {
      std::printf("route leg %zu (%.1f %.1f %.1f -> %.1f %.1f %.1f) is free for %.2f of its length\n", i, a.x, a.y, a.z, b.x, b.y, b.z, clear);
    }
  }
  params.racingLine = true;
  if (race) {
    Navigation::RacePreset(params);
  }
  if (const char *v = std::getenv("NAVPLAN_PARAMS"); v && !SetParams(params, v)) {
    return 2;
  }
  if (const char *v = std::getenv("NAVPLAN_VEHICLE")) {
    std::string error;
    if (!params.vehicle.Parse(v, error)) {
      std::fprintf(stderr, "NAVPLAN_VEHICLE: %s\n", error.c_str());
      return 2;
    }
  }
  std::printf("vehicle %s\n", params.vehicle.Describe().c_str());
  // as the Autopilot plans it: with its retries when the first search fails
  auto line = Navigation::PlanWithRetries(map, start, goal, params, [](const Navigation::PlanResult &aAttempt) {
    if (!aAttempt.ok) {
      std::printf("  attempt: %s\n", aAttempt.info.c_str());
    }
  });
  std::printf("%s\n", line.info.c_str());
  double milliseconds = line.milliseconds;
  if (std::getenv("NAVPLAN_REPEAT")) {
    auto again = Navigation::PlanWithRetries(map, start, goal, params);
    std::printf("again: %s\n", again.info.c_str());
    milliseconds = again.milliseconds;
  }
  for (size_t i = 0; i + 1 < line.rejectedLine.size(); ++i) {
    const auto &a = line.rejectedLine[i], &b = line.rejectedLine[i + 1];
    float forward = map.SegmentClear(a.x, a.y, a.z, b.x, b.y, b.z);
    if (forward < 1.0f) {
      float backward = map.SegmentClear(b.x, b.y, b.z, a.x, a.y, a.z);
      std::printf("line leg %zu (%.1f %.1f %.1f -> %.1f %.1f %.1f): free %.2f forward, %.2f back, ends free %d %d\n", i,
                  a.x, a.y, a.z, b.x, b.y, b.z, forward, backward, map.IsFree(a.x, a.y, a.z), map.IsFree(b.x, b.y, b.z));
    }
  }
  const auto &points = line.rejectedLine.empty() ? line.points : line.rejectedLine;
  if (!Write(prefix + "_route.csv", route.points) || !Write(prefix + "_line.csv", points)) {
    std::fprintf(stderr, "could not write %s_*.csv\n", prefix.c_str());
    return 1;
  }
  // the speed the car flies the line at, and its clearance
  std::vector<float> xs, ys, zs;
  for (const auto &p : points) {
    xs.push_back(p.x);
    ys.push_back(p.y);
    zs.push_back(p.z);
  }
  auto profile = Navigation::Profile(params.vehicle, xs, ys, zs);
  float time = 0.0f;
  if (FILE *f = std::fopen((prefix + "_profile.csv").c_str(), "w")) {
    std::fprintf(f, "d,x,y,z,v,limit,why,clearance\n");
    size_t j = 0;
    float s0 = 0.0f;
    for (size_t i = 0; i < profile.distance.size(); ++i) {
      // the point at this distance along the line
      const float d = profile.distance[i];
      while (j + 2 < points.size() && s0 + Dist(points[j], points[j + 1]) < d) {
        s0 += Dist(points[j], points[j + 1]);
        ++j;
      }
      const float leg = std::max(Dist(points[j], points[j + 1]), 1e-6f), t = std::clamp((d - s0) / leg, 0.0f, 1.0f);
      const float x = points[j].x + t * (points[j + 1].x - points[j].x), y = points[j].y + t * (points[j + 1].y - points[j].y),
                  z = points[j].z + t * (points[j + 1].z - points[j].z);
      std::fprintf(f, "%.1f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,%.0f\n", d, x, y, z, profile.speed[i], profile.limit[i],
                   Why(profile.why[i]), Clearance(map, x, y, z));
      if (i > 0) {
        time += (d - profile.distance[i - 1]) / std::max(0.5f * (profile.speed[i] + profile.speed[i - 1]), 0.5f);
      }
    }
    std::fclose(f);
  }
  std::printf("metrics {\"route_ok\": %s, \"line_ok\": %s, \"racing_line\": %s, \"length\": %.1f, \"time\": %.2f, "
              "\"top\": %.1f, \"ms\": %.1f}\n",
              route.ok ? "true" : "false", line.ok ? "true" : "false", line.rejectedLine.empty() && line.ok ? "true" : "false",
              profile.distance.empty() ? 0.0f : profile.distance.back(), time, profile.top, milliseconds);
  return route.ok && line.ok ? 0 : 1;
}
