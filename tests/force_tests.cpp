// FlightMode.ApplyPhysics and the OnUpdate tail as pure code (src/red4ext/Flight/Force.hpp).
// Expected values are worked by hand from the script.
#include "check.hpp"

#include "Flight/Force.hpp"

using namespace Flight;

namespace {
constexpr double kSmall = 1e-5;

const Quat kIdentity{0, 0, 0, 1};
// 90 degrees about Z
const Quat kYaw90{0, 0, 0.70710678f, 0.70710678f};

void CheckVec(const Vec3 &a, double x, double y, double z, double tol = kSmall) {
  CHECK_NEAR(a.x, x, tol);
  CHECK_NEAR(a.y, y, tol);
  CHECK_NEAR(a.z, z, tol);
}

StatsStatic Car() {
  StatsStatic st;
  st.mass = 1500.0f;
  st.airResistanceFactor = 0.3f;
  st.brakingFrictionFactor = 1.0f;
  st.momentOfInertia = {2850.0f, 2850.0f, 2850.0f};
  return st;
}

// Level, facing +Y, with the given velocity (world = local at identity orientation).
StatsState Flying(Vec3 velocity) {
  StatsState s;
  s.d_orientation = kIdentity;
  s.d_forward = {0, 1, 0};
  s.d_right = {1, 0, 0};
  s.d_up = {0, 0, 1};
  s.d_velocity = s.d_localVelocity = velocity;
  s.d_speed = Length(velocity);
  s.d_speedRatio = s.d_speed / 100.0f;
  s.d_direction = s.d_localDirection = Normalize(velocity);
  return s;
}

ForceInput Base() {
  ForceInput in;
  in.thrusterTensor = {1, 1, 1};
  return in;
}
} // namespace

TEST(force_cap_damping) {
  ForceParams p;
  // response 0.19 * 1500 * 1 / 2850 = 0.1, maxGain 10
  CHECK_NEAR(CapDamping(30, 2, 1, 2850, 1500, p), 20, 1e-4);
  CHECK_NEAR(CapDamping(10, 2, 1, 2850, 1500, p), 10, 1e-4);
  CHECK_NEAR(CapDamping(-30, -2, 1, 2850, 1500, p), -20, 1e-4);
  CHECK_NEAR(CapDamping(30, -2, 1, 2850, 1500, p), 30, 1e-4);
  CHECK_NEAR(CapDamping(30, 0.00005f, 1, 2850, 1500, p), 30, 1e-4);
  CHECK_NEAR(CapDamping(30, 2, 1, 0, 1500, p), 30, 1e-4);
  CHECK_NEAR(CapDamping(30, 2, 0, 2850, 1500, p), 30, 1e-4);
}

TEST(force_air_drag) {
  ForceParams p;
  StatsStatic st = Car();
  StatsState s = Flying({0, 20, 0});
  ForceInput in = Base();
  ForceOutput out;
  ApplyPhysics(p, st, s, in, out);
  // 20 * 20 * 1.2 * 0.3 / 1500
  CheckVec(out.physicsForce, 0, -0.096, 0, 1e-6);
  CheckVec(out.physicsTorque, 0, 0, 0, 1e-6);

  in.dt = 0.5f;
  in.collisionTimer = 10.0f;
  ForceOutput tail = ForceTail(p, st, s, in);
  // -0.096 * 0.5 * 1500
  CheckVec(tail.worldForce, 0, -72, 0, 1e-3);

  p.physxFlightPhysics = false;
  ApplyPhysics(p, st, s, in, out);
  CheckVec(out.physicsForce, 0, 0, 0, 1e-6);

  p.physxFlightPhysics = true;
  st.mass = 0;
  ApplyPhysics(p, st, s, in, out);
  CheckVec(out.physicsForce, 0, 0, 0, 1e-6);
}

TEST(force_angular_damp) {
  ForceParams p;
  StatsStatic st = Car();
  StatsState s = Flying({0, 0, 0});
  s.d_angularVelocity = {0.1f, 0.2f, 0.3f};
  ForceInput in = Base();
  in.pitch = 0.5f;
  ForceOutput out;
  // damp = spin * 2, x scaled by 1 - 0.5; the aero angles are 90 degrees times speedRatio 0
  ApplyPhysics(p, st, s, in, out);
  CheckVec(out.physicsTorque, -0.1, -0.4, -0.6, 1e-6);

  // spin about Z that contacts gave is not damped
  in.contactSpin = {0, 0, 0.3f};
  ApplyPhysics(p, st, s, in, out);
  CHECK_NEAR(out.physicsTorque.z, 0, 1e-6);

  // factor 2 + 1 * 1 * 1.4 = 3.4
  in.pitch = 0;
  in.contactSpin = {0, 0, 0};
  in.angularBrake = 1.0f;
  st.brakingFrictionFactor = 1.4f;
  ApplyPhysics(p, st, s, in, out);
  CheckVec(out.physicsTorque, -0.34, -0.68, -1.02, 1e-5);
}

TEST(force_damp_cap_and_clamp) {
  ForceParams p;
  StatsStatic st = Car();
  StatsState s = Flying({0, 0, 0});
  s.d_angularVelocity = {0, 0, 3};
  ForceInput in = Base();
  ForceOutput out;
  // damp 6, gain 2 under maxGain 10, then clamped to length 3.5
  ApplyPhysics(p, st, s, in, out);
  CHECK_NEAR(out.physicsTorque.z, -3.5, 1e-5);

  // response 0.19 * 1500 / 285 = 1, maxGain 1: damp capped to 3, under the 3.5 clamp
  st.momentOfInertia = {285.0f, 285.0f, 285.0f};
  ApplyPhysics(p, st, s, in, out);
  CHECK_NEAR(out.physicsTorque.z, -3.0, 1e-5);
}

TEST(force_aero_redirect) {
  ForceParams p;
  p.physxFlightPhysics = false;
  p.generalYawAeroFactor = 0.1f;
  p.generalPitchAeroFactor = 0.1f;
  StatsStatic st = Car();
  StatsState s = Flying({3, 4, 0});
  ForceInput in = Base();
  ForceOutput out;
  ApplyPhysics(p, st, s, in, out);
  // speedRatio 0.05, sideY 0.6, yawDir 0.25, aeroFactor 0.8: (0, 0.12, 0) + (-0.072, -0.096, 0)
  CheckVec(out.physicsForce, -0.072, 0.024, 0);
  // yawAngle 36.8699 deg * 0.05 * 0.1; pitchAngle 0
  CheckVec(out.physicsTorque, 0, 0, -0.1843495);
}

TEST(force_aero_reversed) {
  ForceParams p;
  p.physxFlightPhysics = false;
  p.generalYawAeroFactor = 0.1f;
  p.generalPitchAeroFactor = 0.1f;
  StatsStatic st = Car();
  StatsState s = Flying({-3, -4, 0});
  ForceInput in = Base();
  ForceOutput out;
  ApplyPhysics(p, st, s, in, out);
  // The angles use the flipped direction (0.6, 0.8, 0), so the torque is the same as flying forwards.
  // The side terms use the unflipped one: off = (0.6, 1.8, 0), sideY 0.6, aeroFactor -0.8, yawDir 0.25:
  // forward * 0.6 * 0.25 * -0.8 = (0, -0.12, 0), and -localDirection * 0.6 * 0.25 * 0.8 = (0.072, 0.096, 0).
  CheckVec(out.physicsForce, 0.072, -0.024, 0);
  CheckVec(out.physicsTorque, 0, 0, -0.1843495);
}

TEST(force_tail_world) {
  ForceParams p;
  StatsStatic st = Car();
  StatsState s;
  s.d_orientation = kYaw90;
  ForceInput in = Base();
  in.applyPhysics = false;
  in.dt = 0.5f;
  in.collisionTimer = 10.0f;
  in.thrusterTensor = {2, 0.5f, 3};
  in.force = {0, 1, 0};
  in.torque = {1, 1, 1};
  ForceOutput out = ForceTail(p, st, s, in);
  // torque * 750 = (1500, 750, 2250) with y's max(0.5, 1) = 1, then 90 degrees about Z
  CheckVec(out.worldForce, -750, 0, 0, 1e-2);
  CheckVec(out.worldTorque, -750, 1500, 2250, 1e-2);
  CHECK_NEAR(out.collisionTimer, 10, 1e-6);

  in.thrusterTensor = {2, 2, 3};
  in.torque = {0, 1, 0};
  out = ForceTail(p, st, s, in);
  CheckVec(out.worldTorque, -1500, 0, 0, 1e-2);
}

TEST(force_collision_recovery) {
  ForceParams p;
  StatsStatic st = Car();
  st.mass = 2.0f;
  StatsState s;
  s.d_orientation = kIdentity;
  ForceInput in = Base();
  in.applyPhysics = false;
  in.dt = 0.5f;
  in.force = {0, 1, 0};

  in.collisionTimer = 1.2f;
  ForceOutput out = ForceTail(p, st, s, in);
  CheckVec(out.worldForce, 0, 0.5, 0);
  CHECK_NEAR(out.collisionTimer, 1.7, 1e-6);

  in.collisionTimer = 0.5f;
  out = ForceTail(p, st, s, in);
  CheckVec(out.worldForce, 0, 0, 0);
  CHECK_NEAR(out.collisionTimer, 1.0, 1e-6);

  in.collisionTimer = 2.0f;
  out = ForceTail(p, st, s, in);
  CheckVec(out.worldForce, 0, 1, 0);
  CHECK_NEAR(out.collisionTimer, 2.0, 1e-6);
}

TEST(force_dt_zero) {
  ForceParams p;
  StatsStatic st = Car();
  StatsState s;
  s.d_orientation = kIdentity;
  ForceInput in = Base();
  in.applyPhysics = false;
  in.dt = 0;
  in.force = {1, 2, 3};
  in.torque = {1, 2, 3};
  in.collisionTimer = 1.2f;
  ForceOutput out = ForceTail(p, st, s, in);
  CheckVec(out.worldForce, 0, 0, 0);
  CheckVec(out.worldTorque, 0, 0, 0);
  CHECK_NEAR(out.collisionTimer, 1.2, 1e-6);
}

TEST(force_linear_damp) {
  ForceParams p;
  p.generalDampFactorLinear = 0.2f;
  p.brakeFactorLinear = 1.0f;
  StatsStatic st = Car();
  StatsState s = Flying({0, 0, 0});
  ForceInput in = Base();
  in.linearBrake = 0.5f;
  ForceOutput out;
  ApplyPhysics(p, st, s, in, out);
  CHECK_NEAR(out.linearDamp, 0.7, 1e-6);
}

TEST(force_aero_pitch) {
  ForceParams p;
  p.physxFlightPhysics = false;
  p.generalYawAeroFactor = 0.1f;
  p.generalPitchAeroFactor = 0.1f;
  p.generalPitchDirectionalityFactor = 20.0f;
  StatsStatic st = Car();
  StatsState s = Flying({0, 4, 3});
  ForceInput in = Base();
  ForceOutput out;
  ApplyPhysics(p, st, s, in, out);
  // speed 5, speedRatio 0.05, direction (0, 0.8, 0.6), forward (0,1,0): off = forward - direction = (0, 0.2, -0.6).
  // sideY = |off . right| = 0, sideP = |off . up| = 0.6, aeroFactor = 0.8, pitchDir = 0.05 * 20 = 1.
  // forward * 0.6 * 1 * 0.8 = (0, 0.48, 0); -localDirection * 0.6 * 1 * 0.8 = (0, -0.384, -0.288).
  CheckVec(out.physicsForce, 0, 0.096, -0.288);
  // yawAngle is 0 (the XY projections of direction and forward are parallel). pitchAngle projects onto the
  // plane of right: acos(0.8) = 36.8699 deg, and Dot(right, direction x forward) = -0.6 < 0 makes it negative.
  // torque.x -= -36.8699 * 0.05 * 0.1; torque.z stays 0 (swapping up and right in the angles would move it there).
  CheckVec(out.physicsTorque, 0.1843495, 0, 0);
}

TEST(force_contact_spin_orientation) {
  ForceParams p;
  StatsStatic st = Car();
  StatsState s = Flying({0, 0, 0});
  s.d_orientation = kYaw90; // Rotate(q, X) = Y, Rotate(q, Y) = -X; the conjugate turns the other way
  ForceInput in = Base();
  ForceOutput out;

  // world X spin of 1: car frame Rotate(conj q, X) = -Y = (0, -1, 0). spin = (0.2, -0.6, 0.3) - (0, -1, 0)
  // = (0.2, 0.4, 0.3); damp = spin * 2 = (0.4, 0.8, 0.6), no cap (gain 2 < 10, length 1.08 < 3.5).
  // Rotate(q, c) instead would give spin.y = -1.6.
  s.d_angularVelocity = {0.2f, -0.6f, 0.3f};
  in.contactSpin = {1, 0, 0};
  ApplyPhysics(p, st, s, in, out);
  CheckVec(out.physicsTorque, -0.4, -0.8, -0.6);

  // world Y spin of 0.5: Rotate(conj q, Y) = X, so (0.5, 0, 0). spin = (0.9, 0.1, 0) - (0.5, 0, 0) = (0.4, 0.1, 0),
  // damp = (0.8, 0.2, 0).
  s.d_angularVelocity = {0.9f, 0.1f, 0};
  in.contactSpin = {0, 0.5f, 0};
  ApplyPhysics(p, st, s, in, out);
  CheckVec(out.physicsTorque, -0.8, -0.2, 0);
}

TEST(force_roll_yaw_inputs) {
  ForceParams p;
  p.physxFlightPhysics = false;
  p.generalYawDampFactor = 0.4f;
  p.generalSwayWithYaw = 0.3f;
  StatsStatic st = Car();
  StatsState s = Flying({3, 4, 0});
  s.d_angularVelocity = {0.1f, 0.2f, 0.3f};
  ForceInput in = Base();
  in.roll = 0.25f;
  in.yaw = -0.5f; // only |yaw| counts
  ForceOutput out;
  ApplyPhysics(p, st, s, in, out);
  // damp = spin * 2 = (0.2, 0.4, 0.6); y * (1 - 0.25) = 0.3; z * (1 - 0.5) * 0.4 = 0.12; no cap (gains <= 2).
  // The aero torque factors are 0, so the torque is just -damp.
  CheckVec(out.physicsTorque, -0.2, -0.3, -0.12);
  // speedRatio 0.05, direction (0.6, 0.8, 0): off = (-0.6, 0.2, 0), sideY 0.6, sideP 0, aeroFactor 0.8.
  // yawDir = (0.05 + 0.5 * 0.3) * 5 = 1 (0.25 without the sway term).
  // forward * 0.6 * 1 * 0.8 = (0, 0.48, 0); -localDirection * 0.6 * 1 * 0.8 = (-0.288, -0.384, 0).
  CheckVec(out.physicsForce, -0.288, 0.096, 0);
}

TEST(force_tail_with_physics) {
  ForceParams p;
  StatsStatic st = Car();
  StatsState s = Flying({0, 20, 0});
  s.d_orientation = kYaw90; // the car-frame fields above are given independently of it
  s.d_angularVelocity = {0.1f, 0.2f, 0.3f};
  ForceInput in = Base();
  in.dt = 0.5f;
  in.collisionTimer = 10.0f;
  in.thrusterTensor = {2, 0.5f, 3};
  in.force = {1, 2, 3};
  in.torque = {0.5f, -1, 2};
  ForceOutput out = ForceTail(p, st, s, in);
  // ApplyPhysics: drag 20 * 20 * 1.2 * 0.3 / 1500 = 0.096 backwards, force (0, -0.096, 0); the aero angles are 0
  // (direction = forward); damp = spin * 2 = (0.2, 0.4, 0.6) with no cap (gains 2 under 5, 10, 3.33), torque -damp.
  CheckVec(out.physicsForce, 0, -0.096, 0, 1e-6);
  CheckVec(out.physicsTorque, -0.2, -0.4, -0.6, 1e-6);
  // sums: force (1, 1.904, 3), torque (0.3, -1.4, 1.4); both times dt * mass = 750.
  // force (750, 1428, 2250), rotated 90 deg about Z ((x, y, z) -> (-y, x, z)): (-1428, 750, 2250).
  CheckVec(out.worldForce, -1428, 750, 2250, 1e-2);
  // torque (225, -1050, 1050) * tensor (2, max(0.5, 1) = 1, 3) = (450, -1050, 3150), rotated: (1050, 450, 3150).
  CheckVec(out.worldTorque, 1050, 450, 3150, 1e-2);
  CHECK_NEAR(out.collisionTimer, 10, 1e-6);
}

TEST(force_brake_factors) {
  ForceParams p;
  p.brakeFactorAngular = 3.0f;
  p.generalDampFactorLinear = 0.2f;
  p.brakeFactorLinear = 2.5f;
  StatsStatic st = Car();
  st.brakingFrictionFactor = 1.4f;
  StatsState s = Flying({0, 0, 0});
  s.d_angularVelocity = {0.1f, 0.2f, 0.3f};
  ForceInput in = Base();
  in.angularBrake = 0.5f;
  in.linearBrake = 0.4f;
  ForceOutput out;
  ApplyPhysics(p, st, s, in, out);
  // angular factor 2 + 0.5 * 3 * 1.4 = 4.1: damp (0.41, 0.82, 1.23), gains 4.1 under 10, length 1.53 under 3.5
  CheckVec(out.physicsTorque, -0.41, -0.82, -1.23);
  // linear 0.2 + 0.4 * 2.5
  CHECK_NEAR(out.linearDamp, 1.2, 1e-6);
}

TEST(force_cap_roll_tensor_floor) {
  ForceParams p;
  p.generalDampFactorAngular = 12.0f;
  StatsStatic st = Car();
  StatsState s = Flying({0, 0, 0});
  s.d_angularVelocity = {0, 0.1f, 0};
  ForceInput in = Base();
  in.thrusterTensor = {1, 0.5f, 1};
  ForceOutput out;
  ApplyPhysics(p, st, s, in, out);
  // The cap uses max(0.5, 1) = 1: response 0.19 * 1500 / 2850 = 0.1, maxGain 10, gain 12, so damp = 0.1 * 10 = 1.
  // With the raw 0.5 the maxGain would be 20 and damp 1.2.
  CHECK_NEAR(out.physicsTorque.y, -1.0, 1e-5);
}

TEST(force_cap_damping_no_response) {
  ForceParams p;
  // mass 0: response 0, damp returned unchanged
  CHECK_NEAR(CapDamping(30, 2, 1, 2850, 0, p), 30, 1e-4);
  // negative stability constant: response < 0 returns damp (it would otherwise cap to 2 * -5.26 = -10.5)
  p.dampStabilityK = -0.19f;
  CHECK_NEAR(CapDamping(30, 2, 1, 2850, 1500, p), 30, 1e-4);
  p.dampStabilityK = 0.0f;
  CHECK_NEAR(CapDamping(30, 2, 1, 2850, 1500, p), 30, 1e-4);
}
