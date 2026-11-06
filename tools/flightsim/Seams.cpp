#include "Seams.hpp"

#include <cmath>
#include <limits>
#include <unordered_map>

namespace FlightSim {

using namespace Flight;

namespace {
// a corner on the weld grid
struct Key {
  int64_t x, y, z;
  bool operator==(const Key &o) const { return x == o.x && y == o.y && z == o.z; }
};
Key Weld(Vec3 aP) {
  return {std::llround(aP.x / kSeamWeld), std::llround(aP.y / kSeamWeld), std::llround(aP.z / kSeamWeld)};
}
struct EdgeKey {
  Key a, b; // ordered
  bool operator==(const EdgeKey &o) const { return a == o.a && b == o.b; }
};
struct EdgeHash {
  size_t operator()(const EdgeKey &k) const {
    auto h = [](const Key &p) { return size_t(p.x * 73856093) ^ size_t(p.y * 19349663) ^ size_t(p.z * 83492791); };
    return h(k.a) * 31 + h(k.b);
  }
};
EdgeKey Edge(Vec3 aP, Vec3 aQ) {
  Key a = Weld(aP), b = Weld(aQ);
  bool less = a.x != b.x ? a.x < b.x : a.y != b.y ? a.y < b.y : a.z < b.z;
  return less ? EdgeKey{a, b} : EdgeKey{b, a};
}
} // namespace

std::vector<Seam> FindSeams(const Colliders &aWorld) {
  // the drivable triangles' edges, and how many drivable triangles share each
  struct Use {
    int count = 0;
    size_t tri = 0;
    int edge = 0;
  };
  std::unordered_map<EdgeKey, Use, EdgeHash> edges;
  std::vector<size_t> drivable;
  for (size_t i = 0; i < aWorld.Size(); ++i) {
    Vec3 v[3];
    Colliders::Kind kind;
    aWorld.Triangle(i, v[0], v[1], v[2], kind);
    Vec3 n = Cross(Sub(v[1], v[0]), Sub(v[2], v[0]));
    float area = Length(n);
    if (area < 1e-6f || std::fabs(n.z) / area < kSeamSlope)
      continue;
    drivable.push_back(i);
    for (int e = 0; e < 3; ++e) {
      Use &u = edges[Edge(v[e], v[(e + 1) % 3])];
      ++u.count;
      u.tri = i;
      u.edge = e;
    }
  }

  std::vector<Seam> seams;
  for (const auto &[key, use] : edges) {
    if (use.count != 1)
      continue;
    Vec3 v[3];
    Seam s;
    aWorld.Triangle(use.tri, v[0], v[1], v[2], s.inside);
    Vec3 p = v[use.edge], q = v[(use.edge + 1) % 3], c = v[(use.edge + 2) % 3];
    Vec3 along = Sub(q, p);
    s.length = Length(along);
    if (s.length < kSeamMinLength)
      continue;
    s.at = Mul(Flight::Add(p, q), 0.5f);
    // level, across the edge, away from the triangle's third corner
    Vec3 out{along.y, -along.x, 0.0f};
    float ol = Length(out);
    if (ol < 1e-6f)
      continue;
    out = Mul(out, 1.0f / ol);
    if (Dot(out, Sub(c, s.at)) > 0.0f)
      out = Mul(out, -1.0f);
    s.out = out;
    // this side's ground just inside, and the ground across
    Colliders::Hit in, across;
    Vec3 pi = Flight::Add(s.at, Mul(out, -kSeamProbe));
    if (!aWorld.Ray({pi.x, pi.y, s.at.z + kSeamReach}, {pi.x, pi.y, s.at.z - kSeamReach}, in))
      continue;
    float d = kSeamProbe;
    bool found = false;
    for (; d <= kSeamGapMax + 1e-4f; d += kSeamProbe) {
      Vec3 po = Flight::Add(s.at, Mul(out, d));
      if (aWorld.Ray({po.x, po.y, in.position.z + kSeamReach}, {po.x, po.y, in.position.z - kSeamReach}, across)) {
        found = true;
        break;
      }
    }
    if (!found) {
      continue; // the ground ends here: an edge, not a seam
    }
    s.step = across.position.z - in.position.z;
    s.gap = d > kSeamProbe + 1e-4f ? d - kSeamProbe : 0.0f;
    s.outside = across.kind;
    seams.push_back(s);
  }
  return seams;
}

} // namespace FlightSim
