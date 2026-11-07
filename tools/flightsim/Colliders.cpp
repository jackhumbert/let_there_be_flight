#include "Colliders.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

namespace FlightSim {

using namespace Flight;

namespace {
Vec3 Min3(Vec3 a, Vec3 b) { return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}; }
Vec3 Max3(Vec3 a, Vec3 b) { return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}; }
} // namespace

bool Colliders::Load(const std::string &aPath, std::string *aError) {
  auto fail = [&](const char *aWhy) {
    if (aError)
      *aError = aPath + ": " + aWhy;
    return false;
  };
  std::ifstream f(aPath, std::ios::binary);
  if (!f)
    return fail("cannot be read");
  char magic[8];
  uint32_t version = 0, n = 0;
  float bounds[6];
  f.read(magic, 8);
  f.read(reinterpret_cast<char *>(&version), 4);
  f.read(reinterpret_cast<char *>(&n), 4);
  f.read(reinterpret_cast<char *>(bounds), sizeof(bounds));
  if (!f || std::memcmp(magic, "LTBFREG1", 8) != 0)
    return fail("not a region file");
  if (version != 1 && version != 2)
    return fail("region file version is not 1 or 2");
  std::vector<float> corners(size_t(n) * 9);
  std::vector<uint8_t> kinds(n);
  std::vector<uint32_t> meshes(n, 0);
  f.read(reinterpret_cast<char *>(corners.data()), std::streamsize(corners.size() * sizeof(float)));
  f.read(reinterpret_cast<char *>(kinds.data()), std::streamsize(kinds.size()));
  if (version >= 2)
    f.read(reinterpret_cast<char *>(meshes.data()), std::streamsize(meshes.size() * sizeof(uint32_t)));
  if (!f)
    return fail("cut short");
  tris.clear();
  tris.reserve(n);
  for (uint32_t i = 0; i < n; ++i) {
    const float *c = &corners[size_t(i) * 9];
    Add({c[0], c[1], c[2]}, {c[3], c[4], c[5]}, {c[6], c[7], c[8]}, Kind(kinds[i]), meshes[i]);
  }
  Build();
  return true;
}

void Colliders::Add(Vec3 aA, Vec3 aB, Vec3 aC, Kind aKind, uint32_t aMesh) {
  tris.push_back({aA, Sub(aB, aA), Sub(aC, aA), aKind, aMesh});
}

void Colliders::Build(float aCell) {
  cell = aCell;
  constexpr float kInf = std::numeric_limits<float>::infinity();
  lo = {kInf, kInf, kInf};
  hi = {-kInf, -kInf, -kInf};
  for (const Tri &t : tris) {
    lo = Min3(lo, Min3(t.a, Min3(Flight::Add(t.a, t.e1), Flight::Add(t.a, t.e2))));
    hi = Max3(hi, Max3(t.a, Max3(Flight::Add(t.a, t.e1), Flight::Add(t.a, t.e2))));
  }
  if (tris.empty()) {
    nx = ny = 0;
    starts.assign(1, 0);
    items.clear();
    return;
  }
  nx = std::max(1, int32_t(std::ceil((hi.x - lo.x) / cell)) + 1);
  ny = std::max(1, int32_t(std::ceil((hi.y - lo.y) / cell)) + 1);
  // two passes: count each square's triangles, then fill
  auto squares = [&](const Tri &t, int32_t &x0, int32_t &y0, int32_t &x1, int32_t &y1) {
    Vec3 b = Flight::Add(t.a, t.e1), c = Flight::Add(t.a, t.e2);
    x0 = int32_t((std::min({t.a.x, b.x, c.x}) - lo.x) / cell);
    y0 = int32_t((std::min({t.a.y, b.y, c.y}) - lo.y) / cell);
    x1 = int32_t((std::max({t.a.x, b.x, c.x}) - lo.x) / cell);
    y1 = int32_t((std::max({t.a.y, b.y, c.y}) - lo.y) / cell);
  };
  starts.assign(size_t(nx) * ny + 1, 0);
  for (const Tri &t : tris) {
    int32_t x0, y0, x1, y1;
    squares(t, x0, y0, x1, y1);
    for (int32_t y = y0; y <= y1; ++y)
      for (int32_t x = x0; x <= x1; ++x)
        ++starts[size_t(y) * nx + x + 1];
  }
  for (size_t i = 1; i < starts.size(); ++i)
    starts[i] += starts[i - 1];
  items.resize(starts.back());
  std::vector<uint32_t> fill(starts.begin(), starts.end() - 1);
  for (uint32_t i = 0; i < tris.size(); ++i) {
    int32_t x0, y0, x1, y1;
    squares(tris[i], x0, y0, x1, y1);
    for (int32_t y = y0; y <= y1; ++y)
      for (int32_t x = x0; x <= x1; ++x)
        items[fill[size_t(y) * nx + x]++] = i;
  }
}

bool Colliders::Ray(Vec3 aFrom, Vec3 aTo, Hit &aHit) const {
  if (tris.empty())
    return false;
  Vec3 d = Sub(aTo, aFrom);
  int32_t x0 = int32_t(std::floor((std::min(aFrom.x, aTo.x) - lo.x) / cell));
  int32_t y0 = int32_t(std::floor((std::min(aFrom.y, aTo.y) - lo.y) / cell));
  int32_t x1 = int32_t(std::floor((std::max(aFrom.x, aTo.x) - lo.x) / cell));
  int32_t y1 = int32_t(std::floor((std::max(aFrom.y, aTo.y) - lo.y) / cell));
  x0 = std::max(x0, 0);
  y0 = std::max(y0, 0);
  x1 = std::min(x1, nx - 1);
  y1 = std::min(y1, ny - 1);
  float best = 2.0f;
  const Tri *hit = nullptr;
  for (int32_t y = y0; y <= y1; ++y)
    for (int32_t x = x0; x <= x1; ++x) {
      size_t s = size_t(y) * nx + x;
      for (uint32_t k = starts[s]; k < starts[s + 1]; ++k) {
        // Moller-Trumbore, both faces
        const Tri &t = tris[items[k]];
        Vec3 p = Cross(d, t.e2);
        float det = Dot(t.e1, p);
        if (std::fabs(det) < 1e-12f)
          continue;
        float inv = 1.0f / det;
        Vec3 s0 = Sub(aFrom, t.a);
        float u = Dot(s0, p) * inv;
        if (u < 0.0f || u > 1.0f)
          continue;
        Vec3 q = Cross(s0, t.e1);
        float v = Dot(d, q) * inv;
        if (v < 0.0f || u + v > 1.0f)
          continue;
        float tt = Dot(t.e2, q) * inv;
        if (tt >= 0.0f && tt <= 1.0f && tt < best) {
          best = tt;
          hit = &t;
        }
      }
    }
  if (!hit)
    return false;
  Vec3 n = Normalize(Cross(hit->e1, hit->e2));
  if (Dot(n, d) > 0.0f)
    n = Mul(n, -1.0f);
  aHit.position = Flight::Add(aFrom, Mul(d, best));
  aHit.normal = n;
  aHit.t = best;
  aHit.kind = hit->kind;
  return true;
}

GroundRay RayOn(const Colliders &aColliders) {
  return [&aColliders](Vec3 aFrom, Vec3 aTo, Vec3 &aHit, Vec3 &aNormal) {
    Colliders::Hit h;
    if (!aColliders.Ray(aFrom, aTo, h))
      return false;
    aHit = h.position;
    aNormal = h.normal;
    return true;
  };
}

} // namespace FlightSim
