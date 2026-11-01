// The follower (src/red4ext/Navigation/Tracker): Track and Command.
#include "check.hpp"

#include "box_space.hpp"
#include "Navigation/Tracker.hpp"
#include "Navigation/Vec3Ops.hpp"

#include <cmath>

using namespace Navigation;

namespace {
constexpr float kDt = 1.0f / 60.0f;
constexpr double kPos = 1e-4, kAcc = 1e-3;

// points every 5 m from (0,0,0) to (1000,0,0)
Route Straight() {
  std::vector<Vec3> pts;
  for (int i = 0; i <= 200; ++i) {
    pts.push_back({5.0f * float(i), 0, 0});
  }
  return Route::Make(pts);
}

// two samples, s 0 and 1000, that speed and along, limit Top
Trajectory Plan(float aSpeed, float aAlong) {
  Trajectory t;
  TrajectorySample a;
  a.speed = aSpeed;
  a.along = aAlong;
  a.limit = TrajectorySample::Limit::Top;
  TrajectorySample b = a;
  b.s = 1000.0f;
  t.samples = {a, b};
  return t;
}

void CheckVec(const Vec3 &aA, float aX, float aY, float aZ, double aTol) {
  CHECK_NEAR(aA.x, aX, aTol);
  CHECK_NEAR(aA.y, aY, aTol);
  CHECK_NEAR(aA.z, aZ, aTol);
}

// One car on one route: Track, then Command as the script's following branch calls them.
struct Rig {
  Route route = Straight();
  Trajectory traj;
  bool hasTraj = false;
  BoxSpace space;
  VehicleSpec spec;
  TrackerParams params;
  TrackerState state;
  bool routeLine = true, recovering = false, following = true, surgeFree = false;
  Vec3 forward{0.0f, 1.0f, 0.0f};
  float dt = kDt;
  TrackOutput track;
  CommandOutput command;

  Rig() { params.raceJerk = 1e6f; }
  void Plan(float aSpeed, float aAlong) {
    traj = ::Plan(aSpeed, aAlong);
    hasTraj = true;
  }
  const Trajectory *T() const { return hasTraj ? &traj : nullptr; }

  void Run(const Vec3 &aP, const Vec3 &aV) {
    RunTrack(aP, aV);
    RunCommand(aP, aV);
  }
  void RunTrack(const Vec3 &aP, const Vec3 &aV) {
    TrackInput in;
    in.dt = dt;
    in.position = aP;
    in.velocity = aV;
    in.speed = V::Len(aV);
    in.routeLine = routeLine;
    in.planned = hasTraj;
    in.recovering = recovering;
    track = Track(state, params, spec, in, route, T(), space);
  }
  void RunCommand(const Vec3 &aP, const Vec3 &aV) {
    CommandInput in;
    in.dt = dt;
    in.position = aP;
    in.velocity = aV;
    in.speed = V::Len(aV);
    in.following = following;
    in.forward = forward;
    in.routeLine = routeLine;
    in.planned = hasTraj;
    in.recovering = recovering;
    in.target = track.target;
    in.surgeFree = surgeFree;
    command = Command(state, params, in, route, T());
  }
};
} // namespace

TEST(tracker_progress) {
  Rig r;
  r.state.segment = 18;
  r.RunTrack({100, 3, 0}, {0, 0, 0});
  CHECK(r.state.segment == 19);
  CheckVec(r.state.closestNow, 100, 0, 0, kPos);
  CHECK_NEAR(r.state.closestS, 100, kPos);
  CHECK_NEAR(r.state.offRouteNow, 3, kPos);
  CHECK_NEAR(r.state.remaining, 900, kPos);
}

TEST(tracker_replan_flags) {
  Rig r;
  r.RunTrack({100, 9, 0}, {0, 0, 0});
  CHECK(r.track.replanLine);
  r.RunTrack({100, 7, 0}, {0, 0, 0});
  CHECK(!r.track.replanLine);
  // the timer is a member of the input only; set it through a direct call
  TrackInput in;
  in.dt = kDt;
  in.position = {100, 9, 0};
  in.replanTimer = 2.5f;
  CHECK(!Track(r.state, r.params, r.spec, in, r.route, nullptr, r.space).replanLine);
  in.replanTimer = 0;
  in.recovering = true;
  CHECK(!Track(r.state, r.params, r.spec, in, r.route, nullptr, r.space).replanLine);

  // a request already out holds the replan back
  in.replanTimer = 0;
  in.recovering = false;
  in.requestPending = true;
  CHECK(!Track(r.state, r.params, r.spec, in, r.route, nullptr, r.space).replanLine);
  in.requestPending = false;
  CHECK(Track(r.state, r.params, r.spec, in, r.route, nullptr, r.space).replanLine);

  in.routeLine = false;
  in.position = {100, 31, 0};
  CHECK(Track(r.state, r.params, r.spec, in, r.route, nullptr, r.space).requestRoute);
  in.requestPending = true;
  CHECK(!Track(r.state, r.params, r.spec, in, r.route, nullptr, r.space).requestRoute);
}

TEST(tracker_line_feed_forward) {
  Rig r;
  r.params.raceJerk = 60.0f;
  r.Plan(20, 2);
  r.Run({100, 0, 0}, {20, 0, 0});
  CheckVec(r.command.accel, 1, 0, 0, kAcc);

  Rig q;
  q.params.raceJerk = 60.0f;
  q.Plan(20, 2);
  q.state.accelCmd = {2, 0, 0};
  q.Run({100, 0, 0}, {20, 0, 0});
  CheckVec(q.command.accel, 2, 0, 0, kAcc);
}

TEST(tracker_line_spring) {
  Rig r;
  r.Plan(20, 0);
  r.Run({100, 2, 0}, {20, 0, 0});
  CheckVec(r.command.accel, 0, -5, 0, kAcc);
}

TEST(tracker_correction_capped) {
  Rig r;
  r.Plan(40, 0);
  r.Run({100, 0, 0}, {20, 0, 0});
  CheckVec(r.command.accel, 25, 0, 0, kAcc);
}

TEST(tracker_bend_speed) {
  // a circle of radius 100 about the origin, a point every degree
  std::vector<Vec3> pts;
  for (int d = 0; d <= 90; ++d) {
    const float a = float(d) * 3.14159265f / 180.0f;
    pts.push_back({100.0f * std::cos(a), 100.0f * std::sin(a), 0});
  }
  const float c = std::cos(0.78539816f), s = std::sin(0.78539816f);
  const Vec3 p = pts[45];
  const Vec3 tangent = {-s, c, 0};
  // the bend is taken 5 m on, at 45 degrees + 0.05 rad
  const float phi = 0.78539816f + 0.05f;
  const Vec3 inward = {-std::cos(phi), -std::sin(phi), 0};
  const Vec3 along = {-std::sin(phi), std::cos(phi), 0};

  Rig r;
  r.route = Route::Make(pts);
  r.Plan(20, 0);
  r.state.segment = 43;
  r.Run(p, V::Mul(tangent, 20.0f));
  // The line's tangent at the car is the car's own direction, so the velocity error is along it only. The
  // bend is 1/R = 0.01 long, so at the planned speed the bend term is 20^2 / 100 = 4 m/s2 inwards.
  CHECK_NEAR(V::Dot(r.command.accel, inward), 4.0, 0.05);
  CHECK_NEAR(V::Dot(r.command.accel, along), 0.0, 0.05);

  // Faster than planned (v 25, plan 20): bendSpeed = min(25, 20) = 20, so the bend term is still 4.
  // The error is 20 - 25 = -5 along the tangent, times the gain 2.5: -12.5 along it. The tangent at 45 degrees
  // is turned 0.05 rad from the frame (phi = 45 degrees + 0.05 rad), so that part has an inward share
  // -12.5 * sin(45 degrees - phi) = +0.62: 4.62 inwards.
  Rig q;
  q.route = Route::Make(pts);
  q.Plan(20, 0);
  q.state.segment = 43;
  q.Run(p, V::Mul(tangent, 25.0f));
  CHECK_NEAR(V::Dot(q.command.accel, inward), 4.62, 0.05);
  CHECK_NEAR(V::Dot(q.command.accel, along), -12.5, 0.05);

  // Slower than planned (v 15, plan 20): bendSpeed = min(15, 20) = 15, the bend term 15^2 / 100 = 2.25. The
  // error is 20 - 15 = +5 along the tangent, times 2.5: +12.5, with the inward share -12.5 * 0.04998 = -0.62
  // (the bend point is 15 * 0.25 = 3.75 m on, 0.0125 rad short of the frame, which leaves 2.25). The plan's
  // own speed (20^2 / 100 = 4) would give 4 - 0.62 = 3.38 instead of 1.63.
  Rig w;
  w.route = Route::Make(pts);
  w.Plan(20, 0);
  w.state.segment = 43;
  w.Run(p, V::Mul(tangent, 15.0f));
  CHECK_NEAR(V::Dot(w.command.accel, inward), 1.63, 0.05);
  CHECK_NEAR(V::Dot(w.command.accel, along), 12.5, 0.05);

  // No trajectory: the bend is taken at speedCmd, whatever the car flies. speedCmd 30 (set after Track, which
  // gives an unplanned follower 0), v 20, rate 0: the bend term 30^2 / 100 = 9, the error (30 - 20) * 2.5 = 25
  // along the tangent with the inward share -25 * 0.04998 = -1.25: 7.75 inwards (min(20, 30) would give 2.75).
  Rig u;
  u.route = Route::Make(pts);
  u.state.segment = 43;
  u.RunTrack(p, V::Mul(tangent, 20.0f));
  u.state.speedCmd = 30.0f;
  u.state.speedRate = 0.0f;
  u.RunCommand(p, V::Mul(tangent, 20.0f));
  CHECK_NEAR(V::Dot(u.command.accel, inward), 7.75, 0.05);
}

TEST(tracker_aim_pulled_back) {
  Rig r;
  r.route = Route::Make({{0, 0, 0}, {100, 0, 0}, {100, 100, 0}});
  r.routeLine = false;
  r.space.boxes.push_back({{80, 1, -10}, {99, 20, 10}});
  r.Plan(60, 0);
  r.state.aheadSmooth = 40;
  r.RunTrack({60, 0, 0}, {60, 0, 0});
  CHECK_NEAR(r.state.aheadSmooth, 30, kPos);
  CheckVec(r.track.target, 90, 0, 0, kPos);

  Rig q;
  q.route = r.route;
  q.routeLine = false;
  q.space = r.space;
  q.Plan(60, 0);
  q.state.aheadSmooth = 15;
  q.RunTrack({60, 0, 0}, {60, 0, 0});
  CHECK_NEAR(q.state.aheadSmooth, 15.416667, kPos);
  CheckVec(q.track.target, 75.416667f, 0, 0, kPos);
}

TEST(tracker_aim_feed_forward) {
  Rig r;
  r.route = Route::Make({{0, 0, 0}, {500, 0, 0}});
  r.routeLine = false;
  r.Plan(20, -3);
  r.Run({100, 0, 0}, {20, 0, 0});
  CheckVec(r.command.accel, -3, 0, 0, kAcc);
}

namespace {
void BlockedRig(Rig &aRig) {
  aRig.route = Route::Make({{0, 0, 0}, {500, 0, 0}});
  aRig.routeLine = false;
  aRig.space.boxes.push_back({{-1e6f, 30, -1e6f}, {1e6f, 1e6f, 1e6f}});
  aRig.Plan(40, 0);
}
} // namespace

TEST(tracker_blocked_brakes) {
  Rig r;
  BlockedRig(r);
  r.RunTrack({100, 0, 0}, {0, 40, 0});
  CHECK(r.state.blockedAhead);
  CHECK(!r.state.emergencyBrake);
  CHECK_NEAR(r.state.targetSpeed, 25.248762, kAcc);
  CHECK_NEAR(r.state.speedCmd, 25.248762, kAcc);
  CHECK_NEAR(r.state.speedRate, -29.502475, kAcc);
  CHECK_NEAR(r.state.blockedTimer, 1.0 / 60.0, 1e-6);

  Rig q;
  BlockedRig(q);
  q.state.blockedTimer = 0.3f;
  q.RunTrack({100, 0, 0}, {0, 40, 0});
  CHECK(q.state.emergencyBrake);
  CHECK_NEAR(q.state.speedCmd, 25.248762, kAcc);
  CHECK_NEAR(q.state.speedRate, -29.502475, kAcc);

  // on its line, close to it, the prediction is trusted to the planner
  Rig l;
  BlockedRig(l);
  l.routeLine = true;
  l.RunTrack({100, 0, 0}, {0, 40, 0});
  CHECK(!l.state.blockedAhead);
}

TEST(tracker_blocked_clear_again) {
  Rig r;
  r.Plan(20, -2);
  r.state.blockedTimer = 0.3f;
  r.RunTrack({100, 0, 0}, {10, 0, 0});
  CHECK_NEAR(r.state.speedCmd, 20, kAcc);
  CHECK_NEAR(r.state.speedRate, 0, kAcc);
  CHECK_NEAR(r.state.blockedTimer, 0, 1e-6);

  // faster than the plan: max(speedCmd, speed) keeps the car's speed, 30 against the plan's 20
  Rig f;
  f.Plan(20, -2);
  f.state.blockedTimer = 0.3f;
  f.RunTrack({100, 0, 0}, {30, 0, 0});
  CHECK_NEAR(f.state.speedCmd, 30, kAcc);
  CHECK_NEAR(f.state.speedRate, 0, kAcc);
}

// A wall at y = 35 and a car flying at it at 30 m/s: the velocity ray (three pieces of 15 m) enters it in the
// third, 5 m into it, so travelled = 35 and room = 30. The stop cap is sqrt(2 * 0.85 * Decel(0) * 30) =
// sqrt(765) = 27.659 (Decel(0) = 15). The room to stop from there at brakeAccel is 27.659^2 / 12 = 63.75, and
// 30^2 / (2 * 63.75) = 7.06 is within brakeMax 9: braked for as any limit, not an emergency.
TEST(tracker_blocked_room_to_stop) {
  Rig r;
  r.route = Route::Make({{0, 0, 0}, {500, 0, 0}});
  r.routeLine = false;
  r.space.boxes.push_back({{-1e6f, 35, -1e6f}, {1e6f, 1e6f, 1e6f}});
  r.Plan(40, 0);
  r.state.blockedTimer = 0.3f;
  r.RunTrack({100, 0, 0}, {0, 30, 0});
  CHECK(r.state.blockedAhead);
  CHECK(!r.state.emergencyBrake);
  CHECK_NEAR(r.state.blockedTimer, 0.3 + 1.0 / 60.0, 1e-5);
  CHECK_NEAR(r.state.targetSpeed, 27.658633, kAcc);
  CHECK_NEAR(r.state.speedCmd, 27.658633, kAcc);
  // the gap (30 - 27.659) closed at 2 speedGain
  CHECK_NEAR(r.state.speedRate, -4.682735, kAcc);
}

// No usable trajectory: no speed to fly, and the rate is the race decel at the car's speed.
TEST(tracker_no_trajectory) {
  Rig r;
  r.RunTrack({100, 0, 0}, {20, 0, 0});
  CHECK_NEAR(r.state.targetSpeed, 0, kAcc);
  CHECK_NEAR(r.state.speedCmd, 0, kAcc);
  CHECK_NEAR(r.state.speedRate, -Decel(r.spec, 20.0f), kAcc);
}

// One sample is no trajectory: planned is not honoured, whatever its speed.
TEST(tracker_one_sample_not_planned) {
  Rig r;
  TrajectorySample a;
  a.speed = 20.0f;
  a.along = 2.0f;
  r.traj.samples = {a};
  r.hasTraj = true;
  r.RunTrack({100, 0, 0}, {20, 0, 0});
  CHECK_NEAR(r.state.targetSpeed, 0, kAcc);
  CHECK_NEAR(r.state.speedRate, -Decel(r.spec, 20.0f), kAcc);
  r.RunCommand({100, 0, 0}, {20, 0, 0});
  CHECK(!r.state.surge.held);
}

// Where the route has no direction the line's tangent is the car's nose: desired = forward * speedCmd
// = (0, 12, 16) with speedCmd 20, error from rest times lineVelocityGain 2.5 = (0, 30, 40).
TEST(tracker_tangent_fallback) {
  Rig r;
  r.route = Route();
  r.forward = {0.0f, 0.6f, 0.8f};
  r.state.speedCmd = 20.0f;
  r.RunCommand({0, 0, 0}, {0, 0, 0});
  CheckVec(r.command.desiredVelocity, 0, 12, 16, kAcc);
  CheckVec(r.command.accel, 0, 30, 40, kAcc);
}

// Aim mode feeds the bend's acceleration forward: pathCurvature * speedCmd^2. The route runs 6.667 m along x
// and then up y. At 20 m/s the lookahead is 26 m, but the corner pulls it to 6.667 + 10 = 16.667 (taken at
// once, aheadSmooth starts above it). The route points at 0.1, 0.4 and 0.7 of that are (1.667, 0), (6.667, 0)
// (the corner) and (6.667, 5): legs of 5 m, so the bend is ((0,1) - (1,0)) * 2 / 10 = (-0.2, 0.2). It is
// smoothed in at dt / 0.25 = 1/15 a frame: after one frame (-0.2, 0.2) / 15, after two it has taken
// 1 - (14/15)^2 = 0.128889 of it. The car flies exactly at the aim point's speed, so the velocity error is
// nil and the acceleration is pathCurvature * 20^2 (the plan's rate is 0).
TEST(tracker_aim_curvature_feed_forward) {
  Rig r;
  r.route = Route::Make({{0, 0, 0}, {6.666667f, 0, 0}, {6.666667f, 100, 0}});
  r.routeLine = false;
  r.Plan(20, 0);
  r.state.aheadSmooth = 40.0f;
  const Vec3 aim = {6.666667f, 10.0f, 0.0f}; // 16.667 m on along the route
  const Vec3 v = V::Mul(V::Normalize(aim), 20.0f);
  r.Run({0, 0, 0}, v);
  CHECK_NEAR(r.state.aheadSmooth, 16.666667, kPos);
  CheckVec(r.track.target, 6.666667f, 10.0f, 0, kPos);
  CheckVec(r.state.pathCurvature, -0.2f / 15.0f, 0.2f / 15.0f, 0, 1e-5);
  CheckVec(r.command.accel, -0.2f / 15.0f * 400.0f, 0.2f / 15.0f * 400.0f, 0, kAcc);
  r.Run({0, 0, 0}, v);
  CheckVec(r.state.pathCurvature, -0.2f * 0.128889f, 0.2f * 0.128889f, 0, 1e-5);
  CheckVec(r.command.accel, -0.2f * 0.128889f * 400.0f, 0.2f * 0.128889f * 400.0f, 0, 5e-3);
}

// Not following, the script clears the emergency brake and the target speed every frame, so the jerk limit
// applies: 10 m/s2 asked, 20 m/s2/s * 0.1 s = 2 allowed, from a state that had braked hard.
TEST(tracker_rate_limit) {
  Rig r;
  r.following = false;
  r.dt = 0.1f;
  CommandInput in;
  in.dt = 0.1f;
  in.rawAccel = {10, 0, 0};
  in.rawJerk = 20;
  CheckVec(Command(r.state, r.params, in, r.route, nullptr).accel, 2, 0, 0, kAcc);

  Rig q;
  q.state.emergencyBrake = true;
  q.state.targetSpeed = 7.0f;
  CheckVec(Command(q.state, q.params, in, q.route, nullptr).accel, 2, 0, 0, kAcc);
  CHECK(!q.state.emergencyBrake);
  CHECK_NEAR(q.state.targetSpeed, 0, kAcc);
}

// A following frame whose Track set the emergency brake is not rate limited (raceJerk 60, 1 m/s2 a frame).
// The wall of tracker_blocked_brakes, blocked for 0.3 s: speedCmd = 25.248762, speedRate = -29.502475. The
// car flying (0, 40): desired = (25.248762, 0), error (25.248762, -40) * 1.2, plus the rate along the aim
// direction (+x): accel = (30.298514 - 29.502475, -48) = (0.796039, -48), taken whole. Without the brake
// (blocked for no time yet) the same frame is limited to 1 m/s2.
TEST(tracker_emergency_bypasses_limit) {
  Rig r;
  BlockedRig(r);
  r.params.raceJerk = 60.0f;
  r.state.blockedTimer = 0.3f;
  r.Run({100, 0, 0}, {0, 40, 0});
  CHECK(r.state.emergencyBrake);
  CheckVec(r.command.accel, 0.796039f, -48.0f, 0, 5e-3);

  Rig q;
  BlockedRig(q);
  q.params.raceJerk = 60.0f;
  q.Run({100, 0, 0}, {0, 40, 0});
  CHECK(!q.state.emergencyBrake);
  CHECK_NEAR(V::Len(q.command.accel), 1.0, kAcc);
}

TEST(tracker_recovering_holds) {
  Rig r;
  r.recovering = true;
  r.Plan(20, 0);
  r.Run({100, 0, 0}, {20, 0, 0});
  CheckVec(r.command.accel, 0, 0, 0, kAcc);
  CheckVec(r.command.desiredVelocity, 20, 0, 0, kAcc);
}

TEST(tracker_surge_in_command) {
  Rig r;
  r.surgeFree = true;
  r.dt = 0.125f;
  r.params.surge.dwell = 0;
  r.Plan(20, 0);
  r.Run({100, 0, 0}, {18, 0, 0});
  CHECK(r.state.surge.held);

  Rig b;
  b.surgeFree = true;
  b.dt = 0.125f;
  b.params.surge.dwell = 0;
  b.Plan(20, 0);
  b.RunTrack({100, 0, 0}, {18, 0, 0});
  b.state.blockedAhead = true;
  b.RunCommand({100, 0, 0}, {18, 0, 0});
  CHECK(!b.state.surge.held);

  Rig n;
  n.surgeFree = true;
  n.dt = 0.125f;
  n.params.surge.dwell = 0;
  n.Run({100, 0, 0}, {18, 0, 0});
  CHECK(!n.state.surge.held);
}
