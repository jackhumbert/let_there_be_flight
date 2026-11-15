// The follower flown offline (tools/navplan/follow.cpp): Track, Command and the drone attitude over a
// trajectory, a point mass for the car.
#include "check.hpp"

#include "../tools/navplan/follow.hpp"
#include "box_space.hpp"

#include <cmath>
#include <cstdio>

using namespace Navigation;

namespace {

void Segment(std::vector<Vec3> &aPts, const Vec3 &aFrom, const Vec3 &aTo, float aStep) {
  const float dx = aTo.x - aFrom.x, dy = aTo.y - aFrom.y;
  const int n = int(std::lround(std::sqrt(dx * dx + dy * dy) / aStep));
  for (int i = aPts.empty() ? 0 : 1; i <= n; ++i) {
    const float f = float(i) / float(n);
    aPts.push_back({aFrom.x + dx * f, aFrom.y + dy * f, aFrom.z});
  }
}

FollowParams Params() {
  FollowParams params;
  params.tracker.surge.fullSurge = false;
  return params;
}

void Print(const char *aName, const FollowResult &aR, float aRef) {
  float over = -1e9f;
  for (const auto &f : aR.frames) {
    over = std::max(over, f.speed - f.leadSpeed);
  }
  std::printf("  %s: arrived %d ended %d time %.2f ref %.2f off p50 %.3f p95 %.3f max %.3f speed err mean %.3f p95 %.3f "
              "bias %.3f max over %.2f lost %d held %d blocked %d\n",
              aName, aR.arrived ? 1 : 0, aR.ended ? 1 : 0, aR.time, aRef, aR.offP50, aR.offP95, aR.offMax,
              aR.speedErrMean, aR.speedErrP95, aR.speedBiasMean, over, aR.lost, aR.held, aR.blocked);
}

// a straight line along +x at height 10
std::vector<Vec3> Straight(float aLength) {
  std::vector<Vec3> line;
  Segment(line, {0, 0, 10}, {aLength, 0, 10}, 5.0f);
  return line;
}

// a straight, a quarter circle of aRadius to the left (+y), a straight
std::vector<Vec3> Arc(float aRadius, float aLead) {
  std::vector<Vec3> line;
  Segment(line, {0, 0, 10}, {aLead, 0, 10}, 1.0f);
  for (int d = 1; d <= 90; ++d) {
    const float a = float(d) * 3.14159265f / 180.0f;
    line.push_back({aLead + aRadius * std::sin(a), aRadius - aRadius * std::cos(a), 10.0f});
  }
  Segment(line, {aLead + aRadius, aRadius, 10}, {aLead + aRadius, aRadius + aLead, 10}, 1.0f);
  return line;
}

} // namespace

// Bounds are the value measured plus a margin, written beside each check ("measured; bound").

TEST(follow_percentile) {
  // nearest rank: index ceil(q n) - 1 of the sorted values
  std::vector<float> v;
  for (int i = 1; i <= 20; ++i) {
    v.push_back(float(i));
  }
  CHECK(Percentile(v, 0.0) == 1.0f);  // the first
  CHECK(Percentile(v, 0.5) == 10.0f); // 0.5 x 20 = 10th
  CHECK(Percentile(v, 0.95) == 19.0f); // 0.95 x 20 = 19th, not the 20th a floor(q n) index gives
  CHECK(Percentile(v, 0.99) == 20.0f); // 19.8 rounds up to the 20th
  CHECK(Percentile(v, 1.0) == 20.0f);
  const std::vector<float> four = {1.0f, 2.0f, 3.0f, 4.0f};
  CHECK(Percentile(four, 0.5) == 2.0f);  // the 2nd of 4
  CHECK(Percentile(four, 0.75) == 3.0f); // the 3rd
  CHECK(Percentile(four, 0.76) == 4.0f); // 3.04 rounds up to the 4th
  CHECK(Percentile({}, 0.5) == 0.0f);
}

TEST(follow_straight_line) {
  const auto line = Straight(600.0f);
  const VehicleSpec spec;
  const Trajectory traj = BuildTrajectory(spec, line, TrajectoryParams{});
  BoxSpace space;
  const auto r = Follow(spec, Route::Make(line), traj, space, Params());
  Print("straight", r, traj.Duration());
  CHECK(r.arrived);                                // the script's test: under 6 m from the end, under 3 m/s
  CHECK(r.time <= 16.5f);                          // 16.37 s measured against the plan's 16.72; 16.5
  CHECK(r.offMax < 0.02f);                         // 0.013 m measured; 0.02
  CHECK(r.speedErrMean < 0.6f);                    // 0.511 m/s measured, against the speed commanded; 0.6
  CHECK(r.speedErrP95 < 2.4f);                     // 2.148 measured; 2.4
  CHECK(std::fabs(r.speedBiasMean) < 0.4f);        // -0.259 measured: the start's jerk lag, no steady bias
  CHECK(r.lost == 0);
}

TEST(follow_arc_holds_line) {
  const auto line = Arc(100.0f, 100.0f);
  VehicleSpec spec;
  spec.speedCap = 20.0f;
  TrajectoryParams tp;
  tp.startSpeed = 20.0f;
  tp.stopAtEnd = false;
  const Trajectory traj = BuildTrajectory(spec, line, tp);
  BoxSpace space;
  const auto r = Follow(spec, Route::Make(line), traj, space, Params());
  Print("arc", r, traj.Duration());
  CHECK(r.ended && !r.arrived);                    // flown at 20 m/s to the end, never under 3 m/s
  CHECK(std::fabs(r.time - traj.Duration()) < 0.1f); // 17.87 s measured against the plan's 17.85; 0.1
  CHECK(r.offMax < 0.45f);                         // 0.377 m measured; 0.45
  CHECK(r.offP95 < 0.40f);                         // 0.336 measured; 0.40
  CHECK(r.speedErrMean < 0.05f);                   // 0.035 m/s measured; 0.05
  CHECK(r.lost == 0);
}

TEST(follow_steady_cruise) {
  // Worked by hand: a straight line, the plan at a constant 20 m/s (the cap), the car started at 20 m/s.
  // The velocity error is 0, the line's bend is 0 and the plan's rate 0, so the commanded acceleration is 0.
  // The thrust is then what holds off drag and gravity: drag 1.2 x airResistance x v^2 / mass =
  // 1.2 x 2 x 400 / 1500 = 0.64 m/s2 along +x, gravity 9.81 up. The net acceleration is 0, so the speed
  // stays 20 and the car stays on the line.
  const auto line = Straight(600.0f);
  VehicleSpec spec;
  spec.speedCap = 20.0f;
  TrajectoryParams tp;
  tp.startSpeed = 20.0f;
  tp.stopAtEnd = false;
  const Trajectory traj = BuildTrajectory(spec, line, tp);
  BoxSpace space;
  const auto r = Follow(spec, Route::Make(line), traj, space, Params());
  Print("cruise", r, traj.Duration());
  CHECK(r.ended);
  CHECK(r.offMax < 1e-3f);        // 0 measured
  CHECK(r.speedErrMean < 1e-3f);  // 0 measured
  CHECK(r.speedErrP95 < 1e-3f);
  CHECK(r.time > 29.9f && r.time < 30.1f); // 600 m at 20 m/s; 29.98 measured
  const auto &f = r.frames[r.frames.size() / 2];
  CHECK(std::fabs(f.thrust.x - 0.64f) < 1e-3f); // 0.6400 measured
  CHECK(std::fabs(f.thrust.y) < 1e-3f);          // 0.0000 measured
  CHECK(std::fabs(f.thrust.z - 9.81f) < 1e-3f);  // 9.8100 measured
  CHECK(std::fabs(f.accel.x) < 1e-3f && std::fabs(f.accel.y) < 1e-3f && std::fabs(f.accel.z) < 1e-3f);
}

// A plan made for the standard car, flown by a car that cannot do what it asks: the thrust clamps. Both
// cars share the nose-and-tilt model, so the difference is the clamps alone.
namespace {
struct Weak {
  std::vector<Vec3> line;
  Trajectory traj;
  FollowResult good, weak;
};

Weak ArcWith(float aSway) {
  Weak w;
  w.line = Arc(60.0f, 100.0f);
  VehicleSpec plan;
  plan.speedCap = 30.0f;
  TrajectoryParams tp;
  tp.startSpeed = 30.0f;
  tp.stopAtEnd = false;
  w.traj = BuildTrajectory(plan, w.line, tp);
  BoxSpace space;
  w.good = Follow(plan, Route::Make(w.line), w.traj, space, Params());
  VehicleSpec car = plan;
  car.sway = aSway;
  w.weak = Follow(car, Route::Make(w.line), w.traj, space, Params());
  return w;
}
} // namespace

TEST(follow_sway_clamp_limits_turn) {
  // a 60 m radius arc at 30 m/s needs 15 m/s2 sideways; the plan's car (sway 5, tilt, drifted surge) can
  // fly it, a car with 1 m/s2 of sway cannot
  const Weak w = ArcWith(1.0f);
  Print("sway ok", w.good, w.traj.Duration());
  Print("sway 1", w.weak, w.traj.Duration());
  CHECK(w.good.offMax < 1.4f);  // 1.293 m measured; 1.4
  CHECK(w.good.lost == 0);
  CHECK(w.weak.offMax > 18.0f); // 19.03 m measured; the car swings wide: more than 10 x the capable car's
  CHECK(w.weak.offMax < 20.0f);
  CHECK(w.weak.offMax > 10.0f * w.good.offMax);
  CHECK(w.weak.offP95 > 15.0f); // 18.76 measured
}

TEST(follow_lost_where_the_car_cannot_hold) {
  const Weak w = ArcWith(1.0f);
  CHECK(w.good.lost == 0);
  CHECK(w.weak.lost == 1);
  // the arc starts at 100 m; the car is 3.5 m out at s = 149 and passes raceOffLine (4 m) at s = 151.4 m
  CHECK(w.weak.lostAt.size() == 1);
  CHECK(w.weak.lostAt[0] > 148.0f && w.weak.lostAt[0] < 155.0f); // 151.4 measured; 3 m either way
}

TEST(follow_surge_clamp_limits_speeding_up) {
  // the standard car's plan from rest on 600 m; a car with 4 m/s2 of surge (the plan's 15) cannot speed up
  // as asked
  const auto line = Straight(600.0f);
  const VehicleSpec plan;
  const Trajectory traj = BuildTrajectory(plan, line, TrajectoryParams{});
  BoxSpace space;
  const auto good = Follow(plan, Route::Make(line), traj, space, Params());
  VehicleSpec car = plan;
  car.surge = 4.0f;
  const auto weak = Follow(car, Route::Make(line), traj, space, Params());
  Print("surge ok", good, traj.Duration());
  Print("surge 4", weak, traj.Duration());
  CHECK(std::fabs(good.speedBiasMean) < 0.4f); // -0.259 measured
  CHECK(weak.speedBiasMean < -2.9f);           // -3.194 measured: behind the commanded speed throughout
  CHECK(weak.speedErrMean > 3.0f);             // 3.279 measured, against the capable car's 0.511
  CHECK(weak.time > good.time + 1.0f);         // 17.75 s measured against 16.37
}

TEST(follow_surge_hold_settles_in_the_slack) {
  // 2500 m from 60 m/s: the plan reaches its Top limit and holds it (27.65 s). With the surge hold on, the
  // surge is pushed toward full by a weight that falls off over the slack (4 m/s) above the plan, so the car
  // settles a little past the plan with a steady surge (it used to switch between full and the controller's
  // every 1.5 s, which the thrusters showed); with it off it follows the plan.
  const auto line = Straight(2500.0f);
  const VehicleSpec spec;
  TrajectoryParams tp;
  tp.startSpeed = 60.0f;
  tp.stopAtEnd = false;
  const Trajectory traj = BuildTrajectory(spec, line, tp);
  BoxSpace space;
  FollowParams off = Params(), on = Params();
  on.tracker.surge.fullSurge = true;
  const auto without = Follow(spec, Route::Make(line), traj, space, off);
  const auto with = Follow(spec, Route::Make(line), traj, space, on);
  Print("hold off", without, traj.Duration());
  Print("hold on", with, traj.Duration());
  auto over = [](const FollowResult &aR) {
    float m = -1e9f;
    for (const auto &f : aR.frames) {
      m = std::max(m, f.speed - f.leadSpeed);
    }
    return m;
  };
  // the forward thrust over the second half: its range, and the largest change from one frame to the next
  float lo = 1e9f, hi = -1e9f, step = 0.0f;
  for (size_t i = with.frames.size() / 2; i < with.frames.size(); ++i) {
    const float y = with.frames[i].got.y;
    lo = std::min(lo, y);
    hi = std::max(hi, y);
    if (i > with.frames.size() / 2) {
      step = std::max(step, std::fabs(y - with.frames[i - 1].got.y));
    }
  }
  std::printf("  hold on, second half: surge %.3f to %.3f m/s2, largest step %.4f\n", lo, hi, step);
  CHECK(without.held == 0);
  CHECK(over(without) < 0.1f);         // 0.01 m/s measured
  CHECK(with.held > 1000);
  CHECK(over(with) > 1.0f);            // past the plan, but inside the slack
  CHECK(over(with) < on.tracker.surge.slack);
  CHECK(with.speedBiasMean > 1.0f);
  CHECK(with.time < without.time - 0.3f);
  CHECK(with.offMax < 0.3f);
  CHECK(hi - lo < 0.5f);               // steady: no cycle between full and the controller's
  CHECK(step < 0.1f);
}

TEST(follow_blocked_ahead) {
  // The tracker brakes for blocked space only when the line is not held (off the line by raceOffLine, 4 m)
  // and the blocked point is off the route, so it takes a car that has left the line. The swinging-wide car
  // of the sway test heads for a box beside the route's last straight (x 165 to 185, y 75 to 100; the
  // route is at x = 160 there).
  const auto line = Arc(60.0f, 100.0f);
  VehicleSpec plan;
  plan.speedCap = 30.0f;
  TrajectoryParams tp;
  tp.startSpeed = 30.0f;
  tp.stopAtEnd = false;
  const Trajectory traj = BuildTrajectory(plan, line, tp);
  VehicleSpec car = plan;
  car.sway = 1.0f;
  BoxSpace space;
  const auto clear = Follow(car, Route::Make(line), traj, space, Params());
  space.boxes.push_back({{165, 75, 0}, {185, 100, 20}});
  const auto r = Follow(car, Route::Make(line), traj, space, Params());
  Print("blocked", r, traj.Duration());
  CHECK(clear.blocked == 0);
  CHECK(r.blocked > 40 && r.blocked < 70); // 55 frames measured
  CHECK(r.emergency > 10);                 // 22 measured: the stop does not fit the brake, so the hard brake
  float offAtFirst = 0.0f;
  for (const auto &f : r.frames) {
    if (f.blocked) {
      offAtFirst = f.off;
      break;
    }
  }
  CHECK(offAtFirst > 4.0f); // 13.08 m measured, never while the line is held
}
