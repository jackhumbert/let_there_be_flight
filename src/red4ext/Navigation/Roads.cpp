#include "Roads.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

namespace Navigation {

Roads &Roads::Get() {
  static Roads roads;
  return roads;
}

bool Roads::Load(const std::filesystem::path &aPath) {
  std::ifstream in(aPath);
  if (!in) {
    return false;
  }
  lanes.clear();
  size_t points = 0;
  std::string row;
  while (std::getline(in, row)) {
    std::istringstream fields(row);
    Lane lane{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
              -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), {}};
    Vec3 p;
    while (fields >> p.x >> p.y >> p.z) {
      lane.points.push_back(p);
      lane.x0 = std::min(lane.x0, p.x);
      lane.y0 = std::min(lane.y0, p.y);
      lane.x1 = std::max(lane.x1, p.x);
      lane.y1 = std::max(lane.y1, p.y);
    }
    if (lane.points.size() >= 2) {
      points += lane.points.size();
      lanes.push_back(std::move(lane));
    }
  }
  spdlog::info("[Roads] loaded {} lanes, {} points", lanes.size(), points);
  return IsLoaded();
}

void Roads::Near(Vec3 aA, Vec3 aB, float aMargin, std::vector<Vec3> &aOut) const {
  const float x0 = std::min(aA.x, aB.x) - aMargin, x1 = std::max(aA.x, aB.x) + aMargin;
  const float y0 = std::min(aA.y, aB.y) - aMargin, y1 = std::max(aA.y, aB.y) + aMargin;
  const float nan = std::numeric_limits<float>::quiet_NaN();
  for (const Lane &l : lanes) {
    if (l.x1 < x0 || l.x0 > x1 || l.y1 < y0 || l.y0 > y1) {
      continue;
    }
    aOut.insert(aOut.end(), l.points.begin(), l.points.end());
    aOut.push_back({nan, nan, nan});
  }
}

} // namespace Navigation
