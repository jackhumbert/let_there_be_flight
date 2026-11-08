#include "Sim.hpp"

#include "Flight/Handling.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>

namespace FlightSim {

using namespace Flight;

namespace {
constexpr float kPi = 3.14159265f;
// the thruster pod as drawn about its centre (engine_corpo.mesh's bounds, FlightConfiguration::AddColliders)
constexpr float kPodHalfX = 0.33f, kPodHalfY = 0.36f, kPodHalfHeight = 0.245f;

Quat AxisAngle(Vec3 aAxis, float aAngle) {
  float s = std::sin(aAngle * 0.5f);
  return {aAxis.x * s, aAxis.y * s, aAxis.z * s, std::cos(aAngle * 0.5f)};
}

Quat NormalizeQ(Quat q) {
  float l = std::sqrt(q.i * q.i + q.j * q.j + q.k * q.k + q.r * q.r);
  return {q.i / l, q.j / l, q.k / l, q.r / l};
}

// FlightController.OnAction: each axis' InputPID chases the raw key or stick value.
void Feed(ControlAxes &a, const Keys &k) {
  a.surge.state.input = k.surge;
  a.lift.state.input = k.lift;
  a.sway.state.input = k.sway;
  a.yaw.state.input = k.yaw;
  a.roll.state.input = k.roll;
  a.pitch.state.input = k.pitch;
  a.linearBrake.state.input = k.linearBrake;
  a.angularBrake.state.input = k.angularBrake;
}
} // namespace

const char *ModeName(Mode aMode) {
  switch (aMode) {
  case Mode::Standard:
    return "standard";
  case Mode::Hover:
    return "hover";
  case Mode::Fly:
    return "fly";
  case Mode::Drone:
    return "drone";
  case Mode::AntiGravityDrone:
    return "agdrone";
  case Mode::DroneAssist:
    return "droneassist";
  }
  return "?";
}

bool ParseMode(const std::string &aName, Mode &aMode) {
  if (aName == "assist" || aName == "hoverfly") {
    aMode = Mode::Standard;
    return true;
  }
  for (Mode m : {Mode::Standard, Mode::Hover, Mode::Fly, Mode::Drone, Mode::AntiGravityDrone, Mode::DroneAssist})
    if (aName == ModeName(m)) {
      aMode = m;
      return true;
    }
  return false;
}

std::vector<Vec3> ParseUnderside(const std::string &aText) {
  // "cell,x0,y0,nx,ny:z,z,...", row by row along Y, an empty z where nothing is over the square
  std::vector<Vec3> points;
  float cell = 0, x0 = 0, y0 = 0;
  int nx = 0, ny = 0;
  size_t colon = aText.find(':');
  if (colon == std::string::npos ||
      std::sscanf(aText.substr(0, colon).c_str(), "%f,%f,%f,%d,%d", &cell, &x0, &y0, &nx, &ny) != 5)
    return points;
  std::stringstream zs(aText.substr(colon + 1));
  std::string z;
  for (int k = 0; k < nx * ny && std::getline(zs, z, ','); ++k)
    if (!z.empty())
      points.push_back({x0 + (float(k % nx) + 0.5f) * cell, y0 + (float(k / nx) + 0.5f) * cell, std::strtof(z.c_str(), nullptr)});
  return points;
}

Terrain Flat(float aHeight) {
  return [aHeight](float, float) { return aHeight; };
}

std::vector<Car> LoadCars(const std::string &aPath) {
  std::vector<Car> cars;
  std::ifstream in(aPath);
  std::string line;
  std::getline(in, line); // header
  // optional columns, by name: the shape drag areas and the underside (tweak_physics.py --aero)
  int cdaX = -1, cdaY = -1, cdaZ = -1, under = -1, box = -1;
  {
    std::stringstream hs(line);
    std::string name;
    for (int i = 0; std::getline(hs, name, '\t'); ++i) {
      if (!name.empty() && name.back() == '\r')
        name.pop_back();
      cdaX = name == "cdaX" ? i : cdaX;
      cdaY = name == "cdaY" ? i : cdaY;
      cdaZ = name == "cdaZ" ? i : cdaZ;
      under = name == "underside" ? i : under;
      box = name == "boxMinX" ? i : box;
    }
  }
  while (std::getline(in, line)) {
    std::vector<std::string> col;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, '\t'))
      col.push_back(cell);
    if (col.size() < 16)
      continue;
    auto num = [&](size_t i) { return std::strtof(col[i].c_str(), nullptr); };
    Car c;
    c.name = col[1].empty() ? col[0] : col[1];
    c.record = col[0];
    c.mass = num(3);
    c.airResistance = num(4);
    c.brakingFriction = num(5) < 0.001f ? 1.4f : num(5); // FlightStats.UpdateStatic: bikes read 0
    c.inertia = {num(6), num(7), num(8)};
    // FlightComponent's tail scales roll by the tensor's Y kept at 1 or more
    c.tensor = {num(9), std::max(num(10), 1.0f), num(11)};
    c.wheelX = num(12);
    c.wheelY = num(13);
    c.wheelZ = num(14);
    c.bottom = num(15);
    c.thrusters = col.size() > 16 ? num(16) : 0.0f;
    if (cdaY >= 0 && size_t(std::max({cdaX, cdaY, cdaZ})) < col.size())
      c.cda = {num(cdaX), num(cdaY), num(cdaZ)};
    if (under >= 0 && size_t(under) < col.size())
      c.underside = ParseUnderside(col[under]);
    // boxMinX, boxMaxX, boxMinY, boxMaxY, boxMinZ, boxMaxZ, side by side
    if (box >= 0 && size_t(box + 5) < col.size() && !col[box].empty()) {
      c.boxMin = {num(box), num(box + 2), num(box + 4)};
      c.boxMax = {num(box + 1), num(box + 3), num(box + 5)};
    }
    if (c.mass <= 0.0f || c.inertia.x <= 0.0f || c.inertia.y <= 0.0f || c.inertia.z <= 0.0f)
      continue;
    StatsStatic st;
    st.mass = c.mass;
    st.momentOfInertia = c.inertia;
    c.angularGain = TorqueResponse(st, c.tensor);
    cars.push_back(c);
  }
  return cars;
}

Sim::Sim(Car aCar, Terrain aTerrain) : car(aCar), terrain(std::move(aTerrain)) {
  axes = DefaultControlAxes();
}

Ground Sim::FindGround() {
  // FlightComponent.FindGround: a ray from each wheel emitter, world down, hoverModeMaxHoverHeight + 10 long
  float reach = standard.maxHoverHeight + 10.0f;
  Hit hits[4];
  int n = 0;
  for (float sx : {-1.0f, 1.0f})
    for (float sy : {-1.0f, 1.0f}) {
      Vec3 tire = Add(position, Rotate(orientation, {sx * car.wheelX, sy * car.wheelY, car.wheelZ}));
      Hit &hit = hits[n++];
      hit.tire = tire;
      if (groundRay) {
        hit.valid = groundRay(tire, {tire.x, tire.y, tire.z - reach}, hit.position, hit.normal);
        ++rays;
        continue;
      }
      float h = terrain(tire.x, tire.y);
      hit.valid = std::isfinite(h) && tire.z - h <= reach && tire.z >= h;
      hit.position = {tire.x, tire.y, h};
      // the height field's own slope there
      const float e = 0.05f;
      const float dx = (terrain(tire.x + e, tire.y) - terrain(tire.x - e, tire.y)) / (2.0f * e);
      const float dy = (terrain(tire.x, tire.y + e) - terrain(tire.x, tire.y - e)) / (2.0f * e);
      hit.normal = std::isfinite(dx + dy) ? Normalize({-dx, -dy, 1.0f}) : Vec3{0, 0, 1};
    }
  Ground g = GroundFromHits(hits, n, lastDistance);
  lastDistance = g.distance;
  for (int i = 0; i < 4; ++i)
    lastHits[i] = hits[i];
  return g;
}

ModeOutput Sim::Skid(const Controls &aControls, const Ground &aGround) const {
  DroneSkidParams k;
  k.clearance = skid;
  k.turn = skidTurn;
  // the emitters' height over the lowest point, as the configuration measures it: the thruster spheres' bottom or the
  // chassis's underside, whichever is lower
  float lowest = car.bottom + 0.4f - (openBottom > 0.0f ? kPodHalfHeight : sphereRadius);
  for (Vec3 u : car.underside)
    lowest = std::min(lowest, u.z);
  const float over = car.wheelZ - lowest;
  ModeOutput out;
  if (!aGround.found)
    return out;
  if (skidPerThruster > 0.5f) {
    SkidPoint points[4];
    int n = 0;
    for (float sx : {-1.0f, 1.0f})
      for (float sy : {-1.0f, 1.0f}) {
        const Hit &h = lastHits[n];
        points[n].offset = {sx * car.wheelX, sy * car.wheelY, car.wheelZ};
        points[n].clearance = Length(Sub(h.position, h.tire)) - over;
        points[n].normal = Normalize(h.normal);
        ++n;
      }
    return DroneSkidPoints(points, 4, stats, statics, car.angularGain, aControls.lift, k);
  }
  const float a = DroneSkid(aGround.distance - over, ClosingSpeed(stats.d_velocity, aGround.normal), stats.d_up.z,
                            aControls.lift, k);
  out.force = Rotate(Conjugate(stats.d_orientation), {0.0f, 0.0f, a});
  return out;
}

Sim::Clearance Sim::Underside() const {
  Clearance u;
  u.clearance = kClearanceReach;
  auto touch = [&](Vec3 aBody) {
    Vec3 point = Add(position, Rotate(orientation, aBody));
    Vec3 from{point.x, point.y, std::max(position.z, point.z)};
    Vec3 hit, normal;
    // from the centre's height, so a ledge the body has already sunk into is still found, and the ground the rays
    // see above the car (a ceiling) is not
    ++u.rays;
    if (groundRay(from, {point.x, point.y, point.z - kClearanceReach}, hit, normal) &&
        point.z - hit.z < u.clearance) {
      u.clearance = point.z - hit.z;
      u.bodyY = aBody.y;
    }
  };
  // what PhysX holds the car on: LTBF's thruster spheres (0.4 m, at the thruster slots where the wheels were,
  // FlightConfiguration::AddColliders), taken as the tyres' bottom at the four wheel emitters (in game a Shion at rest
  // in Drone had its emitters 0.87 m up, its tyres' height: 2026-11-06), and the chassis's underside, or the centre at
  // the bottom without one
  for (float sx : {-1.0f, 1.0f})
    for (float sy : {-1.0f, 1.0f})
      touch({sx * car.wheelX, sy * car.wheelY, car.bottom});
  if (car.underside.empty())
    touch({0, 0, car.bottom});
  for (Vec3 p : car.underside)
    touch(p);
  return u;
}

void Sim::Place(Vec3 aPosition, float aHeadingDeg, Vec3 aVelocity) {
  position = aPosition;
  velocity = aVelocity;
  angularVelocity = {};
  // heading clockwise from +Y is a negative rotation about Z
  orientation = AxisAngle({0, 0, 1}, -aHeadingDeg * kPi / 180.0f);
  axes = DefaultControlAxes();
  torqueQueue.clear();
  contactSpin = {};
  collisionTimer = 10.0f;
  t = 0;

  statics.mass = car.mass;
  statics.airResistanceFactor = car.airResistance;
  statics.brakingFrictionFactor = car.brakingFriction;
  statics.dragArea = car.cda;
  statics.thrusters = car.thrusters;
  FillInertia(1.0f / 60.0f);
  hover = {{}, hoverPid};
  pitch = {{}, pitchPid};
  roll = {{}, rollPid};

  StatsInput in{orientation, position, {}, angularVelocity, velocity};
  stats = {};
  UpdateDynamic(stats, in);
  UpdateDynamic(stats, in);

  // FlightModeStandard.OnActivateNative
  standardState = {};
  standardState.hovering = 1.0f;
  standardState.referenceZ = stats.d_position.z;
  standardState.holdingAltitude = false;
  PidReset(hover.state);
  Ground g = FindGround();
  standardState.hoverHeight = std::clamp(g.distance, standard.minHoverHeight, standard.maxHoverHeight);
  last = {};
  last.position = position;
  last.velocity = velocity;
}

void Sim::SetState(const State &aState) {
  position = aState.position;
  velocity = aState.velocity;
  // the rotation whose X, Y and Z columns are right, forward and right x forward
  Vec3 r = Normalize(aState.right), f = Normalize(aState.forward), u = Normalize(Cross(r, f));
  float tr = r.x + f.y + u.z;
  Quat q;
  if (tr > 0.0f) {
    float s = std::sqrt(tr + 1.0f) * 2.0f;
    q = {(f.z - u.y) / s, (u.x - r.z) / s, (r.y - f.x) / s, 0.25f * s};
  } else if (r.x > f.y && r.x > u.z) {
    float s = std::sqrt(1.0f + r.x - f.y - u.z) * 2.0f;
    q = {0.25f * s, (f.x + r.y) / s, (u.x + r.z) / s, (f.z - u.y) / s};
  } else if (f.y > u.z) {
    float s = std::sqrt(1.0f + f.y - r.x - u.z) * 2.0f;
    q = {(f.x + r.y) / s, 0.25f * s, (u.y + f.z) / s, (u.x - r.z) / s};
  } else {
    float s = std::sqrt(1.0f + u.z - r.x - f.y) * 2.0f;
    q = {(u.x + r.z) / s, (u.y + f.z) / s, 0.25f * s, (r.y - f.x) / s};
  }
  orientation = NormalizeQ(q);
  angularVelocity = Rotate(orientation, aState.angularVelocity);
  standardState.hovering = aState.hovering;
  standardState.hoverHeight = aState.hoverHeight;
  standardState.referenceZ = aState.referenceZ;
  standardState.holdingAltitude = aState.holdingAltitude;
  lastDistance = aState.groundDistance;
  torqueQueue.clear();
  UpdateDynamic(stats, {orientation, position, {}, angularVelocity, velocity});
  last = {};
  last.position = position;
  last.velocity = velocity;
}

void Sim::Knock(Vec3 aBodySpin, Vec3 aVelocity, float aImpact) {
  Vec3 spin = Rotate(orientation, aBodySpin);
  angularVelocity = Add(angularVelocity, spin);
  contactSpin = Add(contactSpin, spin);
  velocity = Add(velocity, aVelocity);
  // FlightComponent.ProcessImpact with FlightModeStandard's collisionPenalty of 0.5
  collisionTimer = (force.collisionRecoveryDelay + force.collisionRecoveryDuration) * (1.0f - aImpact * 0.5f);
}

Frame Sim::Step(const Keys &aKeys, float aDt) {
  // FlightController.UpdateInputs and the component's normalisation
  Feed(axes, aKeys);
  return Advance(StepControls(axes), nullptr, aDt);
}

Frame Sim::Advance(const Controls &c, const Ground *aGround, float aDt) {
  // FlightComponent.OnUpdate: stats, the mode, the force tail; the velocity as the game reads it (Car::velocityLag)
  Vec3 seen = velocity;
  seen.z -= car.velocityLag * kGravity * std::max(aDt, 1.0f / 60.0f);
  UpdateDynamic(stats, {orientation, position, {}, angularVelocity, seen});
  Ground g = aGround ? *aGround : FindGround();
  if (aGround)
    lastDistance = g.distance;
  // the car's thrust: surge and sway scaled (Flight::ThrustScale; 1 at strength 0)
  const float thrust = ThrustScale(car.thrusters, car.mass, thrustPerThruster, standard.surgeFactor, thrustStrength);
  StandardParams sp = standard;
  sp.surgeFactor *= thrust;
  sp.swayFactor *= thrust;
  DroneParams dp = drone;
  dp.surgeFactor *= thrust;
  dp.swayFactor *= thrust;
  ModeOutput m;
  switch (mode) {
  case Mode::Standard:
  case Mode::Hover:
  case Mode::Fly:
    m = Flight::Standard(standardState, stats, statics, c, sp, WithHandling(standardGains, handling), g,
                         car.angularGain, aDt,
                         mode == Mode::Hover ? HoverBand::Always
                         : mode == Mode::Fly ? HoverBand::Never
                                             : HoverBand::Blend);
    break;
  case Mode::Drone:
  case Mode::AntiGravityDrone:
    m = Drone(stats, statics, c, dp);
    // FlightModeDroneAntiGravity.Update: super.Update, then world up in the car frame times gravity
    if (mode == Mode::AntiGravityDrone)
      m.force = Add(m.force, Mul(stats.d_localUp, kGravity * standard.gravityFactor));
    break;
  case Mode::DroneAssist:
    m = DroneAssist(stats, statics, c, dp, droneAssist, car.angularGain);
    break;
  }
  if (skid > 0.0f && IsDrone(mode)) {
    const ModeOutput s = Skid(c, g);
    m.force = Add(m.force, s.force);
    m.torque = Add(m.torque, s.torque);
  }

  return Tail(m, c, g, aDt);
}

void Sim::FillInertia(float aDt) {
  const Vec3 tensor = car.tensor;
  // CapDamping reads mass x tensor / inertia as the share of a damping gain the body loses per frame (over
  // dampStabilityK); a surveyed car gives it its own, otherwise the inertia is chosen so that share is gain x dt
  if (car.inertia.x > 0.0f)
    statics.momentOfInertia = car.inertia;
  else
    statics.momentOfInertia = {car.mass * tensor.x * force.dampStabilityK / (car.angularGain.x * aDt),
                               car.mass * tensor.y * force.dampStabilityK / (car.angularGain.y * aDt),
                               car.mass * tensor.z * force.dampStabilityK / (car.angularGain.z * aDt)};
}

void Sim::ContactPoints(Frame &aFrame, float aDt) {
  std::vector<Vec3> body;
  if (sphereContacts > 0.5f)
    for (float sx : {-1.0f, 1.0f})
      for (float sy : {-1.0f, 1.0f}) {
        if (openBottom <= 0.0f) {
          body.push_back({sx * car.wheelX, sy * car.wheelY, car.bottom + 0.4f - sphereRadius});
          continue;
        }
        // the box's bottom corners: the pod's footprint, openBottom over the pod's bottom
        const float z = car.bottom + 0.4f - kPodHalfHeight + openBottom;
        for (float cx : {-kPodHalfX, kPodHalfX})
          for (float cy : {-kPodHalfY, kPodHalfY})
            body.push_back({sx * car.wheelX + cx, sy * car.wheelY + cy, z});
      }
  if (car.underside.empty())
    body.push_back({0, 0, car.bottom});
  for (Vec3 p : car.underside)
    body.push_back(p);
  // the ground under each point and how deep the point is in it
  struct Touch {
    Vec3 r, n;
    float depth, impulse = 0.0f;
  };
  std::vector<Touch> touching;
  float lowest = kClearanceReach, lowestY = 0.0f;
  for (Vec3 b : body) {
    const Vec3 r = Rotate(orientation, b);
    const Vec3 p = Add(position, r);
    float gz = -std::numeric_limits<float>::infinity();
    Vec3 n{0, 0, 1};
    if (groundRay) {
      Vec3 hit, normal;
      ++rays;
      if (groundRay({p.x, p.y, std::max(position.z, p.z)}, {p.x, p.y, p.z - kClearanceReach}, hit, normal)) {
        gz = hit.z;
        n = normal.z > 0.0f ? Normalize(normal) : Vec3{0, 0, 1};
      }
    } else {
      gz = terrain(p.x, p.y);
    }
    if (!std::isfinite(gz))
      continue;
    const float clearance = p.z - gz;
    if (clearance < lowest) {
      lowest = clearance;
      lowestY = b.y;
    }
    if (clearance < 0.0f)
      touching.push_back({r, n, -clearance});
  }
  aFrame.clearance = std::min(lowest, kClearanceReach);
  aFrame.clearanceY = lowestY;
  if (touching.empty())
    return;
  aFrame.contact = true;
  // the inverse inertia in the world frame: the body's per-axis inertia turned with it
  const Vec3 inertia = statics.momentOfInertia;
  auto invInertia = [&](Vec3 v) {
    Vec3 b = Rotate(Conjugate(orientation), v);
    b = {inertia.x > 0 ? b.x / inertia.x : 0.0f, inertia.y > 0 ? b.y / inertia.y : 0.0f,
         inertia.z > 0 ? b.z / inertia.z : 0.0f};
    return Rotate(orientation, b);
  };
  const float invMass = 1.0f / car.mass;
  float impact = 0.0f, deepest = 0.0f;
  for (const Touch &t : touching) {
    impact = std::max(impact, -Dot(Add(velocity, Cross(angularVelocity, t.r)), t.n));
    deepest = std::max(deepest, t.depth);
  }
  // sequential impulses: each point pushed out along its normal (a little of its depth as a bias), never pulled, then
  // its friction, bounded by the push so far
  for (int iteration = 0; iteration < 8; ++iteration)
    for (Touch &t : touching) {
      const Vec3 v = Add(velocity, Cross(angularVelocity, t.r));
      const float vn = Dot(v, t.n);
      const float bias = 0.2f * t.depth / aDt;
      const Vec3 rn = Cross(t.r, t.n);
      const float k = invMass + Dot(Cross(invInertia(rn), t.r), t.n);
      float j = (bias - vn) / k;
      const float total = std::max(t.impulse + j, 0.0f);
      j = total - t.impulse;
      t.impulse = total;
      velocity = Add(velocity, Mul(t.n, j * invMass));
      angularVelocity = Add(angularVelocity, invInertia(Mul(rn, j)));
      const Vec3 v2 = Add(velocity, Cross(angularVelocity, t.r));
      const Vec3 slide = Sub(v2, Mul(t.n, Dot(v2, t.n)));
      const float speed = Length(slide);
      if (speed > 1e-4f && t.impulse > 0.0f) {
        const Vec3 dir = Mul(slide, 1.0f / speed);
        const Vec3 rt = Cross(t.r, dir);
        const float kt = invMass + Dot(Cross(invInertia(rt), t.r), dir);
        const float jt = std::min(speed / kt, contactFriction * t.impulse / 8.0f);
        velocity = Sub(velocity, Mul(dir, jt * invMass));
        angularVelocity = Sub(angularVelocity, invInertia(Mul(rt, jt)));
      }
    }
  // what the bias has not taken out yet: half the deepest point's depth, straight up
  position.z += 0.5f * deepest;
  aFrame.impactSpeed = std::max(impact, 0.0f);
}

Vec3 Sim::Response() const {
  return {car.angularGain.x * car.responseError.x, car.angularGain.y * car.responseError.y,
          car.angularGain.z * car.responseError.z};
}

const StatsState &Sim::Observe() {
  UpdateDynamic(stats, {orientation, position, {}, angularVelocity, velocity});
  return stats;
}

Frame Sim::AdvanceMode(const ModeOutput &aMode, const Controls &aControls, float aDt) {
  return Tail(aMode, aControls, Ground{}, aDt);
}

void Sim::Collide(Vec3 aPush, Vec3 aNormal, float aImpact) {
  position = Add(position, aPush);
  const float into = Dot(velocity, aNormal);
  if (into < 0.0f)
    velocity = Sub(velocity, Mul(aNormal, into));
  if (aImpact > 0.0f)
    collisionTimer = (force.collisionRecoveryDelay + force.collisionRecoveryDuration) * (1.0f - std::min(aImpact, 1.0f) * 0.5f);
  last.position = position;
  last.velocity = velocity;
}

Frame Sim::Tail(const ModeOutput &m, const Controls &c, const Ground &g, float aDt) {
  FillInertia(aDt);
  const Vec3 tensor = car.tensor;
  ForceInput fi;
  fi.dt = aDt;
  fi.force = m.force;
  fi.torque = m.torque;
  fi.pitch = c.pitch;
  fi.roll = c.roll;
  fi.yaw = c.yaw;
  fi.linearBrake = c.linearBrake;
  fi.angularBrake = c.angularBrake;
  fi.thrusterTensor = tensor;
  fi.collisionTimer = collisionTimer;
  // ChassisContacts.cpp: the world's spin decays over kContactSpinDecay (0.5 s)
  contactSpin = Mul(contactSpin, std::exp(-aDt / 0.5f));
  fi.contactSpin = contactSpin;
  // the shape's drag in ApplyPhysics, as in game with the developer setting shapeDrag
  ForceParams tail = WithHandling(force, handling);
  tail.shapeDrag = shapeDrag;
  tail.dragK = car.dragK;
  statics.dragArea = car.cda;
  ForceOutput fo = ForceTail(tail, statics, stats, fi);
  collisionTimer = fo.collisionTimer;

  // the body: linear
  Vec3 accel = Mul(fo.worldForce, 1.0f / (aDt * car.mass));
  if (!bodyStep) {
    accel.z -= kGravity * car.gravityScale;
    velocity = Add(velocity, Mul(accel, aDt));
    velocity = Mul(velocity, 1.0f / (1.0f + fo.linearDamp * aDt));
    position = Add(position, Mul(velocity, aDt));
  }

  // the body: angular, the torque answered late
  Vec3 bodyTorque = Add(m.torque, fo.physicsTorque);
  torqueQueue.push_back(bodyTorque);
  size_t lag = static_cast<size_t>(std::lround(car.torqueDelay / aDt));
  Vec3 applied{};
  if (torqueQueue.size() > lag) {
    applied = torqueQueue.front();
    torqueQueue.pop_front();
  }
  Vec3 bodyAccel{applied.x * car.angularGain.x * car.responseError.x, applied.y * car.angularGain.y * car.responseError.y,
                 applied.z * car.angularGain.z * car.responseError.z};
  if (bodyStep) {
    // the solver moves the body: the force without gravity (it has its own), the torque as the body's inertia gives it
    // that angular acceleration
    const Vec3 inertia = statics.momentOfInertia;
    BodyStepIn in;
    in.dt = aDt;
    in.force = Mul(accel, car.mass);
    in.torque = Rotate(orientation, {bodyAccel.x * inertia.x, bodyAccel.y * inertia.y, bodyAccel.z * inertia.z});
    in.linearDamping = fo.linearDamp;
    in.maxAngularSpeed = car.maxAngularSpeed;
    BodyStepOut out;
    bodyStep(in, out);
    position = out.position;
    velocity = out.velocity;
    angularVelocity = out.angularVelocity;
    orientation = out.orientation;
    Frame f;
    if (groundRay) {
      Clearance u = Underside();
      f.clearance = u.clearance;
      f.clearanceY = u.bodyY;
      f.rays = rays + u.rays;
    }
    rays = 0;
    f.contact = out.contactPoints > 0;
    return Finish(f, c, g, aDt);
  }
  angularVelocity = Add(angularVelocity, Mul(Rotate(orientation, bodyAccel), aDt));
  float w = Length(angularVelocity);
  if (w > car.maxAngularSpeed) {
    angularVelocity = Mul(angularVelocity, car.maxAngularSpeed / w);
    w = car.maxAngularSpeed;
  }
  if (w > 1e-6f)
    orientation = NormalizeQ(Mul(AxisAngle(Mul(angularVelocity, 1.0f / w), w * aDt), orientation));

  // the ground: a stop under the body's lowest point, or impulses at the points touching
  Frame f;
  float depth = 0.0f;
  float ground = -std::numeric_limits<float>::infinity();
  if (pointContact > 0.5f) {
    ContactPoints(f, aDt);
    if (groundRay) {
      Vec3 hit, normal;
      if (groundRay(position, {position.x, position.y, position.z - 1000.0f}, hit, normal))
        ground = hit.z;
    } else {
      ground = terrain(position.x, position.y);
    }
  } else if (groundRay) {
    Clearance u = Underside();
    depth = -u.clearance;
    f.clearance = u.clearance;
    f.clearanceY = u.bodyY;
    f.rays = rays + u.rays + 1;
    Vec3 hit, normal;
    if (groundRay(position, {position.x, position.y, position.z - 1000.0f}, hit, normal))
      ground = hit.z;
  } else {
    ground = terrain(position.x, position.y);
    if (std::isfinite(ground)) {
      depth = ground - (position.z + car.bottom);
      f.clearance = std::min(-depth, kClearanceReach);
    }
  }
  rays = 0;
  if (pointContact <= 0.5f && depth > 0.0f) {
    f.contact = true;
    position.z += depth;
    if (velocity.z < 0.0f) {
      f.impactSpeed = -velocity.z;
      velocity.z = 0.0f;
    }
  }

  return Finish(f, c, g, aDt, ground);
}

Frame Sim::Finish(Frame &f, const Controls &c, const Ground &g, float aDt, float aGround) {
  float ground = aGround;
  if (!std::isfinite(ground) && groundRay) {
    Vec3 hit, normal;
    if (groundRay(position, {position.x, position.y, position.z - 1000.0f}, hit, normal))
      ground = hit.z;
  } else if (!std::isfinite(ground) && terrain) {
    ground = terrain(position.x, position.y);
  }
  t += aDt;
  f.t = t;
  f.position = position;
  f.velocity = velocity;
  Vec3 fwd, right, up;
  Axes(orientation, fwd, right, up);
  f.headingDeg = std::atan2(fwd.x, fwd.y) * 180.0f / kPi;
  f.pitchDeg = std::asin(std::clamp(fwd.z, -1.0f, 1.0f)) * 180.0f / kPi;
  f.rollDeg = std::asin(std::clamp(right.z, -1.0f, 1.0f)) * 180.0f / kPi;
  f.tiltDeg = std::acos(std::clamp(up.z, -1.0f, 1.0f)) * 180.0f / kPi;
  f.angularVelocity = Rotate(Conjugate(orientation), angularVelocity);
  f.controls = c;
  f.foundGround = g.found;
  f.groundDistance = g.distance;
  f.groundHeight = std::isfinite(ground) ? ground : 0.0f;
  f.hovering = standardState.hovering;
  f.hoverHeight = standardState.hoverHeight;
  f.referenceZ = standardState.referenceZ;
  f.holdingAltitude = standardState.holdingAltitude;
  last = f;
  return f;
}

} // namespace FlightSim
