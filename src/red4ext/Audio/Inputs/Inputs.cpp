#include "Inputs.hpp"

namespace Inputs {

namespace {
float Dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 Sub(Vec3 a, Vec3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 Scale(Vec3 a, float s) {
  return {a.x * s, a.y * s, a.z * s};
}

float Length(Vec3 a) {
  return std::sqrt(Dot(a, a));
}

float Clamp(float v, float lo, float hi) {
  return std::min(std::max(v, lo), hi);
}
} // namespace

float CollisionRatio(float timer, float delay, float duration) {
  return timer < delay + duration ? std::max(0.0f, (timer - delay) / duration) : 1.0f;
}

float AngleAroundDeg(Vec3 a, Vec3 b, Vec3 axis) {
  const float axisLength = Length(axis);
  if (axisLength <= 0)
    return 0;
  const Vec3 n = Scale(axis, 1.0f / axisLength);
  const Vec3 pa = Sub(a, Scale(n, Dot(a, n)));
  const Vec3 pb = Sub(b, Scale(n, Dot(b, n)));
  if (Length(pa) < 1e-6f || Length(pb) < 1e-6f)
    return 0;
  const float radians = std::atan2(Dot(n, Cross(pa, pb)), Dot(pa, pb));
  return radians * (180.0f / 3.14159265358979f);
}

void Update(const Frame &f, Values &v) {
  if (f.full) {
    const float ratio = CollisionRatio(f.collisionTimer, f.recoveryDelay, f.recoveryDuration);
    v.damage = f.destroyed ? 0.25f : (1.0f - f.healthPercent / 100.0f) * 0.2f;
    const float forceLength = Length(f.force);
    const Vec3 n = forceLength > 0 ? Scale(f.force, 1.0f / forceLength) : Vec3{};
    switch (f.pilot) {
    case Pilot::Autopilot: {
      const float g = 9.81f * f.gravityFactor;
      const Vec3 &w = f.angularVelocity;
      v.surge = f.apSurge * ratio;
      v.sway = f.apSway * ratio;
      v.lift = Clamp((f.force.z - g) / std::max(f.standardModeLiftFactor, 1.0f), -1, 1) * ratio;
      v.yaw = Clamp(w.z, -1, 1) * ratio;
      v.pitch = Clamp(w.x, -1, 1) * ratio;
      v.roll = Clamp(w.y / 1.5f, -1, 1) * ratio;
      break;
    }
    case Pilot::Player:
      v.surge = f.ctlSurge * ratio;
      v.yaw = f.ctlYaw * ratio;
      v.sway = f.ctlSway * ratio;
      if (f.drone) {
        v.lift = f.ctlLift * ratio;
        v.roll = f.ctlRoll * ratio;
        v.pitch = f.ctlPitch * ratio;
      } else {
        v.lift = (Dot(n, f.localUp) * 0.1f + f.ctlLift) * ratio;
        v.roll = f.roll * ratio;
        v.pitch = f.pitch * ratio;
      }
      break;
    case Pilot::Npc:
      v.surge = f.surge * ratio;
      v.lift = (Dot(n, f.localUp) * 0.1f + f.lift) * ratio;
      v.roll = f.roll * ratio;
      v.pitch = f.pitch * ratio;
      v.yaw = f.yaw * ratio;
      break;
    }
    // the script reads the player's controller for NPCs too; kept as is
    v.brake = std::max(f.linearBrake, f.angularBrake);
  }

  v.speed = f.speed;
  v.yawDiff = AngleAroundDeg(f.forward, f.direction, f.up);
  v.pitchDiff = AngleAroundDeg(f.forward, f.direction, f.right);
  if (f.pilot == Pilot::Npc)
    v.inside = 0;
  else
    v.inside = f.outside ? std::max(0.0f, v.inside - 4.0f * f.dt) : std::min(1.0f, v.inside + 4.0f * f.dt);
}

float ThrusterVolume(bool front, float speed) {
  return front ? 1.0f : Clamp(speed / 100.0f, 0, 1);
}

Axes ConeAxes(const Axes &mesh) {
  return {mesh.right, {-mesh.up.x, -mesh.up.y, -mesh.up.z}, mesh.forward};
}

EngineSynth::Inputs ToSynth(const Values &v, float volume, float water, float cone) {
  EngineSynth::Inputs in;
  in.speed = v.speed;
  in.surge = v.surge;
  in.lift = v.lift;
  in.pitch = v.pitch;
  in.roll = v.roll;
  in.sway = v.sway;
  in.yaw = v.yaw;
  in.brake = v.brake;
  in.damage = v.damage;
  in.inside = v.inside;
  in.volume = volume;
  in.water = water;
  in.cone = cone;
  return in;
}

float CarPitch(float mass) {
  return Clamp(700.0f / mass + 0.5f, 0.5f, 1.5f);
}

float GameVolume(bool inMenu, bool hubMenu, bool worldMap, bool radialMenu, int master, int sfx) {
  if (inMenu || hubMenu || worldMap)
    return 0;
  return float(master) / 100.0f * float(sfx) / 100.0f * (radialMenu ? 0.1f : 1.0f);
}

Vec3 ListenerVelocity(Vec3 aVelocity, Vec3 aFrom, Vec3 aTo, float aDt) {
  if (aDt <= 0.0f) {
    return aVelocity;
  }
  const Vec3 v{(aTo.x - aFrom.x) / aDt, (aTo.y - aFrom.y) / aDt, (aTo.z - aFrom.z) / aDt};
  // set to zero at a jump, the cinematic camera's wind fell from its airspeed to the city's wind for a frame at every
  // cut, 30 dB, and thumped as it came back
  if (std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) > 500.0f) {
    return aVelocity;
  }
  const float k = 1.0f - std::exp(-aDt / 0.1f);
  return {aVelocity.x + k * (v.x - aVelocity.x), aVelocity.y + k * (v.y - aVelocity.y), aVelocity.z + k * (v.z - aVelocity.z)};
}

} // namespace Inputs
