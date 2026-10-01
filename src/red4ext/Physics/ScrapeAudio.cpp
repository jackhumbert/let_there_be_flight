#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <cmath>
#include <mutex>
#include <unordered_set>
#include <spdlog/spdlog.h>

// Material-aware scraping audio for flying vehicles sliding on their chassis.
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
// physics::ContactData (0xB8), filled per PxContactPair by the simulation event callback
constexpr size_t kContactPosition = 0x80; // Vector3, max-impulse point
constexpr size_t kContactNormal = 0x8C;   // Vector3, flipped when the vehicle is shape 1
constexpr size_t kContactMaterials = 0x98; // CName[2], one per shape
constexpr size_t kContactState = 0x78;     // 0 found, 2 persists, 1 lost
constexpr size_t kContactImpulse = 0xAC;   // float, summed over the pair's points
constexpr uint8_t kContactLost = 1;
// NotifyContacts' gate: awake vehicles ignore contacts below this summed impulse
constexpr float kVanillaImpulseGate = 100.0f;
// vehicle::BaseObject+0x5E8: vehicle::Audio*
constexpr size_t kBaseObjectAudio = 0x5E8;
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
  // log each ground material a flying vehicle touches once, to tune per-surface friction
  {
    static std::mutex seenLock;
    static std::unordered_set<uint64_t> seen;
    auto material = *reinterpret_cast<const RED4ext::CName *>(contact + kContactMaterials + 8 * (isFirst ? 1 : 0));
    std::lock_guard<std::mutex> _(seenLock);
    if (seen.insert(material.hash).second) {
      spdlog::info("[ScrapeAudio] flying vehicle touched ground material '{}'", material.ToString());
    }
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
}
