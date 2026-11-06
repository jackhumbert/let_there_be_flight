#include "TerrainRuns.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <limits>
#include <random>
#include <sstream>

namespace FlightSim {

using namespace Flight;

namespace {
constexpr float kDeg = 180.0f / 3.14159265f;

float Wrap(float aDeg) {
  while (aDeg > 180.0f)
    aDeg -= 360.0f;
  while (aDeg < -180.0f)
    aDeg += 360.0f;
  return aDeg;
}

float Percentile(std::vector<float> aValues, float aP) {
  if (aValues.empty())
    return 0.0f;
  size_t k = std::min(aValues.size() - 1, size_t(aP * float(aValues.size() - 1) + 0.5f));
  std::nth_element(aValues.begin(), aValues.begin() + k, aValues.end());
  return aValues[k];
}

float Rms(const std::vector<float> &aValues) {
  if (aValues.empty())
    return 0.0f;
  double sum = 0.0;
  for (float v : aValues)
    sum += double(v) * v;
  return float(std::sqrt(sum / aValues.size()));
}

Vec3 Flat2(Vec3 aV) { return {aV.x, aV.y, 0.0f}; }
} // namespace

std::vector<Line> LoadLines(const std::string &aPath) {
  std::vector<Line> lines;
  std::ifstream in(aPath);
  std::string row;
  while (std::getline(in, row)) {
    if (!row.empty() && row.back() == '\r')
      row.pop_back();
    if (row.empty() || row[0] == '#')
      continue;
    std::vector<std::string> col;
    std::stringstream ss(row);
    std::string cell;
    while (std::getline(ss, cell, '\t'))
      col.push_back(cell);
    if (col.size() < 6)
      continue;
    Line l;
    l.name = col[0];
    l.region = col[1];
    l.a = {std::strtof(col[2].c_str(), nullptr), std::strtof(col[3].c_str(), nullptr), 0.0f};
    l.b = {std::strtof(col[4].c_str(), nullptr), std::strtof(col[5].c_str(), nullptr), 0.0f};
    if (col.size() > 6 && !col[6].empty())
      l.z0 = std::strtof(col[6].c_str(), nullptr);
    l.tags = col.size() > 7 ? col[7] : "";
    lines.push_back(l);
  }
  return lines;
}

std::string FormatLine(const Line &aLine) {
  char z0[32] = "";
  if (std::isfinite(aLine.z0))
    std::snprintf(z0, sizeof(z0), "%.2f", aLine.z0);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "%s\t%s\t%.2f\t%.2f\t%.2f\t%.2f\t%s\t%s", aLine.name.c_str(), aLine.region.c_str(),
                aLine.a.x, aLine.a.y, aLine.b.x, aLine.b.y, z0, aLine.tags.c_str());
  return buf;
}

std::vector<Surface> Surfaces(const Colliders &aWorld, float aX, float aY) {
  std::vector<Surface> out;
  float top = aWorld.Max().z + 1.0f, bottom = aWorld.Min().z - 1.0f, above = std::numeric_limits<float>::infinity();
  Colliders::Hit h;
  for (float z = top; out.size() < 64 && aWorld.Ray({aX, aY, z}, {aX, aY, bottom}, h); z = h.position.z - 0.01f) {
    out.push_back({h.position.z, above - h.position.z, h.kind});
    above = h.position.z;
  }
  return out;
}

Profile GroundProfile(const Colliders &aWorld, Vec3 aA, Vec3 aB, float aZ0) {
  Profile p;
  Vec3 d = Sub(Flat2(aB), Flat2(aA));
  p.length = Length(d);
  if (p.length < kStep)
    return p;
  d = Mul(d, 1.0f / p.length);
  Colliders::Hit h;
  // the start: the topmost surface, or the one at or under aZ0 (from kFollowAbove over it)
  float from = std::isfinite(aZ0) ? aZ0 + kFollowAbove : aWorld.Max().z + 1.0f;
  if (!aWorld.Ray({aA.x, aA.y, from}, {aA.x, aA.y, aWorld.Min().z - 1.0f}, h))
    return p;
  p.points.push_back({0.0f, h.position.z, true, h.kind});
  float z = h.position.z;
  p.usable = p.length;
  for (float s = kStep; s <= p.length; s += kStep) {
    Vec3 prev = Flight::Add(aA, Mul(d, s - kStep)), next = Flight::Add(aA, Mul(d, s));
    bool wall = false;
    for (float above : kWallHeights)
      wall = wall || aWorld.Ray({prev.x, prev.y, z + above}, {next.x, next.y, z + above}, h);
    if (wall) {
      p.usable = s - kStep;
      break;
    }
    Profile::Point q{s, z, false, Colliders::Collision};
    if (aWorld.Ray({next.x, next.y, z + kFollowAbove}, {next.x, next.y, aWorld.Min().z - 1.0f}, h)) {
      q.z = h.position.z;
      q.found = true;
      q.kind = h.kind;
      z = q.z;
    }
    p.points.push_back(q);
  }

  // the numbers, over the usable part (the points are all in it)
  const auto &pts = p.points;
  size_t n = pts.size();
  if (n < 2)
    return p;
  int perTwo = int(2.0f / kStep), perTen = int(10.0f / kStep);
  std::vector<float> slopes;
  for (size_t i = 0; i + perTwo < n; ++i)
    if (pts[i].found && pts[i + perTwo].found)
      slopes.push_back(std::fabs(pts[i + perTwo].z - pts[i].z) / 2.0f);
  p.slopeP95 = Percentile(slopes, 0.95f);
  int terrain = 0, gaps = 0;
  for (size_t i = 0; i < n; ++i) {
    terrain += pts[i].found && pts[i].kind == Colliders::Terrain;
    gaps += !pts[i].found;
    if (i == 0 || !pts[i].found || !pts[i - 1].found)
      continue;
    float dz = pts[i].z - pts[i - 1].z;
    p.stepMax = std::max(p.stepMax, std::fabs(dz));
    // a kerb stands out of the slope round it: the change against the median change over 2 m either side
    std::vector<float> round;
    for (size_t k = i > size_t(perTwo) ? i - perTwo : 1; k <= std::min(n - 1, i + perTwo); ++k)
      if (k != i && pts[k].found && pts[k - 1].found)
        round.push_back(pts[k].z - pts[k - 1].z);
    float kerb = std::fabs(dz - Percentile(round, 0.5f));
    p.kerbs += kerb >= 0.08f && kerb <= 0.5f;
    p.steps += std::fabs(dz) > 0.5f;
    p.drops += dz < -1.0f;
  }
  p.terrain = float(terrain) / n;
  p.gaps = float(gaps) / n;
  std::vector<float> bumps;
  for (size_t i = 0; i < n; ++i) {
    size_t lo = i >= size_t(perTen / 2) ? i - perTen / 2 : 0, hi = std::min(n - 1, i + perTen / 2);
    double sum = 0;
    for (size_t k = lo; k <= hi; ++k)
      sum += pts[k].z;
    bumps.push_back(pts[i].z - float(sum / (hi - lo + 1)));
    if (i >= size_t(perTen) && i + perTen < n) {
      float mid = 0.5f * (pts[i - perTen].z + pts[i + perTen].z);
      p.crest = std::max(p.crest, pts[i].z - mid);
      p.dip = std::max(p.dip, mid - pts[i].z);
    }
  }
  p.bumpRms = Rms(bumps);
  return p;
}

std::string Profile::Tags() const {
  std::string t;
  auto add = [&](bool aIf, const char *aTag) {
    if (aIf)
      t += (t.empty() ? "" : ",") + std::string(aTag);
  };
  add(slopeP95 < 0.03f && stepMax < 0.08f, "flat");
  add(slopeP95 >= 0.08f, "slope");
  add(kerbs >= 2, "kerbs");
  add(steps >= 1, "steps");
  add(drops >= 1, "drop");
  add(crest > 0.5f, "crest");
  add(dip > 0.5f, "dip");
  add(bumpRms > 0.1f, "rough");
  add(terrain > 0.5f, "terrain");
  add(gaps > 0.05f, "gaps");
  add(usable < length - 0.01f, "wall");
  return t;
}

std::vector<Line> SampleLines(const Colliders &aWorld, const std::string &aRegion, int aCount, float aLength,
                              float aMinLength, uint32_t aSeed) {
  std::vector<Line> lines;
  // each region its own draw for a seed
  std::mt19937 rng(aSeed ^ uint32_t(std::hash<std::string>{}(aRegion)));
  Vec3 lo = aWorld.Min(), hi = aWorld.Max();
  std::uniform_real_distribution<float> ux(lo.x + kSampleInset, hi.x - kSampleInset),
      uy(lo.y + kSampleInset, hi.y - kSampleInset), ua(0.0f, 2.0f * 3.14159265f), u01(0.0f, 1.0f);
  for (int tries = 0; tries < aCount * 100 && int(lines.size()) < aCount; ++tries) {
    Vec3 a{ux(rng), uy(rng), 0.0f};
    float angle = ua(rng);
    Vec3 b{a.x + std::sin(angle) * aLength, a.y + std::cos(angle) * aLength, 0.0f};
    if (b.x < lo.x + kSampleInset || b.x > hi.x - kSampleInset || b.y < lo.y + kSampleInset ||
        b.y > hi.y - kSampleInset)
      continue;
    // any surface a car fits on: a street under a deck, the deck, a roof
    std::vector<Surface> room;
    for (const Surface &s : Surfaces(aWorld, a.x, a.y))
      if (s.z >= kSampleLowest && s.headroom >= kSampleHeadroom &&
          (std::isinf(s.headroom) || s.headroom <= kSampleHeadroomMax))
        room.push_back(s);
    if (room.empty())
      continue;
    float z0 = room[std::min(room.size() - 1, size_t(u01(rng) * room.size()))].z;
    Profile p = GroundProfile(aWorld, a, b, z0);
    if (p.usable < aMinLength)
      continue;
    Line l;
    l.name = aRegion + "_" + std::to_string(lines.size() + 1);
    l.region = aRegion;
    l.a = a;
    l.b = {a.x + std::sin(angle) * p.usable, a.y + std::cos(angle) * p.usable, 0.0f};
    l.z0 = z0;
    l.tags = GroundProfile(aWorld, l.a, l.b, z0).Tags();
    lines.push_back(l);
  }
  return lines;
}

Run FlyLine(const Setup &aSetup, const Colliders &aWorld, const Line &aLine, const Profile &aProfile, float aSpeed,
            float aHover) {
  Run run;
  char name[128];
  std::snprintf(name, sizeof(name), "%s@%.0f", aLine.name.c_str(), aSpeed * 3.6f);
  run.name = name;
  if (aProfile.points.empty() || aProfile.usable < 10.0f)
    return run;
  Vec3 dir = Mul(Sub(Flat2(aLine.b), Flat2(aLine.a)), 1.0f / aProfile.length);
  float heading = std::atan2(dir.x, dir.y) * kDeg;

  Sim sim(aSetup.car, Flat(-1e9f));
  Configure(sim, aSetup);
  sim.groundRay = RayOn(aWorld);
  // the wheel emitters aHover over the start's ground
  sim.Place({aLine.a.x, aLine.a.y, aProfile.points[0].z + aHover - aSetup.car.wheelZ}, heading, Mul(dir, aSpeed));

  float lead = 1.0f, limit = aProfile.usable / std::max(aSpeed, 1.0f) * 2.0f + 10.0f;
  float look = std::max(12.0f, 0.8f * aSpeed);
  bool blocked = false;
  // how far ahead of the centre the nose is: the underside's front, the front wheels without one
  float reach = aSetup.car.wheelY;
  for (Vec3 u : aSetup.car.underside)
    reach = std::max(reach, u.y);
  auto nose = [&](const Frame &aFrame) {
    float h = aFrame.headingDeg / kDeg;
    return Flight::Add(aFrame.position, Vec3{std::sin(h) * reach, std::cos(h) * reach, 0.0f});
  };
  Frame last = sim.Last();
  float surgeHeld = 0.0f;
  std::vector<size_t> noseHits; // frames the nose ran into rising ground
  last.headingDeg = heading;
  for (float t = 0.0f; t < limit; t += aSetup.dt) {
    Vec3 p = sim.Position();
    float s = Dot(Sub(Flat2(p), Flat2(aLine.a)), dir);
    // done when the nose is a metre from the end (the wall the line was cut at, if any: the car is not quite on the
    // line, so its nose may meet the wall a little early)
    if (s + reach + 1.0f >= aProfile.usable)
      break;
    // pure pursuit: the nose to a point `look` m on along the line
    Vec3 target = Flight::Add(aLine.a, Mul(dir, s + look));
    float want = std::atan2(target.x - p.x, target.y - p.y) * kDeg;
    Keys k;
    k.yaw = std::clamp(Wrap(want - last.headingDeg) / 20.0f, -1.0f, 1.0f); // + turns clockwise, as the heading
    // the speed: proportional, and an integral for the surge drag takes at that speed
    float speed = std::hypot(last.velocity.x, last.velocity.y);
    surgeHeld = std::clamp(surgeHeld + 0.5f * (aSpeed - speed) * aSetup.dt, -1.0f, 1.0f);
    k.surge = std::clamp(0.3f * (aSpeed - speed) + surgeHeld, -1.0f, 1.0f);
    float z = p.z;
    Frame now = last;
    now.position = p;
    Vec3 before = nose(now);
    last = sim.Step(k, aSetup.dt);
    run.frames.push_back(last);
    // the nose ran through something at the centre's height: rising ground (its face up) is the front touching it,
    // anything steeper a wall; or the contact lifted the car onto a wall
    Colliders::Hit h;
    bool noseHit = aWorld.Ray(before, nose(last), h);
    if (noseHit && h.normal.z >= kGroundNormalZ)
      noseHits.push_back(run.frames.size() - 1);
    if ((noseHit && h.normal.z < kGroundNormalZ) ||
        (last.contact && last.position.z - z > kWallLift + std::fabs(last.velocity.z) * aSetup.dt)) {
      blocked = true;
      break;
    }
  }

  // the metrics, after the lead
  std::vector<float> height, clear, accel, pitch, roll, speeds;
  int contacts = 0, front = 0, middle = 0, back = 0;
  float contactTime = 0, impact = 0, accelMax = 0, rays = 0;
  size_t from = std::min(run.frames.size(), size_t(lead / aSetup.dt)), counted = 0;
  float frontY = 0.5f * aSetup.car.wheelY;
  for (size_t i = from; i < run.frames.size(); ++i) {
    const Frame &f = run.frames[i];
    ++counted;
    speeds.push_back(std::hypot(f.velocity.x, f.velocity.y));
    if (f.foundGround)
      height.push_back(f.groundDistance - f.hoverHeight);
    clear.push_back(f.clearance);
    pitch.push_back(f.pitchDeg);
    roll.push_back(f.rollDeg);
    rays += float(f.rays);
    if (f.contact) {
      contactTime += aSetup.dt;
      impact = std::max(impact, f.impactSpeed);
      if (i == 0 || !run.frames[i - 1].contact) {
        ++contacts;
        (f.clearanceY > frontY ? front : f.clearanceY < -frontY ? back : middle)++;
      }
    } else if (i > 0) {
      float a = (f.velocity.z - run.frames[i - 1].velocity.z) / aSetup.dt;
      accel.push_back(a);
      accelMax = std::max(accelMax, std::fabs(a));
    }
  }
  std::vector<float> absHeight;
  for (float h : height)
    absHeight.push_back(std::fabs(h));
  float mean = 0;
  for (float v : speeds)
    mean += v / std::max<size_t>(speeds.size(), 1);
  auto add = [&](const char *aName, float aValue, const char *aUnit) { run.metrics.push_back({aName, aValue, aUnit}); };
  add("speed", mean, "m/s");
  add("height_rms", Rms(height), "m");
  add("height_p95", Percentile(absHeight, 0.95f), "m");
  add("clear_min", clear.empty() ? 0.0f : *std::min_element(clear.begin(), clear.end()), "m");
  add("clear_p05", Percentile(clear, 0.05f), "m");
  add("contacts", float(contacts), "");
  add("contact_time", contactTime, "s");
  add("impact_max", impact, "m/s");
  add("contacts_front", float(front), "");
  add("contacts_middle", float(middle), "");
  add("contacts_back", float(back), "");
  add("accel_max", accelMax, "m/s2");
  add("accel_rms", Rms(accel), "m/s2");
  add("pitch_rms", Rms(pitch), "deg");
  add("roll_rms", Rms(roll), "deg");
  int noses = 0;
  for (size_t k = 0; k < noseHits.size(); ++k)
    noses += noseHits[k] >= from && (k == 0 || noseHits[k - 1] + 1 != noseHits[k]);
  add("nose_hits", float(noses), "");
  add("rays", counted ? rays / counted : 0.0f, "");
  add("flown", run.frames.empty() ? 0.0f : Dot(Sub(Flat2(run.frames.back().position), Flat2(aLine.a)), dir), "m");
  add("blocked", blocked ? 1.0f : 0.0f, "");
  return run;
}

} // namespace FlightSim
