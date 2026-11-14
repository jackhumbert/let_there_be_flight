// The Autopilot across the fleet: every vanilla car and bike in tools/flightsim/vehicles.tsv (one row per physical
// car) flies the same, as far as the parts that read the car allow (docs/design/autopilot.md, "Across the fleet").
//
// The Autopilot reads three numbers of the car for its limits (mass, airResistance, brakingFriction:
// FlightModeAutopilot.RefreshVehicle) and three more for its attitude (inertia, thruster tensor, mass:
// FlyAerobatic). These tests check, for each distinct car:
// - the limits (Navigation/Vehicle) stay sane, and those that should not depend on the car do not;
// - the follower (tools/navplan/follow.cpp, a point mass) flies a synthetic route within bounds of its line and speed;
// - the attitude chain FlyAerobatic -> FlightComponent's force tail -> the PhysX impulse gives the angular
//   acceleration asked for on every car (AngularImpulseFor), and with ApplyPhysics' damping on, stays close.
// No game and no navmap.bin: tools/navplan/fleet.py is the slower check over the real benchmark routes.
#include "check.hpp"

#include "../tools/navplan/follow.hpp"
#include "box_space.hpp"

#include "Flight/Force.hpp"
#include "Navigation/Aerobatics.hpp"
#include "Navigation/Vehicle.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace Navigation;

namespace {

#ifdef LTBF_SOURCE_DIR
const std::string kFleet = LTBF_SOURCE_DIR "/tools/flightsim/vehicles.tsv";
#else
const std::string kFleet = std::string(__FILE__).substr(0, std::string(__FILE__).find_last_of("/\\")) +
                           "/../tools/flightsim/vehicles.tsv";
#endif

struct FleetCar {
  std::string name, type;
  float mass = 0.0f, airResistance = 0.0f, brakingFriction = 0.0f;
  Flight::Vec3 inertia, tensor;
  float cdaFront = 0.0f, cdaTop = 0.0f, thrusters = 0.0f; // the shape's front and roof drag areas, the thrusters (0: unknown)
};

// vehicles.tsv by its header's column names (tools/flightsim/tweak_physics.py COLUMNS)
const std::vector<FleetCar> &Fleet() {
  static const std::vector<FleetCar> cars = [] {
    std::vector<FleetCar> out;
    std::ifstream f(kFleet);
    std::string line;
    if (!std::getline(f, line)) {
      return out;
    }
    std::map<std::string, size_t> col;
    {
      std::stringstream header(line);
      std::string name;
      for (size_t i = 0; std::getline(header, name, '\t'); ++i) {
        col[name] = i;
      }
    }
    while (std::getline(f, line)) {
      std::vector<std::string> cells;
      std::stringstream row(line);
      std::string cell;
      while (std::getline(row, cell, '\t')) {
        cells.push_back(cell);
      }
      auto num = [&](const char *aKey) { return std::stof(cells.at(col.at(aKey))); };
      FleetCar c;
      c.name = cells.at(col.at("name")).empty() ? cells.at(col.at("record")) : cells.at(col.at("name"));
      c.type = cells.at(col.at("type"));
      c.mass = num("mass");
      c.airResistance = num("airResistance");
      c.brakingFriction = num("brakingFriction");
      c.inertia = {num("inertiaX"), num("inertiaY"), num("inertiaZ")};
      c.tensor = {num("tensorX"), num("tensorY"), num("tensorZ")};
      auto optional = [&](const char *aKey) {
        auto it = col.find(aKey);
        return it == col.end() || it->second >= cells.size() || cells[it->second].empty()
                   ? 0.0f : std::stof(cells[it->second]);
      };
      c.cdaFront = optional("cdaY");
      c.cdaTop = optional("cdaZ");
      c.thrusters = optional("thrusters");
      out.push_back(c);
    }
    return out;
  }();
  return cars;
}

// The Autopilot's VehicleSpec for a car: its three numbers, the defaults (the Autopilot's tuning) for the rest.
VehicleSpec Spec(const FleetCar &aCar) {
  VehicleSpec s;
  s.mass = aCar.mass;
  s.airResistance = aCar.airResistance;
  s.brakingFriction = aCar.brakingFriction;
  return s;
}

// The same with the car's shape drag and thrust at aStrength (docs/design/autopilot.md, "Character"): what the
// Autopilot would plan with once the game passes them.
VehicleSpec CharacterSpec(const FleetCar &aCar, float aStrength) {
  VehicleSpec s = Spec(aCar);
  s.cdaFront = aCar.cdaFront;
  s.cdaTop = aCar.cdaTop;
  s.thrusters = aCar.thrusters;
  s.thrustStrength = aStrength;
  return Effective(s);
}

// One car for each distinct (mass, airResistance, brakingFriction): the limits and the follower see only these.
std::vector<FleetCar> DistinctSpecs() {
  std::map<std::tuple<float, float, float>, FleetCar> seen;
  for (const auto &c : Fleet()) {
    seen.emplace(std::make_tuple(c.mass, c.airResistance, c.brakingFriction), c);
  }
  std::vector<FleetCar> out;
  for (const auto &[k, c] : seen) {
    out.push_back(c);
  }
  return out;
}

// Distance to stop from aSpeed braking at PlanDecel (what the trajectory plans), m.
float BrakingDistance(const VehicleSpec &aSpec, float aSpeed) {
  float d = 0.0f;
  const float dv = 0.1f;
  for (float v = aSpeed; v > 0.0f; v -= dv) {
    d += v * dv / PlanDecel(aSpec, v);
  }
  return d;
}

} // namespace

TEST(autopilot_fleet_loads) {
  const auto &cars = Fleet();
  CHECK(cars.size() >= 255); // 261 rows on 2026-11-04 (241 cars, 20 bikes; the stand-in drive model left out)
  int bikes = 0;
  for (const auto &c : cars) {
    bikes += c.type == "bike" ? 1 : 0;
    CHECK(c.mass > 0.0f && c.airResistance > 0.0f && c.brakingFriction > 0.0f);
    CHECK(c.inertia.x > 0.0f && c.inertia.y > 0.0f && c.inertia.z > 0.0f);
    CHECK(c.tensor.x > 0.0f && c.tensor.y > 0.0f && c.tensor.z > 0.0f);
  }
  CHECK(bikes >= 15); // 20
  CHECK(DistinctSpecs().size() >= 60); // 69 distinct (mass, airResistance, brakingFriction)
}

// Bounds are the fleet's measured range on 2026-11-04 plus a margin, written beside each check ("measured; bound").

TEST(autopilot_fleet_limits) {
  const VehicleSpec base;
  float topMin = 1e9f, topMax = 0.0f, brakeMax = 0.0f;
  std::string slowest;
  for (const auto &c : DistinctSpecs()) {
    const VehicleSpec s = Spec(c);
    const float top = TopSpeed(s);
    if (top < topMin) {
      slowest = c.name;
    }
    topMin = std::min(topMin, top);
    topMax = std::max(topMax, top);
    // top speed: where the surge meets the drag, at most the cap; the drag never beats the surge below it
    CHECK(top > 70.0f && top <= s.speedCap);
    CHECK(Drag(s, top) < s.surge);
    CHECK(Accel(s, 0.0f) == s.surge);
    CHECK(Accel(s, top) >= 0.5f);
    // braking: harder with speed, the plan's never more than planDecel and never less than the surge's margin
    CHECK(Decel(s, 60.0f) > Decel(s, 20.0f));
    CHECK(PlanDecel(s, 0.0f) <= s.planDecel && PlanDecel(s, 0.0f) >= s.margin * s.surge - 1e-4f);
    const float brake = BrakingDistance(s, top);
    brakeMax = std::max(brakeMax, brake);
    CHECK(brake < 420.0f); // 400 m from the cap's 120 m/s at most measured; 420
    // turning, climbing and the drone body do not read the car: the same on every car
    CHECK(Lateral(s) == Lateral(base));
    CHECK(LateralAt(s, 5.0f) == LateralAt(base, 5.0f));
    CHECK(DroneAccel(s, s.sway) == DroneAccel(base, base.sway));
    CHECK(PullUp(s) == PullUp(base));
  }
  std::printf("  top speed %.1f (%s) to %.1f m/s, braking from top at most %.0f m\n", topMin, slowest.c_str(), topMax,
              brakeMax);
  CHECK(topMin > 72.0f && topMin < 75.0f); // 72.6 m/s measured: Quartz "Sidewinder" (1300 kg, airResistance 2.9);
                                           // the 300 kg bikes 73.6
  CHECK(topMax == base.speedCap);          // the cap (120): light-drag cars and the heaviest trucks reach it
}

namespace {

// A synthetic route: 800 m straight at 40 m, a 200 m radius quarter turn to the left, 600 m with a 40 m climb over
// its first 300 m, stopping at the end.
std::vector<Vec3> FleetRoute() {
  std::vector<Vec3> line;
  for (float x = 0.0f; x < 800.0f; x += 5.0f) {
    line.push_back({x, 0.0f, 40.0f});
  }
  const float r = 200.0f;
  for (int d = 0; d < 90; d += 2) {
    const float a = float(d) * 3.14159265f / 180.0f;
    line.push_back({800.0f + r * std::sin(a), r - r * std::cos(a), 40.0f});
  }
  for (float y = 0.0f; y <= 600.0f; y += 5.0f) {
    const float climb = std::min(y / 300.0f, 1.0f);
    line.push_back({800.0f + r, r + y, 40.0f + 40.0f * climb * climb * (3.0f - 2.0f * climb)});
  }
  return line;
}

} // namespace

TEST(autopilot_fleet_follow) {
  // Every distinct car plans the route with its own limits and flies it with the follower. The cars differ only in
  // drag, so in top speed (72.6 to 120 m/s). Some cars leave the line once, on the climb's crest (s 1200 to 1250 m,
  // past raceOffLine's 4 m), the faster ones further: a weakness of the follower over a crest at speed, not of one car.
  // Planning the bends at the speed the surge hold flies (FlownTop) and with the surge the drag leaves (2026-11-04)
  // took the worst from 9.89 m to 5.95 m.
  const auto line = FleetRoute();
  const Route route = Route::Make(line);
  BoxSpace space;
  const FollowParams params; // as in game: the surge hold on
  float offMax = 0.0f, offSlow = 0.0f, errMax = 0.0f, lateMax = -1e9f, earlyMax = 1e9f, tMin = 1e9f, tMax = 0.0f;
  std::string worstOff, worstErr;
  for (const auto &c : DistinctSpecs()) {
    const VehicleSpec s = Spec(c);
    const Trajectory traj = BuildTrajectory(s, line, TrajectoryParams{});
    const auto r = Follow(s, route, traj, space, params);
    CHECK(r.arrived);
    CHECK(r.lost <= 1);
    for (float at : r.lostAt) {
      CHECK(at > 1190.0f && at < 1255.0f); // the crest only
    }
    CHECK(r.blocked == 0 && r.emergency == 0);
    if (r.offMax > offMax) {
      offMax = r.offMax;
      worstOff = c.name;
    }
    if (TopSpeed(s) < 100.0f) {
      offSlow = std::max(offSlow, r.offMax);
    }
    if (r.speedErrMean > errMax) {
      errMax = r.speedErrMean;
      worstErr = c.name;
    }
    lateMax = std::max(lateMax, r.time - traj.Duration());
    earlyMax = std::min(earlyMax, r.time - traj.Duration());
    tMin = std::min(tMin, r.time);
    tMax = std::max(tMax, r.time);
  }
  std::printf("  flown %.2f to %.2f s, against the plan %+.2f to %+.2f s; off the line at most %.2f m (%s), %.2f m with a "
              "top speed under 100 m/s; mean speed error at most %.2f m/s (%s)\n",
              tMin, tMax, earlyMax, lateMax, offMax, worstOff.c_str(), offSlow, errMax, worstErr.c_str());
  CHECK(offMax < 6.5f);         // 5.95 m measured (Behemoth); 6.5
  CHECK(offSlow < 5.0f);        // 4.52 m measured; 5.0
  CHECK(errMax < 2.3f);         // 2.06 m/s measured; 2.3
  CHECK(lateMax < -0.6f);       // -0.68 s measured: the surge hold flies every car a little ahead of its plan (up to
                                // its slack; -0.94 s while it held full surge past the plan)
  CHECK(earlyMax > -1.4f);      // -1.28 s measured
  CHECK(tMax - tMin < 3.3f);    // 29.92 to 32.78 s measured: the drag's spread of top speed
}

namespace {

using Flight::Vec3;
using FQuat = Flight::Quat;

struct M3 {
  Vec3 r0, r1, r2; // rows
};

Vec3 MulM(const M3 &m, const Vec3 &v) { return {Flight::Dot(m.r0, v), Flight::Dot(m.r1, v), Flight::Dot(m.r2, v)}; }

// R diag(d) R^T, R the rotation of q (columns: the body's right, forward, up in the world)
M3 WorldInertiaOf(const FQuat &q, const Vec3 &d) {
  const Vec3 ex = Flight::Rotate(q, {1, 0, 0}), ey = Flight::Rotate(q, {0, 1, 0}), ez = Flight::Rotate(q, {0, 0, 1});
  auto row = [&](float ax, float ay, float az) {
    return Vec3{d.x * ax * ex.x + d.y * ay * ey.x + d.z * az * ez.x, d.x * ax * ex.y + d.y * ay * ey.y + d.z * az * ez.y,
                d.x * ax * ex.z + d.y * ay * ey.z + d.z * az * ez.z};
  };
  return {row(ex.x, ey.x, ez.x), row(ex.y, ey.y, ez.y), row(ex.z, ey.z, ez.z)};
}

M3 InverseWorldInertiaOf(const FQuat &q, const Vec3 &d) { return WorldInertiaOf(q, {1.0f / d.x, 1.0f / d.y, 1.0f / d.z}); }

FQuat AxisAngle(const Vec3 &aAxis, float aAngle) {
  const float s = std::sin(0.5f * aAngle);
  return {aAxis.x * s, aAxis.y * s, aAxis.z * s, std::cos(0.5f * aAngle)};
}

FQuat NormalizeQ(const FQuat &q) {
  const float l = std::sqrt(q.i * q.i + q.j * q.j + q.k * q.k + q.r * q.r);
  return {q.i / l, q.j / l, q.k / l, q.r / l};
}

Flight::StatsState StatsOf(const FQuat &q, const Vec3 &aWorldRate) {
  Flight::StatsState s;
  s.d_orientation = q;
  Flight::Axes(q, s.d_forward, s.d_right, s.d_up);
  s.d_angularVelocity = Flight::Rotate(Flight::Conjugate(q), aWorldRate);
  // hovering: no speed, so ApplyPhysics' aero terms are zero
  s.d_direction = s.d_forward;
  s.d_localDirection = {0, 1, 0};
  return s;
}

struct AttitudeRun {
  float trackMax = 0.0f;  // rad off the target while it turns
  float overshoot = 0.0f; // rad past the target once it holds
  float settle = 0.0f;    // s after the target holds until the error stays under 1 degree
};

// FlyAerobatic's attitude tail (Navigation::AttitudeControl, PhysX-owned flight: the world inertia valid) and
// FlightComponent's force tail (Flight::ForceTail), the angular impulse then applied to the body as PushToPhysX does
// (the inverse world inertia). The body's inertia is the PhysX one: the TweakDB inertia over the engine gain
// (kEngineTorqueGain, Flight/Modes.hpp, the flightsim model). The target turns at aRate about the body axis aAxis
// (0 pitch, 1 roll, 2 yaw) for aTurn s, then holds; 60 fps.
AttitudeRun FlyAttitude(const FleetCar &aCar, int aAxis, float aRate, float aTurn, int aDamping) {
  const float dt = 1.0f / 60.0f;
  Flight::StatsStatic st;
  st.mass = aCar.mass;
  st.airResistanceFactor = aCar.airResistance;
  st.brakingFrictionFactor = aCar.brakingFriction;
  st.momentOfInertia = aCar.inertia;
  const Vec3 physx{aCar.inertia.x / 1.13f, aCar.inertia.y / 0.52f, aCar.inertia.z / 0.95f};
  const Vec3 axis = aAxis == 0 ? Vec3{1, 0, 0} : aAxis == 1 ? Vec3{0, 1, 0} : Vec3{0, 0, 1};

  // the car starts level, nose +Y, at rest; the target starts there too
  FQuat q{0, 0, 0, 1};
  Vec3 w{};
  AttitudeControlState state;
  state.targetRight = {1, 0, 0};
  state.targetForward = {0, 1, 0};
  state.targetUp = {0, 0, 1};
  // aDamping 0: no angular damping in ApplyPhysics and no give-back for it; 1: both, as in game; 2: the damping
  // without the give-back
  AttitudeControlParams ap;
  Flight::ForceParams fp;
  if (aDamping != 1) {
    ap.generalDampFactorAngular = 0.0f;
  }
  if (aDamping == 0) {
    fp.generalDampFactorAngular = 0.0f;
  }
  AttitudeRun run;
  float settledSince = -1.0f;
  const float total = aTurn + 3.0f;
  for (int k = 0; float(k) * dt < total; ++k) {
    const float t = float(k) * dt;
    const float angle = aRate * std::min(t, aTurn);
    const FQuat target = AxisAngle(axis, angle);
    const Flight::StatsState s = StatsOf(q, w);

    AttitudeControlInput in;
    in.dt = dt;
    in.fwd = Flight::Rotate(target, {0, 1, 0});
    in.up = Flight::Rotate(target, {0, 0, 1});
    in.raceDecel = 20.0f;
    in.thrusterTensor = aCar.tensor;
    const M3 iw = WorldInertiaOf(q, physx);
    in.inertia.valid = true;
    in.inertia.x = iw.r0;
    in.inertia.y = iw.r1;
    in.inertia.z = iw.r2;
    const AttitudeControlOutput out = AttitudeControl(state, ap, st, s, in);

    Flight::ForceInput fi;
    fi.dt = dt;
    fi.torque = out.torque;
    fi.applyPhysics = true;
    fi.thrusterTensor = aCar.tensor;
    fi.collisionTimer = 100.0f; // long after any hit
    const Flight::ForceOutput fo = Flight::ForceTail(fp, st, s, fi);
    w = Flight::Add(w, MulM(InverseWorldInertiaOf(q, physx), fo.worldTorque));
    const float spin = Flight::Length(w);
    if (spin > 7.0f) {
      w = Flight::Mul(w, 7.0f / spin); // PhysX's max angular velocity
    }
    if (spin > 1e-9f) {
      q = NormalizeQ(Flight::Mul(AxisAngle(Flight::Mul(w, 1.0f / spin), spin * dt), q));
    }

    // the error: the angle of the rotation from the car to the target, signed along the axis
    const FQuat e = Flight::Mul(Flight::Conjugate(target), q);
    const float along = 2.0f * std::atan2(e.i * axis.x + e.j * axis.y + e.k * axis.z, std::fabs(e.r)) *
                        (e.r < 0.0f ? -1.0f : 1.0f);
    const float err = 2.0f * std::atan2(std::sqrt(e.i * e.i + e.j * e.j + e.k * e.k), std::fabs(e.r));
    if (t <= aTurn) {
      run.trackMax = std::max(run.trackMax, err);
    } else {
      run.overshoot = std::max(run.overshoot, along * (aRate > 0.0f ? 1.0f : -1.0f));
      if (err < 1.0f * 3.14159265f / 180.0f) {
        if (settledSince < 0.0f) {
          settledSince = t;
        }
      } else {
        settledSince = -1.0f;
      }
    }
  }
  run.settle = settledSince < 0.0f ? 99.0f : settledSince - aTurn;
  return run;
}

struct KnockRun {
  float worst = 0.0f;     // rad off the held attitude, the most
  float back = 0.0f;      // s from the hit until it stays within 2 degrees
  float driven = 0.0f;    // the largest torque reaching the body while the recovery has it scaled to 0
  float contactDamp = 0.0f; // ApplyPhysics' damping torque on the frame of the hit, when all the spin is the contact's
};

// A hit while the Autopilot holds the car level, as a player's hit plays out (the flightsim's Knock): the contact's
// spin aSpin (world, rad/s) at once, counted as the world's (FlightComponent.contactSpin, which ApplyPhysics leaves
// undamped, fading over 0.5 s in ChassisContacts.cpp), and FlightComponent.ProcessImpact's recovery timer for an
// impact of 1 (the mode's force and torque, the give-back too, scaled to 0 and back over 0.8 s). 60 fps.
KnockRun FlyKnock(const FleetCar &aCar, Vec3 aSpin) {
  const float dt = 1.0f / 60.0f;
  Flight::StatsStatic st;
  st.mass = aCar.mass;
  st.airResistanceFactor = aCar.airResistance;
  st.brakingFrictionFactor = aCar.brakingFriction;
  st.momentOfInertia = aCar.inertia;
  const Vec3 physx{aCar.inertia.x / 1.13f, aCar.inertia.y / 0.52f, aCar.inertia.z / 0.95f};
  FQuat q{0, 0, 0, 1};
  Vec3 w{}, contact{};
  AttitudeControlState state;
  state.targetRight = {1, 0, 0};
  state.targetForward = {0, 1, 0};
  state.targetUp = {0, 0, 1};
  const AttitudeControlParams ap;
  const Flight::ForceParams fp;
  float timer = 100.0f;
  const float hit = 0.5f;
  bool hitDone = false;
  KnockRun run;
  float within = -1.0f;
  for (int k = 0; float(k) * dt < hit + 6.0f; ++k) {
    const float t = float(k) * dt;
    bool hitFrame = false;
    if (!hitDone && t >= hit) {
      w = Flight::Add(w, aSpin);
      contact = Flight::Add(contact, aSpin);
      timer = (fp.collisionRecoveryDelay + fp.collisionRecoveryDuration) * (1.0f - 1.0f * 0.5f);
      hitDone = hitFrame = true;
    }
    contact = Flight::Mul(contact, std::exp(-dt / 0.5f));
    const Flight::StatsState s = StatsOf(q, w);

    AttitudeControlInput in;
    in.dt = dt;
    in.fwd = {0, 1, 0};
    in.up = {0, 0, 1};
    in.raceDecel = 20.0f;
    in.thrusterTensor = aCar.tensor;
    const M3 iw = WorldInertiaOf(q, physx);
    in.inertia.valid = true;
    in.inertia.x = iw.r0;
    in.inertia.y = iw.r1;
    in.inertia.z = iw.r2;
    const AttitudeControlOutput out = AttitudeControl(state, ap, st, s, in);

    Flight::ForceInput fi;
    fi.dt = dt;
    fi.torque = out.torque;
    fi.applyPhysics = true;
    fi.thrusterTensor = aCar.tensor;
    fi.contactSpin = contact;
    fi.collisionTimer = timer;
    const Flight::ForceOutput fo = Flight::ForceTail(fp, st, s, fi);
    if (hitFrame) {
      // all the spin is the contact's this frame (the decay aside): ApplyPhysics damps none of it
      Flight::ForceOutput only;
      Flight::ForceInput ci = fi;
      ci.torque = {};
      ci.contactSpin = Flight::Rotate(s.d_orientation, s.d_angularVelocity);
      Flight::ApplyPhysics(fp, st, s, ci, only);
      run.contactDamp = Flight::Length(only.physicsTorque);
    }
    if (timer < fp.collisionRecoveryDelay) {
      run.driven = std::max(run.driven, Flight::Length(fo.worldTorque));
    }
    timer = fo.collisionTimer;
    w = Flight::Add(w, MulM(InverseWorldInertiaOf(q, physx), fo.worldTorque));
    const float spin = Flight::Length(w);
    if (spin > 7.0f) {
      w = Flight::Mul(w, 7.0f / spin);
    }
    if (spin > 1e-9f) {
      q = NormalizeQ(Flight::Mul(AxisAngle(Flight::Mul(w, 1.0f / spin), spin * dt), q));
    }
    if (t >= hit) {
      const float err = 2.0f * std::atan2(std::sqrt(q.i * q.i + q.j * q.j + q.k * q.k), std::fabs(q.r));
      run.worst = std::max(run.worst, err);
      if (err < 2.0f * 3.14159265f / 180.0f) {
        if (within < 0.0f) {
          within = t;
        }
      } else {
        within = -1.0f;
      }
    }
  }
  run.back = within < 0.0f ? 99.0f : within - hit;
  return run;
}

} // namespace

TEST(autopilot_fleet_attitude_impulse_exact) {
  // AngularImpulseFor's path: the torque FlyAerobatic asks for, through the force tail (x dt x mass x tensor, the
  // tensor's Y raised to 1 both ways) and the inverse world inertia, changes the body's rate by exactly the angular
  // acceleration x dt it wanted, on every car and bike, whatever the inertia and the engine gain. One frame from
  // rest with the target 0.2 rad off on all three axes: alpha = 25 x 0.2 per axis (no rate yet), length 8.66, under
  // the 20 cap.
  const float dt = 1.0f / 60.0f;
  const FQuat target = NormalizeQ(Flight::Mul(AxisAngle({0, 0, 1}, 0.2f), Flight::Mul(AxisAngle({0, 1, 0}, 0.2f),
                                                                                   AxisAngle({1, 0, 0}, 0.2f))));
  const FQuat q{0, 0, 0, 1};
  const Flight::StatsState s = StatsOf(q, {});
  // the rotation error AttitudeControl measures, for the alpha expected (FlightAttitude.RotationError)
  const Flight::Vec3 err = RotationError(s.d_right, s.d_forward, s.d_up, Flight::Rotate(target, {1, 0, 0}),
                                 Flight::Rotate(target, {0, 1, 0}), Flight::Rotate(target, {0, 0, 1}));
  const Flight::Vec3 alpha = Flight::Mul(err, 25.0f);
  float worst = 0.0f;
  for (const auto &c : Fleet()) {
    Flight::StatsStatic st;
    st.mass = c.mass;
    st.momentOfInertia = c.inertia;
    const Flight::Vec3 physx{c.inertia.x / 1.13f, c.inertia.y / 0.52f, c.inertia.z / 0.95f};
    AttitudeControlState state;
    // the target already there last frame: no target rate, so no give-back
    state.targetRight = Flight::Rotate(target, {1, 0, 0});
    state.targetForward = Flight::Rotate(target, {0, 1, 0});
    state.targetUp = Flight::Rotate(target, {0, 0, 1});
    AttitudeControlInput in;
    in.dt = dt;
    in.fwd = state.targetForward;
    in.up = state.targetUp;
    in.raceDecel = 20.0f;
    in.thrusterTensor = c.tensor;
    const M3 iw = WorldInertiaOf(q, physx);
    in.inertia.valid = true;
    in.inertia.x = iw.r0;
    in.inertia.y = iw.r1;
    in.inertia.z = iw.r2;
    const AttitudeControlOutput out = AttitudeControl(state, AttitudeControlParams{}, st, s, in);
    Flight::ForceInput fi;
    fi.dt = dt;
    fi.torque = out.torque;
    fi.applyPhysics = false;
    fi.thrusterTensor = c.tensor;
    fi.collisionTimer = 100.0f;
    const Flight::ForceOutput fo = Flight::ForceTail(Flight::ForceParams{}, st, s, fi);
    const Flight::Vec3 dw = MulM(InverseWorldInertiaOf(q, physx), fo.worldTorque);
    const Flight::Vec3 miss = Flight::Sub(Flight::Mul(dw, 1.0f / dt), alpha);
    worst = std::max(worst, Flight::Length(miss) / Flight::Length(alpha));
  }
  std::printf("  rate change against alpha x dt: worst relative miss %.2e over %zu cars\n", worst, Fleet().size());
  CHECK(worst < 1e-4f); // float rounding only
}

TEST(autopilot_fleet_attitude_loop) {
  // The loop on every car with distinct mass, inertia and tensor: a 0.6 rad/s turn of the target for 1 s about each
  // axis, then a hold.
  // - Without ApplyPhysics' angular damping (and FlyAerobatic's give-back for it) every car flies it the same: the
  //   impulse is exact.
  // - As in game, the damping (2 x the spin in mode torque, cut per car by CapDamping) acts on the body through
  //   mass x tensor / inertia, which spans about 30 times over the fleet, and the give-back for it is the same
  //   Flight::AngularDamping at the rate wanted, CapDamping included. Before 2026-11-04 the give-back was 2 x rateRef,
  //   never cut by CapDamping: more than the damping wherever the cap cuts in, and the cars that answer most
  //   (pitch 11 rad/s2 per unit of torque) tracked 0.33 rad off and overshot 0.36 rad (20 deg), settling in 1.6 s.
  // - The damping without the give-back only damps more on the cars that answer most.
  std::map<std::tuple<float, float, float, float, float, float, float>, FleetCar> distinct;
  for (const auto &c : Fleet()) {
    distinct.emplace(std::make_tuple(c.mass, c.inertia.x, c.inertia.y, c.inertia.z, c.tensor.x, c.tensor.y, c.tensor.z),
                     c);
  }
  const char *names[3] = {"pitch", "roll", "yaw"};
    for (int axis = 0; axis < 3; ++axis) {
    AttitudeRun refBare;
    bool first = true;
    float ngMax = 0.0f, ngMin = 1e9f, ngOver = 0.0f, bareSpread = 0.0f, trackMax = 0.0f, overMax = 0.0f;
    float settleMax = 0.0f, trackMin = 1e9f;
    std::string worst;
    for (const auto &[k, c] : distinct) {
      const AttitudeRun bare = FlyAttitude(c, axis, 0.6f, 1.0f, 0);
      if (first) {
        refBare = bare;
        first = false;
      }
      bareSpread = std::max({bareSpread, std::fabs(bare.trackMax - refBare.trackMax),
                             std::fabs(bare.overshoot - refBare.overshoot)});
      const AttitudeRun damped = FlyAttitude(c, axis, 0.6f, 1.0f, 1);
      const AttitudeRun noGive = FlyAttitude(c, axis, 0.6f, 1.0f, 2);
      ngMax = std::max(ngMax, noGive.trackMax);
      ngMin = std::min(ngMin, noGive.trackMax);
      ngOver = std::max(ngOver, noGive.overshoot);
      if (damped.trackMax > trackMax) {
        trackMax = damped.trackMax;
        worst = c.name;
      }
      trackMin = std::min(trackMin, damped.trackMax);
      overMax = std::max(overMax, damped.overshoot);
      settleMax = std::max(settleMax, damped.settle);
    }
    std::printf("  %s over %zu cars: without damping track %.4f rad, overshoot %.4f, settle %.2f s (spread %.1e); "
                "with it track %.4f to %.4f rad (%s), overshoot at most %.4f, settle at most %.2f s\n",
                names[axis], distinct.size(), refBare.trackMax, refBare.overshoot, refBare.settle, bareSpread, trackMin,
                trackMax, worst.c_str(), overMax, settleMax);
    std::printf("  %s, the damping without the give-back: track %.4f to %.4f rad, overshoot at most %.4f\n",
                names[axis], ngMin, ngMax, ngOver);
    CHECK(bareSpread < 1e-3f);                   // 0 to 6e-8 measured: the same on every car
    CHECK(refBare.trackMax < 0.06f);             // 0.0546 rad measured
    CHECK(refBare.overshoot < 0.07f);            // 0.0651 measured
    CHECK(refBare.settle < 0.7f);                // 0.65 s measured
    CHECK(trackMax < 0.055f);                    // 0.0491, 0.0494, 0.0479 rad at most measured (2026-11-04)
    CHECK(trackMax - trackMin < 0.012f);         // 0.0084, 0.0024, 0.0056 measured: close on every car
    CHECK(overMax < 0.065f);                     // 0.0567, 0.0571, 0.0547 at most measured: about 3 deg
    CHECK(settleMax < 0.8f);                     // 0.72, 0.68, 0.72 s at most measured
    CHECK(ngMax < 0.13f && ngOver < 0.065f);     // 0.121 rad and 0.058 at most measured
  }
}

TEST(autopilot_fleet_knock) {
  // A hit with the Autopilot holding the car level plays out as a player's does, on every car: the contact's spin
  // goes undamped by ApplyPhysics, the recovery scales every torque the mode gives (the give-back for the damping
  // included) to 0 and back, and then the attitude control brings the car back level. The give-back is only ever the
  // damping at the rate the Autopilot wants (Flight::AngularDamping), so it changes nothing about the hit itself.
  std::map<std::tuple<float, float, float, float, float, float, float>, FleetCar> distinct;
  for (const auto &c : Fleet()) {
    distinct.emplace(std::make_tuple(c.mass, c.inertia.x, c.inertia.y, c.inertia.z, c.tensor.x, c.tensor.y, c.tensor.z),
                     c);
  }
  float contactDamp = 0.0f, driven = 0.0f, worstMin = 1e9f, worstMax = 0.0f, backMin = 1e9f, backMax = 0.0f;
  std::string slowest;
  for (const auto &[k, c] : distinct) {
    // a side hit: 3 rad/s of roll, and a front corner's: pitch and yaw as well
    for (const Flight::Vec3 spin : {Flight::Vec3{0, 3.0f, 0}, Flight::Vec3{1.5f, 1.0f, 1.5f}}) {
      const KnockRun r = FlyKnock(c, spin);
      contactDamp = std::max(contactDamp, r.contactDamp);
      driven = std::max(driven, r.driven);
      worstMin = std::min(worstMin, r.worst);
      worstMax = std::max(worstMax, r.worst);
      backMin = std::min(backMin, r.back);
      if (r.back > backMax) {
        backMax = r.back;
        slowest = c.name;
      }
    }
  }
  std::printf("  knock over %zu cars: contact spin damped %.2e, torque in the recovery's dead time %.2e; worst %.3f "
              "to %.3f rad, back within 2 deg in %.2f to %.2f s (%s)\n",
              distinct.size(), contactDamp, driven, worstMin, worstMax, backMin, backMax, slowest.c_str());
  CHECK(contactDamp < 1e-4f); // the contact's spin is not damped: it tumbles as for a player
  CHECK(driven < 1e-6f);      // nothing reaches the body while the recovery has it at 0, the give-back included
  CHECK(backMax < 2.6f);      // 1.63 to 2.25 s measured (2026-11-04): every car back within 2 deg
  CHECK(worstMax < 1.05f);    // 0.67 to 0.97 rad measured: how far the hit throws the car before it is back
}

TEST(autopilot_fleet_character) {
  // Each car's shape drag and thrust at strength 0.5 (VehicleSpec cdaFront, thrusters; Effective): the limits stay
  // sane and every car still plans and flies the synthetic route, now with top speeds from about 50 to the cap.
  std::map<std::tuple<float, float, float, float, float, float>, FleetCar> distinct;
  for (const auto &c : Fleet()) {
    distinct.emplace(std::make_tuple(c.mass, c.airResistance, c.brakingFriction, c.cdaFront, c.thrusters, c.cdaTop), c);
  }
  const auto line = FleetRoute();
  const Route route = Route::Make(line);
  BoxSpace space;
  const FollowParams params;
  float topMin = 1e9f, topMax = 0.0f, surgeMin = 1e9f, surgeMax = 0.0f, offMax = 0.0f, errMax = 0.0f, brakeMax = 0.0f;
  float tMin = 1e9f, tMax = 0.0f;
  std::string slowest, fastest, worstOff;
  int arrived = 0;
  for (const auto &[k, c] : distinct) {
    const VehicleSpec s = CharacterSpec(c, 0.5f);
    const float top = TopSpeed(s);
    if (top < topMin) {
      topMin = top;
      slowest = c.name;
    }
    if (top > topMax) {
      topMax = top;
      fastest = c.name;
    }
    surgeMin = std::min(surgeMin, s.surge);
    surgeMax = std::max(surgeMax, s.surge);
    CHECK(top > 0.0f && top <= s.speedCap);
    CHECK(Drag(s, top) < s.surge);
    CHECK(Accel(s, top) >= 0.5f);
    CHECK(Decel(s, 60.0f) > Decel(s, 20.0f));
    brakeMax = std::max(brakeMax, BrakingDistance(s, top));
    const Trajectory traj = BuildTrajectory(s, line, TrajectoryParams{});
    const auto r = Follow(s, route, traj, space, params);
    arrived += r.arrived ? 1 : 0;
    CHECK(r.arrived);
    CHECK(r.blocked == 0 && r.emergency == 0);
    if (r.offMax > offMax) {
      offMax = r.offMax;
      worstOff = c.name;
    }
    if (r.offMax > 10.0f) {
      std::printf("    %s: surge %.1f, top %.1f, off %.2f m, lost %d at", c.name.c_str(), s.surge, top, r.offMax, r.lost);
      for (float at : r.lostAt) {
        std::printf(" %.0f", at);
      }
      std::printf("\n");
    }
    errMax = std::max(errMax, r.speedErrMean);
    tMin = std::min(tMin, r.time);
    tMax = std::max(tMax, r.time);
  }
  std::printf("  character over %zu cars: top speed %.1f (%s) to %.1f m/s (%s), surge %.1f to %.1f m/s2, braking from "
              "top at most %.0f m; flown %.2f to %.2f s, off the line at most %.2f m (%s), speed error at most %.2f m/s; "
              "%d arrived\n",
              distinct.size(), topMin, slowest.c_str(), topMax, fastest.c_str(), surgeMin, surgeMax, brakeMax, tMin, tMax,
              offMax, worstOff.c_str(), errMax, arrived);
  CHECK(topMin > 45.0f && topMin < 55.0f); // 49.7 m/s measured (2026-11-04): q204's bus, a 2100 kg record on the coach
  CHECK(topMax == VehicleSpec{}.speedCap); // 120: the clean coupes reach the cap
  CHECK(surgeMin > 5.5f && surgeMax < 26.0f); // 6.2 (the Behemoth) to 24.5 m/s2 measured
  CHECK(brakeMax < 420.0f);                // 400 m measured
  CHECK(tMax < 42.0f);                     // 29.7 to 40.6 s measured
  CHECK(errMax < 3.1f);                    // 2.78 m/s measured
  // The heavy cars with a third of today's surge leave the line in the 200 m turn (s 917 to 940) and over the crest.
  // Since the bends are planned with the surge the drag leaves and at the speed the surge hold flies (2026-11-04) the
  // Behemoth stays within 7.6 m (was 20.35). The worst left is a low-drag 6.2 m/s2 car (top 91 m/s) in the turn: it
  // flies the plan's speed, but its roll sits at the 55 deg cap and its 2.1 m/s2 of sway cannot take back the metres
  // it lost entering the turn. 17.33 m measured while the surge hold held full surge past the plan; 19.13 m since its
  // push falls off over the slack (the Behemoth 10.19 m), lost in the turn and once before it (s 825).
  CHECK(offMax < 20.0f);
}
