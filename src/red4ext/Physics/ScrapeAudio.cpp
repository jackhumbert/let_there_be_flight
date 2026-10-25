#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <cmath>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <spdlog/spdlog.h>

// Material-aware scraping audio, sparks and ground scratch decals for flying vehicles sliding
// on their chassis.
//
// vehicle::BaseObject::NotifyContacts drops every contact whose summed pair impulse is under
// 100 while the vehicle is awake (Physics::sleepTimer == 1.0), before any FX, scraping or
// damage runs. A flying car skidding on its body produces small per-frame impulses (and the
// chassis contact constraint in ChassisContacts.cpp keeps them small), so it never scraped.
// vehicle::Audio::NotifyScrapingCollision also rejects contacts whose normal lines up with
// the car's up axis (no scraping on the belly) when the collision map enables it.
//
// For flying vehicles, world contacts that the impulse gate dropped are sent to
// NotifyScrapingCollision directly: contact position, the other surface's physics material
// (mapped to a scrape loop through physical_material_to_audio_metadata_matrix), the
// tangential slide speed (drives the impactVelocity parameter and is checked against the
// map's scrapingMinTangentialVelocityThreshold) and a zero normal, which passes the belly
// angle gate. Vehicle::Audio::UpdateScrapingCollision ends the loop on its own once calls
// stop for scrapingMaxCollisionCooldown.

namespace {
// diagnostics (temporary), see FX_NotifyCollision below
thread_local bool tFromLTBF = false;
std::atomic<uint32_t> gVanillaCalls{0}, gLTBFCalls{0};
std::atomic<int64_t> gLastLog{0};
}

namespace {
// physics::ContactData (0xB8), filled per PxContactPair by the simulation event callback
constexpr size_t kContactPosition = 0x80; // Vector3, max-impulse point
constexpr size_t kContactNormal = 0x8C;   // Vector3, flipped when the vehicle is shape 1
constexpr size_t kContactMaterials = 0x98; // CName[2], one per shape
constexpr size_t kContactState = 0x78;     // 0 found, 2 persists, 1 lost
constexpr size_t kContactImpulse = 0xAC;   // float, summed over the pair's points
constexpr uint8_t kContactLost = 1;
// Ground friction multiplier per physics material (on top of Mod Settings > Ground Friction).
// Names are the .physmat CNames seen on contacts in 2.31; matched by prefix so variants like
// metal_painted / metal_car share "metal". Unknown materials get kUnknownFriction.
struct MaterialFrictionEntry {
  const char *prefix;
  float friction;
};
constexpr MaterialFrictionEntry kMaterialFriction[] = {
    {"asphalt", 1.0f},   {"concrete", 1.0f},  {"stone", 1.0f},   {"default_material", 0.9f},
    {"dirt", 0.8f},      {"sand", 0.7f},      {"wood", 0.7f},    {"trash", 0.6f},
    {"cardboard", 0.6f}, {"upholstery", 0.6f}, {"metal", 0.5f},  {"plastic", 0.5f},
    {"character", 0.4f}, {"glass", 0.3f},
};
constexpr float kUnknownFriction = 0.8f;

float MaterialFriction(RED4ext::CName material) {
  static std::mutex lock;
  static std::unordered_map<uint64_t, float> cache;
  std::lock_guard<std::mutex> _(lock);
  auto it = cache.find(material.hash);
  if (it != cache.end()) {
    return it->second;
  }
  std::string name = material.ToString();
  float friction = kUnknownFriction;
  for (const auto &entry : kMaterialFriction) {
    if (name.rfind(entry.prefix, 0) == 0) {
      friction = entry.friction;
      break;
    }
  }
  cache.emplace(material.hash, friction);
  spdlog::info("[ScrapeAudio] flying vehicle touched ground material '{}' (friction x{})", name, friction);
  return friction;
}

// NotifyContacts' gate: awake vehicles ignore contacts below this summed impulse
constexpr float kVanillaImpulseGate = 100.0f;
// vehicle::Audio* and vehicle::FX* in vehicle::BaseObject (the SDK's acoustics/effects:
// 0x5E8/0x5D8 on 2.31, 0x608/0x5F8 on 2.5)
constexpr size_t kBaseObjectAudio = offsetof(RED4ext::vehicle::BaseObject, acoustics);
constexpr size_t kBaseObjectFX = offsetof(RED4ext::vehicle::BaseObject, effects);
static_assert(kBaseObjectAudio == 0x608 && kBaseObjectFX == 0x5F8, "check against BaseObject::NotifyContacts");
} // namespace

// void vehicle::BaseObject::NotifyContacts(physics::ContactData const &, bool, vehicle::BaseObject *)
// 2.31: 0x140106FD0
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 1399724886, NotifyContacts, RED4ext::vehicle::BaseObject *self,
                          const uint8_t *contact, bool isFirst, RED4ext::vehicle::BaseObject *other) {
  NotifyContacts_Original(self, contact, isFirst, other);

  // world contacts only: vehicle-vs-vehicle keeps the vanilla path
  if (!self || !contact || other) {
    return;
  }
  auto fc = FlightComponent::Get(self);
  if (!fc || !fc->active) {
    return;
  }
  if (contact[kContactState] == kContactLost) {
    return;
  }
  // per-material ground friction (and a one-time log line per new material)
  {
    auto material = *reinterpret_cast<const RED4ext::CName *>(contact + kContactMaterials + 8 * (isFirst ? 1 : 0));
    fc->groundMaterialFriction = MaterialFriction(material);
  }
  // only what the vanilla gate dropped; everything else already went through NotifyContacts
  auto physics = self->physics;
  float impulse = *reinterpret_cast<const float *>(contact + kContactImpulse);
  if (!physics || physics->sleepTimer != 1.0f || impulse >= kVanillaImpulseGate) {
    return;
  }
  auto audio = *reinterpret_cast<void *const *>(reinterpret_cast<const uint8_t *>(self) + kBaseObjectAudio);
  if (!audio || !self->physicsData) {
    return;
  }

  auto position = *reinterpret_cast<const RED4ext::Vector3 *>(contact + kContactPosition);
  auto n = *reinterpret_cast<const RED4ext::Vector3 *>(contact + kContactNormal);
  if (!isFirst) {
    n = RED4ext::Vector3(-n.X, -n.Y, -n.Z);
  }
  auto v = self->physicsData->velocity;
  float vn = v.X * n.X + v.Y * n.Y + v.Z * n.Z;
  float tx = v.X - n.X * vn, ty = v.Y - n.Y * vn, tz = v.Z - n.Z * vn;
  float speed = std::sqrt(tx * tx + ty * ty + tz * tz);
  if (!std::isfinite(speed)) {
    return;
  }
  // the other shape's material: shape 1 when the vehicle is shape 0
  auto material = *reinterpret_cast<const RED4ext::CName *>(contact + kContactMaterials + 8 * (isFirst ? 1 : 0));
  RED4ext::Vector3 noNormal(0.0f, 0.0f, 0.0f);

  // void vehicle::Audio::NotifyScrapingCollision(Vector3 const &, Vector3 const &,
  //                                              vehicle::BaseObject const *, float, CName)
  RED4ext::IHookable::StaticHook<void, 2085232259>(audio, &position, &noNormal,
                                                    static_cast<const RED4ext::vehicle::BaseObject *>(nullptr), speed,
                                                    material);

  // Sparks and ground scratch decals, through the vehicle's own fxCollision table (material ->
  // impact/scratch particles and decals). Called once per frame per contact this keeps the
  // looping scratch effects alive (FX::RetrievePreviousScratch matches them by position and
  // material); FX::UpdateVehicleData breaks the loops that were not renewed this frame.
  auto fx = *reinterpret_cast<void *const *>(reinterpret_cast<const uint8_t *>(self) + kBaseObjectFX);
  if (fx && speed > 0.0001f) {
    // contact point in the vehicle's local frame, from the rigid body pose
    const auto &xform = self->physicsData->currentTransform;
    float px = position.X - static_cast<float>(xform.Position.x.Bits) / 131072.0f;
    float py = position.Y - static_cast<float>(xform.Position.y.Bits) / 131072.0f;
    float pz = position.Z - static_cast<float>(xform.Position.z.Bits) / 131072.0f;
    const auto &q = xform.Orientation;
    // rotate by the conjugate quaternion: v' = v + 2w(-u x v) + 2(-u x (-u x v)), u = (i, j, k)
    float ux = -q.i, uy = -q.j, uz = -q.k, w = q.r;
    float cx = uy * pz - uz * py, cy = uz * px - ux * pz, cz = ux * py - uy * px;
    RED4ext::Vector3 localPos(px + 2.0f * (w * cx + uy * cz - uz * cy), py + 2.0f * (w * cy + uz * cx - ux * cz),
                              pz + 2.0f * (w * cz + ux * cy - uy * cx));
    RED4ext::Vector3 slideDir(-tx / speed, -ty / speed, -tz / speed);
    float deltaV = 0.0f; // no per-frame impact burst, scratches only
    // void vehicle::FX::NotifyCollision(Vector3 const &localPos, Vector3 const &worldPos, Vector3 const &normal,
    //                                   Vector3 const &slideDir, float deltaV, float slideSpeed, CName material)
    tFromLTBF = true;
    RED4ext::IHookable::StaticHook<void, 3095401716>(fx, &localPos, &position, &n, &slideDir, deltaV, speed, material);
    tFromLTBF = false;
  }
}

// --- diagnostics (temporary): are scratch effects being requested, and with what? --------

// void vehicle::FX::NotifyCollision(Vector3 const &localPos, Vector3 const &worldPos, Vector3 const &normal,
//                                   Vector3 const &slideDir, float deltaV, float slideSpeed, CName material)
// 2.31: 0x140108294
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 3095401716, FX_NotifyCollision, uint8_t *self, RED4ext::Vector3 *localPos,
                          RED4ext::Vector3 *worldPos, RED4ext::Vector3 *normal, RED4ext::Vector3 *slideDir, float deltaV,
                          float slideSpeed, RED4ext::CName material) {
  (tFromLTBF ? gLTBFCalls : gVanillaCalls)++;
  auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  auto last = gLastLog.load();
  if (now - last > 2'000'000'000LL && gLastLog.compare_exchange_strong(last, now)) {
    auto params = self ? *reinterpret_cast<float *const *>(self + 0xA8) : nullptr;
    spdlog::info("[ScrapeFX] NotifyCollision calls in 2 s: vanilla {} ltbf {}; last deltaV {:.2f} slide {:.2f} '{}'; "
                 "thresholds impact {:.2f} scratch {:.2f} reuse {:.2f}",
                 gVanillaCalls.exchange(0), gLTBFCalls.exchange(0), deltaV, slideSpeed, material.ToString(),
                 params ? params[1] : -1.0f, params ? params[2] : -1.0f, params ? params[6] : -1.0f);
  }
  FX_NotifyCollision_Original(self, localPos, worldPos, normal, slideDir, deltaV, slideSpeed, material);
}
