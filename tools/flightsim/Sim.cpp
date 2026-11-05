#include "Sim.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>

namespace FlightSim {

using namespace Flight;

namespace {
constexpr float kPi = 3.14159265f;

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
  case Mode::HoverFly:
    return "hoverfly";
  case Mode::Assist:
    return "assist";
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
  for (Mode m : {Mode::HoverFly, Mode::Assist, Mode::Drone, Mode::AntiGravityDrone, Mode::DroneAssist})
    if (aName == ModeName(m)) {
      aMode = m;
      return true;
    }
  return false;
}

Terrain Flat(float aHeight) {
  return [aHeight](float, float) { return aHeight; };
}

std::vector<Car> LoadCars(const std::string &aPath) {
  std::vector<Car> cars;
  std::ifstream in(aPath);
  std::string line;
  std::getline(in, line); // header
  // optional columns, by name: the shape drag areas (tweak_physics.py --aero)
  int cdaX = -1, cdaY = -1, cdaZ = -1;
  {
    std::stringstream hs(line);
    std::string name;
    for (int i = 0; std::getline(hs, name, '\t'); ++i) {
      if (!name.empty() && name.back() == '\r')
        name.pop_back();
      cdaX = name == "cdaX" ? i : cdaX;
      cdaY = name == "cdaY" ? i : cdaY;
      cdaZ = name == "cdaZ" ? i : cdaZ;
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
      float h = terrain(tire.x, tire.y);
      Hit &hit = hits[n++];
      hit.tire = tire;
      hit.valid = std::isfinite(h) && tire.z - h <= reach && tire.z >= h;
      hit.position = {tire.x, tire.y, h};
      hit.normal = {0, 0, 1};
    }
  Ground g = GroundFromHits(hits, n, lastDistance);
  lastDistance = g.distance;
  return g;
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

  // FlightModeHoverFly.Create and Activate
  hoverFlyState = {};
  hoverFlyState.hovering = 1.0f;
  hoverFlyState.referenceZ = stats.d_position.z;
  hoverFlyState.holdingAltitude = false;
  PidReset(hover.state);
  Ground g = FindGround();
  hoverFlyState.hoverHeight = std::clamp(g.distance, standard.minHoverHeight, standard.maxHoverHeight);
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
  hoverFlyState.hovering = aState.hovering;
  hoverFlyState.hoverHeight = aState.hoverHeight;
  hoverFlyState.referenceZ = aState.referenceZ;
  hoverFlyState.holdingAltitude = aState.holdingAltitude;
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
  // FlightComponent.OnUpdate: stats, the mode, the force tail
  UpdateDynamic(stats, {orientation, position, {}, angularVelocity, velocity});
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
  case Mode::HoverFly:
    m = HoverFly(hoverFlyState, stats, statics, c, sp, hoverFly, g, hover, pitch, roll, aDt);
    break;
  case Mode::Assist:
    m = HoverFlyAssist(hoverFlyState, stats, statics, c, sp, assistParams, g, car.angularGain, aDt);
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
  ForceParams tail = force;
  tail.shapeDrag = shapeDrag;
  tail.dragK = car.dragK;
  statics.dragArea = car.cda;
  ForceOutput fo = ForceTail(tail, statics, stats, fi);
  collisionTimer = fo.collisionTimer;

  // the body: linear
  Vec3 accel = Mul(fo.worldForce, 1.0f / (aDt * car.mass));
  accel.z -= kGravity * car.gravityScale;
  velocity = Add(velocity, Mul(accel, aDt));
  velocity = Mul(velocity, 1.0f / (1.0f + fo.linearDamp * aDt));
  position = Add(position, Mul(velocity, aDt));

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
  angularVelocity = Add(angularVelocity, Mul(Rotate(orientation, bodyAccel), aDt));
  float w = Length(angularVelocity);
  if (w > car.maxAngularSpeed) {
    angularVelocity = Mul(angularVelocity, car.maxAngularSpeed / w);
    w = car.maxAngularSpeed;
  }
  if (w > 1e-6f)
    orientation = NormalizeQ(Mul(AxisAngle(Mul(angularVelocity, 1.0f / w), w * aDt), orientation));

  // the ground: a stop under the body's lowest point
  Frame f;
  float ground = terrain(position.x, position.y);
  if (std::isfinite(ground) && position.z + car.bottom < ground) {
    f.contact = true;
    position.z = ground - car.bottom;
    if (velocity.z < 0.0f) {
      f.impactSpeed = -velocity.z;
      velocity.z = 0.0f;
    }
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
  f.hovering = hoverFlyState.hovering;
  f.hoverHeight = hoverFlyState.hoverHeight;
  f.referenceZ = hoverFlyState.referenceZ;
  f.holdingAltitude = hoverFlyState.holdingAltitude;
  last = f;
  return f;
}

} // namespace FlightSim
