// A trajectory read at a distance (src/red4ext/Navigation/Trajectory): SampleAt.
#include "check.hpp"

#include "Navigation/Trajectory.hpp"

using namespace Navigation;

namespace {
// two samples, 10 m apart: 10 then 20 m/s, yaw 3 then -3 rad (across the wrap), Turn then Brake
Trajectory Two() {
  Trajectory t;
  TrajectorySample a, b;
  a.s = 0;
  a.speed = 10;
  a.yaw = 3.0f;
  a.limit = TrajectorySample::Limit::Turn;
  b.s = 10;
  b.speed = 20;
  b.yaw = -3.0f;
  b.position = {10, 0, 0};
  b.limit = TrajectorySample::Limit::Brake;
  t.samples = {a, b};
  return t;
}
} // namespace

TEST(trajectory_sample_lerp) {
  Trajectory t = Two();
  TrajectoryPoint p = SampleAt(t, 5);
  CHECK_NEAR(p.speed, 15, 1e-5);
  CHECK_NEAR(p.position.x, 5, 1e-5);
  CHECK(p.limit == TrajectorySample::Limit::Brake); // t 0.5 is not below 0.5
  CHECK(SampleAt(t, 4.9f).limit == TrajectorySample::Limit::Turn);
}

TEST(trajectory_sample_yaw_short_way) {
  // 3.0 + 0.5 * (-6 + 2 pi): through pi, not through 0
  CHECK_NEAR(SampleAt(Two(), 5).yaw, 3.1415927, 1e-5);
}

TEST(trajectory_sample_clamped) {
  Trajectory t = Two();
  CHECK_NEAR(SampleAt(t, -3).speed, 10, 1e-5);
  CHECK_NEAR(SampleAt(t, 99).speed, 20, 1e-5);
  CHECK_NEAR(SampleAt(Trajectory{}, 5).speed, 0, 1e-5);
}

TEST(trajectory_sample_three) {
  // past the second of three samples: i 1, t 0.5, every field halfway between the second and third
  Trajectory t = Two();
  TrajectorySample &b = t.samples[1];
  b.yaw = 0.0f;
  b.t = 1.0f;
  TrajectorySample c;
  c.s = 30;
  c.t = 3.0f;
  c.speed = 40;
  c.along = 2.0f;
  c.yaw = 1.0f;
  c.pitch = 0.4f;
  c.roll = -0.2f;
  c.drift = 0.6f;
  c.position = {10, 20, 0};
  c.velocity = {0, 40, 0};
  c.acceleration = {-2, 4, 6};
  c.limit = TrajectorySample::Limit::Top;
  t.samples.push_back(c);
  TrajectoryPoint p = SampleAt(t, 20);
  CHECK_NEAR(p.s, 20, 1e-5);
  CHECK_NEAR(p.t, 2.0, 1e-5);
  CHECK_NEAR(p.speed, 30, 1e-5);
  CHECK_NEAR(p.along, 1.0, 1e-5);
  CHECK_NEAR(p.yaw, 0.5, 1e-5);
  CHECK_NEAR(p.pitch, 0.2, 1e-5);
  CHECK_NEAR(p.roll, -0.1, 1e-5);
  CHECK_NEAR(p.drift, 0.3, 1e-5);
  CHECK_NEAR(p.position.x, 10, 1e-5);
  CHECK_NEAR(p.position.y, 10, 1e-5);
  CHECK_NEAR(p.velocity.y, 20, 1e-5);
  CHECK_NEAR(p.acceleration.x, -1, 1e-5);
  CHECK_NEAR(p.acceleration.y, 2, 1e-5);
  CHECK_NEAR(p.acceleration.z, 3, 1e-5);
  CHECK(p.limit == TrajectorySample::Limit::Top);
}

TEST(trajectory_sample_empty) {
  // fewer than two samples: the zero point, whose limit is Top (FlightTrajectory checks IsValid first)
  Trajectory one;
  one.samples.resize(1);
  one.samples[0].speed = 5;
  TrajectoryPoint p = SampleAt(one, 3);
  CHECK_NEAR(p.speed, 0, 1e-6);
  CHECK_NEAR(p.position.x, 0, 1e-6);
  CHECK(p.limit == TrajectorySample::Limit::Top);
}
