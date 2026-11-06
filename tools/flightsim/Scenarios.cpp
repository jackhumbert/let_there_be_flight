#include "Scenarios.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>

namespace FlightSim {

namespace {
constexpr float kDeg = 57.2957795f;
constexpr float kNone = -std::numeric_limits<float>::infinity();

float Speed2D(const Frame &f) { return std::hypot(f.velocity.x, f.velocity.y); }
float YawRate(const Frame &f) { return std::fabs(f.angularVelocity.z) * kDeg; }
float Tilt(const Frame &f) { return std::max(std::fabs(f.pitchDeg), std::fabs(f.rollDeg)); }
float Wrap(float d) {
  while (d > 180.0f)
    d -= 360.0f;
  while (d < -180.0f)
    d += 360.0f;
  return d;
}

// One run being flown: the sim, the frames so far and the metrics.
struct Flown {
  Setup setup;
  Sim sim;
  Run run;

  Flown(const std::string &aName, const Setup &aSetup, Terrain aTerrain)
      : setup(aSetup), sim(aSetup.car, std::move(aTerrain)) {
    Configure(sim, aSetup);
    run.name = aName;
  }

  // Flies keys for seconds; returns the index of the first frame flown.
  size_t Hold(const Keys &aKeys, float aSeconds) {
    size_t first = run.frames.size();
    int n = static_cast<int>(std::lround(aSeconds / setup.dt));
    for (int i = 0; i < n; ++i)
      run.frames.push_back(sim.Step(aKeys, setup.dt));
    return first;
  }
  // Flies keys until done(frame) or seconds; returns the index of the first frame flown.
  size_t Until(const Keys &aKeys, float aSeconds, const std::function<bool(const Frame &)> &aDone) {
    size_t first = run.frames.size();
    int n = static_cast<int>(std::lround(aSeconds / setup.dt));
    for (int i = 0; i < n; ++i) {
      run.frames.push_back(sim.Step(aKeys, setup.dt));
      if (aDone(run.frames.back()))
        break;
    }
    return first;
  }
  // Settles hands off at the hover height it was placed at.
  void Settle(float aSeconds = 3.0f) {
    Hold({}, aSeconds);
    run.frames.clear();
  }

  const Frame &At(size_t i) const { return run.frames[std::min(i, run.frames.size() - 1)]; }
  size_t End() const { return run.frames.size(); }

  template <class F> float Max(size_t from, size_t to, F f) const {
    float m = kNone;
    for (size_t i = from; i < std::min(to, End()); ++i)
      m = std::max(m, f(run.frames[i]));
    return m;
  }
  template <class F> float Min(size_t from, size_t to, F f) const {
    return -Max(from, to, [&](const Frame &x) { return -f(x); });
  }
  // Seconds from frame from until f holds from then on (to the end); the span's length if it never settles.
  template <class F> float SettleTime(size_t from, F f) const {
    size_t last = from;
    for (size_t i = from; i < End(); ++i)
      if (!f(run.frames[i]))
        last = i + 1;
    return (last - from) * setup.dt;
  }
  // The biggest frame-to-frame change of vertical speed, as an acceleration: the lurches the driver feels. Frames
  // the ground stopped are left out (touchdown_speed measures those).
  float MaxVerticalAccel(size_t from, size_t to) const {
    float m = 0.0f;
    for (size_t i = std::max<size_t>(from, 1); i < std::min(to, End()); ++i)
      if (!run.frames[i].contact)
        m = std::max(m, std::fabs(run.frames[i].velocity.z - run.frames[i - 1].velocity.z) / setup.dt);
    return m;
  }
  void Add(const std::string &aName, float aValue, const std::string &aUnit) {
    run.metrics.push_back({aName, aValue, aUnit});
  }
};

Keys Surge(float v) { Keys k; k.surge = v; return k; }
Keys Lift(float v) { Keys k; k.lift = v; return k; }
Keys Yaw(float v) { Keys k; k.yaw = v; return k; }
Keys Pitch(float v) { Keys k; k.pitch = v; return k; }
Keys Sway(float v) { Keys k; k.sway = v; return k; }

// Starts at rest hovering 3 m over flat ground.
Flown Hovering(const std::string &aName, const Setup &aSetup, Terrain aTerrain = Flat()) {
  Flown f(aName, aSetup, std::move(aTerrain));
  f.sim.Place({0, 0, 3.0f}, 0.0f);
  f.Settle();
  return f;
}

// Puts a hovering car at rest rolled and pitched (degrees; roll about its nose, then pitch about its right side).
void TiltTo(Flown &f, float aRollDeg, float aPitchDeg) {
  Vec3 fwd{0, 1, 0}, right{1, 0, 0};
  right = Flight::RotateAxis(right, fwd, aRollDeg / kDeg);
  fwd = Flight::RotateAxis(fwd, right, aPitchDeg / kDeg);
  Sim::State st;
  st.position = f.sim.Last().position;
  st.forward = fwd;
  st.right = right;
  st.hovering = 1.0f;
  st.hoverHeight = st.position.z;
  st.groundDistance = st.position.z;
  f.sim.SetState(st);
}

// The attitudes a hovering car is let go from, and how each one goes: seconds to within 5 deg of level (the run's
// length if never), the most tilt on the way, and whether it went over (past 90 deg further than it started).
Run TiltGrid(const std::string &aName, const Setup &s, float aYaw) {
  Run run;
  run.name = aName;
  int over = 0;
  float worst = 0.0f;
  for (float roll : {30.0f, 60.0f, 90.0f, 120.0f, 150.0f, 175.0f})
    for (float pitch : {0.0f, 30.0f, 60.0f}) {
      Flown f = Hovering(aName, s);
      f.sim.Place({0, 0, 8.0f}, 0.0f);
      TiltTo(f, roll, pitch);
      float start = f.sim.Last().tiltDeg;
      Keys k = Yaw(aYaw);
      f.Hold(k, 8.0f);
      float t = f.SettleTime(0, [](const Frame &x) { return x.tiltDeg < 5.0f; });
      float most = f.Max(0, f.End(), [](const Frame &x) { return x.tiltDeg; });
      bool flipped = f.Max(0, f.End(), [](const Frame &x) { return x.tiltDeg; }) > 170.0f && roll < 150.0f;
      char name[48];
      std::snprintf(name, sizeof(name), "roll%.0f_pitch%.0f_level_time", roll, pitch);
      run.metrics.push_back({name, t, "s"});
      std::snprintf(name, sizeof(name), "roll%.0f_pitch%.0f_max_tilt", roll, pitch);
      run.metrics.push_back({name, most, "deg"});
      over += flipped ? 1 : 0;
      worst = std::max(worst, t);
      (void)start;
      run.frames = f.run.frames; // the last start's, for --csv
    }
  run.metrics.push_back({"flipped_over", static_cast<float>(over), "starts"});
  run.metrics.push_back({"worst_level_time", worst, "s"});
  return run;
}

// Hovering 6 m up, a knock (a collision's spin and push, impact 1: the recovery timer's full outage), with or
// without a turn held: how far it tips, whether it goes over, how long until level, the height it loses.
Run Knocked(const std::string &aName, const Setup &s, Vec3 aSpin, float aYaw) {
  Flown f = Hovering(aName, s);
  f.sim.Place({0, 0, 6.0f}, 0.0f);
  f.Settle(2.0f);
  float z0 = f.sim.Last().position.z;
  f.sim.Knock(aSpin, {3.0f, 0, 0}, 1.0f);
  f.Hold(Yaw(aYaw), 8.0f);
  f.Add("max_tilt", f.Max(0, f.End(), [](const Frame &x) { return x.tiltDeg; }), "deg");
  f.Add("level_time", f.SettleTime(0, [](const Frame &x) { return x.tiltDeg < 5.0f; }), "s");
  f.Add("height_lost", z0 - f.Min(0, f.End(), [](const Frame &x) { return x.position.z; }), "m");
  f.Add("hit_ground", f.Max(0, f.End(), [](const Frame &x) { return x.contact ? 1.0f : 0.0f; }), "");
  f.Add("tilt_at_8s", f.At(f.End() - 1).tiltDeg, "deg");
  return f.run;
}

// Drone: in the open, 500 m up over flat ground, at rest. Nothing holds Drone up but its lift input (the anti-gravity
// drone cancels gravity), so it falls through the run, which changes nothing about how it turns.
Flown Airborne(const std::string &aName, const Setup &aSetup) {
  Flown f(aName, aSetup, Flat());
  f.sim.Place({0, 0, 500.0f}, 0.0f);
  return f;
}

// A body axis' component: 0 pitch (X), 1 roll (Y), 2 yaw (Z).
float Axis(const Vec3 &v, int a) { return a == 0 ? v.x : a == 1 ? v.y : v.z; }
float Spin(const Frame &f) { return Flight::Length(f.angularVelocity) * kDeg; }
bool Finite(const Frame &f) {
  return std::isfinite(f.position.x + f.position.y + f.position.z + f.velocity.x + f.velocity.y + f.velocity.z +
                       f.angularVelocity.x + f.angularVelocity.y + f.angularVelocity.z);
}
// 1 when every frame of the run is a number.
float AllFinite(const Flown &f) {
  for (const Frame &x : f.run.frames)
    if (!Finite(x))
      return 0.0f;
  return 1.0f;
}

// Drone: hold one rotation key 3 s, then let go for 4 s. The rate about that body axis along the way, whether and when
// PhysX's spin cap stops it rising, and how far it turns on after the key is let go.
Run DroneSpin(const std::string &aName, const Setup &s, int aAxis) {
  Flown f = Airborne(aName, s);
  Keys k;
  (aAxis == 0 ? k.pitch : aAxis == 1 ? k.roll : k.yaw) = 1.0f;
  size_t hold = f.Hold(k, 3.0f);
  size_t release = f.Hold({}, 4.0f);
  auto rate = [&](const Frame &x) { return std::fabs(Axis(x.angularVelocity, aAxis)) * kDeg; };
  auto at = [&](float sec) { return rate(f.At(hold + static_cast<size_t>(sec / s.dt) - 1)); };
  f.Add("rate_0.5s", at(0.5f), "deg/s");
  f.Add("rate_1s", at(1.0f), "deg/s");
  f.Add("rate_3s", at(3.0f), "deg/s");
  f.Add("rate_growth_2to3s", at(3.0f) - at(2.0f), "deg/s");
  // the cap: within 2% of maxAngularSpeed; the hold's length when never reached
  size_t i = hold;
  while (i < release && rate(f.At(i)) < 0.98f * s.car.maxAngularSpeed * kDeg)
    ++i;
  f.Add("time_to_cap", (i - hold) * s.dt, "s");
  f.Add("max_spin", f.Max(0, f.End(), Spin), "deg/s");
  f.Add("stop_time", f.SettleTime(release, [&](const Frame &x) { return rate(x) < 5.0f; }), "s");
  float turned = 0.0f;
  for (size_t j = release; j < f.End(); ++j)
    turned += Axis(f.At(j).angularVelocity, aAxis) * s.dt;
  f.Add("coast_after_release", std::fabs(turned) * kDeg, "deg");
  f.Add("finite", AllFinite(f), "");
  return f.run;
}

using Scenario = std::function<Run(const Setup &)>;

const std::map<std::string, Scenario> &Scenarios() {
  static const std::map<std::string, Scenario> scenarios = {
      // hands off, hovering: it should stay put
      {"hands_off",
       [](const Setup &s) {
         Flown f = Hovering("hands_off", s);
         float z0 = f.sim.Last().position.z;
         f.Hold({}, 10.0f);
         f.Add("height_drift", f.Max(0, f.End(), [&](const Frame &x) { return std::fabs(x.position.z - z0); }), "m");
         f.Add("position_drift", f.Max(0, f.End(), [](const Frame &x) { return std::hypot(x.position.x, x.position.y); }), "m");
         f.Add("max_tilt", f.Max(0, f.End(), Tilt), "deg");
         return f.run;
       }},
      // hold a turn key for 3 s, then let go: a steady turn rate, and it stops where it was let go
      {"yaw_hold",
       [](const Setup &s) {
         Flown f = Hovering("yaw_hold", s);
         size_t hold = f.Hold(Yaw(1.0f), 3.0f);
         size_t release = f.Hold({}, 4.0f);
         auto at = [&](float sec) { return YawRate(f.At(hold + static_cast<size_t>(sec / s.dt) - 1)); };
         f.Add("yaw_rate_0.5s", at(0.5f), "deg/s");
         f.Add("yaw_rate_1s", at(1.0f), "deg/s");
         f.Add("yaw_rate_3s", at(3.0f), "deg/s");
         // rate growth over the last second held: 0 when the rate has levelled off
         f.Add("yaw_rate_growth_2to3s", at(3.0f) - at(2.0f), "deg/s");
         f.Add("yaw_stop_time", f.SettleTime(release, [](const Frame &x) { return YawRate(x) < 5.0f; }), "s");
         float h0 = f.At(release - 1).headingDeg;
         float turned = 0.0f, prev = h0;
         for (size_t i = release; i < f.End(); ++i) {
           turned += Wrap(f.At(i).headingDeg - prev);
           prev = f.At(i).headingDeg;
         }
         f.Add("yaw_coast_after_release", std::fabs(turned), "deg");
         return f.run;
       }},
      // a short tap of the turn key: a small, predictable heading change
      {"yaw_tap",
       [](const Setup &s) {
         Flown f = Hovering("yaw_tap", s);
         float h0 = f.sim.Last().headingDeg;
         f.Hold(Yaw(1.0f), 0.25f);
         f.Hold({}, 3.0f);
         f.Add("heading_change", std::fabs(Wrap(f.At(f.End() - 1).headingDeg - h0)), "deg");
         f.Add("settle_time", f.SettleTime(0, [](const Frame &x) { return YawRate(x) < 2.0f; }), "s");
         return f.run;
       }},
      // hold forward 5 s, then let go: how fast it gets going, and whether it stops by itself
      {"surge_release",
       [](const Setup &s) {
         Flown f = Hovering("surge_release", s);
         size_t hold = f.Hold(Surge(1.0f), 5.0f);
         size_t release = f.Hold({}, 30.0f);
         auto at = [&](size_t from, float sec) { return Speed2D(f.At(from + static_cast<size_t>(sec / s.dt) - 1)); };
         f.Add("speed_1s", at(hold, 1.0f), "m/s");
         f.Add("speed_5s", at(hold, 5.0f), "m/s");
         f.Add("speed_3s_after_release", at(release, 3.0f), "m/s");
         f.Add("speed_30s_after_release", at(release, 30.0f), "m/s");
         float x0 = f.At(release).position.y;
         size_t stop = release;
         while (stop < f.End() && Speed2D(f.At(stop)) > 1.0f)
           ++stop;
         f.Add("coast_distance", f.At(stop).position.y - x0, "m");
         f.Add("coast_time", (stop - release) * s.dt, "s");
         f.Add("height_change", f.Max(0, f.End(), [&](const Frame &x) { return std::fabs(x.position.z - f.At(0).position.z); }), "m");
         return f.run;
       }},
      // flying at speed, hold a turn for 1.5 s: how far the velocity lags the nose (the slide)
      {"surge_turn",
       [](const Setup &s) {
         Flown f = Hovering("surge_turn", s);
         f.Hold(Surge(1.0f), 2.0f);
         Keys k = Surge(1.0f);
         k.yaw = 1.0f;
         size_t turn = f.Hold(k, 1.5f);
         size_t after = f.Hold(Surge(1.0f), 3.0f);
         auto slip = [](const Frame &x) {
           if (Speed2D(x) < 2.0f)
             return 0.0f;
           float course = std::atan2(x.velocity.x, x.velocity.y) * kDeg;
           return std::fabs(Wrap(course - x.headingDeg));
         };
         f.Add("speed_at_turn", Speed2D(f.At(turn)), "m/s");
         f.Add("max_slip", f.Max(turn, f.End(), slip), "deg");
         f.Add("slip_settle_time", f.SettleTime(after, [&](const Frame &x) { return slip(x) < 5.0f; }), "s");
         return f.run;
       }},
      // hold up for 1 s from a hover, then let go: a short climb that stops where it was let go
      {"climb_short",
       [](const Setup &s) {
         Flown f = Hovering("climb_short", s);
         size_t hold = f.Hold(Lift(1.0f), 1.0f);
         size_t release = f.Hold({}, 6.0f);
         float zr = f.At(release - 1).position.z;
         f.Add("height_at_release", zr - f.At(0).groundHeight, "m");
         f.Add("max_climb_rate", f.Max(hold, release, [](const Frame &x) { return x.velocity.z; }), "m/s");
         f.Add("overshoot_after_release", f.Max(release, f.End(), [](const Frame &x) { return x.position.z; }) - zr, "m");
         f.Add("final_height", f.At(f.End() - 1).position.z, "m");
         f.Add("max_vertical_accel", f.MaxVerticalAccel(hold, f.End()), "m/s2");
         return f.run;
       }},
      // hold up for 4 s: out of the hover band into free flight, then let go
      {"climb_long",
       [](const Setup &s) {
         Flown f = Hovering("climb_long", s);
         size_t hold = f.Hold(Lift(1.0f), 4.0f);
         size_t release = f.Hold({}, 6.0f);
         float zr = f.At(release - 1).position.z;
         f.Add("height_at_release", zr, "m");
         f.Add("max_climb_rate", f.Max(hold, release, [](const Frame &x) { return x.velocity.z; }), "m/s");
         f.Add("overshoot_after_release", f.Max(release, f.End(), [](const Frame &x) { return x.position.z; }) - zr, "m");
         f.Add("height_drift_last_3s", std::fabs(f.At(f.End() - 1).position.z - f.At(f.End() - 1 - static_cast<size_t>(3.0f / s.dt)).position.z), "m");
         f.Add("max_vertical_accel", f.MaxVerticalAccel(hold, f.End()), "m/s2");
         return f.run;
       }},
      // from 40 m, hold down until it lands: how fast it comes down and how hard it lands
      {"descend_land",
       [](const Setup &s) {
         Flown f("descend_land", s, Flat());
         f.sim.Place({0, 0, 40.0f}, 0.0f);
         f.Settle(2.0f);
         // down until it touches, or comes to rest at the bottom of the hover band (a car that hovers never lands)
         size_t hold = f.Until(Lift(-1.0f), 20.0f, [&](const Frame &x) {
           return x.contact || (x.foundGround && x.groundDistance < s.standard.minHoverHeight + 0.5f &&
                                  std::fabs(x.velocity.z) < 0.2f);
         });
         size_t after = f.Hold({}, 3.0f);
         f.Add("max_sink_rate", -f.Min(hold, after, [](const Frame &x) { return x.velocity.z; }), "m/s");
         f.Add("sink_rate_entering_band", -f.At(hold + [&] {
           size_t i = 0;
           while (hold + i < after && f.At(hold + i).hovering <= 0.0f)
             ++i;
           return i;
         }()).velocity.z, "m/s");
         f.Add("touchdown_speed", f.Max(hold, f.End(), [](const Frame &x) { return x.impactSpeed; }), "m/s");
         f.Add("time_to_land", (after - hold) * s.dt, "s");
         f.Add("max_vertical_accel", f.MaxVerticalAccel(hold, f.End()), "m/s2");
         return f.run;
       }},
      // hovering 3 m over a 15 m roof at 20 m/s with half forward held, off its edge: the drop should not pull the car
      // down
      {"roof_edge_off",
       [](const Setup &s) {
         Flown f("roof_edge_off", s, [](float, float y) { return y < 0.0f ? 15.0f : 0.0f; });
         f.sim.Place({0, -80.0f, 18.0f}, 0.0f);
         f.Settle(2.0f);
         f.sim.Place({0, -60.0f, f.sim.Last().position.z}, 0.0f, {0, 20.0f, 0});
         float z0 = f.sim.Last().position.z;
         f.Hold(Surge(0.5f), 8.0f);
         size_t edge = 0;
         while (edge < f.End() && f.At(edge).position.y < 0.0f)
           ++edge;
         f.Add("height_before_edge", f.At(edge > 0 ? edge - 1 : 0).position.z - 15.0f, "m");
         f.Add("max_drop_after_edge", z0 - f.Min(edge, f.End(), [](const Frame &x) { return x.position.z; }), "m");
         f.Add("max_rise_after_edge", f.Max(edge, f.End(), [](const Frame &x) { return x.position.z; }) - z0, "m");
         f.Add("max_vertical_accel", f.MaxVerticalAccel(edge, f.End()), "m/s2");
         return f.run;
       }},
      // flying 12 m up at 20 m/s with half forward held, onto an 8 m roof (4 m under the car): no jolt, no hitting it
      {"roof_edge_on",
       [](const Setup &s) {
         Flown f("roof_edge_on", s, [](float, float y) { return y > 0.0f ? 8.0f : 0.0f; });
         f.sim.Place({0, -200.0f, 12.0f}, 0.0f);
         f.Settle(3.0f);
         f.sim.Place({0, -60.0f, f.sim.Last().position.z}, 0.0f, {0, 20.0f, 0});
         f.Hold(Surge(0.5f), 8.0f);
         size_t edge = 0;
         while (edge < f.End() && f.At(edge).position.y < 0.0f)
           ++edge;
         float z0 = f.At(edge > 0 ? edge - 1 : 0).position.z;
         f.Add("height_before_edge", z0, "m");
         f.Add("max_rise_after_edge", f.Max(edge, f.End(), [](const Frame &x) { return x.position.z; }) - z0, "m");
         f.Add("max_drop_after_edge", z0 - f.Min(edge, f.End(), [](const Frame &x) { return x.position.z; }), "m");
         f.Add("min_clearance", f.Min(edge, f.End(), [](const Frame &x) { return x.position.z - 0.6f - x.groundHeight; }), "m");
         f.Add("max_vertical_accel", f.MaxVerticalAccel(edge, f.End()), "m/s2");
         return f.run;
       }},
      // hovering 3 m up at 15 m/s with half forward held, over a 1.5 m bump (a parked car): a small, short disturbance
      {"bump",
       [](const Setup &s) {
         Flown f("bump", s, [](float, float y) { return y > 0.0f && y < 4.5f ? 1.5f : 0.0f; });
         f.sim.Place({0, -80.0f, 3.0f}, 0.0f);
         f.Settle(2.0f);
         f.sim.Place({0, -40.0f, f.sim.Last().position.z}, 0.0f, {0, 15.0f, 0});
         float z0 = f.sim.Last().position.z;
         f.Hold(Surge(0.5f), 6.0f);
         f.Add("max_rise", f.Max(0, f.End(), [](const Frame &x) { return x.position.z; }) - z0, "m");
         f.Add("max_drop", z0 - f.Min(0, f.End(), [](const Frame &x) { return x.position.z; }), "m");
         f.Add("settle_time", f.SettleTime(0, [&](const Frame &x) { return std::fabs(x.position.z - z0) < 0.1f; }), "s");
         f.Add("max_vertical_accel", f.MaxVerticalAccel(0, f.End()), "m/s2");
         return f.run;
       }},
      // hold pitch forward for 2 s, then let go: tilt and the speed it gives, without losing height
      {"pitch_hold",
       [](const Setup &s) {
         Flown f = Hovering("pitch_hold", s);
         float z0 = f.sim.Last().position.z;
         size_t hold = f.Hold(Pitch(1.0f), 2.0f);
         size_t release = f.Hold({}, 4.0f);
         f.Add("max_tilt", f.Max(hold, release, Tilt), "deg");
         f.Add("speed_at_release", Speed2D(f.At(release - 1)), "m/s");
         f.Add("height_change", f.Max(0, f.End(), [&](const Frame &x) { return std::fabs(x.position.z - z0); }), "m");
         f.Add("level_time_after_release", f.SettleTime(release, [](const Frame &x) { return Tilt(x) < 2.0f; }), "s");
         return f.run;
       }},
      // let go tilted: it should come back level from any attitude, and never roll on over
      {"tilt_recovery", [](const Setup &s) { return TiltGrid("tilt_recovery", s, 0.0f); }},
      {"tilt_recovery_yaw", [](const Setup &s) { return TiltGrid("tilt_recovery_yaw", s, 1.0f); }},
      // knocked while hovering (a side hit: roll spin), hands off and with a turn held
      {"knock_roll", [](const Setup &s) { return Knocked("knock_roll", s, {0, 4.0f, 0}, 0.0f); }},
      {"knock_roll_yaw", [](const Setup &s) { return Knocked("knock_roll_yaw", s, {0, 4.0f, 0}, 1.0f); }},
      // knocked hovering low (2 m): the recovery cuts the lift, so it may drop to the ground; it must come back up to
      // at least the bottom of the hover band and stay there (Jack, 2026-11-04)
      {"knock_low",
       [](const Setup &s) {
         Flown f = Hovering("knock_low", s);
         f.sim.Place({0, 0, 2.0f}, 0.0f);
         f.Settle(3.0f);
         f.sim.Knock({0, 4.0f, 0}, {3.0f, 0, 0}, 1.0f);
         size_t hit = f.Hold({}, 8.0f);
         float floor = s.standard.minHoverHeight;
         f.Add("hit_ground", f.Max(hit, f.End(), [](const Frame &x) { return x.contact ? 1.0f : 0.0f; }), "");
         f.Add("lowest_distance", f.Min(hit, f.End(), [](const Frame &x) { return x.groundDistance; }), "m");
         f.Add("back_above_min_time",
               f.SettleTime(hit, [&](const Frame &x) { return x.foundGround && x.groundDistance >= floor - 0.05f; }), "s");
         f.Add("final_distance", f.At(f.End() - 1).groundDistance, "m");
         f.Add("final_tilt", f.At(f.End() - 1).tiltDeg, "deg");
         return f.run;
       }},
      // a nose hit: pitch spin
      {"knock_pitch", [](const Setup &s) { return Knocked("knock_pitch", s, {3.0f, 0, 0}, 0.0f); }},
      {"knock_pitch_yaw", [](const Setup &s) { return Knocked("knock_pitch_yaw", s, {3.0f, 0, 0}, 1.0f); }},
      // hold sideways for 2 s, then let go
      // full forward for 60 s, high above the ground (no hover band): the speed it levels off at
      {"top_speed",
       [](const Setup &s) {
         Flown f("top_speed", s, Flat());
         f.sim.Place({0, 0, 200.0f}, 0.0f);
         f.Settle(2.0f);
         f.Hold(Surge(1.0f), 60.0f);
         f.Add("top_speed", Speed2D(f.At(f.End() - 1)), "m/s");
         f.Add("speed_10s", Speed2D(f.At(static_cast<size_t>(10.0f / s.dt))), "m/s");
         f.Add("height_change", std::fabs(f.At(f.End() - 1).position.z - f.At(0).position.z), "m");
         return f.run;
       }},
      {"sway_release",
       [](const Setup &s) {
         Flown f = Hovering("sway_release", s);
         size_t hold = f.Hold(Sway(1.0f), 2.0f);
         size_t release = f.Hold({}, 10.0f);
         f.Add("speed_at_release", Speed2D(f.At(release - 1)), "m/s");
         f.Add("speed_5s_after_release", Speed2D(f.At(release + static_cast<size_t>(5.0f / s.dt))), "m/s");
         f.Add("max_tilt", f.Max(hold, f.End(), Tilt), "deg");
         return f.run;
       }},
      // the brake (linear and angular, as the brake key gives them) held 4 s: hovering 3 m up, at rest 30 m up, and
      // flying at speed 30 m up; the height it gains or loses in each (Jack: holding the brake, the car rises)
      {"brake_hold",
       [](const Setup &s) {
         Keys brake;
         brake.linearBrake = 1.0f;
         brake.angularBrake = 1.0f;
         auto rise = [&](Flown &f, size_t from) {
           float z0 = f.At(from).position.z;
           return f.Max(from, f.End(), [&](const Frame &x) { return x.position.z - z0; });
         };
         auto sink = [&](Flown &f, size_t from) {
           float z0 = f.At(from).position.z;
           return f.Max(from, f.End(), [&](const Frame &x) { return z0 - x.position.z; });
         };
         Flown low = Hovering("brake_hold", s);
         size_t b = low.Hold(brake, 4.0f);
         Flown high = Hovering("brake_hold", s);
         high.sim.Place({0, 0, 30.0f}, 0.0f);
         high.Settle(4.0f);
         size_t bh = high.Hold(brake, 4.0f);
         Flown fast = Hovering("brake_hold", s);
         fast.sim.Place({0, 0, 30.0f}, 0.0f);
         fast.Settle(4.0f);
         fast.Hold(Surge(1.0f), 4.0f);
         size_t bf = fast.Hold(brake, 4.0f);
         low.Add("hover_rise", rise(low, b), "m");
         low.Add("hover_sink", sink(low, b), "m");
         low.Add("high_rise", rise(high, bh), "m");
         low.Add("high_sink", sink(high, bh), "m");
         low.Add("fast_speed_at_brake", Speed2D(fast.At(bf - 1)), "m/s");
         low.Add("fast_rise", rise(fast, bf), "m");
         low.Add("fast_sink", sink(fast, bf), "m");
         low.Add("fast_speed_after_4s", Speed2D(fast.At(fast.End() - 1)), "m/s");
         return low.run;
       }},
      // Drone: hold pitch, roll or yaw 3 s, then let go
      {"drone_pitch", [](const Setup &s) { return DroneSpin("drone_pitch", s, 0); }},
      {"drone_roll", [](const Setup &s) { return DroneSpin("drone_roll", s, 1); }},
      {"drone_yaw", [](const Setup &s) { return DroneSpin("drone_yaw", s, 2); }},
      // Drone: all three rotation keys held 2 s, then hands off: Drone has no levelling, so only the force tail's
      // angular damping stops the spin
      {"drone_spin_release",
       [](const Setup &s) {
         Flown f = Airborne("drone_spin_release", s);
         Keys k;
         k.pitch = k.roll = k.yaw = 1.0f;
         f.Hold(k, 2.0f);
         size_t release = f.Hold({}, 6.0f);
         f.Add("spin_at_release", Spin(f.At(release - 1)), "deg/s");
         f.Add("stop_time", f.SettleTime(release, [](const Frame &x) { return Spin(x) < 5.0f; }), "s");
         float turned = 0.0f;
         for (size_t j = release; j < f.End(); ++j)
           turned += Flight::Length(f.At(j).angularVelocity) * s.dt;
         f.Add("coast_after_release", turned * kDeg, "deg");
         f.Add("spin_after_2s", Spin(f.At(release + static_cast<size_t>(2.0f / s.dt))), "deg/s");
         f.Add("finite", AllFinite(f), "");
         return f.run;
       }},
      // Drone: 1 s hands off, then lift held 1.5 s: the vertical accelerations, and no tilt from them
      {"drone_lift",
       [](const Setup &s) {
         Flown f = Airborne("drone_lift", s);
         size_t off = f.Hold({}, 1.0f);
         size_t lift = f.Hold(Lift(1.0f), 1.5f);
         size_t q = static_cast<size_t>(0.25f / s.dt);
         auto accel = [&](size_t end) { return (f.At(end - 1).velocity.z - f.At(end - 1 - q).velocity.z) / (q * s.dt); };
         f.Add("accel_hands_off", accel(lift), "m/s2");
         f.Add("lift_accel", accel(f.End()), "m/s2");
         f.Add("climb_gained_1.5s", f.At(f.End() - 1).velocity.z - f.At(lift - 1).velocity.z, "m/s");
         f.Add("max_tilt", f.Max(off, f.End(), [](const Frame &x) { return x.tiltDeg; }), "deg");
         f.Add("finite", AllFinite(f), "");
         return f.run;
       }},
      // Drone: surge held 2 s: the forward acceleration and speed
      {"drone_surge",
       [](const Setup &s) {
         Flown f = Airborne("drone_surge", s);
         f.Hold(Surge(1.0f), 2.0f);
         size_t q = static_cast<size_t>(0.25f / s.dt);
         f.Add("surge_accel", (f.At(f.End() - 1).velocity.y - f.At(f.End() - 1 - q).velocity.y) / (q * s.dt), "m/s2");
         f.Add("speed_2s", f.At(f.End() - 1).velocity.y, "m/s");
         f.Add("max_tilt", f.Max(0, f.End(), [](const Frame &x) { return x.tiltDeg; }), "deg");
         f.Add("finite", AllFinite(f), "");
         return f.run;
       }},
      // Drone: yaw held 1 s, then let go with the angular brake held, against let go alone
      {"drone_angular_brake",
       [](const Setup &s) {
         auto fly = [&](float aBrake, float &aStopTime, float &aCoast, float &aRate) {
           Flown f = Airborne("drone_angular_brake", s);
           f.Hold(Yaw(1.0f), 1.0f);
           Keys k;
           k.angularBrake = aBrake;
           size_t release = f.Hold(k, 5.0f);
           float turned = 0.0f;
           for (size_t j = release; j < f.End(); ++j)
             turned += f.At(j).angularVelocity.z * s.dt;
           aStopTime = f.SettleTime(release, [](const Frame &x) { return YawRate(x) < 5.0f; });
           aCoast = std::fabs(turned) * kDeg;
           aRate = YawRate(f.At(release - 1));
           return f;
         };
         float tBrake, coastBrake, rate, tFree, coastFree, rateFree;
         Flown braked = fly(1.0f, tBrake, coastBrake, rate);
         Flown free = fly(0.0f, tFree, coastFree, rateFree);
         braked.Add("rate_at_release", rate, "deg/s");
         braked.Add("stop_time_brake", tBrake, "s");
         braked.Add("stop_time_release", tFree, "s");
         braked.Add("coast_brake", coastBrake, "deg");
         braked.Add("coast_release", coastFree, "deg");
         braked.Add("finite", AllFinite(braked) * AllFinite(free), "");
         return braked.run;
       }},
  };
  return scenarios;
}
} // namespace

void Configure(Sim &aSim, const Setup &aSetup) {
  aSim.standard = aSetup.standard;
  aSim.hoverFly = aSetup.hoverFly;
  aSim.force = aSetup.force;
  aSim.shapeDrag = aSetup.shapeDrag;
  aSim.thrustStrength = aSetup.thrustStrength;
  aSim.thrustPerThruster = aSetup.thrustPerThruster;
  aSim.mode = aSetup.mode;
  aSim.assistParams = aSetup.assistParams;
  aSim.drone = aSetup.drone;
  aSim.droneAssist = aSetup.droneAssist;
}

float Run::Get(const std::string &aName) const {
  for (const Metric &m : metrics)
    if (m.name == aName)
      return m.value;
  return std::numeric_limits<float>::quiet_NaN();
}

std::vector<std::string> ScenarioNames() {
  std::vector<std::string> names;
  for (const auto &[name, _] : Scenarios())
    names.push_back(name);
  return names;
}

std::vector<std::string> ScenarioNames(Mode aMode) {
  std::vector<std::string> names;
  for (const auto &[name, _] : Scenarios())
    if ((name.rfind("drone_", 0) == 0) == IsDrone(aMode))
      names.push_back(name);
  return names;
}

Run RunScenario(const std::string &aName, const Setup &aSetup) {
  auto it = Scenarios().find(aName);
  if (it == Scenarios().end())
    return {aName, {}, {}};
  return it->second(aSetup);
}

} // namespace FlightSim
