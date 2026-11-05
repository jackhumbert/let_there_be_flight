#include "Landing.hpp"

#include "Vec3Ops.hpp"

#include <algorithm>
#include <cmath>

namespace Navigation {

namespace {
// the script's angle constants, so the rings land on the same columns
constexpr float kEighth = 0.7853982f;
constexpr float kTurn = 6.2831853f;
constexpr float kHalfTurn = 3.1415927f;

float Distance2D(const Vec3 &aA, const Vec3 &aB) { return std::hypot(aA.x - aB.x, aA.y - aB.y); }
float DistanceSquared2D(const Vec3 &aA, const Vec3 &aB) {
  const float dx = aA.x - aB.x, dy = aA.y - aB.y;
  return dx * dx + dy * dy;
}
} // namespace

bool ShaftClear(const World &aWorld, const Vec3 &aC, float aFloor, float aRoom, float aShaft) {
  for (float ring = 2.0f; ring <= aShaft; ring += 2.0f) {
    for (int i = 0; i < 8; ++i) {
      const float angle = kEighth * float(i);
      const Vec3 p{aC.x + ring * std::cos(angle), aC.y + ring * std::sin(angle), aFloor + 1.0f};
      Gap g{};
      if (!GapAt(aWorld, p, g) || g.top < aFloor + aRoom) {
        return false;
      }
    }
  }
  return true;
}

bool SpotTaken(const World &aWorld, const Vec3 &aSpot) {
  for (float ring = 0.0f; ring <= 4.0f; ring += 2.0f) {
    const int count = ring > 0.0f ? 8 : 1;
    for (int i = 0; i < count; ++i) {
      const float angle = kEighth * float(i);
      const float x = aSpot.x + ring * std::cos(angle), y = aSpot.y + ring * std::sin(angle);
      if (aWorld.VehicleRay({x, y, aSpot.z + 6.0f}, {x, y, aSpot.z - 2.0f})) {
        return true;
      }
    }
  }
  return false;
}

bool IsFlat(const World &aWorld, const Vec3 &aC, float aFloor, float aFlat) {
  float ring = 2.0f;
  while (ring <= aFlat + 0.01f) {
    for (int i = 0; i < 8; ++i) {
      const float angle = kEighth * float(i);
      const Vec3 p{aC.x + ring * std::cos(angle), aC.y + ring * std::sin(angle), aFloor + 1.0f};
      Gap g{};
      if (!GapAt(aWorld, p, g) || std::fabs(g.bottom - aFloor) > 0.5f) {
        return false;
      }
    }
    ring = ring + 2.0f > aFlat && ring < aFlat ? aFlat : ring + 2.0f;
  }
  return true;
}

bool SpotSupported(const World &aWorld, const Vec3 &aSpot, float aFootprint) {
  int hits = 0;
  float lo = 1000000.0f, hi = -1000000.0f;
  for (int i = -1; i < 8; ++i) {
    const float angle = kEighth * float(i);
    const float r = i < 0 ? 0.0f : aFootprint;
    const float x = aSpot.x + r * std::cos(angle), y = aSpot.y + r * std::sin(angle);
    Vec3 hit{};
    if (aWorld.GroundRay({x, y, aSpot.z + 2.0f}, {x, y, aSpot.z - 6.0f}, hit)) {
      ++hits;
      lo = std::min(lo, hit.z);
      hi = std::max(hi, hit.z);
    }
  }
  return hits == 0 || (hits == 9 && hi - lo < 0.6f);
}

bool FindLandingSpotWithRoom(const World &aWorld, const AutopilotParams &aParams, const Vec3 &aTarget, float aRoom,
                             const KeepClear &aKeep, GroundSpot &aOut) {
  Gap gap{};
  if (!GapAt(aWorld, aTarget, gap) || aTarget.z - gap.bottom > 4.0f) {
    return false; // a point in the air: hover there
  }
  const float level = gap.bottom;
  struct Candidate {
    float d;
    Vec3 spot;
    float top;
  };
  std::vector<Candidate> ring;
  for (int r = 0; r <= aParams.landingSearch; ++r) {
    const int count = r == 0 ? 1 : 6 * r;
    ring.clear();
    for (int i = 0; i < count; ++i) {
      const float angle = kTurn * float(i) / float(count);
      const Vec3 c{aTarget.x + 2.0f * float(r) * std::cos(angle), aTarget.y + 2.0f * float(r) * std::sin(angle),
                   level + 1.0f};
      Gap g{};
      if (GapAt(aWorld, c, g) && std::fabs(g.bottom - level) < 3.0f && g.top - g.bottom > aRoom &&
          IsFlat(aWorld, c, g.bottom, aParams.landingFlat) && ShaftClear(aWorld, c, g.bottom, aRoom, aParams.landingShaft) &&
          (aKeep.radius <= 0.0f || Distance2D(c, aKeep.at) >= aKeep.radius)) {
        ring.push_back({DistanceSquared2D(c, aTarget), {c.x, c.y, g.bottom}, g.top});
      }
    }
    // nearest first, the first of equals as the script keeps it
    std::stable_sort(ring.begin(), ring.end(), [](const Candidate &a, const Candidate &b) { return a.d < b.d; });
    for (const auto &c : ring) {
      if (!SpotTaken(aWorld, c.spot) && SpotSupported(aWorld, c.spot, aParams.landingFootprint)) {
        aOut = {c.spot, c.top};
        return true;
      }
    }
  }
  return false;
}

bool FindLandingSpot(const World &aWorld, const AutopilotParams &aParams, const Vec3 &aTarget, const KeepClear &aKeep,
                     GroundSpot &aOut) {
  return FindLandingSpotWithRoom(aWorld, aParams, aTarget, aParams.approachHeight + 4.0f, aKeep, aOut) ||
         FindLandingSpotWithRoom(aWorld, aParams, aTarget, 6.0f, aKeep, aOut);
}

int32_t FindLandingPad(const World &aWorld, const AutopilotParams &aParams, const Vec3 &aTarget) {
  const auto &pads = aWorld.Pads();
  const int32_t pad = NearestPad(pads, aTarget.x, aTarget.y, aParams.padSnapDistance);
  if (pad >= 0 && std::fabs(aTarget.z - pads[pad].z) < aParams.padSnapLevel) {
    return pad;
  }
  Gap gap{};
  if (aParams.padRange > 0.0f && GapAt(aWorld, aTarget, gap) && aTarget.z - gap.bottom <= 4.0f) {
    return NearestPad(pads, aTarget.x, aTarget.y, aParams.padRange);
  }
  return -1;
}

Vec3 PadHeading(const Helipad &aPad, const Vec3 &aArrival) {
  const float yaw = aPad.yaw * kHalfTurn / 180.0f;
  const Vec3 d{-std::sin(yaw), std::cos(yaw), 0.0f};
  return V::Dot(d, aArrival) >= 0.0f ? d : V::Mul(d, -1.0f);
}

Vec3 LandingHeading(const World &aWorld, const Vec3 &aSpot, const Vec3 &aArrival) {
  const Vec3 up{aSpot.x, aSpot.y, aSpot.z + 1.0f};
  const Vec3 arrive = V::Normalize({aArrival.x, aArrival.y, 0.0f});
  Vec3 best = arrive;
  float bestScore = 0.5f;
  for (int i = 0; i < 8; ++i) {
    const float angle = kHalfTurn * float(i) / 8.0f;
    const Vec3 d{std::cos(angle), std::sin(angle), 0.0f};
    const float open = aWorld.SegmentClear(up, V::Add(up, V::Mul(d, 25.0f))) +
                       aWorld.SegmentClear(up, V::Sub(up, V::Mul(d, 25.0f)));
    const float score = open + 0.15f * std::fabs(V::Dot(d, arrive));
    if (score > bestScore) {
      bestScore = score;
      best = V::Dot(d, arrive) >= 0.0f ? d : V::Mul(d, -1.0f);
    }
  }
  return best;
}

bool ShiftRouteEnd(const World &aWorld, const Route &aRoute, int32_t aSegment, const Vec3 &aTo, float aSpotZ,
                   float aSpan, std::vector<Vec3> &aMoved) {
  const int32_t n = aRoute.Size();
  if (n < 3) {
    return false;
  }
  const float total = aRoute.Length();
  const Vec3 end = aRoute.Point(n - 1);
  const Vec3 offset{aTo.x - end.x, aTo.y - end.y, aTo.z - aSpotZ};
  std::vector<Vec3> moved;
  moved.reserve(size_t(n));
  for (int32_t i = 0; i < n; ++i) {
    Vec3 q = aRoute.Point(i);
    const float left = total - aRoute.Distance(i);
    if (i > aSegment + 1 && left < aSpan) {
      float f = 1.0f - left / aSpan;
      f = f * f * (3.0f - 2.0f * f);
      q = V::Add(q, V::Mul(offset, f));
      if (!aWorld.IsFree(q)) {
        return false;
      }
    }
    moved.push_back(q);
  }
  aMoved = std::move(moved);
  return true;
}

} // namespace Navigation
