#include "PlannerCommon.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace Navigation::detail {

namespace {

// The heights the line may take in a gap, at aAlong of aLength metres along it: lineFloorClearance
// over the floor (but within lineFloorEnds of either end, where it takes off and lands) and
// lineCeilingClearance under anything overhead; the middle of the gap where it has not room for
// both. (A ceiling push alone is undone by the smoothing and leaves a kink under a skybridge.)
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
// first and last two points fixed. The smoothing passes in Line stop after a hundred local
// passes at the fine spacing, so a bump in the floor would leave a ramp and a crest. A point whose
// legs are not free at the solved height goes back to where it was, with its neighbour.
// Over rather than under: the route keeps to the gap it was searched in, so a line high over the
// city could dive under a thin beam with open sky over it. So the heights are first solved without
// anything overhead; where that line passes within lineOverReach under the top of a thin obstacle
// (lineOverThick) over a run of points, away from the ends (lineOverEnds), the points under it
// take the gap above instead, and they are solved again. It only ever lifts the line over
// something, never into it.
namespace {
// The points are evenly spaced (Respace, aSpacing metres), so the bend is taken point to point.
// The heights also cost their height over the band's floor (the floor clearance over the terrain),
// exponentially past aFree; aPull sets the cost's slope there to balance the bend at
// lineHeightWavelength. Without it nothing holds the line down and smooth arcs peak far higher
// than what they clear.
void SolveHeights(std::vector<float> &aZ, const std::vector<float> &aLo, const std::vector<float> &aHi,
                  int aIterations, float aSpacing, float aPull, float aFree, float aScale) {
  const size_t n = aZ.size();
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
    // the pull in this level's units: the bend energy per point scales with spacing^4
    const float spacing = aSpacing * float(stride);
    const float pull = aPull * spacing * spacing * spacing * spacing;
    for (size_t k = 0; k < m; ++k) {
      s[k] = aZ[idx[k]];
      size_t from = k ? (idx[k - 1] + idx[k]) / 2 : 0, to = k + 1 < m ? (idx[k] + idx[k + 1]) / 2 : n - 1;
      slo[k] = -kInf;
      shi[k] = kInf;
      for (size_t i = from; i <= to; ++i) {
        slo[k] = std::max(slo[k], aLo[i]);
        shi[k] = std::min(shi[k], aHi[i]);
      }
      if (slo[k] > shi[k] || k < 2 || k + 2 >= m) {
        slo[k] = shi[k] = s[k];
      }
      s[k] = std::clamp(s[k], slo[k], shi[k]);
    }
    for (int it = 0; it < aIterations; ++it) {
      next = s;
      for (size_t k = 2; k + 2 < m; ++k) {
        // the height cost rises exponentially past aFree over the band's floor (aScale metres per
        // e-fold), and is nothing below it, so small heights do not matter (a pull in proportion to
        // the height drags lines down onto the terrain, into crests at speed)
        float over = s[k] - slo[k] - aFree, steep = 0.0f, push = 0.0f;
        if (over > 0.0f) {
          steep = pull * std::exp(std::min(over / aScale, 20.0f));
          push = aScale * (steep - pull);
        }
        float g = s[k - 2] - 4.0f * s[k - 1] + 6.0f * s[k] - 4.0f * s[k + 1] + s[k + 2] + push;
        next[k] = std::clamp(s[k] - 1.6f / (16.0f + steep) * g, slo[k], shi[k]); // stable below 2 / (16 + slope)
      }
      s.swap(next);
    }
    // back onto every point: the solved ones, the rest in between linearly, within their bands
    for (size_t k = 0; k + 1 < m; ++k) {
      for (size_t i = idx[k]; i <= idx[k + 1]; ++i) {
        float t = idx[k + 1] > idx[k] ? float(i - idx[k]) / float(idx[k + 1] - idx[k]) : 0.0f;
        aZ[i] = std::clamp(s[k] + (s[k + 1] - s[k]) * t, aLo[i], aHi[i]);
      }
    }
  }
}
} // namespace

void SmoothHeights(const NavMap &aMap, std::vector<Vec3> &aPts, const PlanParams &aParams) {
  const size_t n = aPts.size();
  if (aParams.lineHeightIterations <= 0 || n < 9) {
    return;
  }
  std::vector<float> along(n, 0.0f), lo(n), hi(n), z(n), aboveLo(n, kInf), aboveHi(n, kInf), aboveBottom(n, kInf);
  for (size_t i = 0; i < n; ++i) {
    along[i] = i ? along[i - 1] + Distance(aPts[i - 1], aPts[i]) : 0.0f;
    z[i] = lo[i] = hi[i] = aPts[i].z;
  }
  // the pull to the band's floor that balances the bend at lineHeightWavelength: (2 pi / L)^4
  const float spacing = along.back() / float(n - 1);
  const float pull = aParams.lineHeightWavelength > 0.0f ? std::pow(6.2831853f / aParams.lineHeightWavelength, 4.0f) : 0.0f;
  bool ceilings = false;
  for (size_t i = 2; i + 2 < n; ++i) {
    Gap gap;
    if (aMap.FindGap(aPts[i].x, aPts[i].y, aPts[i].z, gap)) {
      Band(gap, along[i], along.back(), aParams, lo[i], hi[i]);
      // a floor that is a thin slab with room under it (a beam, a skybridge) is no street:
      // lineCeilingClearance over it is enough (the full floor clearance humps the line over it,
      // a crest the car slows for)
      for (const Gap &under : aMap.GetGaps(aPts[i].x, aPts[i].y)) {
        if (under.top <= gap.bottom && gap.bottom - under.top <= aParams.lineOverThick) {
          lo[i] = std::min(lo[i], gap.bottom + aParams.lineCeilingClearance);
        }
      }
      // the line where it is now is always allowed (a band can sit above it near a takeoff)
      lo[i] = std::min(lo[i], z[i]);
      hi[i] = std::max(hi[i], z[i]);
      // the gap over whatever is overhead, for the line to go over it instead
      if (aParams.lineOverReach >= 0.0f && std::isfinite(gap.top) && along[i] > aParams.lineOverEnds && along.back() - along[i] > aParams.lineOverEnds) {
        for (const Gap &g : aMap.GetGaps(aPts[i].x, aPts[i].y)) {
          if (g.bottom >= gap.top && g.bottom - gap.top <= aParams.lineOverThick) {
            Band(g, along[i], along.back(), aParams, aboveLo[i], aboveHi[i]);
            // over a beam or a skybridge, not a street: lineCeilingClearance is room enough (the
            // floor clearance is for street furniture the map lacks)
            aboveLo[i] = std::min(aboveLo[i], g.bottom + aParams.lineCeilingClearance);
            aboveBottom[i] = g.bottom;
            ceilings = true;
            break;
          }
        }
      }
    }
  }
  SolveHeights(z, lo, hi, aParams.lineHeightIterations, spacing, pull, aParams.lineHeightFree, aParams.lineHeightScale);
  if (ceilings) {
    std::vector<float> zFree = z, hiFree(n, kInf);
    for (size_t i = 0; i < n; ++i) {
      hiFree[i] = (i < 2 || i + 2 >= n) ? hi[i] : kInf;
    }
    SolveHeights(zFree, lo, hiFree, aParams.lineHeightIterations, spacing, pull, aParams.lineHeightFree, aParams.lineHeightScale);
    // the bend around a run (aFrom..aTo, lineOverWindow either side): over the whole line, the
    // take-off's bend drowned the beam's (only 10% smoother over it, against 79% from another start)
    auto bend = [&](const std::vector<float> &aZ, size_t aFrom, size_t aTo) {
      double sum = 0.0;
      size_t from = 1, to = n - 1;
      while (from < aFrom && along[aFrom] - along[from] > aParams.lineOverWindow) {
        ++from;
      }
      while (to > aTo + 1 && along[to - 1] - along[aTo] > aParams.lineOverWindow) {
        --to;
      }
      for (size_t i = from; i + 1 < n && i < to; ++i) {
        double d = aZ[i - 1] - 2.0 * aZ[i] + aZ[i + 1];
        sum += d * d;
      }
      return sum;
    };
    // each run of points under a thin obstacle that the free line comes within lineOverReach of
    // along most of it is tried over the top, and kept there when the line around it comes out much
    // smoother (lineOverGain of the height solve's own measure) and no more than lineOverRise
    // higher on average. Taken over on nearness alone, runs go over single points (spikes), over
    // towers, and over skybridges far higher than the line needs to be.
    int over = 0;
    for (size_t i = 2; i + 2 < n;) {
      if (!std::isfinite(aboveBottom[i])) {
        ++i;
        continue;
      }
      size_t j = i, close = 0;
      while (j + 2 < n && std::isfinite(aboveBottom[j])) {
        close += zFree[j] >= aboveBottom[j] - aParams.lineOverReach ? 1 : 0;
        ++j;
      }
      if (2 * close > j - i) {
        std::vector<float> tryLo = lo, tryHi = hi, tryZ = z;
        for (size_t k = i; k < j; ++k) {
          tryLo[k] = aboveLo[k];
          tryHi[k] = aboveHi[k];
          tryZ[k] = std::clamp(std::max(z[k], zFree[k]), tryLo[k], tryHi[k]);
        }
        SolveHeights(tryZ, tryLo, tryHi, aParams.lineHeightIterations, spacing, pull, aParams.lineHeightFree, aParams.lineHeightScale);
        double best = bend(z, i, j), e = bend(tryZ, i, j), rise = 0.0;
        for (size_t k = 0; k < n; ++k) {
          rise += tryZ[k] - z[k];
        }
        if (e < aParams.lineOverGain * best && rise / double(n) <= aParams.lineOverRise) {
          lo.swap(tryLo);
          hi.swap(tryHi);
          z.swap(tryZ);
          over += int(j - i);
        }
      }
      i = j;
    }
    if (over) {
      spdlog::info("[Planner] heights: {} points taken over what was overhead", over);
    }
  }
  // a leg not free at its solved heights: its points pinned where they were and the heights solved
  // again around them, so the neighbours blend in (put back alone they leave a spike)
  for (int round = 0; round < 4; ++round) {
    bool pinned = false;
    std::vector<Vec3> at = aPts;
    for (size_t i = 0; i < n; ++i) {
      at[i].z = z[i];
    }
    for (size_t i = 0; i + 1 < n; ++i) {
      if ((z[i] != aPts[i].z || z[i + 1] != aPts[i + 1].z) && !LegRunFree(aMap, at, i)) {
        for (size_t k : {i, i + 1}) {
          lo[k] = hi[k] = z[k] = aPts[k].z;
        }
        pinned = true;
      }
    }
    if (!pinned) {
      break;
    }
    SolveHeights(z, lo, hi, aParams.lineHeightIterations, spacing, pull, aParams.lineHeightFree, aParams.lineHeightScale);
  }
  std::vector<Vec3> solved = aPts;
  for (size_t i = 0; i < n; ++i) {
    solved[i].z = z[i];
  }
  for (int round = 0; round < 8; ++round) {
    bool reverted = false;
    for (size_t i = 0; i + 1 < n; ++i) {
      if (!LegRunFree(aMap, solved, i) && (solved[i].z != aPts[i].z || solved[i + 1].z != aPts[i + 1].z)) {
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
// less lineCorridorMargin; a point inside the margin is moved out where the width allows both
// margins, its neighbours smoothed along, and may stay where it is in a narrower pinch), and the
// offsets minimise the summed squared second differences (bend) plus lineCurvatureRate times the third differences
// (how fast the bend changes, so turns build up and ease off instead of switching on), coarse to
// fine as SmoothHeights. A corridor rather than Line's moves, each kept only if its legs stay
// free with room to the sides: those cannot swing a line wide past an obstacle corner on the
// inside (every step outwards turns the leg into the corner). Car racing line planners solve in a
// corridor (Heilmeier et al. 2020), drone planners add a jerk term. A leg not free at the solved
// offsets goes back to where it was, with its neighbour.
// SmoothLateral's coarse-to-fine solve of the offsets d across the line (each bend weighted).
void SolveLateral(std::vector<float> &d, const std::vector<float> &lo, const std::vector<float> &hi,
                  const std::vector<Vec3> &normal, const std::vector<Vec3> &aPts, const std::vector<float> &weight,
                  float w, float rate, int aIterations) {
  const size_t n = aPts.size();
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
    std::vector<float> s(m), slo(m), shi(m), next(m), wk(m, 1.0f);
    std::vector<Vec3> q(m), wb(m);
    for (size_t k = 0; k < m; ++k) {
      s[k] = d[idx[k]];
      // a coarse point stands for the stretch around it: the narrowest of its widths
      size_t from = k ? (idx[k - 1] + idx[k]) / 2 : 0, to = k + 1 < m ? (idx[k] + idx[k + 1]) / 2 : n - 1;
      slo[k] = -kInf;
      shi[k] = kInf;
      for (size_t i = from; i <= to; ++i) {
        slo[k] = std::max(slo[k], lo[i]);
        shi[k] = std::min(shi[k], hi[i]);
        wk[k] = std::max(wk[k], weight[i]);
      }
      if (slo[k] > shi[k] || k < 3 || k + 3 >= m) {
        slo[k] = shi[k] = s[k];
      }
      s[k] = std::clamp(s[k], slo[k], shi[k]); // out of a margin it is in, the solve smooths the step
    }
    for (int it = 0; it < aIterations; ++it) {
      for (size_t k = 0; k < m; ++k) {
        const Vec3 &p = aPts[idx[k]], &nk = normal[idx[k]];
        q[k] = {p.x + nk.x * s[k], p.y + nk.y * s[k], 0.0f};
      }
      // each bend times its weight: d/dq of the summed weight x squared second difference
      for (size_t k = 1; k + 1 < m; ++k) {
        wb[k] = {wk[k] * (q[k - 1].x - 2 * q[k].x + q[k + 1].x), wk[k] * (q[k - 1].y - 2 * q[k].y + q[k + 1].y), 0.0f};
      }
      next = s;
      for (size_t k = 3; k + 3 < m; ++k) {
        // d/dq of the weighted second differences and of the third differences (-1 6 -15 20 -15 6 -1)
        float gx = wb[k - 1].x - 2 * wb[k].x + wb[k + 1].x;
        float gy = wb[k - 1].y - 2 * wb[k].y + wb[k + 1].y;
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
}

// How much each point's turn costs the car: the speed it would carry there without its turns
// (the car's top speed, speeding up from the start and braking to the end as Vehicle.hpp plans it)
// against what the turn allows (the planned lateral limit / bend), squared; 1 where the turn costs
// nothing.
std::vector<float> TurnCost(const std::vector<Vec3> &aPts, const PlanParams &aParams) {
  const size_t n = aPts.size();
  const auto &car = aParams.vehicle;
  const float lateral = car.margin * Lateral(car);
  std::vector<float> flat(n, 0.0f), free(n, TopSpeed(car)), cost(n, 1.0f);
  for (size_t i = 1; i < n; ++i) {
    flat[i] = std::hypot(aPts[i].x - aPts[i - 1].x, aPts[i].y - aPts[i - 1].y);
  }
  free[0] = 0.0f;
  for (size_t i = 1; i < n; ++i) {
    free[i] = std::min(free[i], std::sqrt(free[i - 1] * free[i - 1] + 2.0f * Accel(car, free[i - 1]) * Distance(aPts[i - 1], aPts[i])));
  }
  free[n - 1] = 0.0f;
  for (size_t i = n - 1; i-- > 0;) {
    free[i] = std::min(free[i], std::sqrt(free[i + 1] * free[i + 1] + 2.0f * PlanDecel(car, free[i + 1]) * Distance(aPts[i], aPts[i + 1])));
  }
  for (size_t i = 2; i + 2 < n; ++i) {
    float h1 = flat[i - 1] + flat[i], h2 = flat[i + 1] + flat[i + 2];
    if (h1 < 1.0f || h2 < 1.0f) {
      continue;
    }
    float ax = (aPts[i].x - aPts[i - 2].x) / h1, ay = (aPts[i].y - aPts[i - 2].y) / h1;
    float bx = (aPts[i + 2].x - aPts[i].x) / h2, by = (aPts[i + 2].y - aPts[i].y) / h2;
    float turn = std::acos(std::clamp(ax * bx + ay * by, -1.0f, 1.0f)) / (0.5f * (h1 + h2));
    if (turn > 1e-6f) {
      cost[i] = std::max(1.0f, free[i] * free[i] * turn / lateral);
    }
  }
  return cost;
}

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
    if ((aParams.lineTakeoff && along[i] <= aParams.lineFloorEnds) || (aParams.lineLanding && along.back() - along[i] <= aParams.lineFloorEnds)) {
      continue; // the take-off and the landing stay as they are (a car already flying has no take-off)
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
    hi[i] = extent[0] - aParams.lineCorridorMargin;
    lo[i] = -(extent[1] - aParams.lineCorridorMargin);
    if (lo[i] > hi[i]) {
      // narrower than both margins: where it already is stays allowed (made to keep to the
      // middle, the line weaves through every pinch)
      hi[i] = std::max(0.0f, hi[i]);
      lo[i] = std::min(0.0f, lo[i]);
    }
  }
  const float w = aParams.lineCurvatureRate;
  // rounds: the bends that cost the car speed (TurnCost) weighted up by that cost and the line
  // solved again, so it spreads those turns over the room there is and leaves the rest alone (the
  // smoothest shape alone slows the car in its turns)
  std::vector<float> weight(n, 1.0f);
  for (int round = 0; round <= aParams.lineTurnRounds; ++round) {
    if (round > 0) {
      std::vector<Vec3> now = aPts;
      for (size_t i = 0; i < n; ++i) {
        now[i].x += normal[i].x * d[i];
        now[i].y += normal[i].y * d[i];
      }
      std::vector<float> cost = TurnCost(now, aParams);
      for (size_t i = 0; i < n; ++i) {
        weight[i] = std::min(weight[i] * cost[i], aParams.lineTurnWeightMax);
      }
    }
    float heaviest = 1.0f;
    for (float x : weight) {
      heaviest = std::max(heaviest, x);
    }
    const float rate = 1.6f / (16.0f * heaviest + 64.0f * w); // stable below 2 / (16 weight + 64 w)
    SolveLateral(d, lo, hi, normal, aPts, weight, w, rate, aParams.lineCorridorIterations);
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
      if (moved && (!LegRunFree(aMap, solved, i) ||
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

} // namespace

// The line: gradient descent on the summed squared second differences of the points (their
// bend), each move kept only where both legs to the neighbours stay free, with lineSide to spare
// or at least no less than before. Points ahead of a corner move out and the apex moves in, so the
// bend spreads over all the room there is. Coarse spacings first (the bend of a long corner
// spreads over a few points per pass there), then finer ones down to lineSpacing; then
// SmoothLateral and SmoothHeights.
std::vector<Vec3> Line(const NavMap &aMap, const std::vector<Vec3> &aRoute, const PlanParams &aParams) {
  using Clock = std::chrono::steady_clock;
  auto ms = [](Clock::time_point aFrom) { return std::chrono::duration<double, std::milli>(Clock::now() - aFrom).count(); };
  const auto t0 = Clock::now();
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
    // the line moves away from a wall gradually (pushed point by point, at a pinch only the points
    // by one side move, and the line wiggles and kinks)
    const int reach = aParams.lineSpread > 0.0f ? int(std::round(aParams.lineSpread / spacing)) : 0;
    const float unit = aParams.linePush / aParams.lineStep; // undone by * step below
    std::vector<Vec3> push(n), spread(n);
    const auto tl0 = Clock::now();
    int ran = 0, checks = 0;
    for (int pass = 0; pass < passes; ++pass) {
      ++ran;
      float moved = 0.0f;
      // away from blocked space close across the line: probe lineClearance to either side and
      // above / below; where one side is blocked and the other free, push towards the free one.
      // (Smoothing alone leaves the line skimming the map's margin wherever a wall is on the
      // inside of a bend.)
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
          ++checks;
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
      if (moved < aParams.lineSettle * float(n)) {
        break; // settled: under lineSettle per point
      }
    }
    spdlog::debug("[Planner] passes at {:.0f} m: {} of {} passes, {} points, {} move checks, {:.0f} ms", spacing, ran, passes, n,
                 checks, ms(tl0));
  }
  const double passesMs = ms(t0);
  const auto t1 = Clock::now();
  SmoothLateral(aMap, pts, aParams);
  const double lateralMs = ms(t1);
  const auto t2 = Clock::now();
  // twice, evenly spaced each time: spaced along the line's old heights, the points stay bunched
  // where the heights then change most (a dive lifted over a beam)
  for (int pass = 0; pass < 2; ++pass) {
    pts = Respace(aMap, pts, aParams.lineSpacing);
    SmoothHeights(aMap, pts, aParams);
  }
  spdlog::info("[Planner] line stages: passes {:.0f} ms, across {:.0f} ms, heights {:.0f} ms ({} points)", passesMs,
               lateralMs, ms(t2), pts.size());
  return pts;
}

std::vector<Vec3> RepairLine(const NavMap &aMap, std::vector<Vec3> aLine, const std::vector<Vec3> &aRoute,
                             const PlanParams &aParams, float aSkip, int32_t &aRepaired) {
  aRepaired = 0;
  if (aLine.size() < 3 || aRoute.size() < 2) {
    return aLine;
  }
  const auto route = Resample(aRoute, aParams.lineSpacing);
  auto nearest = [&](Vec3 aP, size_t aFrom) {
    size_t best = aFrom;
    float bestD = kInf;
    for (size_t k = aFrom; k < route.size(); ++k) {
      float d = Distance(aP, route[k]);
      if (d < bestD) {
        bestD = d;
        best = k;
      }
    }
    return best;
  };
  // a leg is blocked when it, or it with its neighbours, grazes more than the line may (LegRunFree);
  // legs within aSkip of the start are the take-off from under cover and do not count
  auto firstBlocked = [&](size_t aFrom, size_t &aLast) {
    float along = 0.0f;
    for (size_t i = 0; i + 1 < aLine.size(); ++i) {
      if (i >= aFrom && along + Distance(aLine[i], aLine[i + 1]) > aSkip && !LegRunFree(aMap, aLine, i)) {
        aLast = i;
        while (aLast + 2 < aLine.size() && !LegRunFree(aMap, aLine, aLast + 1)) {
          ++aLast;
        }
        return i;
      }
      along += Distance(aLine[i], aLine[i + 1]);
    }
    return aLine.size();
  };
  size_t from = 0;
  for (int32_t round = 0; round < 32; ++round) {
    size_t last = 0;
    const size_t first = firstBlocked(from, last);
    if (first >= aLine.size()) {
      break;
    }
    const size_t n = aLine.size();
    bool fixed = false;
    for (size_t reach = 3;; reach *= 2) {
      const size_t a = first > reach ? first - reach : 0, c = std::min(n - 1, last + 1 + reach);
      const size_t ra = a == 0 ? 0 : nearest(aLine[a], 0), rc = c == n - 1 ? route.size() - 1 : nearest(aLine[c], ra);
      // the line to a, the route from about a to about c (free: its legs were checked), the line on
      // from c; the stretch then evened out by moving each point towards its neighbours' middle
      // wherever both its legs stay free, so the route's corners round off as far as the room lets
      std::vector<Vec3> piece{aLine[a]};
      piece.insert(piece.end(), route.begin() + ra, route.begin() + rc + 1);
      piece.push_back(aLine[c]);
      piece = Resample(piece, aParams.lineSpacing);
      std::vector<Vec3> pts(aLine.begin(), aLine.begin() + a);
      const size_t p0 = pts.size();
      pts.insert(pts.end(), piece.begin(), piece.end());
      const size_t p1 = pts.size() - 1;
      pts.insert(pts.end(), aLine.begin() + c + 1, aLine.end());
      for (int pass = 0; pass < 200; ++pass) {
        float moved = 0.0f;
        for (size_t i = std::max<size_t>(p0, 1); i < p1 && i + 1 < pts.size(); ++i) {
          const Vec3 prev = pts[i - 1], next = pts[i + 1], p = pts[i];
          const Vec3 q{p.x + 0.5f * (0.5f * (prev.x + next.x) - p.x), p.y + 0.5f * (0.5f * (prev.y + next.y) - p.y),
                       p.z + 0.5f * (0.5f * (prev.z + next.z) - p.z)};
          if (LegFree(aMap, prev, q) && LegFree(aMap, q, next)) {
            moved += Distance(p, q);
            pts[i] = q;
          }
        }
        if (moved < 0.01f * float(p1 - p0 + 1)) {
          break;
        }
      }
      float along = 0.0f;
      for (size_t i = 0; i < p0; ++i) {
        along += Distance(pts[i], pts[i + 1]);
      }
      const size_t s0 = p0 > 0 ? p0 - 1 : 0, s1 = std::min(p1 + 1, pts.size() - 1);
      const float skip = std::max(0.0f, aSkip - along);
      if (LongestBlocked(aMap, std::vector<Vec3>(pts.begin() + s0, pts.begin() + s1 + 1), skip) <= kGrazeAllowedLine) {
        from = p1;
        aLine = std::move(pts);
        ++aRepaired;
        fixed = true;
        break;
      }
      if (a == 0 && c == n - 1) {
        break;
      }
    }
    if (!fixed) {
      spdlog::warn("[Planner] the line stays blocked at {:.1f} {:.1f} {:.1f}", aLine[first].x, aLine[first].y, aLine[first].z);
      from = last + 1;
    }
  }
  return aLine;
}

} // namespace Navigation::detail
