#pragma once

// Roads: the game's traffic lanes as polylines (all.lane_polygons of the player's GPS network), loaded
// from roads.tsv in the plugin folder when it is there: one lane per line, x y z per point. Free roam
// keeps its legs near them when freeRoamGuideCost is set (PlanParams::guide). Read once at plugin load
// and read-only afterwards, so queries need no locking.

#include "Planner.hpp"

#include <filesystem>
#include <vector>

namespace Navigation {

class Roads {
public:
  static Roads &Get();

  bool Load(const std::filesystem::path &aPath);
  bool IsLoaded() const { return !lanes.empty(); }
  // the lanes with a point within the box round aA and aB grown by aMargin, appended to aOut as
  // PlanParams::guide wants them (a NaN point after each)
  void Near(Vec3 aA, Vec3 aB, float aMargin, std::vector<Vec3> &aOut) const;

private:
  struct Lane {
    float x0, y0, x1, y1;
    std::vector<Vec3> points;
  };
  std::vector<Lane> lanes;
};

} // namespace Navigation
