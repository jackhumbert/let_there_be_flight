// The cinematic camera's shot maths (src/red4ext/Camera/Cinematic): FlightCinematic.AimAtCar, the attached
// shots' level heading, the aim ahead of the car, the flybys' plan and the distance stop. Expected values are worked by hand from
// src/redscript/Extensions/vehicleCinematicCamera.reds. The orientation checks compare rotated axes, which do not
// depend on the quaternion's sign.
#include "check.hpp"

#include "Camera/Cinematic.hpp"

#include <cmath>

using namespace Cinematic;

namespace {
void CheckVec(const Flight::Vec3 &v, double x, double y, double z, double tol) {
  CHECK_NEAR(v.x, x, tol);
  CHECK_NEAR(v.y, y, tol);
  CHECK_NEAR(v.z, z, tol);
}

Flight::Vec3 Axis(const Flight::Quat &q, int i) {
  return Flight::Rotate(q, {i == 0 ? 1.0f : 0.0f, i == 1 ? 1.0f : 0.0f, i == 2 ? 1.0f : 0.0f});
}

// The frame OrientationFromDir must give for any direction: Y along dir, X level, Z up, a unit quaternion.
void CheckLevelFrame(const Flight::Vec3 &dir) {
  const Flight::Quat q = OrientationFromDir(dir);
  const Flight::Vec3 n = Flight::Normalize(dir);
  CheckVec(Axis(q, 1), n.x, n.y, n.z, 1e-5);
  CHECK_NEAR(Axis(q, 0).z, 0.0, 1e-5);
  CHECK(Axis(q, 2).z > 0.0f);
  CHECK_NEAR(Flight::Dot(Axis(q, 0), Axis(q, 1)), 0.0, 1e-5);
  CHECK_NEAR(q.i * q.i + q.j * q.j + q.k * q.k + q.r * q.r, 1.0, 1e-5);
}

constexpr Flight::Quat kYaw90{0.0f, 0.0f, 0.7071068f, 0.7071068f};
} // namespace

TEST(cinematic_orientation_from_dir) {
  Flight::Quat q = OrientationFromDir({0, 1, 0});
  CheckVec(Axis(q, 1), 0, 1, 0, 1e-5);
  CheckVec(Axis(q, 2), 0, 0, 1, 1e-5);

  q = OrientationFromDir({1, 0, 0});
  CheckVec(Axis(q, 1), 1, 0, 0, 1e-5);
  CheckVec(Axis(q, 0), 0, -1, 0, 1e-5);
  CheckVec(Axis(q, 2), 0, 0, 1, 1e-5);

  // (0,-1,0): trace -1, m22 largest, the last branch
  q = OrientationFromDir({0, -1, 0});
  CheckVec(Axis(q, 1), 0, -1, 0, 1e-5);
  CheckVec(Axis(q, 0), -1, 0, 0, 1e-5);
  CheckVec(Axis(q, 2), 0, 0, 1, 1e-5);

  q = OrientationFromDir({0, 1, 1});
  CheckVec(Axis(q, 1), 0, 0.7071068, 0.7071068, 1e-5);
  CheckVec(Axis(q, 2), 0, -0.7071068, 0.7071068, 1e-5);

  q = OrientationFromDir({-0.3f, -1, 0.2f});
  const Flight::Vec3 n = Flight::Normalize({-0.3f, -1, 0.2f});
  CheckVec(Axis(q, 1), n.x, n.y, n.z, 1e-5);
  CHECK_NEAR(Axis(q, 0).z, 0.0, 1e-5);
}

TEST(cinematic_orientation_from_dir_branches) {
  // With h the horizontal length of the unit direction f, the matrix has m00 = f.y / h, m11 = f.y and m22 = h. With
  // f.y >= 0 the trace is positive. With f.y < 0, m00 <= m11, so the m00 branch (m00 > m11) is never taken, and
  // m11 > m22 needs f.y > 0, so the m11 branch is never taken either: only the trace branch (f.y small against h)
  // and the m22 branch are reachable, and those are covered here.
  // trace branch (trace 3)
  CheckLevelFrame({0, 1, 0});
  CheckLevelFrame({0.4f, 0.8f, -0.3f});
  // trace branch with f.y < 0: f = (0, -0.3, 0.954), trace = -0.3 / 0.954 - 0.3 + 0.954 > 0
  CheckLevelFrame({0, -0.3f, 0.954f});
  // m22 branch, w < 0 before the flip: r.y - f.x < 0 for f = (0.447, -0.894, 0)
  CheckLevelFrame({0.5f, -1, 0});
  // m22 branch, w > 0: r.y - f.x > 0 for f = (-0.447, -0.894, 0)
  CheckLevelFrame({-0.5f, -1, 0});
  CheckLevelFrame({0, -1, 0});
  CheckLevelFrame({-0.3f, -1, 0.2f});
  CheckLevelFrame({-1, -0.01f, 0.1f});
  CheckLevelFrame({1, 1, 0.5f});
  CheckLevelFrame({-1, 1, -0.5f});
}

TEST(cinematic_transform_point) {
  Pose p;
  p.position = {1, 2, 3};
  p.orientation = kYaw90;
  // yaw +90 degrees about Z: Y -> -X, X -> Y
  CheckVec(TransformPoint(p, {0, 20, 0}), -19, 2, 3, 1e-5);
  CheckVec(TransformPoint(p, {1, 0, 0}), 1, 3, 3, 1e-5);
}

TEST(cinematic_aim_far) {
  Pose camera;
  camera.position = {0, -10, 0};
  const Pose out = AimAtCar(camera, {0, 0, 0});
  CHECK(out.position.x == 0.0f && out.position.y == -10.0f && out.position.z == 0.0f);
  CheckVec(Axis(out.orientation, 1), 0, 1, 0, 1e-5);
}

TEST(cinematic_aim_close) {
  Pose camera;
  camera.position = {0, -2, 0};
  Pose out = AimAtCar(camera, {0, 0, 0});
  CheckVec(out.position, 0, -4.5, 0, 1e-5);
  CheckVec(Axis(out.orientation, 1), 0, 1, 0, 1e-5);

  // the clamp's edge: 4.5 m is left alone, 4.4 m is pushed out to 4.5
  camera.position = {0, -4.5f, 0};
  out = AimAtCar(camera, {0, 0, 0});
  CHECK(out.position.y == -4.5f);
  camera.position = {0, -4.4f, 0};
  out = AimAtCar(camera, {0, 0, 0});
  CheckVec(out.position, 0, -4.5, 0, 1e-5);

  // moved along the line to a centre of mass that is not the origin
  camera.position = {10, 20, 30};
  out = AimAtCar(camera, {10, 20, 32});
  CheckVec(out.position, 10, 20, 27.5, 1e-5);
}

TEST(cinematic_aim_vertical) {
  Pose camera;
  camera.position = {0, 0, -10};
  camera.orientation = {0.0f, 0.0f, 0.7071068f, 0.7071068f};
  Pose out = AimAtCar(camera, {0, 0, 0});
  CHECK(out.orientation.i == camera.orientation.i && out.orientation.j == camera.orientation.j);
  CHECK(out.orientation.k == camera.orientation.k && out.orientation.r == camera.orientation.r);
  CheckVec(out.position, 0, 0, -10, 1e-6);

  // too close and vertical: moved out, orientation kept
  camera.position = {0, 0, -2};
  out = AimAtCar(camera, {0, 0, 0});
  CheckVec(out.position, 0, 0, -4.5, 1e-5);
  CHECK(out.orientation.k == camera.orientation.k && out.orientation.r == camera.orientation.r);

  // the threshold 0.98, both sides, from 10 m away: z = 9.7 (0.97) aims, z = 9.9 (0.99) does not
  int calls = 0;
  const OrientFromDir count = [&](const Flight::Vec3 &) {
    ++calls;
    return Flight::Quat{0, 0, 0, 1};
  };
  camera.position = {0, -std::sqrt(100.0f - 9.7f * 9.7f), -9.7f};
  AimAtCar(camera, {0, 0, 0}, count);
  CHECK(calls == 1);
  camera.position = {0, -std::sqrt(100.0f - 9.9f * 9.9f), -9.9f};
  AimAtCar(camera, {0, 0, 0}, count);
  CHECK(calls == 1);
  // looking down from above is the same as up
  camera.position = {0, -std::sqrt(100.0f - 9.9f * 9.9f), 9.9f};
  AimAtCar(camera, {0, 0, 0}, count);
  CHECK(calls == 1);
  camera.position = {0, -std::sqrt(100.0f - 9.7f * 9.7f), 9.7f};
  AimAtCar(camera, {0, 0, 0}, count);
  CHECK(calls == 2);
}

TEST(cinematic_aim_degenerate) {
  Pose camera;
  camera.position = {0.005f, 0, 0};
  camera.orientation = {0.1f, 0.2f, 0.3f, 0.4f};
  int calls = 0;
  const OrientFromDir count = [&](const Flight::Vec3 &) {
    ++calls;
    return Flight::Quat{0, 0, 0, 1};
  };
  Pose out = AimAtCar(camera, {0, 0, 0}, count);
  CHECK(out.position.x == 0.005f && out.position.y == 0.0f && out.position.z == 0.0f);
  CHECK(out.orientation.i == 0.1f && out.orientation.j == 0.2f && out.orientation.k == 0.3f &&
        out.orientation.r == 0.4f);
  CHECK(calls == 0);

  // 0.02 m is past the guard: pushed out to 4.5 m and aimed
  camera.position = {0.02f, 0, 0};
  out = AimAtCar(camera, {0, 0, 0}, count);
  CheckVec(out.position, 4.5, 0, 0, 1e-4);
  CHECK(calls == 1);
}

TEST(cinematic_aim_uses_orient) {
  Pose camera;
  camera.position = {0, -2, 0};
  Flight::Vec3 seen{};
  int calls = 0;
  const OrientFromDir stub = [&](const Flight::Vec3 &dir) {
    seen = dir;
    ++calls;
    return Flight::Quat{0.1f, 0.2f, 0.3f, 0.9f};
  };
  const Pose out = AimAtCar(camera, {0, 0, 0}, stub);
  CHECK(calls == 1);
  // the line to the car from where the camera ends up
  CheckVec(seen, 0, 4.5, 0, 1e-5);
  CHECK(out.orientation.i == 0.1f && out.orientation.j == 0.2f && out.orientation.k == 0.3f &&
        out.orientation.r == 0.9f);
  CheckVec(out.position, 0, -4.5, 0, 1e-5);
}

TEST(cinematic_distance_stop) {
  CHECK_NEAR(StopLimit(30.0f, 10.0f), 30.0, 1e-6); // 12.5 under the stop's own 30
  CHECK_NEAR(StopLimit(30.0f, 24.0f), 30.0, 1e-6); // the two meet
  CHECK_NEAR(StopLimit(30.0f, 40.0f), 50.0, 1e-5); // 1.25 times the start
  CHECK(!DistanceStop({0, 0, 0}, {0, -49, 0}, 30.0f, 40.0f));
  CHECK(DistanceStop({0, 0, 0}, {0, -50, 0}, 30.0f, 40.0f));
  CHECK(DistanceStop({0, 0, 0}, {0, -51, 0}, 30.0f, 40.0f));
  // the stop's own limit when the camera started close
  CHECK(!DistanceStop({0, 0, 0}, {0, -29, 0}, 30.0f, 10.0f));
  CHECK(DistanceStop({0, 0, 0}, {0, -31, 0}, 30.0f, 10.0f));
  // the distance is from the car, any direction
  CHECK(DistanceStop({10, 10, 10}, {10, 10, 45}, 30.0f, 10.0f));
}

TEST(cinematic_level_heading_ignores_bank_and_pitch) {
  // yawed 90 degrees (forward -X), then banked 50 degrees about forward and pitched 20 degrees up: still -X
  const Flight::Quat bank{0.0f, std::sin(0.4363f), 0.0f, std::cos(0.4363f)};
  const Flight::Quat pitch{std::sin(0.1745f), 0.0f, 0.0f, std::cos(0.1745f)};
  const Flight::Quat car = Flight::Mul(kYaw90, Flight::Mul(pitch, bank));
  const Flight::Vec3 h = LevelHeading({0, 0, 0}, car, {0, 0, 0}, 0.0f);
  CheckVec(h, -1, 0, 0, 1e-4);
}

TEST(cinematic_level_heading_eases) {
  // from +Y towards -X: one time constant covers 1 - 1/e of the way (before the length is made 1 again)
  const Flight::Vec3 h = LevelHeading({0, 1, 0}, kYaw90, {0, 0, 0}, kHeadingTime);
  const float k = 1.0f - std::exp(-1.0f);
  const Flight::Vec3 raw{-k, 1.0f - k, 0.0f};
  const Flight::Vec3 n = Flight::Mul(raw, 1.0f / Flight::Length(raw));
  CheckVec(h, n.x, n.y, 0, 1e-4);
  // no time: the heading held
  CheckVec(LevelHeading({0, 1, 0}, kYaw90, {0, 0, 0}, 0.0f), 0, 1, 0, 1e-6);
  // a long time: there
  CheckVec(LevelHeading({0, 1, 0}, kYaw90, {0, 0, 0}, 100.0f), -1, 0, 0, 1e-4);
}

TEST(cinematic_level_heading_nose_up) {
  // pitched straight up: the velocity's level part leads; with none, the heading held
  const Flight::Quat up{0.7071068f, 0.0f, 0.0f, 0.7071068f};
  CheckVec(LevelHeading({0, 0, 0}, up, {5, 0, 30}, 0.0f), 1, 0, 0, 1e-5);
  CheckVec(LevelHeading({0, 1, 0}, up, {0, 0, 30}, 0.1f), 0, 1, 0, 1e-6);
}

TEST(cinematic_aim_point_leads) {
  // the car 20 m ahead moving right at full lead speed: the aim is 20 tan(kLeadAngle) to its right
  Flight::Vec3 aim = AimPoint({0, 0, 0}, {0, 20, 0}, {kLeadSpeed, 0, 0});
  CheckVec(aim, 20.0 * std::tan(kLeadAngle), 20, 0, 1e-4);
  // half the speed across: half the angle
  aim = AimPoint({0, 0, 0}, {0, 20, 0}, {kLeadSpeed / 2, 0, 0});
  CheckVec(aim, 20.0 * std::tan(kLeadAngle / 2), 20, 0, 1e-4);
  // moving straight away or still: no lead
  CheckVec(AimPoint({0, 0, 0}, {0, 20, 0}, {0, 50, 0}), 0, 20, 0, 1e-5);
  CheckVec(AimPoint({0, 0, 0}, {0, 20, 0}, {0, 0, 0}), 0, 20, 0, 1e-5);
  // only the part across the line counts, capped at full lead
  aim = AimPoint({0, 0, 0}, {0, 20, 0}, {40, 40, 0});
  CheckVec(aim, 20.0 * std::tan(kLeadAngle), 20, 0, 1e-4);
}

TEST(cinematic_aim_lead_pushes_from_centre) {
  // 2 m from the centre: out to kMinDistance along the line to the centre, then turned to the aim
  Pose camera;
  camera.position = {0, -2, 0};
  const Pose out = AimAtCarLead(camera, {0, 0, 0}, {1, 0, 0});
  CheckVec(out.position, 0, -4.5, 0, 1e-5);
  const Flight::Vec3 look = Flight::Normalize({1, 4.5f, 0});
  CheckVec(Axis(out.orientation, 1), look.x, look.y, look.z, 1e-5);
}

namespace {
bool Never(const Flight::Vec3 &) { return false; }
} // namespace

TEST(cinematic_flyby_slow_car) {
  // 10 m/s north from the origin, a shot offset 3 m right and 2 m up: the car passes the camera at 3 s, the speed over
  // kFlybyMaxPan out (more than the offset's 3), the camera still
  const Flyby f = PlanFlyby(StraightAhead({0, 0, 0}, {0, 10, 0}), {3, 28, 2}, 10.0f, 1, Never);
  CHECK(f.ok);
  CHECK(f.side == 1);
  CHECK_NEAR(f.pass, kFlybyPassTime, 1e-6);
  CheckVec(f.velocity, 0, 0, 0, 1e-6);
  CheckVec(f.start, 10.0f / kFlybyMaxPan, 30, 2, 1e-4);
}

TEST(cinematic_flyby_fast_car_drifts) {
  // 60 m/s east: the camera drifts east at 45 m/s, to the car's right (south), and the car reaches it at 3 s
  const Flyby f = PlanFlyby(StraightAhead({0, 0, 0}, {60, 0, 0}), {-2, 28, 5}, 60.0f, 1, Never);
  CHECK(f.ok);
  CheckVec(f.velocity, 60 - kFlybyRelSpeed, 0, 0, 1e-3);
  const Flight::Vec3 atPass = FlybyAt(f, f.pass);
  CheckVec(atPass, 180, -kFlybyRelSpeed / kFlybyMaxPan, 5, 1e-3);
  // the car's relative speed past it is kFlybyRelSpeed
  CheckVec(f.start, 180 - 45 * 3, -kFlybyRelSpeed / kFlybyMaxPan, 5, 1e-3);
}

TEST(cinematic_flyby_left_and_wide) {
  // left of the travel, and the offset's 20 m wider than the pan needs
  const Flyby f = PlanFlyby(StraightAhead({0, 0, 0}, {0, 10, 0}), {20, 28, 0}, 10.0f, -1, Never);
  CHECK(f.ok);
  CHECK(f.side == -1);
  CheckVec(FlybyAt(f, f.pass), -20, 30, 0, 1e-4);
}

TEST(cinematic_flyby_tries_other_spots) {
  // a wall east of x = 5: the right side is blocked, raised or not; the left is clear
  auto wall = [](const Flight::Vec3 &p) { return p.x > 5.0f; };
  Flyby f = PlanFlyby(StraightAhead({0, 0, 0}, {0, 10, 0}), {3, 28, 0}, 10.0f, 1, wall);
  CHECK(f.ok);
  CHECK(f.side == -1);
  // blocked east of x = 5 under 2 m up: the raised try (kFlybyRaise) on the right is clear, its sight lines too
  auto low = [](const Flight::Vec3 &p) { return p.x > 5.0f && p.z < 2.0f; };
  f = PlanFlyby(StraightAhead({0, 0, 0}, {0, 10, 0}), {3, 28, 0}, 10.0f, 1, low);
  CHECK(f.ok);
  CHECK(f.side == 1);
  CHECK_NEAR(f.start.z, kFlybyRaise, 1e-5);
  // everything blocked: no flyby
  f = PlanFlyby(StraightAhead({0, 0, 0}, {0, 10, 0}), {3, 28, 0}, 10.0f, 1, [](const Flight::Vec3 &) { return true; });
  CHECK(!f.ok);
}

TEST(cinematic_flyby_sight_line) {
  // a pillar between the camera's right-side spot and the car's path (x 3.5 to 6.5 by y 25 to 35): the right side
  // fails on its view of the car, the left is taken
  auto pillar = [](const Flight::Vec3 &p) { return p.x > 3.5f && p.x < 6.5f && p.y > 25.0f && p.y < 35.0f; };
  const Flyby f = PlanFlyby(StraightAhead({0, 0, 0}, {0, 10, 0}), {3, 28, 0}, 10.0f, 1, pillar);
  CHECK(f.ok);
  CHECK(f.side == -1);
}

TEST(cinematic_flyby_short_route) {
  // a route that ends 22 m ahead at 10 m/s: the pass steps back from 3 s by 0.5 s until it is 5 m short of the end
  Ahead a = StraightAhead({0, 0, 0}, {0, 10, 0});
  a.end = 22.0f;
  Flyby f = PlanFlyby(a, {3, 28, 0}, 10.0f, 1, Never);
  CHECK(f.ok);
  CHECK_NEAR(f.pass, 1.5, 1e-6);
  // 12 m: under kFlybyMinPassTime of route left
  a.end = 12.0f;
  f = PlanFlyby(a, {3, 28, 0}, 10.0f, 1, Never);
  CHECK(!f.ok);
}

TEST(cinematic_fit_distance) {
  // a 2.26 x 5.84 x 1.28 m box, 40 degrees vertical: tan 20 = 0.36397
  const Flight::Vec3 half{1.13f, 2.92f, 0.64f};
  const float tv = std::tan(20.0f * 3.14159265f / 180.0f);
  // seen from ahead: its height decides (the nearest top corners, 2.92 m nearer than the centre)
  CHECK_NEAR(FitDistance({0, 3, 0}, {0, 0, 0}, {0, 0, 0, 1}, half, tv), 2.92 + 0.64 / (kFillHeight * tv), 1e-3);
  // banked on its side: the width stands up, so further out
  const Flight::Quat side{0.0f, 0.7071068f, 0.0f, 0.7071068f};
  CHECK_NEAR(FitDistance({0, 3, 0}, {0, 0, 0}, side, half, tv), 2.92 + 1.13 / (kFillHeight * tv), 1e-3);
  // seen from the side: its length across the frame decides
  CHECK_NEAR(FitDistance({3, 0, 0}, {0, 0, 0}, {0, 0, 0, 1}, half, tv),
             1.13 + 2.92 / (kFillWidth * tv * kAspect), 1e-3);
  // a small box fits at the least distance; no box or a vertical line: the least distance
  CHECK_NEAR(FitDistance({0, 3, 0}, {0, 0, 0}, {0, 0, 0, 1}, {0.1f, 0.1f, 0.1f}, tv), kMinDistance, 1e-6);
  CHECK_NEAR(FitDistance({0, 3, 0}, {0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0}, tv), kMinDistance, 1e-6);
  CHECK_NEAR(FitDistance({0, 0, 3}, {0, 0, 0}, {0, 0, 0, 1}, half, tv), kMinDistance, 1e-6);
}

TEST(cinematic_flyby_closer_pass) {
  // beside the route 28 to 32 m ahead is blocked on both sides: the 3 s pass (30 m) has no spot, the 2 s one (20 m) has
  auto beside = [](const Flight::Vec3 &p) { return p.y > 28.0f && p.y < 32.0f && std::fabs(p.x) > 4.0f; };
  const Flyby f = PlanFlyby(StraightAhead({0, 0, 0}, {0, 10, 0}), {3, 28, 0}, 10.0f, 1, beside);
  CHECK(f.ok);
  CHECK_NEAR(f.pass, kFlybyPassTime - kFlybyCloser, 1e-6);
  CheckVec(f.start, 10.0f / kFlybyMaxPan, 20, 0, 1e-4);
}

TEST(cinematic_fit_distance_any_bank) {
  const Flight::Vec3 half{1.13f, 2.92f, 0.64f};
  const float tv = std::tan(20.0f * 3.14159265f / 180.0f);
  const float round = std::sqrt(1.13f * 1.13f + 0.64f * 0.64f);
  // seen from ahead: the cross-section's radius stands up, whatever the bank
  const float level = FitDistanceAnyBank({0, 3, 0}, {0, 0, 0}, {0, 0, 0, 1}, half, tv);
  CHECK_NEAR(level, 2.92 + round / (kFillHeight * tv), 1e-3);
  const Flight::Quat side{0.0f, 0.7071068f, 0.0f, 0.7071068f};
  CHECK_NEAR(FitDistanceAnyBank({0, 3, 0}, {0, 0, 0}, side, half, tv), level, 1e-3);
  const Flight::Quat bank30{0.0f, std::sin(0.2618f), 0.0f, std::cos(0.2618f)};
  CHECK_NEAR(FitDistanceAnyBank({0, 3, 0}, {0, 0, 0}, bank30, half, tv), level, 1e-3);
  // the heading still counts: yawed 90 degrees, the camera ahead of the old nose sees its side
  CHECK_NEAR(FitDistanceAnyBank({0, 3, 0}, {0, 0, 0}, kYaw90, half, tv),
             round + 2.92 / (kFillWidth * tv * kAspect), 1e-3);
}

TEST(cinematic_ease_distance) {
  CHECK_NEAR(EaseDistance(0.0f, 9.0f, 0.1f), 9.0, 1e-6); // the first frame takes the target
  CHECK_NEAR(EaseDistance(8.0f, 8.0f, 0.1f), 8.0, 1e-6);
  // out over one kFitOutTime: 1 - 1/e of the way; in over one kFitInTime the same, so in is slower
  CHECK_NEAR(EaseDistance(8.0f, 10.0f, kFitOutTime), 8.0 + 2.0 * (1.0 - std::exp(-1.0)), 1e-4);
  CHECK_NEAR(EaseDistance(10.0f, 8.0f, kFitInTime), 10.0 - 2.0 * (1.0 - std::exp(-1.0)), 1e-4);
  CHECK(EaseDistance(10.0f, 8.0f, 0.1f) > 10.0f - (EaseDistance(8.0f, 10.0f, 0.1f) - 8.0f));
  CHECK_NEAR(EaseDistance(8.0f, 10.0f, 0.0f), 8.0, 1e-6);
}
