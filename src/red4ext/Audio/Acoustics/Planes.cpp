#include "Planes.hpp"

#include <cmath>

namespace Acoustics::Planes {

namespace {

float Dot(const Vec3 &a, const Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Minus(const Vec3 &a, const Vec3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 Normalized(const Vec3 &a) {
  const float n = std::sqrt(Dot(a, a));
  return n > 1e-6f ? Vec3{a.x / n, a.y / n, a.z / n} : a;
}

} // namespace

Side SideOf(const Vec3 &aDirection, const Vec3 &aRightDir) {
  constexpr float kDiagonal = 0.7071f;
  if (aDirection.z < -kDiagonal) {
    return Floor;
  }
  if (aDirection.z > kDiagonal) {
    return Ceiling;
  }
  return Dot(aDirection, aRightDir) >= 0 ? Right : Left;
}

Cache::Cache(const Settings &aSettings) : settings(aSettings), hits(aSettings.capacity ? aSettings.capacity : 1) {}

void Cache::Add(const Hit &aHit) {
  if (count == hits.size()) {
    hits[start] = aHit;
    start = (start + 1) % hits.size();
    return;
  }
  hits[(start + count) % hits.size()] = aHit;
  ++count;
}

void Cache::Expire(float aNow, const Vec3 &aCar) {
  // kept hits move down in order, over those dropped
  size_t kept = 0;
  for (size_t i = 0; i < count; ++i) {
    const Hit h = At(i);
    const Vec3 d = Minus(h.position, aCar);
    if (aNow - h.time > settings.maxAge || Dot(d, d) > settings.maxDistance * settings.maxDistance) {
      continue;
    }
    hits[(start + kept) % hits.size()] = h;
    ++kept;
  }
  count = kept;
}

std::optional<Plane> Cache::Fit(Side aSide, const Vec3 &aCar, const Vec3 &aRightDir) const {
  std::vector<const Hit *> candidates;
  candidates.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    const Hit &h = At(i);
    if (SideOf(Normalized(Minus(h.position, aCar)), aRightDir) == aSide) {
      candidates.push_back(&h);
    }
  }
  int bestInliers = 0;
  float bestOff = 0;
  const Hit *best = nullptr;
  for (const Hit *seed : candidates) {
    int inliers = 0;
    for (const Hit *h : candidates) {
      if (std::fabs(Dot(Minus(h->position, seed->position), seed->normal)) <= settings.inlierDistance &&
          Dot(h->normal, seed->normal) >= settings.normalAgree) {
        ++inliers;
      }
    }
    const float off = std::fabs(Dot(Minus(aCar, seed->position), seed->normal));
    if (inliers > bestInliers || (inliers == bestInliers && best && off < bestOff)) {
      bestInliers = inliers;
      bestOff = off;
      best = seed;
    }
  }
  if (!best || bestInliers < settings.minInliers) {
    return std::nullopt;
  }
  Vec3 normal{}, point{};
  for (const Hit *h : candidates) {
    if (std::fabs(Dot(Minus(h->position, best->position), best->normal)) <= settings.inlierDistance &&
        Dot(h->normal, best->normal) >= settings.normalAgree) {
      normal = {normal.x + h->normal.x, normal.y + h->normal.y, normal.z + h->normal.z};
      point = {point.x + h->position.x, point.y + h->position.y, point.z + h->position.z};
    }
  }
  const float n = float(bestInliers);
  point = {point.x / n, point.y / n, point.z / n};
  normal = Normalized(normal);
  if (Dot(Minus(aCar, point), normal) < 0) {
    normal = {-normal.x, -normal.y, -normal.z};
  }
  return Plane{point, normal, bestInliers};
}

} // namespace Acoustics::Planes
