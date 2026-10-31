// The drone attitude (src/red4ext/Navigation/DroneAttitude): EaseAngle, WrapAngle, the heading and the tilt.
#include "check.hpp"

#include "Navigation/DroneAttitude.hpp"
#include "Navigation/Vec3Ops.hpp"

#include <cmath>

using namespace Navigation;

namespace {
constexpr double kTol = 1e-5;

// two samples, s 0 and 1000, every field constant
Trajectory Flat(float aDrift, float aAlong, float aPitch, float aRoll) {
  Trajectory t;
  TrajectorySample a;
  a.drift = aDrift;
  a.along = aAlong;
  a.pitch = aPitch;
  a.roll = aRoll;
  TrajectorySample b = a;
  b.s = 1000.0f;
  t.samples = {a, b};
  return t;
}

// the nose's pitch below the horizon, as the tests read it
double NoseDown(const AttitudeOutput &aOut) { return std::atan2(-aOut.forward.z, aOut.forward.x); }

AttitudeInput Frame(Vec3 aAccel) {
  AttitudeInput in;
  in.dt = 1.0f / 60.0f;
  in.accel = aAccel;
  return in;
}
} // namespace

TEST(attitude_ease_angle) {
  float rate = 0.0f;
  float angle = EaseAngle(0.1f, 1.0f, 0.0f, 0.0f, 2.5f, 1.0f, 3.0f, rate);
  CHECK_NEAR(angle, 0.03, kTol);
  CHECK_NEAR(rate, 0.3, kTol);
  angle = EaseAngle(0.1f, 1.0f, 0.0f, 0.95f, 2.5f, 1.0f, 3.0f, rate);
  CHECK_NEAR(rate, 1.0, kTol); // capped
  CHECK_NEAR(angle, 0.1, kTol);
}

TEST(attitude_wrap_angle) {
  CHECK_NEAR(WrapAngle(4.0f), 4.0 - 6.2831853, kTol);
  CHECK_NEAR(WrapAngle(-4.0f), -4.0 + 6.2831853, kTol);
}

TEST(attitude_hover_level) {
  AttitudeState state;
  AttitudeOutput out = DroneAttitude(state, {}, {}, Frame({0, 0, 0}), 0.0f, nullptr, nullptr);
  CHECK_NEAR(out.up.x, 0, kTol);
  CHECK_NEAR(out.up.y, 0, kTol);
  CHECK_NEAR(out.up.z, 1, kTol);
  CHECK_NEAR(out.forward.x, 1, kTol);
  CHECK_NEAR(out.forward.y, 0, kTol);
  CHECK_NEAR(out.forward.z, 0, kTol);
}

TEST(attitude_tilt_forward) {
  // the body takes half the thrust's angle, atan2(5, 9.81); the thrusters could not take the rest
  // anyway (reach 0.85 * 15 / 11.0107 is above 1, so pi / 2), nothing is capped
  AttitudeState state;
  state.attAccel = {5, 0, 0};
  AttitudeOutput out = DroneAttitude(state, {}, {}, Frame({5, 0, 0}), 0.0f, nullptr, nullptr);
  CHECK_NEAR(NoseDown(out), 0.2356824, kTol);
  CHECK_NEAR(out.up.y, 0, kTol);
}

TEST(attitude_roll_past_sway) {
  // 12 m/s2 to the right of the heading: angle atan2(12, 9.81) = 0.8854754 and the sway (5) reaches
  // asin(0.85 * 5 / 15.4995516) = 0.2777593, so the body takes 0.8854754 - 0.2777593 = 0.6077161
  // (more than its half share)
  AttitudeState state;
  state.attAccel = {0, -12, 0};
  AttitudeOutput out = DroneAttitude(state, {}, {}, Frame({0, -12, 0}), 0.0f, nullptr, nullptr);
  CHECK_NEAR(std::asin(V::Dot(out.up, {0, -1, 0})), 0.6077161, kTol);
}

TEST(attitude_tilt_floor) {
  // the vertical -5.19 is floored to 9.81 (tiltFloor 1 x g), so the pitch is the (5, 0, 9.81) one;
  // unfloored it would be capped at 55 degrees (0.9599)
  AttitudeState state;
  state.attAccel = {5, 0, -15};
  AttitudeOutput out = DroneAttitude(state, {}, {}, Frame({5, 0, -15}), 0.0f, nullptr, nullptr);
  CHECK_NEAR(NoseDown(out), 0.2356824, kTol);
}

TEST(attitude_brake_share) {
  // drag at 20 m/s is 1.2 * 2 * 400 / 1500 = 0.64, fed forward: thrust (-9.36, 0, 9.81); braking, the
  // body takes brakeShare (0.25) of the angle: reverse surge and the brake (along 43) reach pi / 2
  AttitudeState state;
  state.attAccel = {-10, 0, 0};
  AttitudeInput in = Frame({-10, 0, 0});
  in.velocity = {20, 0, 0};
  in.speed = 20;
  AttitudeOutput out = DroneAttitude(state, {}, {}, in, 0.0f, nullptr, nullptr);
  CHECK_NEAR(NoseDown(out), -0.1904821, kTol); // nose up
}

TEST(attitude_reference_added) {
  // following past the take-off length with a flat plan: the plan's pitch 0.3 plus the tilt the
  // commanded acceleration needs less the tilt the plan's own acceleration (0) needs
  Trajectory t = Flat(0, 0, 0.3f, 0);
  AttitudeState state;
  state.attAccel = {5, 0, 0};
  AttitudeInput in = Frame({5, 0, 0});
  in.following = true;
  in.closestS = 300;
  AttitudeOutput out = DroneAttitude(state, {}, {}, in, 0.0f, nullptr, &t);
  CHECK_NEAR(NoseDown(out), 0.5356824, kTol);
}

TEST(attitude_takeoff_lean) {
  // the plan's pitch is 0 and its acceleration 0, so only the take-off lean is left (the drag terms
  // cancel between the tilt and the reference): 10 degrees times the weight w
  Trajectory t = Flat(0, 2, 0, 0);
  for (float speed : {15.0f, 40.0f}) {
    AttitudeState state;
    AttitudeInput in = Frame({0, 0, 0});
    in.following = true;
    in.closestS = 10;
    in.velocity = {speed, 0, 0};
    in.speed = speed;
    AttitudeOutput out = DroneAttitude(state, {}, {}, in, 0.0f, nullptr, &t);
    // 15 m/s: into 1, out 1, w 1. 40 m/s: out (50 - 40) / 25 = 0.4, w = 0.4^2 * (3 - 0.8) = 0.352
    double expected = speed < 20 ? 0.1745329 : 0.1745329 * 0.352;
    CHECK_NEAR(NoseDown(out), expected, kTol);
  }
}

TEST(attitude_yaw_eased) {
  AttitudeState state;
  AttitudeInput in = Frame({0, 0, 0});
  in.dt = 0.1f;
  in.live = true;
  DroneAttitude(state, {}, {}, in, 1.5707964f, nullptr, nullptr);
  CHECK_NEAR(state.droneYaw, 0.05, kTol); // acceleration 9 * pi / 2 capped at 5
  CHECK_NEAR(state.droneYawRate, 0.5, kTol);
  CHECK_NEAR(state.headingRef.x, std::cos(0.05), kTol);
  CHECK_NEAR(state.headingRef.y, std::sin(0.05), kTol);

  // -3 to 3 is the short way round, through pi: 6 less 2 pi is -0.2831853 away
  AttitudeState back;
  back.droneYaw = -3.0f;
  DroneAttitude(back, {}, {}, in, 3.0f, nullptr, nullptr);
  CHECK_NEAR(back.droneYaw, -3.0254867, kTol); // acceleration 9 * -0.2831853 = -2.5486678
}

TEST(attitude_heading_on_line_drift) {
  // (0,0,0) to (0,1000,0) every 5 m
  std::vector<Vec3> points;
  for (int i = 0; i <= 200; ++i) {
    points.push_back({0, 5.0f * i, 0});
  }
  Route route = Route::Make(points);
  Trajectory t = Flat(0.3f, 0, 0, 0);
  AttitudeState state;
  AttitudeInput in;
  in.onLine = true;
  in.closestS = 100;
  in.following = true;
  in.blend = 1;
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, &route, &t), 1.5707964 + 0.3, kTol);
  // coming down the nose keeps to the hover heading
  in.descending = true;
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, &route, &t), 0.0, kTol);
}

TEST(attitude_heading_off_line) {
  AttitudeState state;
  AttitudeInput in;
  in.blend = 1;
  in.velocity = {0, 10, 0};
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, nullptr, nullptr), 1.5707964, kTol);
  // 3 m/s is half the weight: halfway between the hover heading and the path
  in.velocity = {0, 3, 0};
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, nullptr, nullptr), 0.7853982, kTol);
}

// ---- the cases a wrong port would pass: worked by hand from FlightModeAutopilot.DroneAttitude ----
// Float error is below 1e-6 everywhere, so kTol (1e-5) holds. Defaults: heading 0 (h = (1,0,0), side = h x z =
// (0,-1,0), so -y accel rolls positive), g 9.81, body share 0.5, brake share 0.25, surge 15, sway 5, margin 0.85,
// cap 55 deg = 0.9599311, tilt time 0.2, pitch/roll easing 2.5 Hz, rate 1, accel 3.

namespace {
// Flat with the acceleration of both samples set
Trajectory FlatAccel(float aAlong, float aPitch, float aRoll, Vec3 aAccel) {
  Trajectory t = Flat(0, aAlong, aPitch, aRoll);
  for (auto &s : t.samples) {
    s.acceleration = aAccel;
  }
  return t;
}
} // namespace

TEST(attitude_tilt_smoothing) {
  // smooth = dt / 0.2 = 1/12: attAccel = (5 / 12, 0, 0) = 0.4166667. Thrust (0.4166667, 0, 9.81): angle
  // atan2(0.4166667, 9.81) = 0.0424481, body takes half = 0.0212241 (reach is pi / 2, nothing is capped)
  AttitudeState state;
  AttitudeOutput out = DroneAttitude(state, {}, {}, Frame({5, 0, 0}), 0.0f, nullptr, nullptr);
  CHECK_NEAR(state.attAccel.x, 5.0 / 12.0, kTol);
  CHECK_NEAR(state.attAccel.y, 0, kTol);
  CHECK_NEAR(state.dronePitch, 0.0212241, kTol);
  CHECK_NEAR(NoseDown(out), 0.0212241, kTol);

  // dt 0.5 > droneTiltTime: the fraction clamps to 1 (unclamped it would be 2.5 and overshoot to 12.5), so
  // attAccel = accel and the pitch is the unsmoothed 0.5 * atan2(5, 9.81) = 0.2356824
  AttitudeState big;
  AttitudeInput in = Frame({5, 0, 0});
  in.dt = 0.5f;
  out = DroneAttitude(big, {}, {}, in, 0.0f, nullptr, nullptr);
  CHECK_NEAR(big.attAccel.x, 5, kTol);
  CHECK_NEAR(NoseDown(out), 0.2356824, kTol);
}

TEST(attitude_following_terms) {
  // following past the take-off length (closestS 300) at hover speed (no drag); attAccel and attRef start at 0
  // and are smoothed by 1/12: accel (4, -6, 0) -> attAccel (1/3, -1/2, 0); the plan's acceleration (2, 1, 0)
  // -> attRef (1/6, 1/12, 0). Own tilt: pitch 0.0169829, roll +0.0254622 (-y thrust is +side). Reference tilt:
  // pitch 0.0084945 and roll -0.0042471 (its +y is -side). So
  //   pitch = plan.pitch 0.1 + own - ref = 0.1084890
  //   roll  = flip (1: side is always the heading turned right) * plan.roll 0.2 + 0.0254622 + 0.0042471 = 0.2297094
  Trajectory t = FlatAccel(0, 0.1f, 0.2f, {2, 1, 0});
  AttitudeState state;
  AttitudeInput in = Frame({4, -6, 0});
  in.following = true;
  in.closestS = 300;
  DroneAttitude(state, {}, {}, in, 0.0f, nullptr, &t);
  CHECK_NEAR(state.attRef.x, 1.0 / 6.0, kTol); // the smoothing, not the plan's acceleration itself
  CHECK_NEAR(state.attRef.y, 1.0 / 12.0, kTol);
  CHECK_NEAR(state.dronePitch, 0.1084890, kTol);
  CHECK_NEAR(state.droneRoll, 0.2297094, kTol);
}

TEST(attitude_takeoff_guards) {
  // at 15 m/s the lean's weight is 1 (10 degrees = 0.1745329), so only the guards keep it off. The plan's pitch
  // and acceleration are 0 and the drag terms cancel, so the pitch is 0 without the lean
  auto pitchFor = [](float aAlong, float aClosestS) {
    Trajectory t = Flat(0, aAlong, 0, 0);
    AttitudeState state;
    AttitudeInput in = Frame({0, 0, 0});
    in.following = true;
    in.closestS = aClosestS;
    in.velocity = {15, 0, 0};
    in.speed = 15;
    return NoseDown(DroneAttitude(state, {}, {}, in, 0.0f, nullptr, &t));
  };
  CHECK_NEAR(pitchFor(2, 10), 0.1745329, kTol); // control: both hold
  CHECK_NEAR(pitchFor(2, 300), 0.0, kTol);      // closestS >= 250, along > 0
  CHECK_NEAR(pitchFor(2, 250), 0.0, kTol);      // the guard is closestS < length, so 250 itself is out
  CHECK_NEAR(pitchFor(0, 10), 0.0, kTol);       // closestS < 250, along 0
  CHECK_NEAR(pitchFor(-3, 10), 0.0, kTol);      // along < 0
}

TEST(attitude_tilt_cap) {
  // 50 m/s2 forward: angle atan2(50, 9.81) = 1.3772, thrusters reach asin(0.85 * 15 / 50.95) = 0.2527, the
  // rest 0.6886 is past it, so the body takes 1.3772 - 0.2527 = 1.1245, over the cap: 55 deg = 0.9599311.
  // Braking the same way (reverse surge and the brake, 15 at rest): -1.1245, capped to -0.9599311.
  // Sideways 50 (sway 5): reach 0.0834, 1.2938 capped to roll +0.9599311 (-y is +side).
  const double cap = 55.0 * 3.14159265358979 / 180.0;
  AttitudeState fwd;
  fwd.attAccel = {50, 0, 0};
  AttitudeOutput out = DroneAttitude(fwd, {}, {}, Frame({50, 0, 0}), 0.0f, nullptr, nullptr);
  CHECK_NEAR(fwd.dronePitch, cap, kTol);
  CHECK_NEAR(NoseDown(out), cap, kTol);
  AttitudeState back;
  back.attAccel = {-50, 0, 0};
  DroneAttitude(back, {}, {}, Frame({-50, 0, 0}), 0.0f, nullptr, nullptr);
  CHECK_NEAR(back.dronePitch, -cap, kTol);
  AttitudeState side;
  side.attAccel = {0, -50, 0};
  DroneAttitude(side, {}, {}, Frame({0, -50, 0}), 0.0f, nullptr, nullptr);
  CHECK_NEAR(side.droneRoll, cap, kTol);
  CHECK_NEAR(side.dronePitch, 0.0, kTol);
}

TEST(attitude_drag_feed_forward) {
  // Drag(v) = 1.2 * 2 * v^2 / 1500, applied along the velocity: v * Drag / |v|
  VehicleSpec spec;
  Vec3 d = DragFeedForward(spec, {30, 0, 0}, true); // 1.44
  CHECK_NEAR(d.x, 1.44, kTol);
  CHECK_NEAR(d.y, 0, kTol);
  d = DragFeedForward(spec, {0, -6, 8}, true); // speed 10: 0.16 along (0, -0.6, 0.8)
  CHECK_NEAR(d.x, 0, kTol);
  CHECK_NEAR(d.y, -0.096, kTol);
  CHECK_NEAR(d.z, 0.128, kTol);
  d = DragFeedForward(spec, {0.4f, 0, 0}, true); // under 0.5 m/s: none
  CHECK(d.x == 0 && d.y == 0 && d.z == 0);
  d = DragFeedForward(spec, {0.5f, 0, 0}, true); // 0.5 itself is not under: 1.2 * 2 * 0.25 / 1500 = 0.0004
  CHECK_NEAR(d.x, 0.0004, kTol);
  d = DragFeedForward(spec, {30, 0, 0}, false); // the flag off
  CHECK(d.x == 0 && d.y == 0 && d.z == 0);
  spec.drag = false; // the car's drag off: Drag is 0
  d = DragFeedForward(spec, {30, 0, 0}, true);
  CHECK(d.x == 0 && d.y == 0 && d.z == 0);
}

TEST(attitude_zero_dt) {
  // dt 0: smooth 0 and every ease adds rate * 0, so a live frame leaves the state as it was (rates included, they
  // are inside their limits) except headingRef, which becomes the eased yaw's direction
  Trajectory t = Flat(0, 0, 0, 0);
  AttitudeState state;
  state.attAccel = {1, 2, 0};
  state.attRef = {1, 1, 0};
  state.droneYaw = 0.3f;
  state.droneYawRate = 0.1f;
  state.dronePitch = 0.05f;
  state.dronePitchRate = 0.02f;
  state.droneRoll = -0.04f;
  state.droneRollRate = -0.03f;
  AttitudeInput in = Frame({5, 0, 0});
  in.dt = 0.0f;
  in.live = true;
  in.following = true;
  in.closestS = 300;
  AttitudeOutput out = DroneAttitude(state, {}, {}, in, 1.0f, nullptr, &t);
  CHECK_NEAR(state.attAccel.x, 1, kTol);
  CHECK_NEAR(state.attAccel.y, 2, kTol);
  CHECK_NEAR(state.attRef.x, 1, kTol);
  CHECK_NEAR(state.attRef.y, 1, kTol);
  CHECK_NEAR(state.droneYaw, 0.3, kTol);
  CHECK_NEAR(state.droneYawRate, 0.1, kTol);
  CHECK_NEAR(state.dronePitch, 0.05, kTol);
  CHECK_NEAR(state.dronePitchRate, 0.02, kTol);
  CHECK_NEAR(state.droneRoll, -0.04, kTol);
  CHECK_NEAR(state.droneRollRate, -0.03, kTol);
  CHECK_NEAR(state.headingRef.x, std::cos(0.3), kTol);
  CHECK(std::isfinite(out.forward.x) && std::isfinite(out.forward.y) && std::isfinite(out.forward.z));
  CHECK(std::isfinite(out.up.x) && std::isfinite(out.up.y) && std::isfinite(out.up.z));

  // the first frame at dt 0: the state is set to the target (yaw 0.3; attAccel (1, 2, 0) at heading 0.3 gives
  // pitch 0.0781731 and roll -0.0815898) and the ease adds nothing
  AttitudeState first;
  first.attAccel = {1, 2, 0};
  in.live = false;
  in.following = false;
  out = DroneAttitude(first, {}, {}, in, 0.3f, nullptr, nullptr);
  CHECK_NEAR(first.droneYaw, 0.3, kTol);
  CHECK_NEAR(first.dronePitch, 0.0781731, kTol);
  CHECK_NEAR(first.droneRoll, -0.0815898, kTol);
  CHECK_NEAR(first.dronePitchRate, 0, kTol);
  CHECK(std::isfinite(out.forward.x) && std::isfinite(out.up.z));
}

TEST(attitude_heading_tangent_fallback) {
  // route (0,0,0) to (0,1000,0), span 10. Past its end both ends of the tangent are the last point, so the
  // tangent is zero (length < 0.5) and the nose's own forward (0, -1, 0) stands in: yaw -pi / 2. Control: on the
  // route the tangent gives +pi / 2.
  Route route = Route::Make({{0, 0, 0}, {0, 1000, 0}});
  AttitudeState state;
  AttitudeInput in;
  in.onLine = true;
  in.blend = 1;
  in.forward = {0, -1, 0};
  in.closestS = 5000;
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, &route, nullptr), -1.5707964, kTol);
  in.closestS = 100;
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, &route, nullptr), 1.5707964, kTol);
  // a vertical route: the tangent (0,0,1) has length 1 but no horizontal part, Flat is zero, so there is no path
  // and the nose keeps headingRef (here (0,1,0))
  Route climb = Route::Make({{0, 0, 0}, {0, 0, 1000}});
  state.headingRef = {0, 1, 0};
  in.closestS = 100;
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, &climb, nullptr), 1.5707964, kTol);
}

TEST(attitude_heading_previous) {
  // off the line, velocity (0, 3, 0): path weight 3 / 6 = 0.5. headingRef (1, 0, 0), blend 0.5:
  // h = (0.5, 0) + 0.5 * (0.5 * (0,1) + 0.5 * previous).
  AttitudeState state;
  AttitudeInput in;
  in.blend = 0.5f;
  in.velocity = {0, 3, 0};
  // first frame: previous = headingRef (1,0): h = (0.5,0) + 0.5 * (0.5, 0.5) = (0.75, 0.25), atan2 = 0.3217506
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, nullptr, nullptr), 0.3217506, kTol);
  // live, droneYaw -pi / 2: previous = (0,-1), the mix is (0,0), h = (0.5, 0): yaw 0
  state.droneYaw = -1.5707964f;
  in.live = true;
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, nullptr, nullptr), 0.0, kTol);
}

TEST(attitude_heading_lead) {
  // off the line the path is velocity + accel * attitudeLead (0.25): (0,3,0) + (2,0,0) = (2,3,0), weight
  // sqrt(13) / 6 = 0.6009252, headingRef (1,0), blend 1: h = (2/6, 3/6) + 0.3990748 * (1,0) = (0.7324081, 0.5),
  // yaw 0.5990066. Without the lead it is pi / 4 (weight 0.5).
  AttitudeState state;
  AttitudeInput in;
  in.blend = 1;
  in.velocity = {0, 3, 0};
  in.accel = {8, 0, 0};
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, nullptr, nullptr), 0.5990066, kTol);
  in.accel = {0, 0, 0};
  CHECK_NEAR(DroneHeadingYaw(state, {}, in, nullptr, nullptr), 0.7853982, kTol);
  // (On the line the weight is (|path| - 0.15) / 0.35 and path is Flat, a unit vector or zero, so it is 1 or
  // 0, never between: no test for a fractional on-line weight.)
}

TEST(attitude_not_following_ref) {
  // not following: attRef = attAccel after the smoothing: 2 + (4 - 2) / 12 = 2.1666667, whatever attRef was
  Trajectory t = Flat(0, 0, 0, 0);
  Trajectory one;
  one.samples = {t.samples[0]};
  auto run = [&](bool aFollowing, const Trajectory *aTrajectory) {
    AttitudeState state;
    state.attAccel = {2, 0, 0};
    state.attRef = {9, 9, 9};
    AttitudeInput in = Frame({4, 0, 0});
    in.following = aFollowing;
    in.closestS = 300;
    DroneAttitude(state, {}, {}, in, 0.0f, nullptr, aTrajectory);
    return state;
  };
  for (AttitudeState s : {run(false, &t), run(true, nullptr), run(true, &one)}) {
    CHECK_NEAR(s.attRef.x, 2.1666667, kTol);
    CHECK_NEAR(s.attRef.y, 0, kTol);
    CHECK_NEAR(s.attRef.z, 0, kTol);
  }
  // control: following eases it towards the plan's acceleration (0): 9 * 11 / 12 = 8.25
  AttitudeState s = run(true, &t);
  CHECK_NEAR(s.attRef.x, 8.25, kTol);
}

TEST(attitude_two_live_frames) {
  // accel (6, -8, 0) held (attAccel starts there): pitch target 0.2744602, roll target 0.3758706. Each frame is
  // EaseAngle(dt = 1/60, target, angle, rate, 2.5, 1, 3), the rate carried:
  //   pitch frame 1: accel min(6.25 * 0.2744602, 3) = 1.7153763, rate 0.0285896, angle 0.0004765
  //   pitch frame 2: accel 6.25 * (0.2744602 - 0.0004765) - 5 * 0.0285896 = 1.5694, rate 0.0547471, angle 0.0013889
  //   roll  frame 1: accel 2.3491911, rate 0.0391532, angle 0.0006526
  //   roll  frame 2: rate 0.0749756, angle 0.0019021
  AttitudeState state;
  state.attAccel = {6, -8, 0};
  AttitudeInput in = Frame({6, -8, 0});
  in.live = true;
  DroneAttitude(state, {}, {}, in, 0.0f, nullptr, nullptr);
  CHECK_NEAR(state.dronePitch, 0.0004765, kTol);
  CHECK_NEAR(state.dronePitchRate, 0.0285896, kTol);
  CHECK_NEAR(state.droneRoll, 0.0006526, kTol);
  CHECK_NEAR(state.droneRollRate, 0.0391532, kTol);
  DroneAttitude(state, {}, {}, in, 0.0f, nullptr, nullptr);
  CHECK_NEAR(state.dronePitch, 0.0013889, kTol);
  CHECK_NEAR(state.dronePitchRate, 0.0547471, kTol);
  CHECK_NEAR(state.droneRoll, 0.0019021, kTol);
  CHECK_NEAR(state.droneRollRate, 0.0749756, kTol);
}
