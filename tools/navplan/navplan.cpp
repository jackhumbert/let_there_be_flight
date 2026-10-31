// Offline route planning on navmap.bin with the plugin's own planner (src/red4ext/Navigation), for
// tuning and plotting routes without the game. Plans start -> goal twice, the route pulled straight
// alone and then the line the Autopilot flies, and writes next to the given prefix:
//   <prefix>_route.csv    x,y,z: the route pulled straight (the search's corners, before the line)
//   <prefix>_line.csv     x,y,z: the line
//   <prefix>_profile.csv  d,t,x,y,z,v,why,clearance: the line every 2 m with the speed and time the
//                         car flies it with (Navigation/Trajectory.hpp), what sets the speed there
//                         and the distance to the nearest blocked space (up to 10 m)
//   <prefix>_trajectory.csv  the full trajectory: velocity, acceleration, along / lateral m/s2
// and prints one line "metrics {json}" for the tuner (tools/navplan/tune.py).
//
//   navplan <navmap.bin> <sx> <sy> <sz> <gx> <gy> <gz> <out prefix>
//
// Environment:
//   NAVPLAN_RACE=1          the line as the Autopilot plans it (FlightModeAutopilot
//                           race route fields and FlightNavMap::PlanRaceAsync); off: PlanParams defaults
//   NAVPLAN_PARAMS=a=1,b=2  PlanParams fields by name (the table below), applied after the preset
//   NAVPLAN_VEHICLE=a=1,..  the car (VehicleSpec fields by name; tools/navplan/vehicles.json names cars)
//   NAVPLAN_LINE=<csv>     build the trajectory on this line (x,y,z columns) instead of planning one
//   NAVPLAN_REPEAT=1        plan the line again: what a later plan costs once its doorways are known
//   NAVPLAN_AHEAD=h,m       plan ahead (docs/design/trajectory.md step 7): a first leg of h metres
//                           along the doorway route, the rest planned from where the car will be on it
//                           when a full plan's time plus m seconds have passed, the two spliced; prints
//                           "ahead {json}" (time to move, arrival against one full plan, late, dips)
//                           and writes <prefix>_ahead.csv
//   NAVPLAN_DOORWAY_COUNT=x0,y0,x1,y1  work out the doorways over that area instead, and save them
//
// Build: cmake --build build --target navplan (not part of the default build).

#include "Navigation/NavMap.hpp"
#include "Navigation/PlanAhead.hpp"
#include "Navigation/PlannerCommon.hpp"
#include "Navigation/Planner.hpp"
#include "Navigation/Trajectory.hpp"
#include "Navigation/Vehicle.hpp"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
    F(lineTurnRounds), F(lineTurnWeightMax), F(lineStep), F(lineVerticalWeight), F(directMargin), F(maxCruise), F(slotDetourLoss), F(slotDetourShare), F(slotDetourLossMin), F(slotAvoidRadius),
    F(slotAvoidCost), F(lineSettle),
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

// The plan-ahead sequence as the Autopilot flies it (NAVPLAN_AHEAD; Navigation/PlanAhead.hpp): the
// first leg, the rest from its join, spliced, and the trajectory of the whole.
struct Ahead {
  bool ok = false, partial = false, late = false, shortLeg = false;
  double leadMs = 0.0, restMs = 0.0;
  float joinT = 0.0f, joinS = 0.0f, duration = 0.0f, dips = 0.0f, length = 0.0f;
  std::vector<Navigation::Vec3> points;
};

Ahead PlanAhead(const Navigation::NavMap &aMap, Navigation::Vec3 aStart, Navigation::Vec3 aGoal, const PlanParams &aParams,
                const Navigation::AheadParams &aAhead) {
  Ahead out;
  auto leg = Navigation::PlanFirstLeg(aMap, aStart, aGoal, aParams, aAhead);
  out.leadMs = leg.plan.milliseconds;
  out.partial = leg.plan.partial;
  out.shortLeg = leg.shortLeg;
  if (!leg.plan.ok || leg.plan.points.size() < 2) {
    return out;
  }
  if (!leg.plan.partial) {
    out.points = leg.plan.points;
  } else {
    out.joinS = leg.joinS;
    out.joinT = leg.joinT;
    auto rest = Navigation::PlanRest(aMap, leg, aGoal, aParams);
    out.restMs = rest.milliseconds;
    out.late = out.restMs / 1000.0 > out.joinT;
    if (!rest.ok || rest.points.size() < 2) {
      return out;
    }
    out.points = Navigation::SpliceRest(leg, rest);
  }
  out.ok = true;
  const auto whole = Navigation::BuildTrajectory(aParams.vehicle, out.points, {}, &aMap);
  out.duration = whole.Duration();
  out.dips = Navigation::DipLoss(whole);
  out.length = whole.samples.empty() ? 0.0f : whole.samples.back().s;
  return out;
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
      // the raw set to the out prefix (tools/navmap_bake/doorways.py packs it into navmap.bin)
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
  params.line = false; // the pulled route alone first, to compare
  auto route = Navigation::Plan(map, start, goal, params);
  std::printf("%s\n", route.info.c_str());
  for (size_t i = 0; i + 1 < route.points.size(); ++i) {
    const auto &a = route.points[i], &b = route.points[i + 1];
    float clear = map.SegmentClear(a.x, a.y, a.z, b.x, b.y, b.z);
    if (clear < 1.0f) {
      std::printf("route leg %zu (%.1f %.1f %.1f -> %.1f %.1f %.1f) is free for %.2f of its length\n", i, a.x, a.y, a.z, b.x, b.y, b.z, clear);
    }
  }
  params.line = true;
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
  // as the Autopilot plans it: with its retries when the first search fails; or a line given as a
  // CSV (NAVPLAN_LINE, x,y,z columns: a RouteDump from the game) to build its trajectory alone
  Navigation::PlanResult line;
  if (const char *given = std::getenv("NAVPLAN_LINE")) {
    if (FILE *f = std::fopen(given, "r")) {
      char row[512];
      std::fgets(row, sizeof(row), f); // header
      int32_t x = -1, y = -1, z = -1, col = 0;
      for (char *c = std::strtok(row, ",\r\n"); c; c = std::strtok(nullptr, ",\r\n"), ++col) {
        x = std::strcmp(c, "x") == 0 ? col : x;
        y = std::strcmp(c, "y") == 0 ? col : y;
        z = std::strcmp(c, "z") == 0 ? col : z;
      }
      while (std::fgets(row, sizeof(row), f)) {
        float v[32] = {};
        col = 0;
        for (char *c = std::strtok(row, ","); c && col < 32; c = std::strtok(nullptr, ","), ++col) {
          v[col] = std::strtof(c, nullptr);
        }
        line.points.push_back({v[x], v[y], v[z]});
      }
      std::fclose(f);
    }
    line.ok = line.points.size() >= 2;
    line.info = "line from " + std::string(given) + ": " + std::to_string(line.points.size()) + " points";
  } else {
    line = Navigation::PlanRace(map, start, goal, params, [](const Navigation::PlanResult &aAttempt) {
      if (!aAttempt.ok) {
        std::printf("  attempt: %s\n", aAttempt.info.c_str());
      }
    });
  }
  std::printf("%s\n", line.info.c_str());
  double milliseconds = line.milliseconds;
  if (std::getenv("NAVPLAN_REPEAT")) {
    auto again = Navigation::PlanRace(map, start, goal, params);
    std::printf("again: %s\n", again.info.c_str());
    milliseconds = again.milliseconds;
  }
  for (size_t i = 0; i + 1 < line.points.size(); ++i) {
    const auto &a = line.points[i], &b = line.points[i + 1];
    float forward = map.SegmentClear(a.x, a.y, a.z, b.x, b.y, b.z);
    if (forward < 1.0f && !Navigation::detail::LegFree(map, a, b)) {
      float backward = map.SegmentClear(b.x, b.y, b.z, a.x, a.y, a.z);
      std::printf("line leg %zu (%.1f %.1f %.1f -> %.1f %.1f %.1f): free %.2f forward, %.2f back, ends free %d %d\n", i,
                  a.x, a.y, a.z, b.x, b.y, b.z, forward, backward, map.IsFree(a.x, a.y, a.z), map.IsFree(b.x, b.y, b.z));
    }
  }
  const auto &points = line.points;
  if (!Write(prefix + "_route.csv", route.points) || !Write(prefix + "_line.csv", points)) {
    std::fprintf(stderr, "could not write %s_*.csv\n", prefix.c_str());
    return 1;
  }
  // the trajectory the car flies the line with (Trajectory.hpp), and its clearance
  auto trajectory = Navigation::BuildTrajectory(params.vehicle, points, {}, &map);
  if (const char *v = std::getenv("NAVPLAN_AHEAD")) {
    float horizon = 0.0f, margin = 1.0f;
    std::sscanf(v, "%f,%f", &horizon, &margin);
    Navigation::AheadParams ahead;
    ahead.horizon = horizon;
    ahead.margin = margin;
    ahead.restGuess = float(milliseconds / 1000.0); // a full plan's time: the rest's is shorter
    auto a = PlanAhead(map, start, goal, params, ahead);
    Write(prefix + "_ahead.csv", a.points);
    std::printf("ahead {\"ok\": %s, \"partial\": %s, \"late\": %s, \"short\": %s, \"horizon\": %.0f, \"margin\": %.2f, "
                "\"lead_ms\": %.1f, \"rest_ms\": %.1f, \"join_t\": %.2f, \"join_s\": %.1f, \"duration\": %.2f, \"dips\": %.1f, "
                "\"length\": %.1f, \"full_ms\": %.1f, \"full_duration\": %.2f, \"full_dips\": %.1f, "
                "\"move_s\": %.2f, \"arrive_s\": %.2f, \"full_arrive_s\": %.2f}\n",
                a.ok ? "true" : "false", a.partial ? "true" : "false", a.late ? "true" : "false", a.shortLeg ? "true" : "false",
                horizon, margin, a.leadMs, a.restMs, a.joinT, a.joinS, a.duration, a.dips, a.length, milliseconds,
                trajectory.Duration(), Navigation::DipLoss(trajectory), a.leadMs / 1000.0, a.leadMs / 1000.0 + a.duration,
                milliseconds / 1000.0 + trajectory.Duration());
  }
  for (const auto &v : trajectory.violations) {
    std::printf("trajectory: %s\n", v.c_str());
  }
  if (FILE *f = std::fopen((prefix + "_profile.csv").c_str(), "w")) {
    std::fprintf(f, "d,t,x,y,z,v,why,clearance\n");
    for (const auto &q : trajectory.samples) {
      std::fprintf(f, "%.1f,%.3f,%.2f,%.2f,%.2f,%.2f,%s,%.0f\n", q.s, q.t, q.position.x, q.position.y, q.position.z, q.speed,
                   Navigation::LimitName(q.limit), Clearance(map, q.position.x, q.position.y, q.position.z));
    }
    std::fclose(f);
  }
  if (FILE *f = std::fopen((prefix + "_trajectory.csv").c_str(), "w")) {
    std::fprintf(f, "s,t,x,y,z,vx,vy,vz,ax,ay,az,speed,along,lateral,limit,yaw,pitch,roll,room,room_loss\n");
    for (const auto &q : trajectory.samples) {
      std::fprintf(f, "%.1f,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,%.4f,%.4f,%.4f,%.1f,%.1f\n", q.s, q.t, q.position.x,
                   q.position.y, q.position.z, q.velocity.x, q.velocity.y, q.velocity.z, q.acceleration.x, q.acceleration.y,
                   q.acceleration.z, q.speed, q.along, q.lateral, Navigation::LimitName(q.limit), q.yaw, q.pitch, q.roll, q.room, q.roomLoss);
    }
    std::fclose(f);
  }
  const float length = trajectory.samples.empty() ? 0.0f : trajectory.samples.back().s, time = trajectory.Duration();
  std::printf("metrics {\"route_ok\": %s, \"line_ok\": %s, \"repaired\": %d, \"length\": %.1f, \"time\": %.2f, "
              "\"top\": %.1f, \"ms\": %.1f}\n",
              route.ok ? "true" : "false", line.ok ? "true" : "false", line.repaired,
              length, time, trajectory.top, milliseconds);
  return route.ok && line.ok ? 0 : 1;
}
