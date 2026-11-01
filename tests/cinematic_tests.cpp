// The cinematic camera's shot maths (src/red4ext/Camera/Cinematic): FlightCinematic.AimAtCar, the fixed shot's fit
// loop and initial scale, and the distance stop. Expected values are worked by hand from
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
  // toCar as it was before the camera moved out
  CheckVec(seen, 0, 2, 0, 1e-6);
  CHECK(out.orientation.i == 0.1f && out.orientation.j == 0.2f && out.orientation.k == 0.3f &&
        out.orientation.r == 0.9f);
  CheckVec(out.position, 0, -4.5, 0, 1e-5);
}

TEST(cinematic_initial_scale) {
  CHECK_NEAR(InitialScale(0.0f), 1.0, 1e-6);
  CHECK_NEAR(InitialScale(10.0f), 1.0, 1e-6); // 0.5 clamps up
  CHECK_NEAR(InitialScale(20.0f), 1.0, 1e-6);
  CHECK_NEAR(InitialScale(60.0f), 3.0, 1e-6);
  CHECK_NEAR(InitialScale(-60.0f), 3.0, 1e-6); // the speed's sign is dropped
  CHECK_NEAR(InitialScale(100.0f), 5.0, 1e-6);
  CHECK_NEAR(InitialScale(250.0f), 5.0, 1e-6); // 12.5 clamps down
  CHECK_NEAR(InitialScale(-250.0f), 5.0, 1e-6);
}

TEST(cinematic_fit_ahead) {
  Pose shot;
  const Flight::Vec3 offset{0, 20, 0};
  int calls = 0;

  // never blocked: one free check, the scale kept
  auto never = [&](const Flight::Vec3 &) {
    ++calls;
    return false;
  };
  CHECK_NEAR(FitAhead(shot, offset, 5.0f, never), 5.0, 1e-6);
  CHECK(calls == 1);

  // always blocked: 5, 4, 3.2, 2.56, 2.048, 1.6384, 1.31072, 1.048576 are checked, then 0.8389 clamps to 1
  calls = 0;
  auto always = [&](const Flight::Vec3 &) {
    ++calls;
    return true;
  };
  CHECK_NEAR(FitAhead(shot, offset, 5.0f, always), 1.0, 1e-6);
  CHECK(calls == 8);

  // blocked where y > 50: 100, 80, 64 and 51.2 are blocked, 40.96 is free
  calls = 0;
  auto far = [&](const Flight::Vec3 &p) {
    ++calls;
    return p.y > 50.0f;
  };
  CHECK_NEAR(FitAhead(shot, offset, 5.0f, far), 2.048, 1e-5);
  CHECK(calls == 5);

  // yawed 90 degrees: the offset's Y maps to -X
  shot.orientation = kYaw90;
  auto west = [](const Flight::Vec3 &p) { return p.x < -50.0f; };
  CHECK_NEAR(FitAhead(shot, offset, 5.0f, west), 2.048, 1e-5);

  // the shot's position counts: y = -10 + 20 scale, blocked above 50 -> 90, 70, 54 blocked, 41.2 free
  shot = Pose{};
  shot.position = {0, -10, 0};
  CHECK_NEAR(FitAhead(shot, offset, 5.0f, far), 2.56, 1e-5);

  // only Y is scaled
  Flight::Vec3 seen;
  auto record = [&](const Flight::Vec3 &p) {
    seen = p;
    return false;
  };
  FitAhead(Pose{}, {3, 20, 7}, 2.0f, record);
  CheckVec(seen, 3, 40, 7, 1e-6);

  // a start of 1 or below never checks, and is returned as it came
  calls = 0;
  CHECK_NEAR(FitAhead(Pose{}, offset, 1.0f, always), 1.0, 1e-6);
  CHECK_NEAR(FitAhead(Pose{}, offset, 0.5f, always), 0.5, 1e-6);
  CHECK(calls == 0);
}

TEST(cinematic_fit_ahead_uses_transform) {
  Pose shot;
  shot.position = {100, 0, 0};
  int transformed = 0;
  float lastY = 0.0f;
  const TransformPointFn stub = [&](const Pose &p, const Flight::Vec3 &v) {
    ++transformed;
    lastY = v.y;
    CHECK(p.position.x == 100.0f);
    return Flight::Vec3{v.y, 0, 0};
  };
  // the stub's point is (y, 0, 0): blocked where x > 50 -> y = 100, 80, 64, 51.2 blocked; 40.96 free
  auto blocked = [](const Flight::Vec3 &p) { return p.x > 50.0f; };
  CHECK_NEAR(FitAhead(shot, {0, 20, 0}, 5.0f, blocked, stub), 2.048, 1e-5);
  CHECK(transformed == 5);
  CHECK_NEAR(lastY, 40.96, 1e-3);
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
