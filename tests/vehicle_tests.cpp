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

TEST(vehicle_lateral_drag_leaves) {
  // the drifted nose turns the car with the surge the drag leaves: at rest the speed-free bound, at the speed full surge
  // holds only the sway and the lift's tilt, and in between less the faster, never more than the bound
  VehicleSpec spec;
  spec.drone = true;
  const float top = FlownTop(spec);
  CHECK(top > 0.0f && top <= spec.speedCap);
  CHECK_NEAR(Lateral(spec, 0.0f), Lateral(spec), 1e-4f);
  CHECK_NEAR(LateralAt(spec, 9.81f, 0.0f), LateralAt(spec, 9.81f), 1e-4f);
  if (top < spec.speedCap) {
    CHECK_NEAR(Lateral(spec, top), DroneAccel(spec, spec.sway), 1e-3f);
  }
  float last = Lateral(spec, 0.0f);
  for (float v = 10.0f; v <= 120.0f; v += 10.0f) {
    const float here = Lateral(spec, v);
    CHECK(here <= last + 1e-5f);
    CHECK(here >= DroneAccel(spec, spec.sway) - 1e-5f);
    last = here;
  }
}

TEST(vehicle_flown_top) {
  // full surge against the drag, past TopSpeed's planned share of it, and the cap with no drag
  VehicleSpec spec;
  CHECK(FlownTop(spec) >= TopSpeed(spec));
  spec.drag = false;
  CHECK_NEAR(FlownTop(spec), spec.speedCap, 1e-4f);
}

TEST(vehicle_drag_at_slope) {
  // drone style with a roof area, a climb or a dive drags with the front and the roof in the path's proportions;
  // level it is Drag, and without a roof area (or plane style) every slope is Drag
  VehicleSpec spec;
  spec.cdaFront = 0.8f;
  spec.cdaTop = 12.0f;
  CHECK_NEAR(DragAt(spec, 50.0f, 0.0f), Drag(spec, 50.0f), 1e-4f);
  const float k = 0.5f * 1.2f * spec.dragK / spec.mass;
  CHECK_NEAR(DragAt(spec, 50.0f, 1.0f), k * 12.0f * 2500.0f, 1e-3f);
  CHECK_NEAR(DragAt(spec, 50.0f, -0.6f), k * (0.8f * 0.64f + 12.0f * 0.36f) * 2500.0f, 1e-3f);
  CHECK(TopAt(spec, 0.7f) < TopAt(spec, 0.3f));
  CHECK_NEAR(TopAt(spec, 0.0f), TopSpeed(spec), 1e-3f);
  CHECK(AccelAt(spec, 40.0f, 0.5f) < Accel(spec, 40.0f));
  spec.drone = false;
  CHECK_NEAR(DragAt(spec, 50.0f, 1.0f), Drag(spec, 50.0f), 1e-4f);
  spec.drone = true;
  spec.cdaTop = 0.0f;
  CHECK_NEAR(DragAt(spec, 50.0f, 1.0f), Drag(spec, 50.0f), 1e-4f);
}
