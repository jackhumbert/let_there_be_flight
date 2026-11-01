// Thruster (src/red4ext/Flight/Thruster.hpp): IFlightThruster.Update's maths, the bike GetPitch / GetYaw overrides and
// the four UpdateGetDisplacement bodies. Expected values are worked by hand from the scripts.
#include "check.hpp"

#include "Flight/Thruster.hpp"

using namespace Flight;

namespace {
constexpr double kTol = 1e-6;
// degrees, where acos enters
constexpr double kDeg = 1e-3;
constexpr double kPi = 3.14159265358979323846;

void CheckVec(const Vec3 &v, double x, double y, double z, double tol) {
  CHECK_NEAR(v.x, x, tol);
  CHECK_NEAR(v.y, y, tol);
  CHECK_NEAR(v.z, z, tol);
}

// The force whose angle from Up about the X axis is deg (GetAngleDegAroundAxis(Up, f, Right) == deg).
Vec3 PitchForce(double deg) {
  double r = deg * kPi / 180.0;
  return {0.0f, (float)-std::sin(r), (float)std::cos(r)};
}

// The force whose angle from Up about the Y axis is deg (GetAngleDegAroundAxis(Up, f, Forward) == deg).
Vec3 YawForce(double deg) {
  double r = deg * kPi / 180.0;
  return {(float)std::sin(r), 0.0f, (float)std::cos(r)};
}

ThrusterParams Car(bool isFront, bool isRight) {
  ThrusterParams p;
  p.isFront = isFront;
  p.isRight = isRight;
  return p;
}

// Yaw window values that are all different: the left window is [-32, 62] and clamps to [-20, 50]; the right window is
// [-62, 32] and clamps to [-50, 20].
ThrusterParams YawCar(bool isFront, bool isRight) {
  ThrusterParams p = Car(isFront, isRight);
  p.maxThrusterAngleOutside = 50.0f;
  p.maxThrusterAngleInside = 20.0f;
  p.thrusterAngleAllowance = 12.0f;
  p.mainThrusterYawFactor = 5.0f;
  return p;
}
} // namespace

TEST(thruster_normalize) {
  CheckVec(ThrusterNormalize({30, 0, 40}, 10.0f), 0.6, 0, 0.8, kTol);
  CheckVec(ThrusterNormalize({3, 0, 4}, 10.0f), 0.3, 0, 0.4, kTol);
  CheckVec(ThrusterNormalize({0, 0, 0}, 10.0f), 0, 0, 0, kTol);
  // a length exactly at the threshold is divided, which equals the normal there
  CheckVec(ThrusterNormalize({6, 0, 8}, 10.0f), 0.6, 0, 0.8, kTol);
  // below the threshold it is not normalised: length 0.5 stays 0.5 over threshold 1
  CheckVec(ThrusterNormalize({0, -0.5f, 0}, 1.0f), 0, -0.5, 0, kTol);
  CheckVec(ThrusterNormalize({0, 0, -20}, 10.0f), 0, 0, -1, kTol);
}

TEST(thruster_pitch_car) {
  const Vec3 f{0, 0.5f, 0.8660254f}; // -30 degrees
  ThrusterParams left = Car(false, false), right = Car(false, true);
  CHECK_NEAR(ThrusterPitch(left, f, {0, 0, 0}), 30.0, kDeg);
  CHECK_NEAR(ThrusterPitch(right, f, {0, 0, 0}), -30.0, kDeg);
  // torque.y scales by 1 - 0.5 * 0.5; the sign of torque.y does not matter
  CHECK_NEAR(ThrusterPitch(left, f, {0, 0.5f, 0}), 22.5, kDeg);
  CHECK_NEAR(ThrusterPitch(left, f, {0, -0.5f, 0}), 22.5, kDeg);
  CHECK_NEAR(ThrusterPitch(right, f, {0, 1.0f, 0}), -15.0, kDeg);
  // torque x and z do not enter
  CHECK_NEAR(ThrusterPitch(left, f, {0.7f, 0, 0.9f}), 30.0, kDeg);
  // +100 degrees is inside the window (105) and clamps to 90
  CHECK_NEAR(ThrusterPitch(left, {0, -0.9848078f, -0.1736482f}, {0, 0, 0}), -90.0, kDeg);
  CHECK_NEAR(ThrusterPitch(right, {0, -0.9848078f, -0.1736482f}, {0, 0, 0}), 90.0, kDeg);
  // +150 degrees is past 105: dropped to 0
  CHECK_NEAR(ThrusterPitch(left, {0, -0.5f, -0.8660254f}, {0, 0, 0}), 0.0, kDeg);
  // the negative side: -100 clamps to -90, -150 is dropped
  CHECK_NEAR(ThrusterPitch(left, {0, 0.9848078f, -0.1736482f}, {0, 0, 0}), 90.0, kDeg);
  CHECK_NEAR(ThrusterPitch(right, {0, 0.9848078f, -0.1736482f}, {0, 0, 0}), -90.0, kDeg);
  CHECK_NEAR(ThrusterPitch(left, {0, 0.5f, -0.8660254f}, {0, 0, 0}), 0.0, kDeg);
}

TEST(thruster_pitch_window) {
  // max pitch 70 and allowance 12 (window 82): 75 and 80 clamp to 70, 85 is dropped, on both sides
  ThrusterParams left = Car(false, false);
  left.maxThrusterAnglePitch = 70.0f;
  left.thrusterAngleAllowance = 12.0f;
  CHECK_NEAR(ThrusterPitch(left, PitchForce(60), {0, 0, 0}), -60.0, kDeg);
  CHECK_NEAR(ThrusterPitch(left, PitchForce(75), {0, 0, 0}), -70.0, kDeg);
  CHECK_NEAR(ThrusterPitch(left, PitchForce(80), {0, 0, 0}), -70.0, kDeg);
  CHECK_NEAR(ThrusterPitch(left, PitchForce(85), {0, 0, 0}), 0.0, kDeg);
  CHECK_NEAR(ThrusterPitch(left, PitchForce(-75), {0, 0, 0}), 70.0, kDeg);
  CHECK_NEAR(ThrusterPitch(left, PitchForce(-80), {0, 0, 0}), 70.0, kDeg);
  CHECK_NEAR(ThrusterPitch(left, PitchForce(-85), {0, 0, 0}), 0.0, kDeg);
}

TEST(thruster_yaw_car) {
  const Vec3 f{0.5f, 0, 0.8660254f}; // +30 degrees about Y
  const Vec3 t{0, 0, 0.2f};
  // FL (front, left): tor +0.2 * 5 = 1, clamp(30, -20, 50) * -1 = -30 -> -29
  CHECK_NEAR(ThrusterYaw(YawCar(true, false), f, t, true), -29.0, kDeg);
  // FR: tor -0.2 * 5 = -1, clamp(30, -50, 20) = 20 -> 19
  CHECK_NEAR(ThrusterYaw(YawCar(true, true), f, t, true), 19.0, kDeg);
  // BL: tor -0.2 * 5 = -1, -30 -> -31
  CHECK_NEAR(ThrusterYaw(YawCar(false, false), f, t, true), -31.0, kDeg);
  // BR: tor +0.2 * 5 = 1, +20 -> 21
  CHECK_NEAR(ThrusterYaw(YawCar(false, true), f, t, true), 21.0, kDeg);
  // inactive is 180 whatever the force
  CHECK_NEAR(ThrusterYaw(YawCar(true, false), f, t, false), 180.0, kDeg);
  CHECK_NEAR(ThrusterYaw(YawCar(false, true), YawForce(80), t, false), 180.0, kDeg);
  // +80 is past both windows: dropped, only the torque term stays
  CHECK_NEAR(ThrusterYaw(YawCar(true, false), YawForce(80), t, true), 1.0, kDeg);
  CHECK_NEAR(ThrusterYaw(YawCar(true, true), YawForce(80), t, true), -1.0, kDeg);
}

TEST(thruster_yaw_car_window) {
  const Vec3 t{0, 0, 0.2f};
  // left (FL, torque term +1): +60 is inside 62 and clamps to 50 -> -49; +64 is dropped -> 1
  CHECK_NEAR(ThrusterYaw(YawCar(true, false), YawForce(60), t, true), -49.0, kDeg);
  CHECK_NEAR(ThrusterYaw(YawCar(true, false), YawForce(64), t, true), 1.0, kDeg);
  // left: -30 is inside -32 and clamps to -20 -> +20 + 1; -34 is dropped -> 1
  CHECK_NEAR(ThrusterYaw(YawCar(true, false), YawForce(-30), t, true), 21.0, kDeg);
  CHECK_NEAR(ThrusterYaw(YawCar(true, false), YawForce(-34), t, true), 1.0, kDeg);
  // right (BR, torque term +1): +30 is inside 32 and clamps to 20; +34 is dropped; -60 is inside -62 and clamps to
  // -50; -64 is dropped
  CHECK_NEAR(ThrusterYaw(YawCar(false, true), YawForce(30), t, true), 21.0, kDeg);
  CHECK_NEAR(ThrusterYaw(YawCar(false, true), YawForce(34), t, true), 1.0, kDeg);
  CHECK_NEAR(ThrusterYaw(YawCar(false, true), YawForce(-60), t, true), -49.0, kDeg);
  CHECK_NEAR(ThrusterYaw(YawCar(false, true), YawForce(-64), t, true), 1.0, kDeg);
}

TEST(thruster_roll) {
  CHECK_NEAR(ThrusterRoll(Car(false, true)), 0.0, kTol);
  CHECK_NEAR(ThrusterRoll(Car(false, false)), 180.0, kTol);
  // the bikes inherit GetRoll
  ThrusterParams bike = Car(true, false);
  bike.shape = ThrusterShape::BikeFront;
  CHECK_NEAR(ThrusterRoll(bike), 180.0, kTol);
  bike.shape = ThrusterShape::BikeBack;
  bike.isRight = true;
  CHECK_NEAR(ThrusterRoll(bike), 0.0, kTol);
}

TEST(thruster_euler) {
  // force +30 about Y has no part about X: pitch 0; roll 180 (left); yaw -29 as in thruster_yaw_car
  ThrusterAngles a = ThrusterEuler(YawCar(true, false), {0.5f, 0, 0.8660254f}, {0, 0, 0.2f}, true);
  CHECK_NEAR(a.pitch, 0.0, kDeg);
  CHECK_NEAR(a.roll, 180.0, kDeg);
  CHECK_NEAR(a.yaw, -29.0, kDeg);
  // right, inactive: pitch -30, roll 0, yaw 180
  ThrusterAngles b = ThrusterEuler(Car(false, true), {0, 0.5f, 0.8660254f}, {0, 0, 0}, false);
  CHECK_NEAR(b.pitch, -30.0, kDeg);
  CHECK_NEAR(b.roll, 0.0, kDeg);
  CHECK_NEAR(b.yaw, 180.0, kDeg);
}

TEST(thruster_bike) {
  ThrusterParams front = Car(true, false), back = Car(false, false);
  front.shape = ThrusterShape::BikeFront;
  back.shape = ThrusterShape::BikeBack;
  // inside differs from outside and must be ignored: both bike windows use maxThrusterAngleOutside
  front.maxThrusterAngleInside = back.maxThrusterAngleInside = 5.0f;
  const Vec3 pf{0, 0.5f, 0.8660254f}; // -30 degrees
  CHECK_NEAR(ThrusterPitch(front, pf, {0, 0, 0}), 30.0, kDeg);
  CHECK_NEAR(ThrusterPitch(back, pf, {0, 0, 0}), 30.0, kDeg);
  // isRight does not flip a bike's pitch
  front.isRight = back.isRight = true;
  CHECK_NEAR(ThrusterPitch(front, pf, {0, 0, 0}), 30.0, kDeg);
  CHECK_NEAR(ThrusterPitch(back, pf, {0, 0, 0}), 30.0, kDeg);
  front.isRight = back.isRight = false;
  // pitch clamps to the pitch limit, drops past the window and scales with torque.y
  CHECK_NEAR(ThrusterPitch(front, {0, -0.9848078f, -0.1736482f}, {0, 0, 0}), -90.0, kDeg);
  CHECK_NEAR(ThrusterPitch(back, {0, -0.5f, -0.8660254f}, {0, 0, 0}), 0.0, kDeg);
  CHECK_NEAR(ThrusterPitch(front, pf, {0, 0.5f, 0}), 22.5, kDeg);

  const Vec3 yf{0.5f, 0, 0.8660254f}; // +30 degrees about Y
  const Vec3 t{0, 0, 0.1f};
  CHECK_NEAR(ThrusterYaw(front, yf, t, true), 33.0, kDeg);
  CHECK_NEAR(ThrusterYaw(back, yf, t, true), -33.0, kDeg);
  CHECK_NEAR(ThrusterYaw(front, yf, t, false), 180.0, kDeg);
  CHECK_NEAR(ThrusterYaw(back, yf, t, false), 180.0, kDeg);
  // +70 is inside 75 and clamps to 60; +80 is past 75 and dropped, leaving the torque term
  CHECK_NEAR(ThrusterYaw(front, YawForce(70), t, true), 63.0, kDeg);
  CHECK_NEAR(ThrusterYaw(back, YawForce(70), t, true), -63.0, kDeg);
  CHECK_NEAR(ThrusterYaw(front, YawForce(80), t, true), 3.0, kDeg);
  CHECK_NEAR(ThrusterYaw(back, YawForce(80), t, true), -3.0, kDeg);
  // the negative side
  CHECK_NEAR(ThrusterYaw(front, YawForce(-70), t, true), -57.0, kDeg);
  CHECK_NEAR(ThrusterYaw(back, YawForce(-70), t, true), 57.0, kDeg);
  CHECK_NEAR(ThrusterYaw(front, YawForce(-80), t, true), 3.0, kDeg);
  CHECK_NEAR(ThrusterYaw(back, YawForce(-80), t, true), -3.0, kDeg);
}

TEST(thruster_fx) {
  const Vec3 up{0, 0, 1}, f{0, 0, 0.5f}, t{0.2f, 0.3f, 0.1f};
  float bb = -9.0f;
  // Regular, not front: x = -0.2, amount 0.5 - 0.2 + 0.3 = 0.6, blackboard 0.6 + 0.1
  float r = ThrusterFxDisplacement(ThrusterFxKind::Regular, up, f, t, false, false, bb);
  CHECK_NEAR(r, 0.6, kTol);
  CHECK_NEAR(bb, 0.7, kTol);
  // Main, front and right: x = 0.2, y = -0.3, blackboard 0.5 + 0.2 - 0.3
  r = ThrusterFxDisplacement(ThrusterFxKind::Main, up, f, t, true, true, bb);
  CHECK_NEAR(r, 0.5, kTol);
  CHECK_NEAR(bb, 0.4, kTol);
  // Side, front and right: vec (-1,0,0), tor -0.1, (-0.5 - 0.1) * 0.1
  r = ThrusterFxDisplacement(ThrusterFxKind::Side, up, {0.5f, 0, 0}, t, true, true, bb);
  CHECK_NEAR(r, 0.0, kTol);
  CHECK_NEAR(bb, -0.06, kTol);
  // Base
  bb = 5.0f;
  r = ThrusterFxDisplacement(ThrusterFxKind::Base, up, f, t, true, true, bb);
  CHECK_NEAR(r, 0.0, kTol);
  CHECK_NEAR(bb, 0.0, kTol);
}

TEST(thruster_fx_regular) {
  // Dot(up, g) = 0.6 * 0.2 + 0.8 * 0.25 = 0.32
  const Vec3 up{0, 0.6f, 0.8f}, g{0, 0.2f, 0.25f}, t{0.2f, 0.3f, 0.1f};
  float bb = 0;
  // front: 0.32 + 0.2 + 0.3 = 0.82, blackboard + 0.1
  float r = ThrusterFxDisplacement(ThrusterFxKind::Regular, up, g, t, true, false, bb);
  CHECK_NEAR(r, 0.82, kTol);
  CHECK_NEAR(bb, 0.92, kTol);
  // back: 0.32 - 0.2 + 0.3 = 0.42
  r = ThrusterFxDisplacement(ThrusterFxKind::Regular, up, g, t, false, false, bb);
  CHECK_NEAR(r, 0.42, kTol);
  CHECK_NEAR(bb, 0.52, kTol);
  // isRight does not enter
  r = ThrusterFxDisplacement(ThrusterFxKind::Regular, up, g, t, false, true, bb);
  CHECK_NEAR(r, 0.42, kTol);
  CHECK_NEAR(bb, 0.52, kTol);
  // amount 2 returns 1 and the blackboard 1
  r = ThrusterFxDisplacement(ThrusterFxKind::Regular, {0, 0, 1}, {0, 0, 1}, {0.5f, 0.5f, 0.5f}, true, false, bb);
  CHECK_NEAR(r, 1.0, kTol);
  CHECK_NEAR(bb, 1.0, kTol);
  // back, amount -1 - 0.5 - 0.5 = -2 returns -1 and the blackboard -1
  r = ThrusterFxDisplacement(ThrusterFxKind::Regular, {0, 0, 1}, {0, 0, -1}, {0.5f, -0.5f, 0.5f}, false, false, bb);
  CHECK_NEAR(r, -1.0, kTol);
  CHECK_NEAR(bb, -1.0, kTol);
  // only the blackboard clamps: amount 0.9 is returned as is, the blackboard 0.9 + 0.5 -> 1
  r = ThrusterFxDisplacement(ThrusterFxKind::Regular, {0, 0, 1}, {0, 0, 0.9f}, {0, 0, 0.5f}, true, false, bb);
  CHECK_NEAR(r, 0.9, kTol);
  CHECK_NEAR(bb, 1.0, kTol);
}

TEST(thruster_fx_main) {
  // Dot(up, f) = 0.25; t (0.2, 0.3, 0.1), and torque.z does not enter
  const Vec3 up{0, 0, 1}, f{0, 0, 0.25f}, t{0.2f, 0.3f, 0.1f};
  float bb = 0;
  // front, right: x +0.2, y -0.3
  float r = ThrusterFxDisplacement(ThrusterFxKind::Main, up, f, t, true, true, bb);
  CHECK_NEAR(r, 0.25, kTol);
  CHECK_NEAR(bb, 0.15, kTol);
  // front, left: x +0.2, y +0.3
  r = ThrusterFxDisplacement(ThrusterFxKind::Main, up, f, t, true, false, bb);
  CHECK_NEAR(r, 0.25, kTol);
  CHECK_NEAR(bb, 0.75, kTol);
  // back, right: x -0.2, y -0.3
  r = ThrusterFxDisplacement(ThrusterFxKind::Main, up, f, t, false, true, bb);
  CHECK_NEAR(r, 0.25, kTol);
  CHECK_NEAR(bb, -0.25, kTol);
  // back, left: x -0.2, y +0.3
  r = ThrusterFxDisplacement(ThrusterFxKind::Main, up, f, t, false, false, bb);
  CHECK_NEAR(r, 0.25, kTol);
  CHECK_NEAR(bb, 0.35, kTol);
  // clamps
  r = ThrusterFxDisplacement(ThrusterFxKind::Main, up, {0, 0, 2}, t, true, false, bb);
  CHECK_NEAR(r, 1.0, kTol);
  CHECK_NEAR(bb, 1.0, kTol);
  r = ThrusterFxDisplacement(ThrusterFxKind::Main, up, {0, 0, -2}, t, true, false, bb);
  CHECK_NEAR(r, -1.0, kTol);
  CHECK_NEAR(bb, -1.0, kTol);
}

TEST(thruster_fx_side) {
  // f.x = 0.5, torque.z = 0.1; only torque.z and the X part of the force enter
  const Vec3 up{0, 0, 1}, f{0.5f, 0, 0}, t{0.7f, 0.8f, 0.1f};
  float bb = 0;
  // left, back: vec (-1,0,0), tor +0.1 -> (-0.5 + 0.1) * 0.1
  float r = ThrusterFxDisplacement(ThrusterFxKind::Side, up, f, t, false, false, bb);
  CHECK_NEAR(r, 0.0, kTol);
  CHECK_NEAR(bb, -0.04, kTol);
  // right, back: vec (+1,0,0), tor -0.1 -> (0.5 - 0.1) * 0.1
  r = ThrusterFxDisplacement(ThrusterFxKind::Side, up, f, t, false, true, bb);
  CHECK_NEAR(r, 0.0, kTol);
  CHECK_NEAR(bb, 0.04, kTol);
  // left, front: vec (+1,0,0), tor +0.1 -> (0.5 + 0.1) * 0.1
  r = ThrusterFxDisplacement(ThrusterFxKind::Side, up, f, t, true, false, bb);
  CHECK_NEAR(r, 0.0, kTol);
  CHECK_NEAR(bb, 0.06, kTol);
  // right, front: vec (-1,0,0), tor -0.1 -> (-0.5 - 0.1) * 0.1
  r = ThrusterFxDisplacement(ThrusterFxKind::Side, up, f, t, true, true, bb);
  CHECK_NEAR(r, 0.0, kTol);
  CHECK_NEAR(bb, -0.06, kTol);
  // clamps: a force of 50 gives 5 -> 1 and -50 gives -5 -> -1
  ThrusterFxDisplacement(ThrusterFxKind::Side, up, {50, 0, 0}, t, true, false, bb);
  CHECK_NEAR(bb, 1.0, kTol);
  ThrusterFxDisplacement(ThrusterFxKind::Side, up, {-50, 0, 0}, t, true, false, bb);
  CHECK_NEAR(bb, -1.0, kTol);
}

TEST(thruster_deviation) {
  ThrusterParams p;
  // 0.3 + (1.6 * 0.025 - 0.3) * 0.25 = 0.235
  CHECK_NEAR(ThrusterDeviation(p, 0.3f, 1.6f), 0.235, kTol);
  // a different lerp amount: 0.3 + (0.04 - 0.3) * 0.5 = 0.17
  p.boneLerpAmount = 0.5f;
  CHECK_NEAR(ThrusterDeviation(p, 0.3f, 1.6f), 0.17, kTol);
  // a negative amount: 0.3 + (-0.04 - 0.3) * 0.5 = 0.13
  CHECK_NEAR(ThrusterDeviation(p, 0.3f, -1.6f), 0.13, kTol);
}

TEST(thruster_scale) {
  CheckVec(ThrusterScale({0, 0, 0}, true), 0.1, 0.1, 0.1, kTol);
  CheckVec(ThrusterScale({1, 1, 1}, false), 0.9, 0.9, 0.9, kTol);
  // the components are independent: (0.5, 0.2, 1) active -> 0.55, 0.28, 1; inactive -> 0.45, 0.18, 0.9
  CheckVec(ThrusterScale({0.5f, 0.2f, 1.0f}, true), 0.55, 0.28, 1.0, kTol);
  CheckVec(ThrusterScale({0.5f, 0.2f, 1.0f}, false), 0.45, 0.18, 0.9, kTol);
}

TEST(thruster_yaw_car_defaults) {
  // the script's defaults (outside 60, inside 15, allowance 15, factor 0.5), away from every window edge; t.z 0.2
  // makes the torque term +-0.1. Front left: dir -1, the window [-30, 75]; front right: dir +1, [-75, 30].
  ThrusterParams fl;
  fl.isFront = true;
  ThrusterParams fr = fl;
  fr.isRight = true;
  const Vec3 t{0, 0, 0.2f};
  // +10 degrees about Y: (sin 10, 0, cos 10). FL: -10 + 0.1; FR: 10 - 0.1
  const Vec3 ten{0.17364818f, 0, 0.98480775f};
  CHECK_NEAR(ThrusterYaw(fl, ten, t, true), -9.9, 1e-3);
  CHECK_NEAR(ThrusterYaw(fr, ten, t, true), 9.9, 1e-3);
  // -20 degrees about Y: inside the window but past the inside angle 15 for the left one, so clamped:
  // FL: clamp(-20, -15, 60) = -15, times -1, plus 0.1 = 15.1; FR: clamp(-20, -60, 15) = -20, minus 0.1 = -20.1
  const Vec3 minus20{-0.34202014f, 0, 0.93969262f};
  CHECK_NEAR(ThrusterYaw(fl, minus20, t, true), 15.1, 1e-3);
  CHECK_NEAR(ThrusterYaw(fr, minus20, t, true), -20.1, 1e-3);
}
