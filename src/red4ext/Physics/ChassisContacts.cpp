#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <cmath>
#include <spdlog/spdlog.h>

// Chassis contact support for flying vehicles.
//
// Vehicles integrate their own rigid body (vehicle::RigidBody) at a fixed 60 Hz: every step
// applies gravity and solves *wheel* contacts only, then integrates the pose. PhysX resolves
// chassis contacts asynchronously and the result only comes back once per frame, in
// PhysXSynchronizer::SyncFromPhysX, as a velocity impulse (PhysX velocity minus the velocity
// that was pushed to it). Between those, nothing constrains the chassis, so a vehicle resting
// on its body (in flight its wheels do not hold it up: a landed drone, a car on its roof or a
// curb) sinks by gravity every step, gets pushed back out once per frame, keeps a phantom
// downward velocity of about g*dt and visibly jitters.
//
// SyncFromPhysX's velocity change is the contact correction; its upward-pointing part,
// smoothed over frames, is the support normal. While PhysX keeps reporting support, each fixed step clamps the velocity into that
// surface to a small creep instead of letting gravity accumulate. The creep keeps the contact
// (and therefore the signal) alive; once PhysX stops pushing back (driven off a ledge, tipped
// over an edge) the support lapses after a few frames and the vehicle falls normally.
// See docs/drone-ground-jitter-analysis.md for the call order this relies on.

namespace {
// velocity change from SyncFromPhysX below this is not treated as a contact
constexpr float kSupportThreshold = 0.03f;
// frames a support normal stays valid without PhysX reporting it again
constexpr uint32_t kSupportFrames = 3;
// allowed speed into a supporting surface, keeps PhysX reporting the contact
constexpr float kCreepSpeed = 0.05f;
// a contact correction only counts as support if it points at least this much up: the
// correction also carries friction impulses in arbitrary horizontal directions
constexpr float kMinSupportUp = 0.3f;
// per-frame blend of a new correction direction into the cached normal; friction noise
// averages out while slopes still converge to their real normal
constexpr float kNormalBlend = 0.1f;
// only a settled vehicle is clamped. A vehicle touching down on a corner has to pivot about
// that point while its centre of mass keeps falling; clamping the centre's velocity there
// made it sink far too slowly. PhysX corrections that change the spin, or spin itself,
// mean an off-centre contact.
constexpr float kMaxSettledSpinCorrection = 0.05f; // rad/s per frame
constexpr float kMaxSettledSpin = 0.2f;            // rad/s

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
  auto before = rb ? rb->velocity : RED4ext::Vector3();
  auto beforeSpin = rb ? rb->angularVelocity : RED4ext::Vector3();
  SyncFromPhysX_Original(self, chassis, rb, a4, consumedDt);

  auto fc = GetActiveFlightComponent(rb);
  if (!fc) {
    return;
  }
  float dx = rb->velocity.X - before.X;
  float dy = rb->velocity.Y - before.Y;
  float dz = rb->velocity.Z - before.Z;
  float len = std::sqrt(dx * dx + dy * dy + dz * dz);
  float sx = rb->angularVelocity.X - beforeSpin.X;
  float sy = rb->angularVelocity.Y - beforeSpin.Y;
  float sz = rb->angularVelocity.Z - beforeSpin.Z;
  fc->supportSettled = std::sqrt(sx * sx + sy * sy + sz * sz) < kMaxSettledSpinCorrection;
  if (len > kSupportThreshold && std::isfinite(len) && dz / len > kMinSupportUp) {
    auto &n = fc->supportNormal;
    if (fc->framesSinceSupport > kSupportFrames) {
      // new contact: start from straight up rather than a stale normal
      n = RED4ext::Vector3(0.0f, 0.0f, 1.0f);
    }
    float nx = n.X + (dx / len - n.X) * kNormalBlend;
    float ny = n.Y + (dy / len - n.Y) * kNormalBlend;
    float nz = n.Z + (dz / len - n.Z) * kNormalBlend;
    float nlen = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (nlen > 0.0001f) {
      n = RED4ext::Vector3(nx / nlen, ny / nlen, nz / nlen);
    }
    fc->framesSinceSupport = 0;
  } else if (fc->framesSinceSupport < 1000) {
    fc->framesSinceSupport++;
  }
}

// void vehicle::RigidBody::IntegrateMotion(float)
// 2.31: 0x1406F8F40, runs once per fixed step after gravity and the wheel contact solve
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 2021199885, RigidBody_IntegrateMotion, RED4ext::vehicle::PhysicsData *rb,
                          float deltaTime) {
  auto fc = GetActiveFlightComponent(rb);
  if (fc && fc->framesSinceSupport <= kSupportFrames && fc->supportSettled) {
    auto &w = rb->angularVelocity;
    if (w.X * w.X + w.Y * w.Y + w.Z * w.Z > kMaxSettledSpin * kMaxSettledSpin) {
      RigidBody_IntegrateMotion_Original(rb, deltaTime);
      return;
    }
    auto &n = fc->supportNormal;
    auto &v = rb->velocity;
    float vn = v.X * n.X + v.Y * n.Y + v.Z * n.Z;
    if (vn < -kCreepSpeed) {
      float remove = vn + kCreepSpeed;
      v.X -= n.X * remove;
      v.Y -= n.Y * remove;
      v.Z -= n.Z * remove;
    }
  }
  RigidBody_IntegrateMotion_Original(rb, deltaTime);
}
