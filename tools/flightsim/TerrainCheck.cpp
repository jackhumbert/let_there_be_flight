#include "TerrainCheck.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>

namespace FlightSim {

using namespace Flight;

namespace {
constexpr float kDeg = 57.2957795f;
constexpr size_t kRowColumns = 31, kRayColumns = 10;

float Percentile(std::vector<float> aValues, float aP) {
  if (aValues.empty())
    return 0.0f;
  size_t k = std::min(aValues.size() - 1, size_t(aP * float(aValues.size() - 1) + 0.5f));
  std::nth_element(aValues.begin(), aValues.begin() + k, aValues.end());
  return aValues[k];
}
} // namespace

TerrainRecording LoadTerrainRecording(const std::string &aPath) {
  TerrainRecording rec;
  rec.rows = LoadRecording(aPath);
  std::ifstream in(aPath);
  std::string line;
  std::getline(in, line);
  while (std::getline(in, line)) {
    std::vector<float> v;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ','))
      v.push_back(std::strtof(cell.c_str(), nullptr));
    if (v.size() < kRowColumns)
      continue; // LoadRecording skipped it too
    std::array<RecordedRay, 4> rays{};
    if (v.size() >= kRowColumns + 4 * kRayColumns)
      for (size_t i = 0; i < 4; ++i) {
        const float *r = &v[kRowColumns + i * kRayColumns];
        rays[i] = {{r[0], r[1], r[2]}, {r[4], r[5], r[6]}, {r[7], r[8], r[9]}, r[3] > 0.5f};
      }
    rec.rays.push_back(rays);
  }
  return rec;
}

GroundReport GroundCheck(const TerrainRecording &aRecording, const Colliders &aRegion, const Car &aCar, float aDz,
                         size_t aSpots) {
  GroundReport g;
  // the car's underside points, body frame: the tyres, and the chassis's profile (the centre at the bottom without)
  std::vector<Vec3> under;
  for (float sx : {-1.0f, 1.0f})
    for (float sy : {-1.0f, 1.0f})
      under.push_back({sx * aCar.wheelX, sy * aCar.wheelY, aCar.bottom});
  if (aCar.underside.empty())
    under.push_back({0, 0, aCar.bottom});
  under.insert(under.end(), aCar.underside.begin(), aCar.underside.end());
  g.gameClearanceMin = kClearanceReach;
  bool touching = false;
  std::vector<float> dz, angle;
  struct Cell {
    double gameZ = 0, regionZ = 0;
    int game = 0, region = 0, rays = 0;
    float worst = 0;
  };
  std::map<std::pair<int, int>, Cell> cells;
  Vec3 wheelSum;
  int wheelCount = 0;
  for (size_t i = 0; i < aRecording.rays.size() && i < aRecording.rows.size(); ++i) {
    const Recorded &row = aRecording.rows[i];
    Vec3 up = Cross(row.right, row.forward);
    float clear = kClearanceReach;
    for (Vec3 p : under) {
      Vec3 w = Flight::Add(row.position,
                           Flight::Add(Mul(row.right, p.x), Flight::Add(Mul(row.forward, p.y), Mul(up, p.z))));
      Colliders::Hit h;
      if (aRegion.Ray({w.x, w.y, std::max(row.position.z, w.z)}, {w.x, w.y, w.z - kClearanceReach}, h))
        clear = std::min(clear, w.z - h.position.z);
    }
    g.gameClearanceMin = std::min(g.gameClearanceMin, clear);
    g.gameTouchFrames += clear < kTouch;
    g.gameTouches += clear < kTouch && !touching;
    touching = clear < kTouch;
    for (const RecordedRay &r : aRecording.rays[i]) {
      if (Length(r.origin) < 1e-3f)
        continue; // no wheel emitter
      Vec3 d = Sub(r.origin, row.position);
      wheelSum = Flight::Add(wheelSum, Vec3{std::fabs(Dot(d, row.right)), std::fabs(Dot(d, row.forward)), Dot(d, up)});
      ++wheelCount;
      ++g.rays;
      // the same ray: from the emitter straight down, as far as FindGround casts (to the recorded hit, or 17 m)
      Colliders::Hit h;
      bool region = aRegion.Ray(r.origin, {r.origin.x, r.origin.y, r.origin.z - 17.0f}, h);
      if (r.valid && region) {
        ++g.both;
        float diff = r.hit.z - h.position.z;
        dz.push_back(std::fabs(diff));
        angle.push_back(std::acos(std::clamp(Dot(Normalize(r.normal), h.normal), -1.0f, 1.0f)) * kDeg);
        if (std::fabs(diff) < aDz)
          continue;
      } else if (r.valid) {
        ++g.gameOnly;
      } else if (region) {
        ++g.regionOnly;
      } else {
        ++g.neither;
        continue;
      }
      Cell &c = cells[{int(std::floor(r.origin.x / 2.0f)), int(std::floor(r.origin.y / 2.0f))}];
      ++c.rays;
      if (r.valid) {
        c.gameZ += r.hit.z;
        ++c.game;
      }
      if (region) {
        c.regionZ += h.position.z;
        ++c.region;
      }
      c.worst = std::max(c.worst, r.valid && region ? std::fabs(r.hit.z - h.position.z) : 100.0f);
    }
  }
  g.dzP50 = Percentile(dz, 0.5f);
  g.dzP95 = Percentile(dz, 0.95f);
  g.dzMax = dz.empty() ? 0.0f : *std::max_element(dz.begin(), dz.end());
  g.normalP95 = Percentile(angle, 0.95f);
  if (wheelCount)
    g.wheel = Mul(wheelSum, 1.0f / float(wheelCount));
  std::vector<std::pair<float, GroundReport::Spot>> spots;
  constexpr float kNan = std::numeric_limits<float>::quiet_NaN();
  for (const auto &[key, c] : cells)
    spots.push_back({c.worst * float(c.rays),
                     {float(key.first) * 2.0f + 1.0f, float(key.second) * 2.0f + 1.0f,
                      c.game ? float(c.gameZ / c.game) : kNan, c.region ? float(c.regionZ / c.region) : kNan,
                      c.rays}});
  std::sort(spots.begin(), spots.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
  for (size_t i = 0; i < spots.size() && i < aSpots; ++i)
    g.spots.push_back(spots[i].second);
  return g;
}

std::vector<TerrainPiece> TerrainReplay(const TerrainRecording &aRecording, const Colliders &aRegion,
                                        const Setup &aSetup, int aHoverFlyMode, float aWindow,
                                        const std::string &aTrace) {
  std::vector<TerrainPiece> pieces;
  const auto &rows = aRecording.rows;
  FILE *trace = aTrace.empty() ? nullptr : std::fopen(aTrace.c_str(), "w");
  if (trace)
    std::fprintf(trace, "t,piece,game_z,sim_z,game_vz,sim_vz,game_distance,sim_distance,sim_contact\n");
  size_t a = 0;
  while (a < rows.size()) {
    if (rows[a].mode != aHoverFlyMode) {
      ++a;
      continue;
    }
    // the window: aWindow s of rows in Hover & Fly
    size_t b = a + 1;
    while (b < rows.size() && rows[b].mode == aHoverFlyMode && rows[b].t - rows[a].t < aWindow)
      ++b;
    if (b - a >= 10) {
      Sim sim(aSetup.car, Flat(-1e9f));
      Configure(sim, aSetup);
      sim.groundRay = RayOn(aRegion);
      const Recorded &r0 = rows[a];
      sim.Place(r0.position, 0.0f);
      sim.SetState({r0.position, r0.velocity, r0.forward, r0.right, r0.angularVelocity, r0.hovering, r0.hoverHeight,
                    r0.referenceZ, r0.distance, r0.holding});
      TerrainPiece p;
      p.start = r0.t;
      double h2 = 0, d2 = 0;
      int hn = 0, dn = 0;
      bool wasContact = false;
      for (size_t i = a + 1; i < b; ++i) {
        const Recorded &r = rows[i];
        Frame f = sim.Advance(r.controls, nullptr, r.dt);
        float dh = f.position.z - r.position.z;
        h2 += double(dh) * dh;
        ++hn;
        p.heightMax = std::max(p.heightMax, std::fabs(dh));
        // the game found ground when all four of its rays hit (GroundFromHits)
        const auto &rays = aRecording.rays[i];
        bool gameFound = rays[0].valid && rays[1].valid && rays[2].valid && rays[3].valid;
        if (gameFound && f.foundGround) {
          float dd = f.groundDistance - r.distance;
          d2 += double(dd) * dd;
          ++dn;
        }
        if (std::fabs(r.velocity.z) > std::fabs(p.gameClimb))
          p.gameClimb = r.velocity.z;
        if (std::fabs(f.velocity.z) > std::fabs(p.simClimb))
          p.simClimb = f.velocity.z;
        p.simContacts += f.contact && !wasContact;
        wasContact = f.contact;
        if (trace)
          std::fprintf(trace, "%.4f,%zu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d\n", r.t, pieces.size(), r.position.z,
                       f.position.z, r.velocity.z, f.velocity.z, gameFound ? r.distance : -1.0f,
                       f.foundGround ? f.groundDistance : -1.0f, f.contact ? 1 : 0);
      }
      p.heightRms = hn ? float(std::sqrt(h2 / hn)) : 0.0f;
      p.distanceRms = dn ? float(std::sqrt(d2 / dn)) : 0.0f;
      p.length = rows[b - 1].t - r0.t;
      pieces.push_back(p);
    }
    a = b;
  }
  if (trace)
    std::fclose(trace);
  return pieces;
}

} // namespace FlightSim
