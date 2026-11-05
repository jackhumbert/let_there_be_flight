// The drag area from the collision shape (Flight/Aero.hpp).
#include "Flight/Aero.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Flight {

namespace {

struct P2 {
  float x, y;
};

float Cross(P2 o, P2 a, P2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); }

// counter-clockwise convex hull (monotone chain); fewer than 3 points when the points are degenerate
std::vector<P2> Hull(std::vector<P2> aPoints) {
  std::sort(aPoints.begin(), aPoints.end(), [](P2 a, P2 b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
  if (aPoints.size() < 3)
    return aPoints;
  std::vector<P2> h(2 * aPoints.size());
  size_t k = 0;
  for (size_t i = 0; i < aPoints.size(); ++i) {
    while (k >= 2 && Cross(h[k - 2], h[k - 1], aPoints[i]) <= 0.0f)
      --k;
    h[k++] = aPoints[i];
  }
  for (size_t i = aPoints.size() - 1, t = k + 1; i-- > 0;) {
    while (k >= t && Cross(h[k - 2], h[k - 1], aPoints[i]) <= 0.0f)
      --k;
    h[k++] = aPoints[i];
  }
  h.resize(k - 1);
  return h;
}

// the union of the pieces' projections onto the plane across axis aAxis (0 X, 1 Y, 2 Z), m2
float Silhouette(const std::vector<ShapePiece> &aPieces, int aAxis, float aCell) {
  std::vector<std::vector<P2>> hulls;
  float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
  for (const auto &piece : aPieces) {
    std::vector<P2> flat;
    flat.reserve(piece.size());
    for (const Vec3 &v : piece) {
      const P2 p = aAxis == 0 ? P2{v.y, v.z} : aAxis == 1 ? P2{v.x, v.z} : P2{v.x, v.y};
      flat.push_back(p);
      x0 = std::min(x0, p.x), y0 = std::min(y0, p.y), x1 = std::max(x1, p.x), y1 = std::max(y1, p.y);
    }
    auto h = Hull(std::move(flat));
    if (h.size() >= 3)
      hulls.push_back(std::move(h));
  }
  if (hulls.empty())
    return 0.0f;
  const int nx = int(std::ceil((x1 - x0) / aCell)) + 1, ny = int(std::ceil((y1 - y0) / aCell)) + 1;
  std::vector<uint8_t> mask(size_t(nx) * size_t(ny), 0);
  for (const auto &h : hulls) {
    float hx0 = 1e9f, hy0 = 1e9f, hx1 = -1e9f, hy1 = -1e9f;
    for (P2 p : h)
      hx0 = std::min(hx0, p.x), hy0 = std::min(hy0, p.y), hx1 = std::max(hx1, p.x), hy1 = std::max(hy1, p.y);
    const int i0 = std::max(0, int(std::floor((hx0 - x0) / aCell))), i1 = std::min(nx, int(std::ceil((hx1 - x0) / aCell)) + 1);
    const int j0 = std::max(0, int(std::floor((hy0 - y0) / aCell))), j1 = std::min(ny, int(std::ceil((hy1 - y0) / aCell)) + 1);
    for (int j = j0; j < j1; ++j) {
      const P2 c{0.0f, y0 + (float(j) + 0.5f) * aCell};
      for (int i = i0; i < i1; ++i) {
        const P2 q{x0 + (float(i) + 0.5f) * aCell, c.y};
        bool inside = true;
        for (size_t k = 0; k < h.size() && inside; ++k)
          inside = Cross(h[k], h[(k + 1) % h.size()], q) >= 0.0f;
        if (inside)
          mask[size_t(j) * size_t(nx) + size_t(i)] = 1;
      }
    }
  }
  size_t count = 0;
  for (uint8_t m : mask)
    count += m;
  return float(count) * aCell * aCell;
}

float Interpolate(float aX, const float *aXs, const float *aYs, int aN) {
  if (aX <= aXs[0])
    return aYs[0];
  for (int i = 1; i < aN; ++i)
    if (aX <= aXs[i])
      return aYs[i - 1] + (aYs[i] - aYs[i - 1]) * (aX - aXs[i - 1]) / (aXs[i] - aXs[i - 1]);
  return aYs[aN - 1];
}

} // namespace

Vec3 SilhouetteAreas(const std::vector<ShapePiece> &aPieces, float aCell) {
  return {Silhouette(aPieces, 0, aCell), Silhouette(aPieces, 1, aCell), Silhouette(aPieces, 2, aCell)};
}

float ShapeHeight(const std::vector<ShapePiece> &aPieces) {
  float lo = 1e9f, hi = -1e9f;
  for (const auto &piece : aPieces)
    for (const Vec3 &v : piece)
      lo = std::min(lo, v.z), hi = std::max(hi, v.z);
  return hi >= lo ? hi - lo : 0.0f;
}

Vec3 DragAreaOf(Vec3 aAreas, float aHeight, bool aBike) {
  if (aBike)
    return {1.0f * aAreas.x + 0.6f, 0.6f * aAreas.y + 0.35f, 1.0f * aAreas.z + 0.3f};
  static const float kHeights[] = {1.25f, 1.35f, 1.8f, 2.35f};
  static const float kFront[] = {0.30f, 0.33f, 0.42f, 0.70f};
  static const float kSideHeights[] = {1.8f, 2.35f};
  static const float kSide[] = {1.0f, 1.1f};
  const float front = Interpolate(aHeight, kHeights, kFront, 4);
  const float side = Interpolate(aHeight, kSideHeights, kSide, 2);
  return {side * aAreas.x, front * aAreas.y, side * aAreas.z};
}

ShapePiece RoundPiece(const std::vector<Vec3> &aCentres, float aRadius) {
  ShapePiece out;
  constexpr int kRing = 16;
  for (const Vec3 &c : aCentres)
    for (int i = 0; i < kRing; ++i) {
      const float a = 2.0f * 3.14159265f * float(i) / float(kRing);
      const float u = aRadius * std::cos(a), w = aRadius * std::sin(a);
      out.push_back({c.x, c.y + u, c.z + w});
      out.push_back({c.x + u, c.y, c.z + w});
      out.push_back({c.x + u, c.y + w, c.z});
    }
  return out;
}

} // namespace Flight
