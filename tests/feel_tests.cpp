// How the flight modes feel, on every surveyed car and bike (docs/design/flight-modes.md, Feel spec): the flightsim scenarios
// (tools/flightsim) flown over tools/flightsim/vehicles.tsv, each metric checked against a bound.
//
// - Drone (Flight::Drone) and the anti-gravity drone: a regression guard on what they do today. Their torque is a
//   fixed factor x input, so each car spins at a rate set by its own TorqueResponse until PhysX's 7 rad/s cap; the
//   bounds are the fleet's spread as measured, a little widened, so a change that makes the slowest
//   car slower or the longest coast longer turns this red.
// - DroneAssist (the proposal): the same rate on every car.
// - Standard (the Hover & Fly redesign): the draft spec's bounds, on every car, at each Handling choice (Loose, Default
//   and Tight replace the force tail's handling values, Flight/Handling.hpp).
#include "check.hpp"

#include "../tools/flightsim/Scenarios.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace FlightSim;

namespace {

constexpr float kDeg = 57.2957795f;

const std::vector<Car> &Fleet() {
  static const std::vector<Car> cars = LoadCars(LTBF_SOURCE_DIR "/tools/flightsim/vehicles.tsv");
  return cars;
}

// Every car through the scenarios named, in the mode given, at a Handling choice (Flight::Handling, Default unless
// given): runs[car][scenario]. The cars are shared out over the cores.
std::vector<std::vector<Run>> FlyFleet(Mode aMode, const std::vector<std::string> &aNames, float aHandling = 1.0f) {
  const std::vector<Car> &cars = Fleet();
  std::vector<std::vector<Run>> runs(cars.size());
  unsigned workers = std::max(1u, std::min(16u, std::thread::hardware_concurrency()));
  std::vector<std::thread> threads;
  for (unsigned w = 0; w < workers; ++w)
    threads.emplace_back([&, w] {
      for (size_t i = w; i < cars.size(); i += workers) {
        Setup s;
        s.car = cars[i];
        s.mode = aMode;
        s.handling = aHandling;
        for (const std::string &name : aNames) {
          Run r = RunScenario(name, s);
          r.frames.clear(); // only the metrics are checked
          runs[i].push_back(std::move(r));
        }
      }
    });
  for (std::thread &t : threads)
    t.join();
  return runs;
}

// Checks one metric of one scenario on every car: lo <= value <= hi, and a number. Names the first few cars out of
// bounds and the fleet's range.
void Bounds(const std::vector<std::vector<Run>> &aRuns, const char *aScenario, const char *aMetric, float aLo, float aHi,
            int aLine) {
  int bad = 0;
  float lo = INFINITY, hi = -INFINITY;
  for (size_t i = 0; i < aRuns.size(); ++i)
    for (const Run &r : aRuns[i]) {
      if (r.name != aScenario)
        continue;
      float v = r.Get(aMetric);
      lo = std::fmin(lo, v);
      hi = std::fmax(hi, v);
      if (std::isfinite(v) && v >= aLo && v <= aHi)
        continue;
      if (bad++ < 5)
        std::printf("    %s %s.%s = %g, out of %g..%g\n", Fleet()[i].name.c_str(), aScenario, aMetric, v, aLo, aHi);
    }
  if (bad || !(lo <= hi)) {
    char text[256];
    std::snprintf(text, sizeof(text), "%s.%s: %d of %zu cars out of %g..%g (fleet %g..%g)", aScenario, aMetric, bad,
                  aRuns.size(), aLo, aHi, lo, hi);
    Check::Fail(__FILE__, aLine, text);
  }
}
#define BOUNDS(runs, scenario, metric, lo, hi) Bounds(runs, scenario, metric, lo, hi, __LINE__)

const std::vector<std::string> kDroneScenarios{"drone_pitch", "drone_roll", "drone_yaw", "drone_spin_release",
                                               "drone_lift", "drone_surge", "drone_angular_brake"};

} // namespace

TEST(feel_fleet_loads) {
  // 261 physical cars, bikes included (the stand-in drive model's 67 left out)
  CHECK(Fleet().size() >= 255);
  for (const Car &c : Fleet())
    CHECK(c.angularGain.x > 0.0f && c.angularGain.y > 0.0f && c.angularGain.z > 0.0f && std::isfinite(c.angularGain.x) &&
          std::isfinite(c.angularGain.y) && std::isfinite(c.angularGain.z));
}

TEST(feel_drone_fleet) {
  auto runs = FlyFleet(Mode::Drone, kDroneScenarios);
  float cap = Car{}.maxAngularSpeed * kDeg + 0.5f;
  for (const char *s : {"drone_pitch", "drone_roll", "drone_yaw", "drone_spin_release", "drone_lift", "drone_surge",
                        "drone_angular_brake"})
    BOUNDS(runs, s, "finite", 1, 1);
  for (const char *s : {"drone_pitch", "drone_roll", "drone_yaw"}) {
    BOUNDS(runs, s, "max_spin", 0, cap);
    BOUNDS(runs, s, "rate_3s", 0, cap);
  }
  // the slowest car's rate after 1 s held (66, 66 and 131 deg/s: the 1300 kg default drive model the quest props use)
  BOUNDS(runs, "drone_pitch", "rate_1s", 60, cap);
  BOUNDS(runs, "drone_roll", "rate_1s", 60, cap);
  BOUNDS(runs, "drone_yaw", "rate_1s", 120, cap);
  // how far it turns on after the key is let go (worst 860, 1124 and 837 deg, the same props)
  BOUNDS(runs, "drone_pitch", "coast_after_release", 0, 900);
  BOUNDS(runs, "drone_roll", "coast_after_release", 0, 1150);
  BOUNDS(runs, "drone_yaw", "coast_after_release", 0, 900);
  // hands off after a spin: what is left 2 s later (worst 56 deg/s)
  BOUNDS(runs, "drone_spin_release", "spin_after_2s", 0, 60);
  // the angular brake: stops a 1 s yaw within 1.5 s on every car (worst 1.45), and never later than letting go alone
  BOUNDS(runs, "drone_angular_brake", "stop_time_brake", 0, 1.5f);
  for (const std::vector<Run> &car : runs)
    for (const Run &r : car)
      if (r.name == "drone_angular_brake")
        CHECK(r.Get("stop_time_brake") <= r.Get("stop_time_release"));
  // lift and surge are accelerations already (the force tail scales by mass): the same on every car but for air drag
  BOUNDS(runs, "drone_lift", "accel_hands_off", -9.85f, -9.5f);
  BOUNDS(runs, "drone_lift", "lift_accel", 28, 31);
  BOUNDS(runs, "drone_lift", "max_tilt", 0, 0.01f);
  BOUNDS(runs, "drone_surge", "surge_accel", 13, 15.5f);
  BOUNDS(runs, "drone_surge", "max_tilt", 0, 0.01f);
}

TEST(feel_antigravity_drone_fleet) {
  auto runs = FlyFleet(Mode::AntiGravityDrone, {"drone_lift", "drone_yaw"});
  BOUNDS(runs, "drone_lift", "finite", 1, 1);
  // gravity cancelled: hands off it neither climbs nor sinks; lift is Drone's 40 m/s2 less air drag
  BOUNDS(runs, "drone_lift", "accel_hands_off", -0.05f, 0.05f);
  BOUNDS(runs, "drone_lift", "lift_accel", 33, 40);
  BOUNDS(runs, "drone_yaw", "max_spin", 0, Car{}.maxAngularSpeed * kDeg + 0.5f);
}

TEST(feel_drone_assist_fleet) {
  auto runs = FlyFleet(Mode::DroneAssist, kDroneScenarios);
  Flight::DroneAssistParams a;
  for (const char *s : {"drone_pitch", "drone_roll", "drone_yaw", "drone_spin_release", "drone_angular_brake"})
    BOUNDS(runs, s, "finite", 1, 1);
  // the rate asked for, on every car, from 0.5 s on, and never the cap
  float pitch = a.maxRate.x * kDeg, roll = a.maxRate.y * kDeg, yaw = a.maxRate.z * kDeg;
  BOUNDS(runs, "drone_pitch", "rate_0.5s", 0.98f * pitch, 1.02f * pitch);
  BOUNDS(runs, "drone_pitch", "rate_3s", 0.98f * pitch, 1.02f * pitch);
  BOUNDS(runs, "drone_roll", "rate_0.5s", 0.98f * roll, 1.02f * roll);
  BOUNDS(runs, "drone_roll", "rate_3s", 0.98f * roll, 1.02f * roll);
  BOUNDS(runs, "drone_yaw", "rate_0.5s", 0.98f * yaw, 1.02f * yaw);
  BOUNDS(runs, "drone_yaw", "rate_3s", 0.98f * yaw, 1.02f * yaw);
  for (const char *s : {"drone_pitch", "drone_roll", "drone_yaw"}) {
    BOUNDS(runs, s, "max_spin", 0, 1.05f * std::fmax(pitch, std::fmax(roll, yaw)));
    // let go: stopped within 0.3 s, under 15 deg further
    BOUNDS(runs, s, "stop_time", 0, 0.3f);
    BOUNDS(runs, s, "coast_after_release", 0, 15);
  }
  BOUNDS(runs, "drone_spin_release", "stop_time", 0, 0.3f);
  BOUNDS(runs, "drone_spin_release", "spin_after_2s", 0, 1);
  BOUNDS(runs, "drone_angular_brake", "stop_time_brake", 0, 0.3f);
}

namespace {
// The feel spec (docs/design/flight-modes.md, Feel spec) on Standard at one Handling choice.
void StandardSpec(float aHandling) {
  auto runs = FlyFleet(Mode::Standard, {"yaw_hold", "knock_roll_yaw", "knock_pitch_yaw", "tilt_recovery",
                                      "tilt_recovery_yaw", "knock_low", "surge_turn", "climb_short", "climb_long"},
                       aHandling);
  // hold a turn: a steady 85 to 95 deg/s
  BOUNDS(runs, "yaw_hold", "yaw_rate_1s", 85, 95);
  BOUNDS(runs, "yaw_hold", "yaw_rate_3s", 85, 95);
  // knocked with a turn held: level within 2.5 s
  BOUNDS(runs, "knock_roll_yaw", "level_time", 0, 2.5f);
  BOUNDS(runs, "knock_pitch_yaw", "level_time", 0, 2.5f);
  // let go tilted, from any attitude, a turn held or not: level within 3 s, never over
  BOUNDS(runs, "tilt_recovery", "worst_level_time", 0, 3);
  BOUNDS(runs, "tilt_recovery_yaw", "worst_level_time", 0, 3);
  BOUNDS(runs, "tilt_recovery", "flipped_over", 0, 0);
  BOUNDS(runs, "tilt_recovery_yaw", "flipped_over", 0, 0);
  // knocked hovering low: back above the hover band's minimum within 1.5 s
  BOUNDS(runs, "knock_low", "back_above_min_time", 0, 1.5f);
  // a turn at speed: the velocity within 15 deg of the nose
  BOUNDS(runs, "surge_turn", "max_slip", 0, 15);
  // a climb let go: stops within 2 m
  BOUNDS(runs, "climb_short", "overshoot_after_release", -2, 2);
  BOUNDS(runs, "climb_long", "overshoot_after_release", -2, 2);
}
} // namespace

TEST(feel_standard_fleet) { StandardSpec(1.0f); }

TEST(feel_handling_loose) { StandardSpec(0.0f); }

TEST(feel_handling_tight) { StandardSpec(2.0f); }
