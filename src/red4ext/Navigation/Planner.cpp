#include "Planner.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <cstring>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace Navigation {

namespace {

constexpr float kInf = std::numeric_limits<float>::infinity();
constexpr float kTopCap = 1000.0f; // finite stand-in for open sky in the height profile

struct GapM {
  float b, t; // metres; t is +infinity for open sky
};

uint64_t MakeKey(int32_t aCx, int32_t aCy, uint32_t aGap, int32_t aWidth) {
  return ((uint64_t(aCy) * uint64_t(aWidth) + uint64_t(aCx)) << 8) | aGap;
}
int32_t KeyCx(uint64_t aKey, int32_t aWidth) { return int32_t((aKey >> 8) % uint64_t(aWidth)); }
int32_t KeyCy(uint64_t aKey, int32_t aWidth) { return int32_t((aKey >> 8) / uint64_t(aWidth)); }
uint32_t KeyGap(uint64_t aKey) { return uint32_t(aKey & 0xFF); }

GapM GapAt(NavMap::View &aView, int32_t aCx, int32_t aCy, uint32_t aGap) {
  const uint16_t *g;
  uint32_t n = aView.Column(aCx, aCy, g);
  if (aGap >= n) {
    return {kInf, -kInf};
  }
  return {aView.Map().VoxelZ(g[2 * aGap]), aView.Map().VoxelZ(g[2 * aGap + 1])};
}

// Height the route would like to fly at in a gap: preferredHeight above the floor, or halfway up
// a gap with something overhead if that is lower.
float Preferred(GapM aGap, float aHeight) {
  float z = aGap.b + aHeight;
  if (std::isfinite(aGap.t)) {
    z = std::min(z, 0.5f * (aGap.b + aGap.t));
  }
  return z;
}

// The gap nearest to p (containing it if possible). If p's column has nothing (the car is inside
// the inflated margin of a wall), the nearest column within a few cells is used.
bool Anchor(NavMap::View &aView, Vec3 aP, int32_t &aCx, int32_t &aCy, uint32_t &aGap) {
  const NavMap &map = aView.Map();
  int32_t c0x, c0y;
  if (!map.CellOf(aP.x, aP.y, c0x, c0y)) {
    return false;
  }
  float best = kInf;
  for (int32_t r = 0; r <= 6 && !std::isfinite(best); ++r) {
    for (int32_t dy = -r; dy <= r; ++dy) {
      for (int32_t dx = -r; dx <= r; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != r) {
          continue;
        }
        const uint16_t *g;
        uint32_t n = aView.Column(c0x + dx, c0y + dy, g);
        for (uint32_t i = 0; i < n; ++i) {
          float b = map.VoxelZ(g[2 * i]), t = map.VoxelZ(g[2 * i + 1]);
          float d = aP.z < b ? b - aP.z : (aP.z >= t ? aP.z - t : 0.0f);
          d += float(r) * map.CellSize();
          if (d < best) {
            best = d;
            aCx = c0x + dx;
            aCy = c0y + dy;
            aGap = i;
          }
        }
      }
    }
  }
  return std::isfinite(best);
}

float Clear(const NavMap &aMap, Vec3 aA, Vec3 aB) { return aMap.SegmentClear(aA.x, aA.y, aA.z, aB.x, aB.y, aB.z); }

// The leg and two parallel legs offset sideways by aSide must all be free.
bool ClearWide(const NavMap &aMap, Vec3 aA, Vec3 aB, float aSide) {
  if (Clear(aMap, aA, aB) < 1.0f) {
    return false;
  }
  float dx = aB.x - aA.x, dy = aB.y - aA.y;
  float len = std::hypot(dx, dy);
  if (aSide <= 0.0f || len < 1e-3f) {
    return true;
  }
  float px = -dy / len * aSide, py = dx / len * aSide;
  for (float s : {1.0f, -1.0f}) {
    Vec3 a{aA.x + s * px, aA.y + s * py, aA.z}, b{aB.x + s * px, aB.y + s * py, aB.z};
    if (Clear(aMap, a, b) < 1.0f) {
      return false;
    }
  }
  return true;
}

constexpr int kTurnBase = 4; // cells a bend is judged over (turnCost)

float Distance(Vec3 aA, Vec3 aB) {
  float dx = aB.x - aA.x, dy = aB.y - aA.y, dz = aB.z - aA.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Longest stretch (m) of the polyline that is blocked, sampled every half metre along its whole
// length (so where the legs are cut does not change the answer).
constexpr float kGrazeStep = 0.5f;
constexpr float kGrazeAllowed = 1.0f; // m: the planner's own legs graze this much between their samples
// m, the finished line: moves are checked at SegmentClear's 1 m sampling, the line at half a metre,
// which finds a little more; still half the 3 m the bake grew solids by for the car
constexpr float kGrazeAllowedLine = 1.5f;

// The longest run of blocked space along the polyline, from aSkip metres on (the stretch from a
// car under cover out to the free space is blocked on the map and is flown anyway).
float LongestBlocked(const NavMap &aMap, const std::vector<Vec3> &aPoints, float aSkip = 0.0f) {
  float run = 0.0f, longest = 0.0f, carry = 0.0f, along = 0.0f;
  for (size_t i = 0; i + 1 < aPoints.size(); ++i) {
    Vec3 a = aPoints[i], b = aPoints[i + 1];
    float len = Distance(a, b);
    for (float s = carry; s <= len; s += kGrazeStep) {
      float t = len > 1e-4f ? s / len : 0.0f;
      bool free = along + s < aSkip || aMap.IsFree(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
      run = free ? 0.0f : run + kGrazeStep;
      longest = std::max(longest, run);
      carry = s + kGrazeStep - len;
    }
    along += len;
  }
  return longest;
}

// The leg is free but for a graze no longer than the planner's own legs have.
bool LegFree(const NavMap &aMap, Vec3 aA, Vec3 aB) {
  return Clear(aMap, aA, aB) >= 1.0f || LongestBlocked(aMap, {aA, aB}) <= kGrazeAllowed;
}

// How much of the leg is free, the leg and its two side-offset copies taken together (0..1).
float ClearSides(const NavMap &aMap, Vec3 aA, Vec3 aB, float aSide) {
  float clear = Clear(aMap, aA, aB);
  float dx = aB.x - aA.x, dy = aB.y - aA.y;
  float len = std::hypot(dx, dy);
  if (aSide <= 0.0f || len < 1e-3f || clear < 1.0f) {
    return clear;
  }
  float px = -dy / len * aSide, py = dx / len * aSide;
  for (float s : {1.0f, -1.0f}) {
    clear = std::min(clear, Clear(aMap, {aA.x + s * px, aA.y + s * py, aA.z}, {aB.x + s * px, aB.y + s * py, aB.z}));
  }
  return clear;
}

// Each leg of the polyline cut into equal steps of at most aSpacing, its corners kept: every new
// leg lies on an old one, so a free polyline stays free.
std::vector<Vec3> Resample(const std::vector<Vec3> &aPoints, float aSpacing) {
  std::vector<Vec3> out{aPoints.front()};
  for (size_t i = 0; i + 1 < aPoints.size(); ++i) {
    Vec3 a = aPoints[i], b = aPoints[i + 1];
    int steps = std::max(1, int(std::ceil(Distance(a, b) / aSpacing)));
    for (int k = 1; k <= steps; ++k) {
      float t = float(k) / float(steps);
      out.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t});
    }
  }
  return out;
}

// Racing line: gradient descent on the summed squared second differences of the points (their
// bend), each move kept only where both legs to the neighbours stay free, with lineSide to spare
// or at least no less than before. Points ahead of a corner move out and the apex moves
// in, so the bend spreads over all the room there is. Coarse spacings first (the bend of a long
// corner spreads over a few points per pass there), then finer ones down to lineSpacing.
// The heights the line may take in a gap, at aAlong of aLength metres along it: lineFloorClearance
// over the floor (but within lineFloorEnds of either end, where it takes off and lands) and
// lineCeilingClearance under anything overhead; the middle of the gap where it has not room for
// both. The ceiling was only a push of linePush a pass, which the smoothing undid: west-east went
// under a skybridge a metre from its underside, one point left out of line with the rest, and the
// speed plan braked from 96 to 39 m/s for that kink (2026-10-27).
void Band(const Gap &aGap, float aAlong, float aLength, const PlanParams &aParams, float &aLo, float &aHi) {
  const bool ends = aAlong <= aParams.lineFloorEnds || aLength - aAlong <= aParams.lineFloorEnds;
  aLo = aGap.bottom;
  aHi = std::isfinite(aGap.top) ? aGap.top - aParams.lineCeilingClearance : kInf;
  if (!ends) {
    aLo = aGap.bottom + aParams.lineFloorClearance;
  }
  if (aLo > aHi) {
    aLo = aHi = 0.5f * (aGap.bottom + aGap.top);
  }
}

// The line's heights solved on their own, its path across kept: the smoothest heights (least
// summed squared second difference) within each point's height band (Band: the floor clearance
// below, anything overhead), coarse to fine (every 16th point, then every 8th, ... each coarse
// point held to the band of the stretch it stands for, so a bump between them still counts), the
// first and last two points fixed. The smoothing passes in RacingLine aim at the same but stop
// after a hundred local passes at the fine spacing, so a bump in the floor left a short ramp and a
// crest at its top: from the city centre to Dogtown two 5 m bumps 110 m apart braked the car from
// 60 to 44 m/s and again to 40 (racePushOver, 2026-10-28). A point whose legs are not free at the
// solved height goes back to where it was, with its neighbour.
void SmoothHeights(const NavMap &aMap, std::vector<Vec3> &aPts, const PlanParams &aParams) {
  const size_t n = aPts.size();
  if (aParams.lineHeightIterations <= 0 || n < 9) {
    return;
  }
  std::vector<float> along(n, 0.0f), lo(n), hi(n), z(n);
  for (size_t i = 0; i < n; ++i) {
    along[i] = i ? along[i - 1] + Distance(aPts[i - 1], aPts[i]) : 0.0f;
    z[i] = lo[i] = hi[i] = aPts[i].z;
  }
  for (size_t i = 2; i + 2 < n; ++i) {
    Gap gap;
    if (aMap.FindGap(aPts[i].x, aPts[i].y, aPts[i].z, gap)) {
      Band(gap, along[i], along.back(), aParams, lo[i], hi[i]);
      // the line where it is now is always allowed (a band can sit above it near a takeoff)
      lo[i] = std::min(lo[i], z[i]);
      hi[i] = std::max(hi[i], z[i]);
    }
  }
  for (size_t stride : {16u, 8u, 4u, 2u, 1u}) {
    std::vector<size_t> idx;
    for (size_t i = 0; i < n; i += stride) {
      idx.push_back(i);
    }
    if (idx.back() != n - 1) {
      idx.push_back(n - 1);
    }
    const size_t m = idx.size();
    if (m < 7) {
      continue;
    }
    std::vector<float> s(m), slo(m), shi(m), next(m);
    for (size_t k = 0; k < m; ++k) {
      s[k] = z[idx[k]];
      size_t from = k ? (idx[k - 1] + idx[k]) / 2 : 0, to = k + 1 < m ? (idx[k] + idx[k + 1]) / 2 : n - 1;
      slo[k] = -kInf;
      shi[k] = kInf;
      for (size_t i = from; i <= to; ++i) {
        slo[k] = std::max(slo[k], lo[i]);
        shi[k] = std::min(shi[k], hi[i]);
      }
      if (slo[k] > shi[k] || k < 2 || k + 2 >= m) {
        slo[k] = shi[k] = s[k];
      }
    }
    for (int it = 0; it < aParams.lineHeightIterations; ++it) {
      next = s;
      for (size_t k = 2; k + 2 < m; ++k) {
        float g = s[k - 2] - 4.0f * s[k - 1] + 6.0f * s[k] - 4.0f * s[k + 1] + s[k + 2];
        next[k] = std::clamp(s[k] - 0.1f * g, slo[k], shi[k]); // stable below 0.125
      }
      s.swap(next);
    }
    // back onto every point: the solved ones, the rest in between linearly, within their bands
    for (size_t k = 0; k + 1 < m; ++k) {
      for (size_t i = idx[k]; i <= idx[k + 1]; ++i) {
        float t = idx[k + 1] > idx[k] ? float(i - idx[k]) / float(idx[k + 1] - idx[k]) : 0.0f;
        z[i] = std::clamp(s[k] + (s[k + 1] - s[k]) * t, lo[i], hi[i]);
      }
    }
  }
  std::vector<Vec3> solved = aPts;
  for (size_t i = 0; i < n; ++i) {
    solved[i].z = z[i];
  }
  for (int round = 0; round < 8; ++round) {
    bool reverted = false;
    for (size_t i = 0; i + 1 < n; ++i) {
      if (!LegFree(aMap, solved[i], solved[i + 1]) && (solved[i].z != aPts[i].z || solved[i + 1].z != aPts[i + 1].z)) {
        solved[i] = aPts[i];
        solved[i + 1] = aPts[i + 1];
        reverted = true;
      }
    }
    if (!reverted) {
      break;
    }
  }
  aPts = solved;
}
// The line's path across solved on its own, its heights kept: each point may move along its
// horizontal normal within the free width there (marched out to lineCorridorMax at its height,
// less lineCorridorMargin; where it already is always allowed), and the offsets minimise the
// summed squared second differences (bend) plus lineCurvatureRate times the third differences
// (how fast the bend changes, so turns build up and ease off instead of switching on), coarse to
// fine as SmoothHeights. A corridor rather than RacingLine's moves, each kept only if its legs stay
// free with room to the sides: by the gap into Dogtown every step outwards turned the leg by a
// tower's corner and swept its side clearance into it, so the line came in tight and turned 50
// degrees in 25 m after a gentle bend (2026-10-28). The same failure as elastic bands at obstacle
// corners; racing line planners solve in a corridor (Heilmeier et al. 2020), drone planners add a
// jerk term. A leg not free at the solved offsets goes back to where it was, with its neighbour.
void SmoothLateral(const NavMap &aMap, std::vector<Vec3> &aPts, const PlanParams &aParams) {
  const size_t n = aPts.size();
  if (aParams.lineCorridorIterations <= 0 || n < 12) {
    return;
  }
  std::vector<float> along(n, 0.0f), lo(n, 0.0f), hi(n, 0.0f), d(n, 0.0f);
  std::vector<Vec3> normal(n, {0.0f, 0.0f, 0.0f});
  for (size_t i = 1; i < n; ++i) {
    along[i] = along[i - 1] + Distance(aPts[i - 1], aPts[i]);
  }
  const float step = 0.5f;
  for (size_t i = 1; i + 1 < n; ++i) {
    if (along[i] <= aParams.lineFloorEnds || along.back() - along[i] <= aParams.lineFloorEnds) {
      continue; // the take-off and the landing stay as they are
    }
    float tx = aPts[i + 1].x - aPts[i - 1].x, ty = aPts[i + 1].y - aPts[i - 1].y;
    float tl = std::hypot(tx, ty);
    if (tl < 1e-3f) {
      continue;
    }
    normal[i] = {ty / tl, -tx / tl, 0.0f}; // to the right of the line
    const Vec3 &p = aPts[i];
    float extent[2] = {0.0f, 0.0f};
    for (int s = 0; s < 2; ++s) {
      float sign = s ? -1.0f : 1.0f;
      float r = step;
      while (r <= aParams.lineCorridorMax &&
             aMap.IsFree(p.x + normal[i].x * r * sign, p.y + normal[i].y * r * sign, p.z)) {
        r += step;
      }
      extent[s] = r - step;
    }
    hi[i] = std::max(0.0f, extent[0] - aParams.lineCorridorMargin);
    lo[i] = std::min(0.0f, -(extent[1] - aParams.lineCorridorMargin));
  }
  const float w = aParams.lineCurvatureRate;
  const float rate = 1.6f / (16.0f + 64.0f * w); // stable below 2 / (16 + 64 w)
  for (size_t stride : {16u, 8u, 4u, 2u, 1u}) {
    std::vector<size_t> idx;
    for (size_t i = 0; i < n; i += stride) {
      idx.push_back(i);
    }
    if (idx.back() != n - 1) {
      idx.push_back(n - 1);
    }
    const size_t m = idx.size();
    if (m < 9) {
      continue;
    }
    std::vector<float> s(m), slo(m), shi(m), next(m);
    std::vector<Vec3> q(m);
    for (size_t k = 0; k < m; ++k) {
      s[k] = d[idx[k]];
      // a coarse point stands for the stretch around it: the narrowest of its widths
      size_t from = k ? (idx[k - 1] + idx[k]) / 2 : 0, to = k + 1 < m ? (idx[k] + idx[k + 1]) / 2 : n - 1;
      slo[k] = -kInf;
      shi[k] = kInf;
      for (size_t i = from; i <= to; ++i) {
        slo[k] = std::max(slo[k], lo[i]);
        shi[k] = std::min(shi[k], hi[i]);
      }
      if (slo[k] > shi[k] || k < 3 || k + 3 >= m) {
        slo[k] = shi[k] = s[k];
      }
      slo[k] = std::min(slo[k], s[k]);
      shi[k] = std::max(shi[k], s[k]);
    }
    for (int it = 0; it < aParams.lineCorridorIterations; ++it) {
      for (size_t k = 0; k < m; ++k) {
        const Vec3 &p = aPts[idx[k]], &nk = normal[idx[k]];
        q[k] = {p.x + nk.x * s[k], p.y + nk.y * s[k], 0.0f};
      }
      next = s;
      for (size_t k = 3; k + 3 < m; ++k) {
        // d/dq of the squared second differences (1 -4 6 -4 1) and third differences (-1 6 -15 20 -15 6 -1)
        float gx = q[k - 2].x - 4 * q[k - 1].x + 6 * q[k].x - 4 * q[k + 1].x + q[k + 2].x;
        float gy = q[k - 2].y - 4 * q[k - 1].y + 6 * q[k].y - 4 * q[k + 1].y + q[k + 2].y;
        gx += w * (-q[k - 3].x + 6 * q[k - 2].x - 15 * q[k - 1].x + 20 * q[k].x - 15 * q[k + 1].x + 6 * q[k + 2].x - q[k + 3].x);
        gy += w * (-q[k - 3].y + 6 * q[k - 2].y - 15 * q[k - 1].y + 20 * q[k].y - 15 * q[k + 1].y + 6 * q[k + 2].y - q[k + 3].y);
        const Vec3 &nk = normal[idx[k]];
        next[k] = std::clamp(s[k] - rate * (gx * nk.x + gy * nk.y), slo[k], shi[k]);
      }
      s.swap(next);
    }
    for (size_t k = 0; k + 1 < m; ++k) {
      for (size_t i = idx[k]; i <= idx[k + 1]; ++i) {
        float t = idx[k + 1] > idx[k] ? float(i - idx[k]) / float(idx[k + 1] - idx[k]) : 0.0f;
        d[i] = std::clamp(s[k] + (s[k + 1] - s[k]) * t, lo[i], hi[i]);
      }
    }
  }
  std::vector<Vec3> solved = aPts;
  for (size_t i = 0; i < n; ++i) {
    solved[i].x += normal[i].x * d[i];
    solved[i].y += normal[i].y * d[i];
  }
  for (int round = 0; round < 8; ++round) {
    bool reverted = false;
    for (size_t i = 0; i + 1 < n; ++i) {
      bool moved = solved[i].x != aPts[i].x || solved[i].y != aPts[i].y || solved[i + 1].x != aPts[i + 1].x ||
                   solved[i + 1].y != aPts[i + 1].y;
      if (moved && (!LegFree(aMap, solved[i], solved[i + 1]) ||
                    ClearSides(aMap, solved[i], solved[i + 1], aParams.lineSide) <
                        std::min(1.0f, ClearSides(aMap, aPts[i], aPts[i + 1], aParams.lineSide)))) {
        solved[i] = aPts[i];
        solved[i + 1] = aPts[i + 1];
        reverted = true;
      }
    }
    if (!reverted) {
      break;
    }
  }
  aPts = solved;
}

std::vector<Vec3> RacingLine(const NavMap &aMap, const std::vector<Vec3> &aRoute, const PlanParams &aParams) {
  std::vector<Vec3> pts = aRoute;
  const float side = aParams.lineSide;
  for (auto [spacing, passes] : {std::pair{40.0f, 300}, {20.0f, 300}, {10.0f, 200}, {aParams.lineSpacing, 100}}) {
    pts = Resample(pts, spacing);
    const size_t n = pts.size();
    if (n < 7) {
      continue;
    }
    // route distance from the start, for the floor clearance's exemption at the ends
    std::vector<float> along(n, 0.0f);
    for (size_t i = 1; i < n; ++i) {
      along[i] = along[i - 1] + Distance(pts[i - 1], pts[i]);
    }
    // the clearance push spread over the points within lineSpread either side (tapering off), so
    // the line moves away from a wall gradually: pushed point by point, at a pinch by the gap into
    // Dogtown (a tower on one side, a wall on the other) only the points by the tower moved, and
    // the line wiggled and then turned 50 degrees in 25 m (2026-10-28)
    const int reach = aParams.lineSpread > 0.0f ? int(std::round(aParams.lineSpread / spacing)) : 0;
    const float unit = aParams.linePush / aParams.lineStep; // undone by * step below
    std::vector<Vec3> push(n), spread(n);
    for (int pass = 0; pass < passes; ++pass) {
      float moved = 0.0f;
      // away from blocked space close across the line: probe lineClearance to either side and
      // above / below; where one side is blocked and the other free, push towards the free one.
      // (Smoothing alone leaves the line skimming the map's margin wherever a wall is on the
      // inside of a bend, and the car's tracking error then reaches the wall, 2026-10-26)
      for (size_t i = 2; i + 2 < n; ++i) {
        const Vec3 &b = pts[i - 1], &p = pts[i], &c = pts[i + 1];
        push[i] = {0.0f, 0.0f, 0.0f};
        float tx = c.x - b.x, ty = c.y - b.y;
        float tl = std::hypot(tx, ty);
        if (tl > 1e-3f) {
          const float r = aParams.lineClearance;
          const Vec3 dirs[2] = {{-ty / tl, tx / tl, 0.0f}, {0.0f, 0.0f, 1.0f}};
          for (const Vec3 &dir : dirs) {
            bool plus = aMap.IsFree(p.x + dir.x * r, p.y + dir.y * r, p.z + dir.z * r);
            bool minus = aMap.IsFree(p.x - dir.x * r, p.y - dir.y * r, p.z - dir.z * r);
            if (plus != minus) {
              float k = (plus ? 1.0f : -1.0f) * unit;
              push[i].x += dir.x * k;
              push[i].y += dir.y * k;
              push[i].z += dir.z * k;
            }
          }
        }
      }
      for (size_t i = 2; i + 2 < n; ++i) {
        Vec3 sum{0.0f, 0.0f, 0.0f};
        for (int j = -reach; j <= reach; ++j) {
          long k = long(i) + j;
          if (k < 2 || k + 2 >= long(n)) {
            continue;
          }
          float w = 1.0f - float(std::abs(j)) / float(reach + 1);
          sum.x += push[k].x * w;
          sum.y += push[k].y * w;
          sum.z += push[k].z * w;
        }
        // no stronger than one point's own push, across and up / down
        float across = std::hypot(sum.x, sum.y);
        if (across > unit) {
          sum.x *= unit / across;
          sum.y *= unit / across;
        }
        sum.z = std::clamp(sum.z, -unit, unit);
        spread[i] = sum;
      }
      for (size_t i = 2; i + 2 < n; ++i) {
        const Vec3 &a = pts[i - 2], &b = pts[i - 1], &p = pts[i], &c = pts[i + 1], &d = pts[i + 2];
        // d/dp of |b - 2p + c|^2 + |a - 2b + p|^2 + |p - 2c + d|^2, halved
        Vec3 g{a.x - 4 * b.x + 6 * p.x - 4 * c.x + d.x, a.y - 4 * b.y + 6 * p.y - 4 * c.y + d.y,
               (a.z - 4 * b.z + 6 * p.z - 4 * c.z + d.z) * aParams.lineVerticalWeight};
        g.x -= spread[i].x;
        g.y -= spread[i].y;
        g.z -= spread[i].z;
        // outside its height band (Band): moved into it even where the line is straight
        bool outside = false;
        {
          Gap here;
          if (aMap.FindGap(p.x, p.y, p.z, here)) {
            float lo, hi;
            Band(here, along[i], along.back(), aParams, lo, hi);
            outside = p.z < lo - 0.01f || p.z > hi + 0.01f;
          }
        }
        if (!outside && g.x * g.x + g.y * g.y + g.z * g.z < 1e-4f) {
          continue;
        }
        float before = -1.0f; // only needed when the move is not free with room to the sides
        for (float step = aParams.lineStep; step > aParams.lineStep * 0.2f; step *= 0.5f) {
          Vec3 q{p.x - g.x * step, p.y - g.y * step, p.z - g.z * step};
          // into the height band of its gap: clear of the floor below and anything overhead
          Gap gap;
          if (aMap.FindGap(q.x, q.y, q.z, gap)) {
            float lo, hi;
            Band(gap, along[i], along.back(), aParams, lo, hi);
            q.z = std::clamp(q.z, lo, hi);
          }
          float after = std::min(ClearSides(aMap, b, q, side), ClearSides(aMap, q, c, side));
          bool ok = after >= 1.0f;
          // (grazes judged over the two legs either side together: one per leg can add up)
          if (!ok && LongestBlocked(aMap, {a, b, q, c, d}) <= kGrazeAllowed) {
            // free, but closer to the sides than lineSide: fine where the line already was
            if (before < 0.0f) {
              before = std::min(ClearSides(aMap, b, p, side), ClearSides(aMap, p, c, side));
            }
            ok = after >= before;
          }
          if (ok) {
            moved += Distance(p, q);
            pts[i] = q;
            break;
          }
        }
      }
      if (moved < 0.0005f * float(n)) {
        break; // settled: under half a millimetre per point
      }
    }
  }
  SmoothLateral(aMap, pts, aParams);
  SmoothHeights(aMap, pts, aParams);
  return pts;
}

bool TryDirect(NavMap::View &aView, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams, PlanResult &aOut) {
  const NavMap &map = aView.Map();
  float dx = aGoal.x - aStart.x, dy = aGoal.y - aStart.y;
  int steps = std::max(1, int(std::ceil(std::sqrt(dx * dx + dy * dy) / (0.5f * map.CellSize()))));
  float highest = -kInf;
  for (int i = 0; i <= steps; ++i) {
    float t = float(i) / float(steps);
    int32_t cx, cy;
    if (!map.CellOf(aStart.x + dx * t, aStart.y + dy * t, cx, cy)) {
      return false;
    }
    const uint16_t *g;
    uint32_t n = aView.Column(cx, cy, g);
    if (!n) {
      return false; // no data or no reachable space: the straight line is not safe
    }
    highest = std::max(highest, map.VoxelZ(g[2 * (n - 1)])); // floor of the top gap
  }
  float cruise = std::max({highest + aParams.directMargin, aStart.z, aGoal.z});
  if (cruise > aParams.maxCruise) {
    return false;
  }
  std::vector<Vec3> points = {aStart, {aStart.x, aStart.y, cruise}, {aGoal.x, aGoal.y, cruise}, aGoal};
  for (size_t i = 0; i + 1 < points.size(); ++i) {
    if (Clear(map, points[i], points[i + 1]) < 1.0f) {
      return false;
    }
  }
  aOut.direct = true;
  aOut.points = std::move(points);
  return true;
}

// --- doorways: the search in two layers ------------------------------------------------------
// The map in blocks of kBlock x kBlock columns (32 m). A block's doorways are where a car can cross
// its edge into the next block: for each run of side-by-side crossable columns in one height
// layer, the columns in its middle (and at both ends of a long run). The doorways of a block are
// joined by the cheapest way between them inside it, and to their partners across the edge. The
// first search over doorways finds which blocks the route goes through; the plan itself is then
// searched only in those blocks and their neighbours. Doorways come from the 2 m map, so every
// street and alley is still there (a search over coarse cells could not thread the city's streets
// and went round it over the water, 2026-10-27). A block is worked out the first time a search
// reaches it and kept for the session: the map does not change.
constexpr int32_t kBlock = 16;
constexpr char kDoorwayTag[8] = {'L', 'T', 'B', 'F', 'D', 'O', 'O', 'R'};

// the fine search's cost of a step, without the turn and bearing parts, which depend on the way in
struct StepCosts {
  float cell = 2.0f, minOverlap = 0.5f, minPassage = 0.0f, preferredHeight = 25.0f, climbCost = 3.0f, underCost = 0.5f;
  bool operator==(const StepCosts &aOther) const {
    return cell == aOther.cell && minOverlap == aOther.minOverlap && minPassage == aOther.minPassage &&
           preferredHeight == aOther.preferredHeight && climbCost == aOther.climbCost && underCost == aOther.underCost;
  }
  bool Passable(GapM aFrom, GapM aTo) const {
    float overlap = std::min(aFrom.t, aTo.t) - std::max(aFrom.b, aTo.b);
    return overlap >= minOverlap && overlap >= minPassage;
  }
  float Cost(GapM aFrom, GapM aTo, bool aDiagonal) const {
    float step = aDiagonal ? cell * 1.41421356f : cell;
    return step * (1.0f + (std::isfinite(aTo.t) ? underCost : 0.0f)) +
           climbCost * std::max(0.0f, Preferred(aTo, preferredHeight) - Preferred(aFrom, preferredHeight));
  }
};

int32_t BlockOf(int32_t aCell) { return aCell >= 0 ? aCell / kBlock : (aCell - kBlock + 1) / kBlock; }

// Cheapest costs from aSources over the columns x0 <= x < x1, y0 <= y < y1 (aBackward: the costs to
// reach the sources instead, each step counted the way it is flown).
std::unordered_map<uint64_t, float> RegionCosts(NavMap::View &aView, const StepCosts &aCosts, int32_t aWidth, int32_t aX0,
                                                int32_t aY0, int32_t aX1, int32_t aY1,
                                                const std::vector<std::pair<uint64_t, float>> &aSources, bool aBackward) {
  static const int32_t kDx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  static const int32_t kDy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  std::unordered_map<uint64_t, float> dist;
  using Entry = std::pair<float, uint64_t>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
  for (auto [key, cost] : aSources) {
    auto it = dist.find(key);
    if (it == dist.end() || cost < it->second) {
      dist[key] = cost;
      open.push({cost, key});
    }
  }
  while (!open.empty()) {
    auto [d, key] = open.top();
    open.pop();
    if (d > dist[key]) {
      continue;
    }
    int32_t cx = KeyCx(key, aWidth), cy = KeyCy(key, aWidth);
    GapM cur = GapAt(aView, cx, cy, KeyGap(key));
    for (int k = 0; k < 8; ++k) {
      int32_t nx = cx + kDx[k], ny = cy + kDy[k];
      if (nx < aX0 || ny < aY0 || nx >= aX1 || ny >= aY1) {
        continue;
      }
      const uint16_t *gaps;
      uint32_t n = aView.Column(nx, ny, gaps);
      for (uint32_t j = 0; j < n; ++j) {
        GapM next{aView.Map().VoxelZ(gaps[2 * j]), aView.Map().VoxelZ(gaps[2 * j + 1])};
        if (!aCosts.Passable(cur, next)) {
          continue;
        }
        float nd = d + (aBackward ? aCosts.Cost(next, cur, k >= 4) : aCosts.Cost(cur, next, k >= 4));
        uint64_t nextKey = MakeKey(nx, ny, j, aWidth);
        auto it = dist.find(nextKey);
        if (it == dist.end() || nd < it->second) {
          dist[nextKey] = nd;
          open.push({nd, nextKey});
        }
      }
    }
  }
  return dist;
}

class Doorways {
public:
  struct Link {
    uint32_t to;
    float cost;
  };
  struct Door {
    int32_t cx, cy;
    uint32_t gap;
    std::vector<Link> links;
  };

  Doorways(const StepCosts &aCosts, int32_t aWidth) : costs(aCosts), width(aWidth) {}

  const StepCosts costs;
  const int32_t width;
  std::vector<Door> doors;

  // the doorways of block (aBx, aBy), working it out the first time
  const std::vector<uint32_t> &Block(NavMap::View &aView, int32_t aBx, int32_t aBy) {
    int64_t id = (int64_t(aBy) << 32) ^ int64_t(uint32_t(aBx));
    auto it = blocks.find(id);
    if (it != blocks.end()) {
      return it->second;
    }
    std::vector<uint32_t> mine;
    for (int side = 0; side < 4; ++side) {
      Edge(aView, aBx, aBy, side, mine);
    }
    std::sort(mine.begin(), mine.end());
    mine.erase(std::unique(mine.begin(), mine.end()), mine.end());
    // inside the block: from each doorway to the others
    const int32_t x0 = aBx * kBlock, y0 = aBy * kBlock;
    for (uint32_t from : mine) {
      uint64_t fromKey = MakeKey(doors[from].cx, doors[from].cy, doors[from].gap, width);
      auto dist = RegionCosts(aView, costs, width, x0, y0, x0 + kBlock, y0 + kBlock, {{fromKey, 0.0f}}, false);
      for (uint32_t to : mine) {
        if (to == from) {
          continue;
        }
        auto d = dist.find(MakeKey(doors[to].cx, doors[to].cy, doors[to].gap, width));
        if (d != dist.end()) {
          Join(from, to, d->second);
        }
      }
    }
    return blocks[id] = std::move(mine);
  }

  // The whole set as bytes, for a bake: its costs, the doorways and their links, the blocks done.
  std::vector<uint8_t> Save() const {
    std::vector<uint8_t> out;
    auto put = [&](const void *aData, size_t aSize) {
      out.insert(out.end(), static_cast<const uint8_t *>(aData), static_cast<const uint8_t *>(aData) + aSize);
    };
    const uint32_t version = 1;
    put(&version, 4);
    put(&costs, sizeof(costs));
    put(&width, 4);
    uint32_t n = uint32_t(doors.size());
    put(&n, 4);
    for (const auto &door : doors) {
      put(&door.cx, 4);
      put(&door.cy, 4);
      uint8_t gap = uint8_t(door.gap);
      put(&gap, 1);
      uint16_t links = uint16_t(door.links.size());
      put(&links, 2);
      for (const auto &link : door.links) {
        put(&link.to, 4);
        put(&link.cost, 4);
      }
    }
    n = uint32_t(blocks.size());
    put(&n, 4);
    for (const auto &[id, mine] : blocks) {
      put(&id, 8);
      uint32_t count = uint32_t(mine.size());
      put(&count, 4);
      put(mine.data(), mine.size() * 4);
    }
    return out;
  }

  // A set saved by Save, if it was made with these costs for a map this wide.
  bool Load(const std::vector<uint8_t> &aData) {
    size_t at = 0;
    bool ok = true;
    auto get = [&](void *aTo, size_t aSize) {
      if (!ok || at + aSize > aData.size()) {
        ok = false;
        return;
      }
      std::memcpy(aTo, aData.data() + at, aSize);
      at += aSize;
    };
    uint32_t version = 0;
    StepCosts saved;
    int32_t savedWidth = 0;
    get(&version, 4);
    get(&saved, sizeof(saved));
    get(&savedWidth, 4);
    if (!ok || version != 1 || !(saved == costs) || savedWidth != width) {
      return false;
    }
    uint32_t n = 0;
    get(&n, 4);
    std::vector<Door> loaded(ok ? n : 0);
    for (auto &door : loaded) {
      uint8_t gap = 0;
      uint16_t links = 0;
      get(&door.cx, 4);
      get(&door.cy, 4);
      get(&gap, 1);
      get(&links, 2);
      door.gap = gap;
      door.links.resize(ok ? links : 0);
      for (auto &link : door.links) {
        get(&link.to, 4);
        get(&link.cost, 4);
      }
    }
    get(&n, 4);
    std::unordered_map<int64_t, std::vector<uint32_t>> loadedBlocks;
    for (uint32_t i = 0; ok && i < n; ++i) {
      int64_t id = 0;
      uint32_t count = 0;
      get(&id, 8);
      get(&count, 4);
      std::vector<uint32_t> mine(ok ? count : 0);
      get(mine.data(), mine.size() * 4);
      loadedBlocks[id] = std::move(mine);
    }
    if (!ok) {
      return false;
    }
    doors = std::move(loaded);
    blocks = std::move(loadedBlocks);
    byKey.clear();
    for (uint32_t i = 0; i < doors.size(); ++i) {
      byKey[MakeKey(doors[i].cx, doors[i].cy, doors[i].gap, width)] = i;
    }
    return true;
  }

  // the doorway at (column, gap), if there is one
  bool Find(uint64_t aKey, uint32_t &aDoor) const {
    auto it = byKey.find(aKey);
    if (it == byKey.end()) {
      return false;
    }
    aDoor = it->second;
    return true;
  }

private:
  std::unordered_map<uint64_t, uint32_t> byKey;
  std::unordered_map<int64_t, std::vector<uint32_t>> blocks;

  uint32_t At(int32_t aCx, int32_t aCy, uint32_t aGap) {
    uint64_t key = MakeKey(aCx, aCy, aGap, width);
    auto it = byKey.find(key);
    if (it != byKey.end()) {
      return it->second;
    }
    doors.push_back({aCx, aCy, aGap, {}});
    return byKey[key] = uint32_t(doors.size() - 1);
  }

  void Join(uint32_t aFrom, uint32_t aTo, float aCost) {
    for (auto &link : doors[aFrom].links) {
      if (link.to == aTo) {
        link.cost = std::min(link.cost, aCost);
        return;
      }
    }
    doors[aFrom].links.push_back({aTo, aCost});
  }

  // The crossings over one edge of the block: side 0 east, 1 north, 2 west, 3 south. Each run of
  // crossings along the edge that stays in one height layer gets doorways at its middle (and at
  // its ends if it is long), on both sides of the edge, joined across.
  void Edge(NavMap::View &aView, int32_t aBx, int32_t aBy, int aSide, std::vector<uint32_t> &aMine) {
    struct Crossing {
      int32_t k;
      uint32_t in, out;
      float lo, hi;
    };
    std::vector<std::vector<Crossing>> runs;
    std::vector<size_t> open; // runs still being extended, by index
    for (int32_t k = 0; k < kBlock; ++k) {
      int32_t ix, iy, ox, oy;
      switch (aSide) {
      case 0:
        ix = aBx * kBlock + kBlock - 1, iy = aBy * kBlock + k, ox = ix + 1, oy = iy;
        break;
      case 1:
        ix = aBx * kBlock + k, iy = aBy * kBlock + kBlock - 1, ox = ix, oy = iy + 1;
        break;
      case 2:
        ix = aBx * kBlock, iy = aBy * kBlock + k, ox = ix - 1, oy = iy;
        break;
      default:
        ix = aBx * kBlock + k, iy = aBy * kBlock, ox = ix, oy = iy - 1;
        break;
      }
      const uint16_t *gi, *go;
      uint32_t ni = aView.Column(ix, iy, gi), no = aView.Column(ox, oy, go);
      std::vector<size_t> next;
      for (uint32_t i = 0; i < ni; ++i) {
        GapM a{aView.Map().VoxelZ(gi[2 * i]), aView.Map().VoxelZ(gi[2 * i + 1])};
        for (uint32_t j = 0; j < no; ++j) {
          GapM b{aView.Map().VoxelZ(go[2 * j]), aView.Map().VoxelZ(go[2 * j + 1])};
          if (!costs.Passable(a, b) || !costs.Passable(b, a)) {
            continue;
          }
          Crossing c{k, i, j, std::max(a.b, b.b), std::min(a.t, b.t)};
          // carried on from a run at the previous column whose height range it overlaps
          size_t run = runs.size();
          for (size_t r : open) {
            const Crossing &last = runs[r].back();
            if (std::min(last.hi, c.hi) - std::max(last.lo, c.lo) >= costs.minOverlap) {
              run = r;
              break;
            }
          }
          if (run == runs.size()) {
            runs.push_back({});
          }
          if (std::find(next.begin(), next.end(), run) == next.end()) {
            runs[run].push_back(c);
            next.push_back(run);
          }
        }
      }
      open = std::move(next);
    }
    auto cell = [&](int32_t aK, bool aOut, int32_t &aX, int32_t &aY) {
      switch (aSide) {
      case 0:
        aX = aBx * kBlock + kBlock - 1 + (aOut ? 1 : 0), aY = aBy * kBlock + aK;
        break;
      case 1:
        aX = aBx * kBlock + aK, aY = aBy * kBlock + kBlock - 1 + (aOut ? 1 : 0);
        break;
      case 2:
        aX = aBx * kBlock - (aOut ? 1 : 0), aY = aBy * kBlock + aK;
        break;
      default:
        aX = aBx * kBlock + aK, aY = aBy * kBlock - (aOut ? 1 : 0);
        break;
      }
    };
    for (const auto &run : runs) {
      // one doorway an opening (with doorways at the ends of long ones as well, a 1 km square of
      // the city had 206 000 links: about 50 MB for the city in a bake)
      std::vector<size_t> picks{run.size() / 2};
      for (size_t p : picks) {
        const Crossing &c = run[p];
        int32_t ix, iy, ox, oy;
        cell(c.k, false, ix, iy);
        cell(c.k, true, ox, oy);
        uint32_t in = At(ix, iy, c.in), out = At(ox, oy, c.out);
        GapM a = GapAt(aView, ix, iy, c.in), b = GapAt(aView, ox, oy, c.out);
        Join(in, out, costs.Cost(a, b, false));
        Join(out, in, costs.Cost(b, a, false));
        aMine.push_back(in);
      }
    }
  }
};

// One set of doorways per map and set of step costs (the Race and its no-minPassage retry), kept
// for the session; plans running at once take turns.
std::mutex doorwaysLock;
std::vector<std::unique_ptr<Doorways>> doorwaySets;

// The set for these costs; a new one starts from the map's baked doorways when they were made
// with the same costs (the Race's), else empty, filled in as searches reach blocks.
Doorways &DoorwaysFor(const NavMap &aMap, const StepCosts &aCosts) {
  for (auto &set : doorwaySets) {
    if (set->costs == aCosts && set->width == aMap.Width()) {
      return *set;
    }
  }
  doorwaySets.push_back(std::make_unique<Doorways>(aCosts, aMap.Width()));
  std::vector<uint8_t> baked;
  if (aMap.Section(kDoorwayTag, baked) && doorwaySets.back()->Load(baked)) {
    spdlog::info("[NavMap] {} baked doorways", doorwaySets.back()->doors.size());
  }
  return *doorwaySets.back();
}

} // namespace

DoorwayCount WorkOutDoorways(const NavMap &aMap, const PlanParams &aParams, float aX0, float aY0, float aX1, float aY1) {
  auto t0 = std::chrono::steady_clock::now();
  DoorwayCount count;
  NavMap::View view(aMap);
  StepCosts costs;
  costs.cell = aMap.CellSize();
  costs.minOverlap = aMap.VoxelHeight();
  costs.minPassage = aParams.minPassage;
  costs.preferredHeight = aParams.preferredHeight;
  costs.climbCost = aParams.climbCost;
  costs.underCost = aParams.underCost;
  std::lock_guard guard(doorwaysLock);
  Doorways &doorways = DoorwaysFor(aMap, costs);
  int32_t cx0, cy0, cx1, cy1;
  aMap.CellOf(aX0, aY0, cx0, cy0);
  aMap.CellOf(aX1, aY1, cx1, cy1);
  for (int32_t by = BlockOf(cy0); by <= BlockOf(cy1); ++by) {
    for (int32_t bx = BlockOf(cx0); bx <= BlockOf(cx1); ++bx) {
      doorways.Block(view, bx, by);
      ++count.blocks;
    }
  }
  count.doors = uint32_t(doorways.doors.size());
  for (const auto &door : doorways.doors) {
    count.links += uint32_t(door.links.size());
  }
  count.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  count.saved = doorways.Save();
  return count;
}

PlanResult PlanWithRetries(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams,
                           const std::function<void(const PlanResult &)> &aAttempt) {
  auto params = aParams;
  auto result = Plan(aMap, aStart, aGoal, params);
  // a long trip at the Race's weight 1.2 ran out of expansions: Northside to the Pacifica coast
  // (5.3 km) failed at 3 million, and the minPassage retry failed the same way; at 1.5 it took 1.1
  // million (2026-10-27)
  for (float weight : {1.5f, 2.0f}) {
    if (result.ok || result.expansions <= params.maxExpansions || params.heuristicWeight >= weight) {
      break;
    }
    if (aAttempt) {
      aAttempt(result);
    }
    params.heuristicWeight = weight;
    result = Plan(aMap, aStart, aGoal, params);
  }
  if (!result.ok && params.minPassage > 0.0f) {
    // no way through without a squeeze point (a start or destination tucked under something):
    // better a tight route than none
    if (aAttempt) {
      aAttempt(result);
    }
    params.minPassage = 0.0f;
    result = Plan(aMap, aStart, aGoal, params);
  }
  if (aAttempt) {
    aAttempt(result);
  }
  return result;
}

PlanResult Plan(const NavMap &aMap, Vec3 aStart, Vec3 aGoal, const PlanParams &aParams) {
  auto t0 = std::chrono::steady_clock::now();
  PlanResult out;
  auto finish = [&](const char *aWhat) {
    out.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    out.length = 0;
    for (size_t i = 0; i + 1 < out.points.size(); ++i) {
      out.length += Distance(out.points[i], out.points[i + 1]);
    }
    out.info = fmt::format("{}: {} points, {:.0f} m, {} expansions, {} grid nodes, {:.1f} ms", aWhat,
                           out.points.size(), out.length, out.expansions, out.gridNodes, out.milliseconds);
    if (out.doorwayExpansions > 0) {
      out.info += fmt::format(" ({} doorway expansions, {} blocks)", out.doorwayExpansions, out.doorwayBlocks);
    }
    if (out.searchMilliseconds > 0.0) {
      out.info += fmt::format(" (search {:.1f} ms, line {:.1f} ms)", out.searchMilliseconds, out.milliseconds - out.searchMilliseconds);
    }
    return out;
  };

  if (!aMap.IsLoaded()) {
    return finish("no nav map");
  }
  NavMap::View view(aMap);
  const int32_t width = aMap.Width();
  const float cell = aMap.CellSize();

  int32_t sx, sy, gx, gy;
  uint32_t sg, gg;
  if (!Anchor(view, aStart, sx, sy, sg)) {
    return finish("start is not near free space");
  }
  if (!Anchor(view, aGoal, gx, gy, gg)) {
    return finish("goal is not near free space");
  }
  // goal height: inside its gap, as close to the requested height as allowed
  GapM goalGap = GapAt(view, gx, gy, gg);
  Vec3 goal = aGoal;
  goal.z = std::clamp(goal.z, goalGap.b, std::min(goalGap.t, kTopCap) - aMap.VoxelHeight());

  if (aParams.tryDirect && TryDirect(view, aStart, goal, aParams, out)) {
    out.ok = true;
    return finish("direct");
  }

  // A* over (column, gap)
  struct Rec {
    float g;
    uint64_t parent;
  };
  using Entry = std::pair<float, uint64_t>;

  float goalX, goalY;
  aMap.CellCentre(gx, gy, goalX, goalY);
  auto heuristic = [&](int32_t aCx, int32_t aCy) {
    float x, y;
    aMap.CellCentre(aCx, aCy, x, y);
    return aParams.heuristicWeight * std::hypot(x - goalX, y - goalY);
  };
  const uint64_t goalKey = MakeKey(gx, gy, gg, width);
  static const int32_t kDx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  static const int32_t kDy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  const float minOverlap = aMap.VoxelHeight();

  // columns a search may use (none: all of them)
  struct Corridor {
    int32_t x0 = 0, y0 = 0, w = 0, h = 0;
    std::vector<uint8_t> cells;
    bool Has(int32_t aX, int32_t aY) const {
      int32_t x = aX - x0, y = aY - y0;
      return x >= 0 && y >= 0 && x < w && y < h && cells[size_t(y) * w + x];
    }
  };

  // The take-off is part of the search: it starts from every free gap within startRadius, each at
  // the cost of getting there from the start (the distance, rises counted three times over, and
  // blocked map along the way five times over), so it finds the way out from under a deck or
  // between buildings itself. One anchor gap could be a pocket the map closed off: from under a
  // structure in Kabuki the search had one expansion and no route (2026-10-27).
  std::vector<std::pair<uint64_t, float>> seeds;
  {
    const int32_t r = int32_t(std::ceil(aParams.startRadius / cell));
    for (int32_t dy = -r; dy <= r; ++dy) {
      for (int32_t dx = -r; dx <= r; ++dx) {
        int32_t cx = sx + dx, cy = sy + dy;
        float x, y;
        aMap.CellCentre(cx, cy, x, y);
        float flat = std::hypot(x - aStart.x, y - aStart.y);
        if (flat > aParams.startRadius) {
          continue;
        }
        const uint16_t *g;
        uint32_t count = view.Column(cx, cy, g);
        for (uint32_t i = 0; i < count; ++i) {
          GapM gap = GapAt(view, cx, cy, i);
          float z = std::clamp(std::max(aStart.z, gap.b + aParams.startLift), gap.b, std::min(gap.t, kTopCap) - aMap.VoxelHeight());
          if (z < gap.b) {
            continue; // no room
          }
          Vec3 at{x, y, z};
          float len = Distance(aStart, at);
          seeds.push_back({MakeKey(cx, cy, i, width), flat + 3.0f * std::max(0.0f, z - aStart.z) + std::max(0.0f, aStart.z - z) +
                                                          5.0f * (1.0f - Clear(aMap, aStart, at)) * len});
        }
      }
    }
    if (seeds.empty()) {
      seeds.push_back({MakeKey(sx, sy, sg, width), 0.0f});
    }
  }

  // One search, only over the columns aAllowed marks (none: anywhere). aPath gets the nodes from a
  // start seed to the goal; aLimit says the search ran out of expansions.
  auto search = [&](const Corridor *aAllowed, std::vector<uint64_t> &aPath, bool &aLimit) -> bool {
    std::unordered_map<uint64_t, Rec> recs;
    recs.reserve(size_t(1) << 20);
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    uint32_t expansions = 0;
    aLimit = false;

    for (auto [key, cost] : seeds) {
      auto it = recs.find(key);
      if (it == recs.end() || cost < it->second.g) {
        recs[key] = {cost, key};
        open.push({cost + heuristic(KeyCx(key, width), KeyCy(key, width)), key});
      }
    }

    uint64_t endKey = 0;
    bool found = false;
    while (!open.empty()) {
      auto [f, key] = open.top();
      open.pop();
      int32_t cx = KeyCx(key, width), cy = KeyCy(key, width);
      float g = recs[key].g;
      if (f > g + heuristic(cx, cy) + 1e-3f) {
        continue; // stale entry
      }
      GapM cur = GapAt(view, cx, cy, KeyGap(key));
      if (key == goalKey) {
        endKey = key;
        found = true;
        break;
      }
      if (++expansions > aParams.maxExpansions) {
        aLimit = true;
        break;
      }
      float curPreferred = Preferred(cur, aParams.preferredHeight);
      // for turnCost: the nodes kTurnBase and kTurnBase - 1 steps back along the search's path, so
      // a bend is judged over a few cells (a straight line at 25 degrees is a staircase of 0 and 45
      // degree steps; judged step by step it would turn at every stair)
      int32_t backX = cx, backY = cy, nearX = cx, nearY = cy;
      bool haveBack = false;
      if (aParams.turnCost > 0.0f) {
        uint64_t k = key;
        int steps = 0;
        while (steps < kTurnBase) {
          uint64_t parent = recs[k].parent;
          if (parent == k) {
            break;
          }
          k = parent;
          ++steps;
          if (steps == kTurnBase - 1) {
            nearX = KeyCx(k, width);
            nearY = KeyCy(k, width);
          }
        }
        if (steps == kTurnBase) {
          backX = KeyCx(k, width);
          backY = KeyCy(k, width);
          haveBack = true;
        }
      }
      for (int d = 0; d < 8; ++d) {
        int32_t nx = cx + kDx[d], ny = cy + kDy[d];
        if (aAllowed && !aAllowed->Has(nx, ny)) {
          continue;
        }
        const uint16_t *gaps;
        uint32_t n = view.Column(nx, ny, gaps);
        float step = d < 4 ? cell : cell * 1.41421356f;
        for (uint32_t j = 0; j < n; ++j) {
          GapM next{aMap.VoxelZ(gaps[2 * j]), aMap.VoxelZ(gaps[2 * j + 1])};
          float overlap = std::min(cur.t, next.t) - std::max(cur.b, next.b);
          if (overlap < minOverlap) {
            continue;
          }
          // squeeze points: a passage lower than minPassage is left out (but for reaching the goal)
          if (overlap < aParams.minPassage && MakeKey(nx, ny, j, width) != goalKey) {
            continue;
          }
          float turn = 0.0f;
          if (haveBack) {
            // angle between the last kTurnBase steps and the kTurnBase steps ending at the next cell
            float ax = float(cx - backX), ay = float(cy - backY);
            float bx = float(nx - nearX), by = float(ny - nearY);
            float angle = std::fabs(std::atan2(ax * by - ay * bx, ax * bx + ay * by));
            turn = aParams.turnCost * angle / 0.78539816f;
          }
          // bearing: a step heading away from the goal costs more, (1 - cos) / 2 of bearingCost per
          // metre, so the route keeps going the way a watcher expects it to go
          float bearing = 0.0f;
          if (aParams.bearingCost > 0.0f) {
            float tx = float(gx - cx), ty = float(gy - cy);
            float tl = std::hypot(tx, ty);
            if (tl > 0.5f) {
              float cosine = (float(kDx[d]) * tx + float(kDy[d]) * ty) / (tl * std::hypot(float(kDx[d]), float(kDy[d])));
              bearing = aParams.bearingCost * step * 0.5f * (1.0f - cosine);
            }
          }
          float cost = turn + bearing + step * (1.0f + (std::isfinite(next.t) ? aParams.underCost : 0.0f)) +
                       aParams.climbCost * std::max(0.0f, Preferred(next, aParams.preferredHeight) - curPreferred);
          uint64_t nextKey = MakeKey(nx, ny, j, width);
          float ng = g + cost;
          auto it = recs.find(nextKey);
          if (it == recs.end() || ng < it->second.g) {
            recs[nextKey] = {ng, key};
            open.push({ng + heuristic(nx, ny), nextKey});
          }
        }
      }
    }
    out.expansions += expansions;
    if (!found) {
      return false;
    }
    aPath.clear();
    for (uint64_t k = endKey;; k = recs[k].parent) {
      aPath.push_back(k);
      if (recs[k].parent == k) {
        break; // a start seed
      }
    }
    std::reverse(aPath.begin(), aPath.end());
    return true;
  };

  // The doorways first (a long plan): which blocks the route goes through, then the plan itself
  // only in those blocks and the ones round them (the whole map if that finds no way).
  std::vector<uint64_t> path;
  bool limit = false;
  bool found = false;
  if (aParams.doorways && Distance(aStart, goal) > aParams.doorwaysFrom) {
    std::lock_guard guard(doorwaysLock);
    StepCosts costs;
    costs.cell = cell;
    costs.minOverlap = minOverlap;
    costs.minPassage = aParams.minPassage;
    costs.preferredHeight = aParams.preferredHeight;
    costs.climbCost = aParams.climbCost;
    costs.underCost = aParams.underCost;
    Doorways &doorways = DoorwaysFor(aMap, costs);
    // from the start seeds to the doorways of the blocks they are in, and from the doorways of the
    // goal's block and the ones round it to the goal
    int32_t sx0 = width, sy0 = aMap.Height(), sx1 = -1, sy1 = -1;
    for (auto [key, cost] : seeds) {
      sx0 = std::min(sx0, KeyCx(key, width));
      sy0 = std::min(sy0, KeyCy(key, width));
      sx1 = std::max(sx1, KeyCx(key, width));
      sy1 = std::max(sy1, KeyCy(key, width));
    }
    const int32_t bsx0 = BlockOf(sx0), bsy0 = BlockOf(sy0), bsx1 = BlockOf(sx1), bsy1 = BlockOf(sy1);
    auto fromStart = RegionCosts(view, costs, width, bsx0 * kBlock, bsy0 * kBlock, (bsx1 + 1) * kBlock, (bsy1 + 1) * kBlock, seeds, false);
    const int32_t gbx = BlockOf(gx), gby = BlockOf(gy);
    auto toGoal = RegionCosts(view, costs, width, (gbx - 1) * kBlock, (gby - 1) * kBlock, (gbx + 2) * kBlock, (gby + 2) * kBlock,
                              {{goalKey, 0.0f}}, true);

    // A* over doorways; a start doorway is its own parent
    constexpr uint32_t kNone = std::numeric_limits<uint32_t>::max();
    std::unordered_map<uint32_t, Rec> doorRecs;
    using DoorEntry = std::pair<float, uint32_t>;
    std::priority_queue<DoorEntry, std::vector<DoorEntry>, std::greater<DoorEntry>> doorOpen;
    auto doorHeuristic = [&](uint32_t aDoor) { return heuristic(doorways.doors[aDoor].cx, doorways.doors[aDoor].cy); };
    for (int32_t by = bsy0; by <= bsy1; ++by) {
      for (int32_t bx = bsx0; bx <= bsx1; ++bx) {
        const std::vector<uint32_t> blockDoors = doorways.Block(view, bx, by);
        for (uint32_t door : blockDoors) {
          const auto &d = doorways.doors[door];
          auto it = fromStart.find(MakeKey(d.cx, d.cy, d.gap, width));
          if (it != fromStart.end()) {
            doorRecs[door] = {it->second, uint64_t(door)};
            doorOpen.push({it->second + doorHeuristic(door), door});
          }
        }
      }
    }
    // the goal's doorways: their cost onwards to the goal
    std::unordered_map<uint32_t, float> goalDoors;
    for (int32_t by = gby - 1; by <= gby + 1; ++by) {
      for (int32_t bx = gbx - 1; bx <= gbx + 1; ++bx) {
        const std::vector<uint32_t> blockDoors = doorways.Block(view, bx, by);
        for (uint32_t door : blockDoors) {
          const auto &d = doorways.doors[door];
          auto it = toGoal.find(MakeKey(d.cx, d.cy, d.gap, width));
          if (it != toGoal.end()) {
            goalDoors[door] = it->second;
          }
        }
      }
    }
    float best = kInf;
    uint32_t bestDoor = kNone;
    while (!doorOpen.empty()) {
      auto [f, door] = doorOpen.top();
      doorOpen.pop();
      float g = doorRecs[door].g;
      if (f > g + doorHeuristic(door) + 1e-3f) {
        continue; // stale entry
      }
      if (f >= best) {
        break; // nothing left beats the way to the goal already found
      }
      if (++out.doorwayExpansions > aParams.maxExpansions) {
        break;
      }
      auto onward = goalDoors.find(door);
      if (onward != goalDoors.end() && g + onward->second < best) {
        best = g + onward->second;
        bestDoor = door;
      }
      // its links inside its own block (Block can add doorways: no references kept across it)
      doorways.Block(view, BlockOf(doorways.doors[door].cx), BlockOf(doorways.doors[door].cy));
      const auto links = doorways.doors[door].links;
      for (auto [to, cost] : links) {
        float ng = g + cost;
        auto it = doorRecs.find(to);
        if (it == doorRecs.end() || ng < it->second.g) {
          doorRecs[to] = {ng, uint64_t(door)};
          doorOpen.push({ng + doorHeuristic(to), to});
        }
      }
    }
    if (bestDoor != kNone) {
      // the blocks of the doorways on the way, with the start's and the goal's, and doorwayMargin
      // blocks round each
      std::vector<std::pair<int32_t, int32_t>> through;
      for (uint32_t door = bestDoor;; door = uint32_t(doorRecs[door].parent)) {
        through.push_back({BlockOf(doorways.doors[door].cx), BlockOf(doorways.doors[door].cy)});
        if (doorRecs[door].parent == door) {
          break;
        }
      }
      for (int32_t by = bsy0; by <= bsy1; ++by) {
        for (int32_t bx = bsx0; bx <= bsx1; ++bx) {
          through.push_back({bx, by});
        }
      }
      through.push_back({gbx, gby});
      int32_t bx0 = through[0].first, bx1 = bx0, by0 = through[0].second, by1 = by0;
      for (auto [bx, by] : through) {
        bx0 = std::min(bx0, bx);
        bx1 = std::max(bx1, bx);
        by0 = std::min(by0, by);
        by1 = std::max(by1, by);
      }
      const int32_t margin = aParams.doorwayMargin;
      Corridor corridor;
      corridor.x0 = (bx0 - margin) * kBlock;
      corridor.y0 = (by0 - margin) * kBlock;
      corridor.w = (bx1 - bx0 + 1 + 2 * margin) * kBlock;
      corridor.h = (by1 - by0 + 1 + 2 * margin) * kBlock;
      corridor.cells.assign(size_t(corridor.w) * corridor.h, 0);
      for (auto [bx, by] : through) {
        for (int32_t y = (by - margin) * kBlock; y < (by + margin + 1) * kBlock; ++y) {
          uint8_t *row = &corridor.cells[size_t(y - corridor.y0) * corridor.w];
          std::fill(row + ((bx - margin) * kBlock - corridor.x0), row + ((bx + margin + 1) * kBlock - corridor.x0), uint8_t(1));
        }
      }
      out.doorwayBlocks = uint32_t(through.size());
      found = search(&corridor, path, limit);
    }
  }
  if (!found) {
    found = search(nullptr, path, limit);
  }

  if (!found) {
    return finish(limit ? "search limit reached" : "no route");
  }
  const size_t n = path.size();
  out.gridNodes = uint32_t(n);

  // height profile
  std::vector<GapM> gaps(n);
  std::vector<float> xs(n), ys(n), lo(n), hi(n), z(n), seg(n, 0.0f);
  for (size_t i = 0; i < n; ++i) {
    int32_t cx = KeyCx(path[i], width), cy = KeyCy(path[i], width);
    gaps[i] = GapAt(view, cx, cy, KeyGap(path[i]));
    aMap.CellCentre(cx, cy, xs[i], ys[i]);
  }
  for (size_t i = 0; i < n; ++i) {
    // stay inside this gap and the gaps it is joined to, so each step between columns is free
    float l = gaps[i].b, h = std::min(gaps[i].t, kTopCap);
    for (size_t j : {i - 1, i + 1}) {
      if (j < n) {
        l = std::max(l, gaps[j].b);
        h = std::min(h, std::min(gaps[j].t, kTopCap));
      }
    }
    if (h - l < aMap.VoxelHeight()) {
      l = gaps[i].b;
      h = std::min(gaps[i].t, kTopCap);
    }
    lo[i] = l;
    hi[i] = h - aMap.VoxelHeight();
    z[i] = std::clamp(Preferred(gaps[i], aParams.preferredHeight), lo[i], hi[i]);
    if (i + 1 < n) {
      seg[i] = std::hypot(xs[i + 1] - xs[i], ys[i + 1] - ys[i]);
    }
  }
  z[0] = std::clamp(std::max(aStart.z, lo[0] + aParams.startLift), lo[0], hi[0]);
  z[n - 1] = goal.z;
  for (size_t i = n - 1; i-- > 0;) { // climb early before obstacles
    z[i] = std::min(std::max(z[i], z[i + 1] - aParams.maxGradient * seg[i]), hi[i]);
  }
  for (size_t i = 1; i < n; ++i) { // descend gradually after them
    z[i] = std::min(std::max(z[i], z[i - 1] - aParams.maxGradient * seg[i - 1]), hi[i]);
  }

  std::vector<Vec3> dense(n);
  for (size_t i = 0; i < n; ++i) {
    dense[i] = {xs[i], ys[i], z[i]};
  }
  dense.back() = goal;
  // the route begins at the start itself, then the gap the search left from (the take-off)
  float startLeg = 0.0f;
  if (Distance(aStart, dense.front()) > 1.0f) {
    startLeg = Distance(aStart, dense.front());
    dense.insert(dense.begin(), aStart);
  } else {
    dense.front() = aStart;
  }
  const size_t nd = dense.size();

  // string pulling: from each anchor, the furthest node with a free straight leg. Legs keep
  // legClearance to the sides where that still allows a leg of a useful length; in tight spots
  // the plain test is used, so the route does not turn into a staircase of single cells.
  auto furthest = [&](size_t aFrom, float aSide) {
    size_t good = aFrom + 1, bad = nd;
    for (size_t stride = 2;; stride *= 2) {
      size_t j = std::min(nd - 1, aFrom + stride);
      if (ClearWide(aMap, dense[aFrom], dense[j], aSide)) {
        good = j;
        if (j == nd - 1) {
          break;
        }
      } else {
        bad = j;
        break;
      }
    }
    while (bad < nd && bad - good > 1) {
      size_t mid = (good + bad) / 2;
      if (ClearWide(aMap, dense[aFrom], dense[mid], aSide)) {
        good = mid;
      } else {
        bad = mid;
      }
    }
    return good;
  };
  const size_t minWideNodes = size_t(std::ceil(20.0f / cell));
  out.points.push_back(dense.front());
  size_t a = 0;
  while (a + 1 < nd) {
    size_t good = furthest(a, aParams.legClearance);
    if (good < nd - 1 && good - a < minWideNodes) {
      good = std::max(good, furthest(a, 0.0f));
    }
    out.points.push_back(dense[good]);
    a = good;
  }
  if (out.points.size() == 1) {
    out.points.push_back(goal); // start and goal share a column
  }
  out.ok = true;
  out.searchMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  if (aParams.racingLine && out.points.size() > 2) {
    // kept only if the finished line is free all the way; otherwise the pulled route stays
    auto line = RacingLine(aMap, out.points, aParams);
    // (the stretch out from under cover to the free space is blocked on the map: not counted)
    if (LongestBlocked(aMap, line, startLeg + 5.0f) <= kGrazeAllowedLine) {
      out.points = std::move(line);
      return finish("racing line");
    }
    size_t blocked = 0;
    while (blocked + 1 < line.size() && LegFree(aMap, line[blocked], line[blocked + 1])) {
      ++blocked;
    }
    out.rejectedLine = line;
    return finish(fmt::format("route (racing line blocked at leg {} of {}, {:.1f} {:.1f} {:.1f})", blocked, line.size() - 1,
                              line[blocked].x, line[blocked].y, line[blocked].z)
                      .c_str());
  }
  return finish("route");
}

} // namespace Navigation
