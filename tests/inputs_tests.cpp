// The audio inputs (src/red4ext/Audio/Inputs): the rules of FlightComponent.UpdateAudioParams and
// IFlightThruster.Update, with the numbers worked out by hand from the redscript.
#include "check.hpp"

#include "Audio/Inputs/Inputs.hpp"

#include <cmath>
#include <string>

using namespace Inputs;

namespace {
Frame PlayerFrame() {
  Frame f;
  f.pilot = Pilot::Player;
  f.ctlSurge = 0.5f;
  f.ctlYaw = -0.3f;
  f.ctlSway = 0.2f;
  f.ctlLift = 0.4f;
  f.roll = 0.1f;
  f.pitch = -0.2f;
  f.force = {0, 0, 20};
  f.linearBrake = 0.3f;
  f.angularBrake = 0.6f;
  return f;
}

// the rotation of quaternion (x, y, z, w) applied to v
Vec3 Rotate(const float q[4], Vec3 v) {
  const float x = q[0], y = q[1], z = q[2], w = q[3];
  const Vec3 u{x, y, z};
  const float d = u.x * v.x + u.y * v.y + u.z * v.z;
  const Vec3 c{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x};
  return {2 * d * u.x + (w * w - (x * x + y * y + z * z)) * v.x + 2 * w * c.x,
          2 * d * u.y + (w * w - (x * x + y * y + z * z)) * v.y + 2 * w * c.y,
          2 * d * u.z + (w * w - (x * x + y * y + z * z)) * v.z + 2 * w * c.z};
}
} // namespace

TEST(inputs_player_hover) {
  Values v;
  Update(PlayerFrame(), v);
  CHECK_NEAR(v.surge, 0.5, 1e-5);
  CHECK_NEAR(v.yaw, -0.3, 1e-5);
  CHECK_NEAR(v.sway, 0.2, 1e-5);
  CHECK_NEAR(v.lift, 0.5, 1e-5);
  CHECK_NEAR(v.roll, 0.1, 1e-5);
  CHECK_NEAR(v.pitch, -0.2, 1e-5);
  CHECK_NEAR(v.brake, 0.6, 1e-5);
}

TEST(inputs_player_drone) {
  Frame f = PlayerFrame();
  f.drone = true;
  f.ctlRoll = 0.7f;
  f.ctlPitch = -0.4f;
  Values v;
  Update(f, v);
  CHECK_NEAR(v.lift, 0.4, 1e-5);
  CHECK_NEAR(v.roll, 0.7, 1e-5);
  CHECK_NEAR(v.pitch, -0.4, 1e-5);
}

TEST(inputs_autopilot) {
  Frame f;
  f.pilot = Pilot::Autopilot;
  f.apSurge = 0.8f;
  f.apSway = -0.1f;
  f.force = {0, 0, 15};
  f.gravityFactor = 1;
  f.standardModeLiftFactor = 25;
  f.angularVelocity = {0.5f, -3, 2};
  Values v;
  Update(f, v);
  CHECK_NEAR(v.surge, 0.8, 1e-5);
  CHECK_NEAR(v.sway, -0.1, 1e-5);
  CHECK_NEAR(v.lift, (15 - 9.81) / 25, 1e-5);
  CHECK_NEAR(v.pitch, 0.5, 1e-5);
  CHECK_NEAR(v.roll, -1, 1e-5);
  CHECK_NEAR(v.yaw, 1, 1e-5);
}

TEST(inputs_npc) {
  Frame f;
  f.pilot = Pilot::Npc;
  f.surge = 0.6f;
  f.lift = 0.2f;
  Values v;
  v.sway = 0.7f;
  v.inside = 0.5f;
  Update(f, v);
  CHECK_NEAR(v.sway, 0.7, 1e-5);
  CHECK_NEAR(v.surge, 0.6, 1e-5);
  CHECK_NEAR(v.lift, 0.2, 1e-5);
  CHECK_NEAR(v.inside, 0, 1e-5);
}

TEST(inputs_destroyed) {
  Frame f;
  Values v;
  f.destroyed = true;
  Update(f, v);
  CHECK_NEAR(v.damage, 0.25, 1e-5);
  f.destroyed = false;
  f.healthPercent = 40;
  Update(f, v);
  CHECK_NEAR(v.damage, 0.12, 1e-5);
}

TEST(inputs_collision_recovery) {
  Frame f;
  f.pilot = Pilot::Player;
  f.ctlSurge = 1;
  f.recoveryDelay = 1;
  f.recoveryDuration = 2;
  Values v;
  f.collisionTimer = 2;
  CHECK_NEAR(CollisionRatio(2, 1, 2), 0.5, 1e-5);
  Update(f, v);
  CHECK_NEAR(v.surge, 0.5, 1e-5);
  f.collisionTimer = 0.5f;
  Update(f, v);
  CHECK_NEAR(v.surge, 0, 1e-5);
  f.collisionTimer = 3;
  Update(f, v);
  CHECK_NEAR(v.surge, 1, 1e-5);
}

TEST(inputs_inside_fade) {
  Frame f;
  f.pilot = Pilot::Player;
  f.outside = false;
  f.dt = 0.1f;
  Values v;
  Update(f, v);
  CHECK_NEAR(v.inside, 0.4, 1e-5);
  Update(f, v);
  Update(f, v);
  CHECK_NEAR(v.inside, 1.0, 1e-5);
  f.outside = true;
  Update(f, v);
  CHECK_NEAR(v.inside, 0.6, 1e-5);
}

TEST(inputs_partial_frame) {
  Frame f = PlayerFrame();
  Values v;
  Update(f, v);
  Frame p;
  p.pilot = Pilot::Player;
  p.full = false;
  p.speed = 33;
  p.outside = false;
  p.dt = 0.1f;
  p.forward = {0, 1, 0};
  p.direction = {-1, 1, 0};
  Update(p, v);
  CHECK_NEAR(v.surge, 0.5, 1e-5);
  CHECK_NEAR(v.damage, 0, 1e-5);
  CHECK_NEAR(v.brake, 0.6, 1e-5);
  CHECK_NEAR(v.speed, 33, 1e-5);
  CHECK_NEAR(v.yawDiff, 45, 1e-3);
  CHECK_NEAR(v.inside, 0.4, 1e-5);
}

TEST(inputs_angle_around) {
  const float r = std::sqrt(0.5f);
  CHECK_NEAR(AngleAroundDeg({0, 1, 0}, {-r, r, 0}, {0, 0, 1}), 45, 1e-3);
  CHECK_NEAR(AngleAroundDeg({0, 1, 0}, {0, r, r}, {1, 0, 0}), 45, 1e-3);
  CHECK_NEAR(AngleAroundDeg({0, 1, 0}, {0, r, -r}, {1, 0, 0}), -45, 1e-3);
  CHECK_NEAR(AngleAroundDeg({0, 0, 1}, {0, 1, 1}, {0, 0, 1}), 0, 1e-3);
}

TEST(inputs_cone_axes) {
  Axes mesh{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  Axes cone = ConeAxes(mesh);
  CHECK_NEAR(cone.forward.x, 0, 1e-5);
  CHECK_NEAR(cone.forward.y, 0, 1e-5);
  CHECK_NEAR(cone.forward.z, -1, 1e-5);
  CHECK_NEAR(cone.up.x, 0, 1e-5);
  CHECK_NEAR(cone.up.y, 1, 1e-5);
  CHECK_NEAR(cone.up.z, 0, 1e-5);

  // a mesh yawed 30 degrees about z, against q_mesh * (-sqrt(1/2), 0, 0, sqrt(1/2))
  const float yaw = 30.0f * 3.14159265358979f / 180.0f;
  const float c = std::cos(yaw), s = std::sin(yaw);
  mesh = {{c, s, 0}, {-s, c, 0}, {0, 0, 1}};
  cone = ConeAxes(mesh);
  const float h = std::sqrt(0.5f);
  const float a[4] = {0, 0, std::sin(yaw / 2), std::cos(yaw / 2)};
  const float b[4] = {-h, 0, 0, h};
  const float q[4] = {a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
                      a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
                      a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
                      a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]};
  const Vec3 right = Rotate(q, {1, 0, 0}), forward = Rotate(q, {0, 1, 0}), up = Rotate(q, {0, 0, 1});
  CHECK_NEAR(cone.right.x, right.x, 1e-5);
  CHECK_NEAR(cone.right.y, right.y, 1e-5);
  CHECK_NEAR(cone.right.z, right.z, 1e-5);
  CHECK_NEAR(cone.forward.x, forward.x, 1e-5);
  CHECK_NEAR(cone.forward.y, forward.y, 1e-5);
  CHECK_NEAR(cone.forward.z, forward.z, 1e-5);
  CHECK_NEAR(cone.up.x, up.x, 1e-5);
  CHECK_NEAR(cone.up.y, up.y, 1e-5);
  CHECK_NEAR(cone.up.z, up.z, 1e-5);
}

TEST(inputs_thruster_volume) {
  CHECK_NEAR(ThrusterVolume(false, 50), 0.5, 1e-5);
  CHECK_NEAR(ThrusterVolume(false, 150), 1, 1e-5);
  CHECK_NEAR(ThrusterVolume(true, 0), 1, 1e-5);
}

TEST(inputs_game_volume) {
  CHECK_NEAR(GameVolume(false, false, false, false, 80, 50), 0.4, 1e-5);
  CHECK_NEAR(GameVolume(false, false, false, true, 80, 50), 0.04, 1e-5);
  CHECK_NEAR(GameVolume(true, false, false, false, 80, 50), 0, 1e-5);
  CHECK_NEAR(CarPitch(1400), 1.0, 1e-5);
  CHECK_NEAR(CarPitch(350), 1.5, 1e-5);
  CHECK_NEAR(CarPitch(7000), 0.6, 1e-5);
}

TEST(inputs_value_names) {
  const char *expected[13] = {"speed", "surge", "yawDiff", "lift", "yaw", "pitchDiff", "brake",
                              "inside", "damage", "roll", "pitch", "sway", "scrape"};
  Values v;
  v.speed = 1, v.surge = 2, v.yawDiff = 3, v.lift = 4, v.yaw = 5, v.pitchDiff = 6, v.brake = 7;
  v.inside = 8, v.damage = 9, v.roll = 10, v.pitch = 11, v.sway = 12, v.scrape = 13;
  for (int i = 0; i < 13; ++i) {
    CHECK(std::string(kNames[i]) == expected[i]);
    CHECK_NEAR(Get(v, i), i + 1, 1e-6);
  }
}

TEST(inputs_to_synth) {
  Values v;
  v.speed = 1, v.surge = 2, v.lift = 3, v.pitch = 4, v.roll = 5, v.sway = 6, v.yaw = 7;
  v.brake = 8, v.damage = 9, v.inside = 10, v.scrape = 11;
  // the values the synth does not take stay out
  v.yawDiff = 99, v.pitchDiff = 98;
  const EngineSynth::Inputs in = ToSynth(v, 0.25f, 0.75f, 120.0f);
  CHECK_NEAR(in.speed, 1, 0);
  CHECK_NEAR(in.surge, 2, 0);
  CHECK_NEAR(in.lift, 3, 0);
  CHECK_NEAR(in.pitch, 4, 0);
  CHECK_NEAR(in.roll, 5, 0);
  CHECK_NEAR(in.sway, 6, 0);
  CHECK_NEAR(in.yaw, 7, 0);
  CHECK_NEAR(in.brake, 8, 0);
  CHECK_NEAR(in.damage, 9, 0);
  CHECK_NEAR(in.inside, 10, 0);
  CHECK_NEAR(in.scrape, 11, 0);
  CHECK_NEAR(in.volume, 0.25, 0);
  CHECK_NEAR(in.water, 0.75, 0);
  CHECK_NEAR(in.cone, 120, 0);
}
