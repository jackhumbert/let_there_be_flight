#include "Utils/FlightModule.hpp"
#include "Flight/Component.hpp"
#include "Flight/DeveloperSettings.hpp"
#include "Flight/Scrape.hpp"
#include "Physics/ScrapeEffects.hpp"
#include <RED4ext/Scripting/Natives/vehicleAcoustics.hpp>
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

} // namespace

float ScrapeEffects::FrictionOfMaterial(uint64_t aMaterial) {
  return aMaterial ? MaterialFriction(RED4ext::CName(aMaterial)) : 1.0f;
}

namespace {
// What a flying car scrapes (docs/design/scrape-effects.md, Contacts), summed over 2 s per material and logged: the
// contacts, how many the vanilla gate let through, the slide speed and impulse at most, where on the car (its own
// frame: the lowest and highest point, the widest from the centre line) and how much the contact faced up. The
// game's scrape loop is logged when it changes. Both only with the developer setting verboseLog.
struct ScrapeTally {
  uint32_t contacts = 0, vanilla = 0;
  float maxSpeed = 0, maxImpulse = 0, minZ = 1e9f, maxZ = -1e9f, maxSide = 0, normalUp = 0;
};
std::mutex gTallyLock;
std::unordered_map<uint64_t, std::pair<std::string, ScrapeTally>> gTally;
int64_t gTallyStart = 0;
uint64_t gLastLoop = 0;

void Tally(RED4ext::CName aMaterial, float aSpeed, float aImpulse, const RED4ext::Vector3 &aLocal, float aNormalUp,
           bool aVanilla) {
  if (!Flight::Developer().verboseLog)
    return;
  std::lock_guard<std::mutex> _(gTallyLock);
  auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  if (gTallyStart == 0)
    gTallyStart = now;
  auto &entry = gTally[aMaterial.hash];
  if (entry.first.empty())
    entry.first = aMaterial.ToString();
  ScrapeTally &t = entry.second;
  ++t.contacts;
  t.vanilla += aVanilla ? 1 : 0;
  t.maxSpeed = std::max(t.maxSpeed, aSpeed);
  t.maxImpulse = std::max(t.maxImpulse, aImpulse);
  t.minZ = std::min(t.minZ, aLocal.Z);
  t.maxZ = std::max(t.maxZ, aLocal.Z);
  t.maxSide = std::max(t.maxSide, std::fabs(aLocal.X));
  t.normalUp += aNormalUp;
  if (now - gTallyStart > 2'000'000'000LL) {
    for (auto &[hash, e] : gTally) {
      const ScrapeTally &x = e.second;
      spdlog::info("[ScrapeLog] '{}': {} contacts ({} past the vanilla gate), slide {:.1f} m/s, impulse {:.0f} at most; "
                   "on the car z {:.2f}..{:.2f}, {:.2f} off the centre line; facing up {:.2f}",
                   e.first, x.contacts, x.vanilla, x.maxSpeed, x.maxImpulse, x.minZ, x.maxZ, x.maxSide,
                   x.normalUp / std::max<uint32_t>(x.contacts, 1));
    }
    gTally.clear();
    gTallyStart = now;
  }
}

// a contact point in the vehicle's frame, from the rigid body pose
RED4ext::Vector3 ToLocal(const RED4ext::vehicle::BaseObject *aSelf, const RED4ext::Vector3 &aWorld) {
  const auto &xform = aSelf->physicsData->currentTransform;
  float px = aWorld.X - static_cast<float>(xform.Position.x.Bits) / 131072.0f;
  float py = aWorld.Y - static_cast<float>(xform.Position.y.Bits) / 131072.0f;
  float pz = aWorld.Z - static_cast<float>(xform.Position.z.Bits) / 131072.0f;
  const auto &q = xform.Orientation;
  // rotate by the conjugate quaternion: v' = v + 2w(-u x v) + 2(-u x (-u x v)), u = (i, j, k)
  float ux = -q.i, uy = -q.j, uz = -q.k, w = q.r;
  float cx = uy * pz - uz * py, cy = uz * px - ux * pz, cz = ux * py - uy * px;
  return RED4ext::Vector3(px + 2.0f * (w * cx + uy * cz - uz * cy), py + 2.0f * (w * cy + uz * cx - ux * cz),
                          pz + 2.0f * (w * cz + ux * cy - uy * cx));
}

Flight::Vec3 Vec(const RED4ext::Vector3 &a) { return {a.X, a.Y, a.Z}; }
Flight::Quat Orientation(const RED4ext::vehicle::BaseObject *aSelf) {
  const auto &q = aSelf->physicsData->currentTransform.Orientation;
  return {q.i, q.j, q.k, q.r};
}

// a contact's impulse is summed over one physics step (s), the vehicles' fixed 60 Hz
constexpr float kContactStep = 1.0f / 60.0f;
constexpr float kGravity = 9.81f;

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
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 1399724886, BaseObject_NotifyContacts, RED4ext::vehicle::BaseObject *self,
                          const uint8_t *contact, bool isFirst, RED4ext::vehicle::BaseObject *other) {
  BaseObject_NotifyContacts_Original(self, contact, isFirst, other);

  // world contacts only: vehicle-vs-vehicle keeps the vanilla path
  if (!self || !contact || other) {
    return;
  }
  auto fc = FlightComponent::Get(self);
  if (!fc || !fc->active) {
    return;
  }
  // a pair reported lost: never seen here (the game does not pass them on), so a scrape ends by its hold
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
  if (self->physicsData) {
    auto at = *reinterpret_cast<const RED4ext::Vector3 *>(contact + kContactPosition);
    auto nn = *reinterpret_cast<const RED4ext::Vector3 *>(contact + kContactNormal);
    auto other = *reinterpret_cast<const RED4ext::CName *>(contact + kContactMaterials + 8 * (isFirst ? 1 : 0));
    const RED4ext::Vector3 local = ToLocal(self, at);
    // the contact point's own slide, the car's spin included: a car dragging its nose while it turns slides there
    // faster than its centre
    const float slide = Flight::SlideAt(Vec(self->physicsData->velocity), Vec(self->physicsData->angularVelocity),
                                        Orientation(self), {local.X, local.Y, local.Z}, {nn.X, nn.Y, nn.Z});
    Tally(other, slide, impulse, local, std::fabs(nn.Z),
          !physics || physics->sleepTimer != 1.0f || impulse >= kVanillaImpulseGate);
    // the car's scrapes (Flight::ScrapeTracker): every world contact, whatever the vanilla gate does with it
    Flight::ScrapeContact c;
    c.local = {local.X, local.Y, local.Z};
    c.normal = {nn.X, nn.Y, nn.Z};
    c.slide = slide;
    c.impulse = impulse;
    // the push as a share of the car's weight: the impulse is over one physics step
    c.load = impulse * self->physicsData->inverseMass / (kGravity * kContactStep);
    c.surface = ScrapeEffects::SurfaceOfMaterial(other.hash);
    c.material = other.hash;
    fc->scrapes.Add(c, Flight::ScrapeParams{});
  }
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
  // the contact point's velocity, the car's spin included (as the tracker's slide above)
  const Flight::Vec3 vp =
      Flight::Add(Vec(self->physicsData->velocity),
                  Flight::Cross(Vec(self->physicsData->angularVelocity),
                                Flight::Rotate(Orientation(self), Vec(ToLocal(self, position)))));
  const RED4ext::Vector3 v(vp.x, vp.y, vp.z);
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
  // the game's scrape loop, logged when it changes
  if (self->acoustics && Flight::Developer().verboseLog) {
    auto loop = self->acoustics->scrapingLoopStart;
    std::lock_guard<std::mutex> _(gTallyLock);
    if (loop.hash != gLastLoop) {
      gLastLoop = loop.hash;
      spdlog::info("[ScrapeLog] scrape loop now '{}' (on '{}', slide {:.1f} m/s, timer {:.2f})", loop.ToString(),
                   material.ToString(), speed, self->acoustics->scrapingTimer);
    }
  }

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
    RED4ext::IHookable::StaticHook<void, 3095401716>(fx, &localPos, &position, &n, &slideDir, deltaV, speed, material);
  }
}
