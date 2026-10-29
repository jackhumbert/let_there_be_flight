// Offline route planning on navmap.bin with the plugin's own planner (src/red4ext/Navigation), for
// tuning and plotting routes without the game. Plans start -> goal twice, the plain route and the
// racing line, and writes each as CSV (x,y,z) next to the given prefix; tools/navplan/plot.py
// draws them over the map from above.
//
//   navplan <navmap.bin> <sx> <sy> <sz> <gx> <gy> <gz> <out prefix> [line height] [line climb cost]
//
// The optional pair overrides preferredHeight / climbCost for the racing line plan.
//
// Build: cmake --build build --target navplan (not part of the default build).

#include "Navigation/NavMap.hpp"
#include "Navigation/Planner.hpp"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
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
} // namespace

int main(int argc, char **argv) {
  if (argc != 9 && argc != 11) {
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
  if (const char *area = std::getenv("NAVPLAN_DOORWAY_COUNT")) {
    // x0,y0,x1,y1: work out the doorways over the area with the Race's costs, and count them
    float x0, y0, x1, y1;
    if (std::sscanf(area, "%f,%f,%f,%f", &x0, &y0, &x1, &y1) == 4) {
      Navigation::PlanParams race;
      race.minPassage = 12.0f; // FlightModeAutopilot.raceMinPassage
      race.preferredHeight = 40.0f;
      race.climbCost = 0.5f;
      auto n = Navigation::WorkOutDoorways(map, race, x0, y0, x1, y1);
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

  Navigation::PlanParams params;
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
  if (argc == 11) {
    params.preferredHeight = std::strtof(argv[9], nullptr);
    params.climbCost = std::strtof(argv[10], nullptr);
  }
  // tuning without a rebuild: NAVPLAN_<NAME> overrides the racing line parameter of that name
  if (const char *v = std::getenv("NAVPLAN_LINE_CLEARANCE")) {
    params.lineClearance = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_DOORWAYS")) {
    params.doorways = std::atoi(v) != 0;
  }
  if (const char *v = std::getenv("NAVPLAN_BEARING")) {
    params.bearingCost = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_LINE_PUSH")) {
    params.linePush = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_VERTICAL_WEIGHT")) {
    params.lineVerticalWeight = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_TURN_COST")) {
    params.turnCost = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_MIN_PASSAGE")) {
    params.minPassage = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_HEURISTIC")) {
    params.heuristicWeight = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_UNDER_COST")) {
    params.underCost = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_MAX_GRADIENT")) {
    params.maxGradient = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_CORRIDOR_ITERATIONS")) {
    params.lineCorridorIterations = std::atoi(v);
  }
  if (const char *v = std::getenv("NAVPLAN_CORRIDOR_MARGIN")) {
    params.lineCorridorMargin = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_CURVATURE_RATE")) {
    params.lineCurvatureRate = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_LINE_SPREAD")) {
    params.lineSpread = std::strtof(v, nullptr);
  }
  if (const char *v = std::getenv("NAVPLAN_HEIGHT_ITERATIONS")) {
    params.lineHeightIterations = std::atoi(v);
  }
  if (const char *v = std::getenv("NAVPLAN_LINE_SIDE")) {
    params.lineSide = std::strtof(v, nullptr);
  }
  // as the Autopilot plans it: with its retries when the first search fails
  auto line = Navigation::PlanWithRetries(map, start, goal, params, [](const Navigation::PlanResult &aAttempt) {
    if (!aAttempt.ok) {
      std::printf("  attempt: %s\n", aAttempt.info.c_str());
    }
  });
  std::printf("%s\n", line.info.c_str());
  if (std::getenv("NAVPLAN_REPEAT")) {
    // the same plan again: what a later plan costs once the doorways it needs are worked out
    auto again = Navigation::PlanWithRetries(map, start, goal, params);
    std::printf("again: %s\n", again.info.c_str());
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
  // a rejected racing line is written too, to see where it is blocked
  if (!Write(prefix + "_route.csv", route.points) || !Write(prefix + "_line.csv", line.rejectedLine.empty() ? line.points : line.rejectedLine)) {
    std::fprintf(stderr, "could not write %s_*.csv\n", prefix.c_str());
    return 1;
  }
  return route.ok && line.ok ? 0 : 1;
}
