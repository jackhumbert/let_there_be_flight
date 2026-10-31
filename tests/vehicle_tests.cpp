// The car's limits (src/red4ext/Navigation/Vehicle): the drone body's tilt for a thrust direction.
#include "check.hpp"

#include "Navigation/Vehicle.hpp"

#include <cmath>

using namespace Navigation;

TEST(vehicle_body_angle_share) {
  // the thrusters reach further (asin(0.85 * 15 / 20) = 0.691) than the rest of the angle (0.2): the body takes its share
  const VehicleSpec spec;
  CHECK_NEAR(DroneBodyAngle(spec, 0.4f, 20.0f, 15.0f, 0.5f), 0.2f, 1e-5f);
}

TEST(vehicle_body_angle_past_reach) {
  // the thrusters reach asin(0.85 * 5 / 20) = 0.2141 only: the body takes the rest, on either side
  const VehicleSpec spec;
  CHECK_NEAR(DroneBodyAngle(spec, 1.0f, 20.0f, 5.0f, 0.5f), 0.7858673f, 1e-5f);
  CHECK_NEAR(DroneBodyAngle(spec, -1.0f, 20.0f, 5.0f, 0.5f), -0.7858673f, 1e-5f);
}

TEST(vehicle_body_angle_small_thrust) {
  // a thrust under 0.1 is taken as 0.1: reach asin(0.85 * 0.01 / 0.1) = 0.0851 (asin(0.17) were it not), and with
  // no thrust at all the result is still a number
  const VehicleSpec spec;
  CHECK_NEAR(DroneBodyAngle(spec, 1.0f, 0.05f, 0.01f, 0.5f), 1.0f - std::asin(0.085f), 1e-5f);
  CHECK_NEAR(DroneBodyAngle(spec, 0.7f, 0.0f, 0.0f, 0.5f), 0.7f, 1e-5f);
}

TEST(vehicle_body_angle_margin) {
  // the planned share of the thrusters is the spec's margin: reach asin(0.5 * 5 / 20) = 0.1253
  VehicleSpec spec;
  spec.margin = 0.5f;
  CHECK_NEAR(DroneBodyAngle(spec, 1.0f, 20.0f, 5.0f, 0.5f), 1.0f - std::asin(0.125f), 1e-5f);
}

TEST(vehicle_body_angle_no_thrusters) {
  // no thrusters across the body: it takes all the angle
  const VehicleSpec spec;
  CHECK_NEAR(DroneBodyAngle(spec, 0.7f, 20.0f, 0.0f, 0.5f), 0.7f, 1e-5f);
}
