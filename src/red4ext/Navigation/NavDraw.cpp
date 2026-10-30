#include "NavDraw.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Navigation {

namespace {
// Free height ranges of a column within [aZMin, aZMax], bottom up.
void FreeRanges(NavMap::View &aView, int32_t aCx, int32_t aCy, float aZMin, float aZMax,
                std::vector<std::pair<float, float>> &aOut) {
  aOut.clear();
  const auto &map = aView.Map();
  const uint16_t *gaps = nullptr;
  uint32_t count = aView.Column(aCx, aCy, gaps);
  for (uint32_t g = 0; g < count; ++g) {
    float bottom = std::max(map.VoxelZ(gaps[2 * g]), aZMin), top = std::min(map.VoxelZ(gaps[2 * g + 1]), aZMax);
    if (top > bottom) {
      aOut.emplace_back(bottom, top);
    }
  }
}

// Height ranges free in exactly one of two columns (sorted ranges in, sorted ranges out).
void Differ(const std::vector<std::pair<float, float>> &aA, const std::vector<std::pair<float, float>> &aB,
            std::vector<std::pair<float, float>> &aOut) {
  aOut.clear();
  std::vector<float> cuts;
  for (const auto &[b, t] : aA) {
    cuts.push_back(b);
    cuts.push_back(t);
  }
  for (const auto &[b, t] : aB) {
    cuts.push_back(b);
    cuts.push_back(t);
  }
  std::sort(cuts.begin(), cuts.end());
  auto inside = [](const std::vector<std::pair<float, float>> &aR, float aZ) {
    for (const auto &[b, t] : aR) {
      if (aZ >= b && aZ < t) {
        return true;
      }
    }
    return false;
  };
  for (size_t i = 0; i + 1 < cuts.size(); ++i) {
    float lo = cuts[i], hi = cuts[i + 1];
    if (hi - lo < 1e-3f) {
      continue;
    }
    float mid = 0.5f * (lo + hi);
    if (inside(aA, mid) != inside(aB, mid)) {
      if (!aOut.empty() && std::abs(aOut.back().second - lo) < 1e-3f) {
        aOut.back().second = hi;
      } else {
        aOut.emplace_back(lo, hi);
      }
    }
  }
}

// Height ranges where the four columns around a cell corner meet at a corner of the free space:
// one or three of them free, or two diagonal ones (two side by side make a flat wall, no edge).
// aA, aB, aC, aD: the cell, its +x, +y and +x+y neighbours.
void Corners(const std::vector<std::pair<float, float>> *aColumns[4], std::vector<std::pair<float, float>> &aOut) {
  aOut.clear();
  std::vector<float> cuts;
  for (int32_t c = 0; c < 4; ++c) {
    for (const auto &[b, t] : *aColumns[c]) {
      cuts.push_back(b);
      cuts.push_back(t);
    }
  }
  std::sort(cuts.begin(), cuts.end());
  auto inside = [](const std::vector<std::pair<float, float>> &aR, float aZ) {
    for (const auto &[b, t] : aR) {
      if (aZ >= b && aZ < t) {
        return true;
      }
    }
    return false;
  };
  for (size_t i = 0; i + 1 < cuts.size(); ++i) {
    float lo = cuts[i], hi = cuts[i + 1];
    if (hi - lo < 1e-3f) {
      continue;
    }
    float mid = 0.5f * (lo + hi);
    bool f[4];
    int32_t free = 0;
    for (int32_t c = 0; c < 4; ++c) {
      f[c] = inside(*aColumns[c], mid);
      free += f[c];
    }
    bool corner = free == 1 || free == 3 || (free == 2 && f[0] == f[3]);
    if (corner) {
      if (!aOut.empty() && std::abs(aOut.back().second - lo) < 1e-3f) {
        aOut.back().second = hi;
      } else {
        aOut.emplace_back(lo, hi);
      }
    }
  }
}

} // namespace

void Outlines(const NavMap &aMap, float aX, float aY, float aRadius, const std::vector<float> &aHeights,
              std::vector<DrawPoint> &aOut) {
  aOut.clear();
  int32_t cx, cy;
  if (!aMap.IsLoaded() || !aMap.CellOf(aX, aY, cx, cy) || aHeights.empty()) {
    return;
  }
  const float cell = aMap.CellSize(), half = 0.5f * cell;
  const int32_t n = std::max(1, int32_t(std::ceil(aRadius / cell)));
  const int32_t side = 2 * n + 1;
  NavMap::View view(aMap);
  // each column's gaps once, then a blocked grid per height
  std::vector<std::vector<std::pair<float, float>>> columns(size_t(side) * side);
  std::vector<uint8_t> inside(size_t(side) * side, 0);
  for (int32_t j = 0; j < side; ++j) {
    for (int32_t i = 0; i < side; ++i) {
      int32_t di = i - n, dj = j - n;
      if (di * di + dj * dj > n * n) {
        continue;
      }
      size_t k = size_t(j) * side + i;
      inside[k] = 1;
      const uint16_t *gaps = nullptr;
      uint32_t count = view.Column(cx + di, cy + dj, gaps);
      for (uint32_t g = 0; g < count; ++g) {
        columns[k].emplace_back(aMap.VoxelZ(gaps[2 * g]), aMap.VoxelZ(gaps[2 * g + 1]));
      }
    }
  }
  std::vector<uint8_t> blocked(size_t(side) * side);
  for (uint32_t h = 0; h < aHeights.size(); ++h) {
    const float z = aHeights[h];
    for (size_t k = 0; k < blocked.size(); ++k) {
      bool free = false;
      for (const auto &[bottom, top] : columns[k]) {
        if (z >= bottom && z < top) {
          free = true;
          break;
        }
      }
      blocked[k] = inside[k] && !free;
    }
    for (int32_t j = 0; j < side; ++j) {
      for (int32_t i = 0; i < side; ++i) {
        size_t k = size_t(j) * side + i;
        if (!blocked[k]) {
          continue;
        }
        float x, y;
        aMap.CellCentre(cx + i - n, cy + j - n, x, y);
        // an edge towards each free neighbour inside the disc
        auto edge = [&](int32_t ni, int32_t nj, float ax, float ay, float bx, float by) {
          if (ni < 0 || nj < 0 || ni >= side || nj >= side) {
            return;
          }
          size_t m = size_t(nj) * side + ni;
          if (inside[m] && !blocked[m]) {
            aOut.push_back({ax, ay, z});
            aOut.push_back({bx, by, z});
          }
        };
        edge(i + 1, j, x + half, y - half, x + half, y + half);
        edge(i - 1, j, x - half, y - half, x - half, y + half);
        edge(i, j + 1, x - half, y + half, x + half, y + half);
        edge(i, j - 1, x - half, y - half, x + half, y - half);
      }
    }
  }
}


void NavEdges(const NavMap &aMap, float aX, float aY, float aRadius, float aZMin, float aZMax,
              std::vector<DrawPoint> &aOut) {
  aOut.clear();
  int32_t cx, cy;
  if (aMap.IsLoaded() && aMap.CellOf(aX, aY, cx, cy)) {
    const float cell = aMap.CellSize();
    NavMap::View view(aMap);
    std::vector<std::pair<float, float>> here, differ, east, north, diagonal;
    // rings: (step in cells, outer radius in m)
    const std::pair<int32_t, float> rings[] = {{1, 60.0f}, {2, 150.0f}, {4, aRadius}};
    float inner = 0.0f;
    for (const auto &[step, outerRing] : rings) {
      const float outer = std::min(outerRing, aRadius);
      if (outer <= inner) {
        continue;
      }
      const int32_t n = int32_t(std::ceil(outer / (cell * step)));
      const float half = 0.5f * cell * step;
      for (int32_t j = -n; j <= n; ++j) {
        for (int32_t i = -n; i <= n; ++i) {
          float d = std::hypot(float(i), float(j)) * cell * step;
          if (d < inner || d >= outer) {
            continue;
          }
          int32_t ax = cx + i * step, ay = cy + j * step;
          FreeRanges(view, ax, ay, aZMin, aZMax, here);
          float x, y;
          aMap.CellCentre(ax, ay, x, y);
          FreeRanges(view, ax + step, ay, aZMin, aZMax, east);
          FreeRanges(view, ax, ay + step, aZMin, aZMax, north);
          FreeRanges(view, ax + step, ay + step, aZMin, aZMax, diagonal);
          // the +x and +y neighbours: each shared edge once
          for (int32_t k = 0; k < 2; ++k) {
            Differ(here, k == 0 ? east : north, differ);
            float ex0, ey0, ex1, ey1;
            if (k == 0) {
              ex0 = ex1 = x + half;
              ey0 = y - half;
              ey1 = y + half;
            } else {
              ey0 = ey1 = y + half;
              ex0 = x - half;
              ex1 = x + half;
            }
            for (const auto &[lo, hi] : differ) {
              aOut.push_back({ex0, ey0, lo});
              aOut.push_back({ex1, ey1, lo});
              aOut.push_back({ex0, ey0, hi});
              aOut.push_back({ex1, ey1, hi});
            }
          }
          // the corner the four share: vertical edges
          const std::vector<std::pair<float, float>> *columns[4] = {&here, &east, &north, &diagonal};
          Corners(columns, differ);
          for (const auto &[lo, hi] : differ) {
            aOut.push_back({x + half, y + half, lo});
            aOut.push_back({x + half, y + half, hi});
          }
        }
      }
      inner = outer;
    }
  }
}

} // namespace Navigation
