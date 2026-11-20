#include "Trip.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <limits>

namespace FlightSim {

namespace {

using NV = Navigation::Vec3;

NV N(const Vec3 &aV) { return {aV.x, aV.y, aV.z}; }
Vec3 F(const NV &aV) { return {aV.x, aV.y, aV.z}; }

const char *const kStatuses[] = {"idle",       "planning", "following", "free roam", "descending",
                                 "landed",     "arrived",  "no route",  "no nav map"};

bool Finite(const Vec3 &aV) { return std::isfinite(aV.x) && std::isfinite(aV.y) && std::isfinite(aV.z); }

float Distance2D(const NV &aA, const NV &aB) { return std::hypot(aA.x - aB.x, aA.y - aB.y); }

// m to free space from aP on the nav map: 0 when free, else the nearest free point along the six axes, searched in
// 0.25 m steps out to 4 m (4 when none)
float InsideDepth(const Navigation::World &aWorld, const NV &aP) {
  if (aWorld.IsFree(aP)) {
    return 0.0f;
  }
  const NV dirs[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  for (float d = 0.25f; d <= 4.0f; d += 0.25f) {
    for (const auto &u : dirs) {
      if (aWorld.IsFree({aP.x + u.x * d, aP.y + u.y * d, aP.z + u.z * d})) {
        return d;
      }
    }
  }
  return 4.0f;
}

// A survey row with its bottom above the centre (a car whose rig was not read right, vehicles.tsv's Tanishi T400)
// would rest with its centre under the floor and fall through it: a mid-size car's bottom instead.
Car Resting(Car aCar) {
  if (aCar.bottom > -0.2f) {
    aCar.bottom = -0.55f;
  }
  return aCar;
}

double Ms(std::chrono::steady_clock::time_point aFrom) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - aFrom).count();
}

} // namespace

const char *TripStatusName(int8_t aStatus) {
  return aStatus >= 0 && size_t(aStatus) < std::size(kStatuses) ? kStatuses[aStatus] : "?";
}

int8_t TripStatusCode(const std::string &aStatus) {
  for (size_t i = 0; i < std::size(kStatuses); ++i) {
    if (aStatus == kStatuses[i]) {
      return int8_t(i);
    }
  }
  return -1;
}

float TripPercentile(const std::vector<float> &aSorted, double aQ) {
  if (aSorted.empty()) {
    return 0.0f;
  }
  const double n = double(aSorted.size());
  const auto rank = int64_t(std::ceil(aQ * n - 1e-9));
  return aSorted[size_t(std::min<int64_t>(std::max<int64_t>(rank, 1), int64_t(aSorted.size())) - 1)];
}

// --- the world ---

bool TripWorld::VehicleRay(const NV &aFrom, const NV &aTo) const {
  ++rays;
  return MapWorld::VehicleRay(aFrom, aTo);
}

float TripWorld::Surface(float aX, float aY, float aZ) const {
  float best = -std::numeric_limits<float>::infinity();
  const float half = CarHalfHeight();
  for (const auto &g : Gaps({aX, aY, aZ})) {
    const float s = g.bottom - half;
    if (s <= aZ) {
      best = std::max(best, s);
    }
  }
  return best;
}

bool TripWorld::GroundRay(const NV &aFrom, const NV &aTo, NV &aHit) const {
  ++rays;
  bool hit = MapWorld::GroundRay(aFrom, aTo, aHit);
  // the floor under a ray cast straight down (the landing checks cast no other kind)
  if (groundFromMap && aTo.z < aFrom.z && std::fabs(aTo.x - aFrom.x) < 0.01f && std::fabs(aTo.y - aFrom.y) < 0.01f) {
    const float s = Surface(aFrom.x, aFrom.y, aFrom.z);
    if (std::isfinite(s) && s >= aTo.z && (!hit || s > aHit.z)) {
      aHit = {aFrom.x, aFrom.y, s};
      hit = true;
    }
  }
  return hit;
}

// --- the trip ---

Trip::Trip(const Navigation::NavMap &aMap, const std::vector<Navigation::Helipad> &aPads,
           const std::vector<Navigation::Gate> &aGates, TripSetup aSetup)
    : setup(std::move(aSetup)), world(aMap, aPads, aGates), queue(aMap, false, setup.planDelay), rng(setup.seed),
      ap(world, queue, setup.params, rng), refZ(setup.start.z),
      sim(Resting(setup.car), [this](float aX, float aY) { return world.Surface(aX, aY, refZ); }) {
  world.groundFromMap = setup.groundFromMap;
  ap.planBase = setup.planBase;
  // the Autopilot flies the body; Sim's own mode is never stepped
  sim.Place(F(setup.start), setup.headingDeg);
  pitchPid = {{}, sim.pitchPid};
  rollPid = {{}, sim.rollPid};
  boxIn.assign(setup.boxes.size(), false);
  knockDone.assign(setup.knocks.size(), false);
  teleportDone.assign(setup.teleports.size(), false);
}

void Trip::Input() {
  in.dt = setup.dt;
  in.stats = sim.Observe();
  in.statics = sim.Statics();
  in.controls = setup.controls;
  in.isPlayerMounted = setup.mounted;
  in.collisionTimer = sim.CollisionTimer();
  const Car &car = sim.Vehicle();
  in.thrusterTensor = car.tensor;
  // the inertia the body answers with: per body axis mass x tensor over the car's response (rad/s2 per unit of mode
  // torque, Flight::TorqueResponse; the force tail keeps the tensor's Y at 1 or more), turned into the world as
  // R diag(I) R^T, by rows
  const Vec3 t{car.tensor.x, std::max(car.tensor.y, 1.0f), car.tensor.z};
  const Vec3 g = car.angularGain;
  const Vec3 d{car.mass * t.x / std::max(g.x, 1e-6f), car.mass * t.y / std::max(g.y, 1e-6f),
               car.mass * t.z / std::max(g.z, 1e-6f)};
  const Flight::Quat q = sim.Orientation();
  const Vec3 ex = Flight::Rotate(q, {1, 0, 0}), ey = Flight::Rotate(q, {0, 1, 0}), ez = Flight::Rotate(q, {0, 0, 1});
  auto row = [&](float ax, float ay, float az) {
    return Vec3{d.x * ax * ex.x + d.y * ay * ey.x + d.z * az * ez.x, d.x * ax * ex.y + d.y * ay * ey.y + d.z * az * ez.y,
                d.x * ax * ex.z + d.y * ay * ey.z + d.z * az * ez.z};
  };
  in.inertia.valid = true;
  in.inertia.x = row(ex.x, ey.x, ez.x);
  in.inertia.y = row(ex.y, ey.y, ez.y);
  in.inertia.z = row(ex.z, ey.z, ez.z);
  in.settings = sim.force;
  in.standard = sim.standard;
  in.tracked = {setup.tracked && !setup.external, setup.destination};
  in.pitchPid = &pitchPid;
  in.rollPid = &rollPid;
  // the set-down on the wheels: the body rests with its bottom on the floor (Sim's stop), and the ground under the four
  // wheel emitters is the world's ground ray straight down from each, as the game casts from them (from just over
  // them, 12 m down)
  in.restHeight = -car.bottom;
  in.wheelGroundValid = false;
  if (ap.WantsWheelGround()) {
    const Vec3 p = sim.Position();
    float low = 1e9f, high = -1e9f, sum = 0.0f;
    bool all = true;
    for (const float sx : {-1.0f, 1.0f}) {
      for (const float sy : {-1.0f, 1.0f}) {
        const Vec3 e = Flight::Add(p, Flight::Rotate(q, {sx * car.wheelX, sy * car.wheelY, car.wheelZ}));
        NV hit;
        if (!world.GroundRay({e.x, e.y, e.z + 0.5f}, {e.x, e.y, e.z - 12.0f}, hit)) {
          all = false;
          continue;
        }
        low = std::min(low, hit.z);
        high = std::max(high, hit.z);
        sum += hit.z;
      }
    }
    in.wheelGroundValid = all;
    in.wheelGroundZ = sum * 0.25f;
    in.wheelGroundSpread = all ? high - low : 0.0f;
  }
}

void Trip::Change(const std::string &aFrom) {
  if (ap.status != aFrom) {
    changes.push_back({time, aFrom, ap.status});
  }
}

void Trip::Engage() {
  if (engaged) {
    return;
  }
  engaged = true;
  Input();
  ap.Activate(in);
  if (setup.external) {
    // FlightSummonSystem, after Activate (which clears these)
    ap.hasExternalDestination = true;
    ap.externalDestination = setup.destination;
    ap.freeRoamChosen = false;
    ap.landingKeepClear = setup.keepClear;
    ap.landingKeepClearRadius = setup.keepClearRadius;
    ap.padLandingOffset = setup.padLandingOffset;
  }
  const auto events = ap.TakeEvents();
  flightOff |= events.flightOff;
  exitAutopilot |= events.exitAutopilot;
}

void Trip::Stream() {
  const Vec3 p = sim.Position();
  for (size_t i = 0; i < setup.boxes.size(); ++i) {
    if (boxIn[i]) {
      continue;
    }
    const auto &b = setup.boxes[i];
    bool now = b.appearAt < 0.0f && b.appearWithin < 0.0f;
    now |= b.appearAt >= 0.0f && time >= b.appearAt;
    if (b.appearWithin >= 0.0f) {
      const float dx = std::max({b.box.first.x - p.x, 0.0f, p.x - b.box.second.x});
      const float dy = std::max({b.box.first.y - p.y, 0.0f, p.y - b.box.second.y});
      now |= std::hypot(dx, dy) <= b.appearWithin;
    }
    if (now) {
      boxIn[i] = true;
      (b.kind == TripBox::Kind::Vehicle ? world.vehicles : world.ground).push_back(b.box);
    }
  }
}

// The body against the solid boxes streamed in: the car as a box (its wheel base out to the body, yawed with the
// nose, from its bottom up 1.4 m), overlapping when no axis (the box's X and Y, the car's right and nose, up)
// separates them, and pushed out the shortest way through one of the box's faces
void Trip::Contacts(TripFrame &aFrame) {
  const Car &car = sim.Vehicle();
  const float halfWidth = std::max(car.wheelX + 0.25f, 0.6f), halfLength = std::max(car.wheelY + 0.6f, 1.0f);
  for (size_t i = 0; i < setup.boxes.size(); ++i) {
    if (!boxIn[i] || !setup.boxes[i].solid) {
      continue;
    }
    const auto &[lo, hi] = setup.boxes[i].box;
    const Vec3 p = sim.Position();
    const float bottom = p.z + car.bottom, top = bottom + 1.4f;
    if (top <= lo.z || bottom >= hi.z) {
      continue;
    }
    Vec3 fwd = Flight::Rotate(sim.Orientation(), {0, 1, 0});
    fwd.z = 0.0f;
    const float fl = std::hypot(fwd.x, fwd.y);
    fwd = fl > 1e-4f ? Vec3{fwd.x / fl, fwd.y / fl, 0.0f} : Vec3{0, 1, 0};
    const Vec3 right{fwd.y, -fwd.x, 0.0f};
    const float cx = 0.5f * (lo.x + hi.x), cy = 0.5f * (lo.y + hi.y), hx = 0.5f * (hi.x - lo.x),
                hy = 0.5f * (hi.y - lo.y);
    float depth = std::numeric_limits<float>::infinity();
    Vec3 normal{};
    bool apart = false;
    for (const Vec3 a : {Vec3{1, 0, 0}, Vec3{0, 1, 0}, right, fwd}) {
      const float carC = p.x * a.x + p.y * a.y, boxC = cx * a.x + cy * a.y;
      const float carR = halfWidth * std::fabs(Flight::Dot(right, a)) + halfLength * std::fabs(Flight::Dot(fwd, a));
      const float boxR = hx * std::fabs(a.x) + hy * std::fabs(a.y);
      const float overlap = carR + boxR - std::fabs(carC - boxC);
      if (overlap <= 0.0f) {
        apart = true;
        break;
      }
      // pushed out through one of the box's faces (a flat face, the car slides along it)
      if (std::fabs(a.z) < 0.5f && (a.x == 1.0f || a.y == 1.0f) && overlap < depth) {
        depth = overlap;
        normal = carC >= boxC ? a : Flight::Mul(a, -1.0f);
      }
    }
    if (apart) {
      continue;
    }
    if (hi.z - bottom < depth) {
      depth = hi.z - bottom;
      normal = {0, 0, 1};
    }
    if (top - lo.z < depth) {
      depth = top - lo.z;
      normal = {0, 0, -1};
    }
    const float speed = -Flight::Dot(sim.Velocity(), normal);
    const bool hit = speed > 2.0f;
    sim.Collide(Flight::Mul(normal, depth), normal, hit ? std::min(speed / 10.0f, 1.0f) : 0.0f);
    aFrame.contact = true;
    aFrame.impactSpeed = std::max(aFrame.impactSpeed, std::max(speed, 0.0f));
    aFrame.hit |= hit;
  }
}

void Trip::Frame() {
  Engage();
  Stream();
  const float dt = setup.dt;
  // the hits and teleports due
  for (size_t i = 0; i < setup.knocks.size(); ++i) {
    const auto &k = setup.knocks[i];
    const bool due = (k.at >= 0.0f && time >= k.at) ||
                     (k.atRemaining >= 0.0f && ap.status == "following" && ap.RouteSize() >= 2 &&
                      ap.tracker.remaining > 0.0f && ap.tracker.remaining < k.atRemaining);
    if (!knockDone[i] && due) {
      knockDone[i] = true;
      sim.Knock(k.bodySpin, k.velocity, k.impact);
      ++knocks;
    }
  }
  for (size_t i = 0; i < setup.teleports.size(); ++i) {
    if (!teleportDone[i] && time >= setup.teleports[i].at) {
      teleportDone[i] = true;
      sim.Place(F(setup.teleports[i].to), setup.teleports[i].headingDeg);
      Input();
      const std::string before = ap.status;
      const int32_t requestBefore = ap.request;
      ap.Teleported(in);
      if (ap.request != 0 && ap.request != requestBefore) {
        ++requests;
      }
      Change(before);
    }
  }

  Input();
  const std::string before = ap.status;
  const int32_t requestBefore = ap.request;
  const int64_t raysBefore = world.rays;
  const auto t0 = std::chrono::steady_clock::now();
  out = ap.Step(in);
  TripFrame f;
  f.stepMs = float(Ms(t0));
  f.rays = int32_t(world.rays - raysBefore);
  f.planFrame = ap.request != 0 && ap.request != requestBefore;
  requests += f.planFrame ? 1 : 0;
  flightOff |= out.events.flightOff;
  exitAutopilot |= out.events.exitAutopilot;
  notifications.insert(notifications.end(), out.events.notifications.begin(), out.events.notifications.end());
  Change(before);

  // the force tail and the body: the mode's force and torque, and the input fields it wrote (the player's on a
  // frame they have the controls)
  // flight switched off: the mode's force is gone and the car drops onto its wheels (the floor)
  const Flight::ModeOutput mode = flightOff ? Flight::ModeOutput{} : Flight::ModeOutput{out.force, out.torque};
  const Flight::Controls controls = out.writeControls ? out.controls : in.controls;
  f.nan = !Finite(out.force) || !Finite(out.torque);
  refZ = sim.Position().z;
  const FlightSim::Frame body = sim.AdvanceMode(mode, controls, dt);
  f.contact = body.contact;
  f.impactSpeed = body.impactSpeed;
  Contacts(f);
  queue.Advance(dt);
  time += dt;

  const Vec3 p = sim.Position(), v = sim.Velocity();
  f.t = time;
  f.p = N(p);
  f.v = N(v);
  f.nan |= !Finite(p) || !Finite(v);
  f.speed = Flight::Length(v);
  f.status = TripStatusCode(ap.status);
  f.following = ap.status == "following" && ap.FollowsTrajectory();
  f.s = ap.tracker.closestS;
  f.remaining = ap.tracker.remaining;
  if (f.following) {
    f.off = ap.tracker.offRouteNow;
    f.leadSpeed = Navigation::SampleAt(ap.trajectory, f.s + f.speed * setup.params.lineLead).speed;
  }
  f.headingDeg = body.headingDeg;
  f.pitchDeg = body.pitchDeg;
  f.rollDeg = body.rollDeg;
  f.tiltDeg = body.tiltDeg;
  f.attitudeErrorDeg = ap.attitudeError * 57.29578f;
  f.inside = f.nan ? 0.0f : InsideDepth(world, f.p);
  // low over the floor: the centre under the nav map's floor, which is a half height over the surface
  f.low = f.contact;
  if (!f.low && !f.nan) {
    const float s = world.Surface(p.x, p.y, p.z);
    f.low = std::isfinite(s) && p.z - s < world.CarHalfHeight() + 0.25f;
  }
  frames.push_back(f);

  // the end: landed, no route, no nav map, handed back or flight off, then settle seconds more
  const bool end = ap.status == "landed" || ap.status == "no route" || ap.status == "no nav map" || flightOff ||
                   exitAutopilot;
  if (end && endAt < 0.0f) {
    endAt = time;
  }
  if ((endAt >= 0.0f && time >= endAt + setup.settle) || time >= setup.maxTime) {
    ended = true;
  }
  if (setup.onFrame && !setup.onFrame(*this)) {
    ended = true;
  }
}

TripResult Trip::Run() {
  Engage();
  while (!ended) {
    Frame();
  }
  return Result();
}

TripResult Trip::Result() const {
  TripResult r;
  r.start = setup.start;
  r.frames = frames;
  r.changes = changes;
  r.status = ap.status;
  r.landed = std::any_of(changes.begin(), changes.end(), [](const auto &c) { return c.to == "landed"; });
  r.time = endAt >= 0.0f ? endAt : time;
  std::vector<float> off, err, step;
  double errSum = 0.0, stepSum = 0.0;
  bool descending = false;
  for (const auto &f : frames) {
    if (f.following) {
      off.push_back(f.off);
      if (f.leadSpeed >= 0.0f) {
        err.push_back(std::fabs(f.speed - f.leadSpeed));
        errSum += err.back();
      }
    }
    if (!f.planFrame) {
      step.push_back(f.stepMs);
      stepSum += f.stepMs;
    }
    const std::string status = TripStatusName(f.status);
    descending |= status == "descending";
    if (descending && r.touchdownSpeed < 0.0f && f.contact && (status == "descending" || status == "landed")) {
      r.touchdownSpeed = f.impactSpeed;
    }
    r.impactMax = std::max(r.impactMax, f.impactSpeed);
    r.hits += f.hit ? 1 : 0;
    r.rays += f.rays;
    r.raysMaxFrame = std::max(r.raysMaxFrame, f.rays);
    r.nan |= f.nan;
    (f.low ? r.insideContactMax : r.insideMax) = std::max(f.low ? r.insideContactMax : r.insideMax, f.inside);
  }
  std::sort(off.begin(), off.end());
  std::sort(err.begin(), err.end());
  std::sort(step.begin(), step.end());
  r.offP50 = TripPercentile(off, 0.50);
  r.offP95 = TripPercentile(off, 0.95);
  r.offP99 = TripPercentile(off, 0.99);
  r.offMax = off.empty() ? 0.0f : off.back();
  r.speedErrMean = err.empty() ? 0.0f : float(errSum / double(err.size()));
  r.speedErrP95 = TripPercentile(err, 0.95);
  r.stepMsMean = step.empty() ? 0.0f : float(stepSum / double(step.size()));
  r.stepMsP99 = TripPercentile(step, 0.99);
  r.stepMsMax = step.empty() ? 0.0f : step.back();
  r.requests = requests;
  r.replans = std::max(requests - 1, 0);
  r.knocks = knocks;
  r.pendingAtEnd = queue.Pending();
  r.floorDepth = std::max(0.0f, world.CarHalfHeight() + sim.Vehicle().bottom);
  r.landingSpot = ap.landingSpot;
  r.landingHeading = ap.landingHeading;
  if (r.landed && !frames.empty()) {
    r.landingDistance = Distance2D(frames.back().p, ap.landingSpot);
    const Vec3 nose = Flight::Rotate(sim.Orientation(), {0, 1, 0});
    const float a = std::atan2(nose.y, nose.x) - std::atan2(ap.landingHeading.y, ap.landingHeading.x);
    r.landingHeadingDeg = std::fabs(std::remainder(a, 6.2831853f)) * 57.29578f;
  }
  r.flightOff = flightOff;
  r.exitAutopilot = exitAutopilot;
  r.notifications = notifications;
  return r;
}

// --- checks ---

bool LegalStatusChange(const std::string &aFrom, const std::string &aTo) {
  struct Edge {
    const char *from, *to;
  };
  static const Edge kEdges[] = {
      // Activate leaves it idle; the first frame asks (or starts free roam, or finds no nav map)
      {"idle", "planning"},
      {"idle", "free roam"},
      {"idle", "no nav map"},
      {"planning", "following"},
      {"planning", "no route"},
      {"planning", "free roam"},
      {"planning", "no nav map"},
      // a replan with nothing left to follow, the destination moved, wedged, a teleport
      {"following", "planning"},
      {"following", "descending"},
      {"following", "arrived"},
      {"following", "free roam"},
      // a plan that came back empty while the old route was followed
      {"following", "no route"},
      {"free roam", "planning"},
      {"free roam", "following"},
      {"descending", "landed"},
      {"descending", "arrived"},
      // the destination moved more than 20 m
      {"descending", "planning"},
      {"arrived", "planning"},
      {"arrived", "idle"},
      {"no route", "planning"},
  };
  for (const auto &e : kEdges) {
    if (aFrom == e.from && aTo == e.to) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> CheckTrip(const TripResult &aResult, const Navigation::AutopilotParams &aParams,
                                   const TripLimits &aLimits) {
  std::vector<std::string> broken;
  char line[256];
  auto add = [&](const char *aText) { broken.emplace_back(aText); };
  for (const auto &f : aResult.frames) {
    if (f.nan) {
      std::snprintf(line, sizeof(line), "NaN at %.2f s", f.t);
      add(line);
      break;
    }
  }
  const TripFrame *worst = nullptr;
  for (const auto &f : aResult.frames) {
    const float limit = f.low ? aLimits.insideContact + aResult.floorDepth : aLimits.inside;
    if (Distance2D(f.p, aResult.start) < aLimits.startClear) {
      continue;
    }
    if (f.inside > limit && (!worst || f.inside > worst->inside)) {
      worst = &f;
    }
  }
  if (worst) {
    std::snprintf(line, sizeof(line), "%.2f m into a solid at %.2f s (%.1f, %.1f, %.1f)%s", worst->inside, worst->t,
                  worst->p.x, worst->p.y, worst->p.z, worst->low ? " low over the floor" : "");
    add(line);
  }
  for (const auto &c : aResult.changes) {
    if (!LegalStatusChange(c.from, c.to)) {
      std::snprintf(line, sizeof(line), "status %s to %s at %.2f s", c.from.c_str(), c.to.c_str(), c.t);
      add(line);
    }
  }
  if (aResult.pendingAtEnd > 0) {
    std::snprintf(line, sizeof(line), "%zu plan requests left in the queue", aResult.pendingAtEnd);
    add(line);
  }
  if (aResult.raysMaxFrame > aLimits.raysPerFrame) {
    std::snprintf(line, sizeof(line), "%d rays in one frame (budget %d)", aResult.raysMaxFrame, aLimits.raysPerFrame);
    add(line);
  }
  if (aLimits.mustLand) {
    if (!aResult.landed) {
      std::snprintf(line, sizeof(line), "not landed: %s at %.1f s", aResult.status.c_str(), aResult.time);
      add(line);
    } else {
      if (aResult.touchdownSpeed > aParams.touchdownRate + aLimits.touchdownMargin) {
        std::snprintf(line, sizeof(line), "touched down at %.2f m/s (touchdownRate %.2f)", aResult.touchdownSpeed,
                      aParams.touchdownRate);
        add(line);
      }
      if (aResult.landingDistance > aLimits.landingDistance) {
        std::snprintf(line, sizeof(line), "landed %.2f m from the spot", aResult.landingDistance);
        add(line);
      }
      if (aResult.landingHeadingDeg > aLimits.landingHeadingDeg) {
        std::snprintf(line, sizeof(line), "landed %.1f deg off the landing heading", aResult.landingHeadingDeg);
        add(line);
      }
    }
  }
  return broken;
}

bool SameTrip(const TripResult &aA, const TripResult &aB) {
  if (aA.frames.size() != aB.frames.size() || aA.status != aB.status || aA.changes.size() != aB.changes.size()) {
    return false;
  }
  for (size_t i = 0; i < aA.frames.size(); ++i) {
    const auto &a = aA.frames[i], &b = aB.frames[i];
    if (a.p.x != b.p.x || a.p.y != b.p.y || a.p.z != b.p.z || a.v.x != b.v.x || a.v.y != b.v.y || a.v.z != b.v.z ||
        a.status != b.status || a.rays != b.rays) {
      return false;
    }
  }
  return true;
}

// The start of the route: over the first kStartWindow s after the follower takes the route, how far the car flew over
// the trajectory's speed (m/s) and off the line (m). The surge hold at the start ran 6 to 18 m/s over the plan and 5 to
// 20 m off on the benchmark routes (2026-11-05); these keep it in sight until it is fixed.
constexpr float kStartWindow = 8.0f; // s
void StartMetrics(const TripResult &r, float &aOver, float &aOff) {
  aOver = 0.0f;
  aOff = 0.0f;
  float start = -1.0f;
  for (const auto &q : r.frames) {
    if (!q.following)
      continue;
    if (start < 0.0f)
      start = q.t;
    if (q.t - start > kStartWindow)
      break;
    if (q.leadSpeed >= 0.0f)
      aOver = std::max(aOver, q.speed - q.leadSpeed);
    aOff = std::max(aOff, q.off);
  }
}

std::string TripJson(const TripResult &r) {
  float startOver = 0.0f, startOff = 0.0f;
  StartMetrics(r, startOver, startOff);
  std::string changes;
  for (const auto &c : r.changes) {
    char one[96];
    std::snprintf(one, sizeof(one), "%s%s@%.1f", changes.empty() ? "" : ", ", c.to.c_str(), c.t);
    changes += one;
  }
  char text[2048];
  std::snprintf(text, sizeof(text),
                "{\"landed\": %s, \"status\": \"%s\", \"time\": %.2f, \"off_p50\": %.2f, \"off_p95\": %.2f, "
                "\"off_p99\": %.2f, \"off_max\": %.2f, \"speed_err_mean\": %.2f, \"speed_err_p95\": %.2f, "
                "\"landing_distance\": %.2f, \"landing_heading\": %.1f, \"touchdown_speed\": %.2f, \"impact_max\": %.2f, "
                "\"requests\": %d, \"replans\": %d, \"hits\": %d, \"rays\": %lld, \"rays_max_frame\": %d, "
                "\"step_ms_mean\": %.3f, \"step_ms_p99\": %.3f, \"step_ms_max\": %.3f, \"pending\": %zu, \"nan\": %s, "
                "\"inside_max\": %.2f, \"inside_contact_max\": %.2f, \"start_over\": %.2f, \"start_off\": %.2f, "
                "\"changes\": \"%s\"}",
                r.landed ? "true" : "false", r.status.c_str(), r.time, r.offP50, r.offP95, r.offP99, r.offMax,
                r.speedErrMean, r.speedErrP95, r.landingDistance, r.landingHeadingDeg, r.touchdownSpeed, r.impactMax,
                r.requests, r.replans, r.hits, static_cast<long long>(r.rays), r.raysMaxFrame, r.stepMsMean, r.stepMsP99,
                r.stepMsMax, r.pendingAtEnd, r.nan ? "true" : "false", r.insideMax, r.insideContactMax, startOver,
                startOff, changes.c_str());
  return text;
}

bool WriteTripCsv(const std::string &aPath, const TripResult &aResult) {
  FILE *f = std::fopen(aPath.c_str(), "w");
  if (!f) {
    return false;
  }
  std::fprintf(f, "t,x,y,z,vx,vy,vz,speed,status,s,off,lead_speed,remaining,heading,pitch,roll,tilt,att_err,rays,step_ms,"
                  "inside,contact,low,impact,hit\n");
  for (const auto &q : aResult.frames) {
    std::fprintf(f, "%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,%.1f,%.2f,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.2f,%d,%.4f,%.2f,"
                    "%d,%d,%.2f,%d\n",
                 q.t, q.p.x, q.p.y, q.p.z, q.v.x, q.v.y, q.v.z, q.speed, TripStatusName(q.status), q.s, q.off, q.leadSpeed,
                 q.remaining, q.headingDeg, q.pitchDeg, q.rollDeg, q.tiltDeg, q.attitudeErrorDeg, q.rays, q.stepMs,
                 q.inside, q.contact ? 1 : 0, q.low ? 1 : 0, q.impactSpeed, q.hit ? 1 : 0);
  }
  std::fclose(f);
  return true;
}

} // namespace FlightSim
