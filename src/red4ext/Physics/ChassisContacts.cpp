#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <cmath>
#include <spdlog/spdlog.h>

// Chassis contact handling for flying vehicles.
//
// Vehicles integrate their own rigid body (vehicle::RigidBody) at a fixed 60 Hz: every step
// applies gravity and solves *wheel* contacts only, then integrates the pose. PhysX resolves
// chassis contacts asynchronously and the result only comes back once per frame, in
// PhysXSynchronizer::SyncFromPhysX, as a velocity impulse (PhysX velocity minus the velocity
// that was pushed to it). In flight the wheels do not hold the vehicle up, so a vehicle on its
// body (a landed drone, a car on its roof, skidding along the ground) sank by gravity every
// step, was pushed back out once per frame and jittered; scraping over mesh seams could also
// return a large depenetration impulse that launched it.
//
// Two fixes, both only while flight is active:
// 1. SyncFromPhysX: the correction along its own direction is limited to the approach speed
//    plus a small bounce, so a contact can stop the vehicle but not launch it.
// 2. The correction is also an impulse J applied at the contact point r: J = m * dv and
//    I * dw = r x J, so r (perpendicular to J) = (J x I*dw) / |J|^2. That gives a smoothed
//    support normal and contact offset. Every fixed step (RigidBody::IntegrateMotion), the
//    velocity into the surface is limited to a small creep, weighted down as the contact
//    moves off the centre of mass (a corner touching down pivots and falls freely). Once
//    PhysX stops reporting the contact (off a ledge) it lapses and the vehicle falls.
//    Applying it as an impulse at r with an angular part rocked the vehicle diagonally:
//    SyncFromPhysX's spin change also contains PushToPhysX's orientation chase, so r is
//    only good enough as a weight.
// See docs/drone-ground-jitter-analysis.md for the engine call order this relies on.

namespace {
using RED4ext::Vector3;

// velocity change from SyncFromPhysX below this is not treated as a contact
constexpr float kSupportThreshold = 0.03f;
// frames a support normal stays valid without PhysX reporting it again
constexpr uint32_t kSupportFrames = 3;
// allowed speed of the contact point into the surface, keeps PhysX reporting the contact
constexpr float kCreepSpeed = 0.05f;
// a correction only counts as support if it points at least this much up: corrections also
// carry friction impulses in arbitrary horizontal directions
constexpr float kMinSupportUp = 0.3f;
// per-frame blend of a new correction into the cached normal and contact offset
constexpr float kNormalBlend = 0.1f;
constexpr float kOffsetBlend = 0.3f;
// contact offsets further than this from the centre of mass are estimation noise
constexpr float kMaxOffset = 3.0f;
// contact offset at which the support has faded out completely (a corner far from the
// centre of mass must be free to pivot and fall)
constexpr float kFullReleaseOffset = 1.5f;
// launch guard: a correction may remove the approach speed (with this much restitution)
// plus kLaunchMargin, nothing more
constexpr float kRestitution = 0.2f;
constexpr float kLaunchMargin = 0.5f;

float Dot(const Vector3 &a, const Vector3 &b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }
Vector3 Cross(const Vector3 &a, const Vector3 &b) {
  return Vector3(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);
}
Vector3 Scale(const Vector3 &a, float s) { return Vector3(a.X * s, a.Y * s, a.Z * s); }
Vector3 Add(const Vector3 &a, const Vector3 &b) { return Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z); }
Vector3 Sub(const Vector3 &a, const Vector3 &b) { return Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z); }
Vector3 Lerp(const Vector3 &a, const Vector3 &b, float t) { return Add(a, Scale(Sub(b, a), t)); }
// inertia tensors are symmetric, so row/column order does not matter
Vector3 Mul(const RED4ext::Matrix &m, const Vector3 &v) {
  return Vector3(m.X.X * v.X + m.Y.X * v.Y + m.Z.X * v.Z, m.X.Y * v.X + m.Y.Y * v.Y + m.Z.Y * v.Z,
                 m.X.Z * v.X + m.Y.Z * v.Y + m.Z.Z * v.Z);
}

FlightComponent *GetActiveFlightComponent(RED4ext::vehicle::PhysicsData *rb) {
  if (!rb || !rb->vehicle) {
    return nullptr;
  }
  auto fc = FlightComponent::Get(rb->vehicle);
  return (fc && fc->active) ? fc : nullptr;
}
} // namespace

// void vehicle::PhysXSynchronizer::SyncFromPhysX(ChassisComponent &, RigidBody &, float, float)
// 2.31: 0x1401A56DC
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 547299556, SyncFromPhysX, void *self, void *chassis,
                          RED4ext::vehicle::PhysicsData *rb, float a4, float consumedDt) {
  auto v0 = rb ? rb->velocity : Vector3();
  auto w0 = rb ? rb->angularVelocity : Vector3();
  SyncFromPhysX_Original(self, chassis, rb, a4, consumedDt);

  auto fc = GetActiveFlightComponent(rb);
  if (!fc) {
    return;
  }
  auto dv = Sub(rb->velocity, v0);
  auto dw = Sub(rb->angularVelocity, w0);
  float len = std::sqrt(Dot(dv, dv));
  if (!std::isfinite(len) || len <= kSupportThreshold) {
    if (fc->framesSinceSupport < 1000) {
      fc->framesSinceSupport++;
    }
    return;
  }
  auto dir = Scale(dv, 1.0f / len);

  // 1. launch guard
  float approach = -Dot(v0, dir);
  float allowed = std::fmax(approach, 0.0f) * (1.0f + kRestitution) + kLaunchMargin;
  if (len > allowed) {
    float s = allowed / len;
    dv = Scale(dv, s);
    dw = Scale(dw, s);
    rb->velocity = Add(v0, dv);
    rb->angularVelocity = Add(w0, dw);
  }

  // 2. support normal and contact offset
  if (dir.Z <= kMinSupportUp || rb->inverseMass <= 0.0f) {
    if (fc->framesSinceSupport < 1000) {
      fc->framesSinceSupport++;
    }
    return;
  }
  auto J = Scale(dv, 1.0f / rb->inverseMass);
  auto T = Mul(rb->worldInertiaTensor, dw);
  float jj = Dot(J, J);
  auto r = jj > 0.000001f ? Scale(Cross(J, T), 1.0f / jj) : Vector3();
  float rl = std::sqrt(Dot(r, r));
  if (!std::isfinite(rl)) {
    r = Vector3();
  } else if (rl > kMaxOffset) {
    r = Scale(r, kMaxOffset / rl);
  }

  bool fresh = fc->framesSinceSupport > kSupportFrames;
  auto n = Lerp(fresh ? Vector3(0.0f, 0.0f, 1.0f) : fc->supportNormal, dir, kNormalBlend);
  float nl = std::sqrt(Dot(n, n));
  if (nl > 0.0001f) {
    fc->supportNormal = Scale(n, 1.0f / nl);
  }
  fc->supportOffset = fresh ? r : Lerp(fc->supportOffset, r, kOffsetBlend);
  fc->framesSinceSupport = 0;
}

// void vehicle::RigidBody::IntegrateMotion(float)
// 2.31: 0x1406F8F40, runs once per fixed step after gravity and the wheel contact solve
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 2021199885, RigidBody_IntegrateMotion, RED4ext::vehicle::PhysicsData *rb,
                          float deltaTime) {
  auto fc = GetActiveFlightComponent(rb);
  if (fc && fc->framesSinceSupport <= kSupportFrames) {
    // Linear only, at the centre of mass. Applying the impulse at the estimated contact
    // point (with an angular part) fed back into PushToPhysX's orientation chase and rocked
    // the vehicle diagonally, so the offset only weights the support: full when the contact
    // is under the centre of mass, none for a corner far off it, which then pivots and falls
    // freely.
    const auto &n = fc->supportNormal;
    const auto &r = fc->supportOffset;
    float offCentre = std::sqrt(Dot(r, r));
    float weight = 1.0f - std::fmin(offCentre / kFullReleaseOffset, 1.0f);
    float vn = Dot(rb->velocity, n);
    if (weight > 0.0f && vn < -kCreepSpeed) {
      rb->velocity = Sub(rb->velocity, Scale(n, (vn + kCreepSpeed) * weight));
    }
  }
  RigidBody_IntegrateMotion_Original(rb, deltaTime);
}
