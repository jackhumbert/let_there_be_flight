#include "World.hpp"

#include <algorithm>
#include <cmath>

namespace Navigation {

bool GapAt(const World &aWorld, const Vec3 &aP, Gap &aGap) {
  for (const auto &g : aWorld.Gaps(aP)) {
    if (aP.z < g.top) {
      aGap = g;
      return true;
    }
  }
  return false;
}

namespace {
// indices of the items within aRadius of (x, y) in the horizontal, nearest first
template <typename T> std::vector<int32_t> InRange(const std::vector<T> &aItems, float aX, float aY, float aRadius) {
  std::vector<std::pair<float, int32_t>> found;
  const float range = aRadius * aRadius;
  for (size_t i = 0; i < aItems.size(); ++i) {
    const float dx = aItems[i].x - aX, dy = aItems[i].y - aY;
    const float d = dx * dx + dy * dy;
    if (d <= range) {
      found.emplace_back(d, int32_t(i));
    }
  }
  std::sort(found.begin(), found.end());
  std::vector<int32_t> out;
  out.reserve(found.size());
  for (const auto &[d, i] : found) {
    out.push_back(i);
  }
  return out;
}
} // namespace

int32_t NearestPad(const std::vector<Helipad> &aPads, float aX, float aY, float aMaxDistance) {
  int32_t best = -1;
  float bestDistance = aMaxDistance * aMaxDistance;
  for (size_t i = 0; i < aPads.size(); ++i) {
    const float dx = aPads[i].x - aX, dy = aPads[i].y - aY;
    const float d = dx * dx + dy * dy;
    if (d <= bestDistance) {
      bestDistance = d;
      best = int32_t(i);
    }
  }
  return best;
}

std::vector<int32_t> PadsInRange(const std::vector<Helipad> &aPads, float aX, float aY, float aRadius) {
  return InRange(aPads, aX, aY, aRadius);
}

std::vector<int32_t> GatesInRange(const std::vector<Gate> &aGates, float aX, float aY, float aRadius) {
  return InRange(aGates, aX, aY, aRadius);
}

float RayBox(const Vec3 &aFrom, const Vec3 &aTo, const Box &aBox) {
  const float a[3] = {aFrom.x, aFrom.y, aFrom.z}, b[3] = {aTo.x, aTo.y, aTo.z};
  const float mn[3] = {aBox.first.x, aBox.first.y, aBox.first.z}, mx[3] = {aBox.second.x, aBox.second.y, aBox.second.z};
  float tmin = 0.0f, tmax = 1.0f;
  for (int k = 0; k < 3; ++k) {
    const float d = b[k] - a[k];
    if (std::fabs(d) < 1e-9f) {
      if (a[k] < mn[k] || a[k] > mx[k]) {
        return -1.0f;
      }
      continue;
    }
    float t1 = (mn[k] - a[k]) / d, t2 = (mx[k] - a[k]) / d;
    if (t1 > t2) {
      std::swap(t1, t2);
    }
    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    if (tmin > tmax) {
      return -1.0f;
    }
  }
  return tmin;
}

bool MapWorld::VehicleRay(const Vec3 &aFrom, const Vec3 &aTo) const {
  return std::any_of(vehicles.begin(), vehicles.end(), [&](const Box &b) { return RayBox(aFrom, aTo, b) >= 0.0f; });
}

bool MapWorld::GroundRay(const Vec3 &aFrom, const Vec3 &aTo, Vec3 &aHit) const {
  float best = 2.0f;
  for (const auto &b : ground) {
    const float t = RayBox(aFrom, aTo, b);
    if (t >= 0.0f && t < best) {
      best = t;
    }
  }
  if (best > 1.0f) {
    return false;
  }
  aHit = {aFrom.x + (aTo.x - aFrom.x) * best, aFrom.y + (aTo.y - aFrom.y) * best, aFrom.z + (aTo.z - aFrom.z) * best};
  return true;
}

} // namespace Navigation
