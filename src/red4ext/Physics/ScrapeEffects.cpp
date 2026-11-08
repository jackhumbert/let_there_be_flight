#include "Physics/ScrapeEffects.hpp"

#include "Flight/Component.hpp"
#include "Flight/FlightGround.hpp"
#include "Flight/FlightStats.hpp"
#include "Flight/Scrape.hpp"
#include "stdafx.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/WorldTransform.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/FxResource.hpp>
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <cmath>
#include <mutex>
#include <unordered_map>

float GetFlightSettingsFloat(RED4ext::CRTTISystem *rtti, RED4ext::CName aProperty, float aFallback);

namespace {
using RED4ext::Handle;
using RED4ext::IScriptable;

// the effects of a surface: sparks (or dust, splinters) and the trail decal at each depth (Flight::TrailDepth: light,
// as drawn, deep); empty paths for none
struct SurfaceFx {
  const char *sparks;
  const char *trail[3];
};
#define LTBF_TRAIL(name)                                                                                               \
  { R"(user\jackhumbert\effects\)" name "_light.effect", R"(user\jackhumbert\effects\)" name ".effect",              \
    R"(user\jackhumbert\effects\)" name "_deep.effect" }
SurfaceFx EffectsOf(Flight::Surface aSurface) {
  switch (aSurface) {
  case Flight::Surface::Hard:
    return {R"(user\jackhumbert\effects\scrape_sparks_hard.effect)", LTBF_TRAIL("scrape_trail_hard")};
  case Flight::Surface::Metal:
    return {R"(user\jackhumbert\effects\scrape_sparks_metal.effect)", LTBF_TRAIL("scrape_trail_metal")};
  case Flight::Surface::Glass:
    return {R"(user\jackhumbert\effects\scrape_sparks_glass.effect)", {"", "", ""}};
  case Flight::Surface::Wood:
    return {R"(base\fx\vehicles\_damage\scratches\car_scratches_wood.effect)", {"", "", ""}};
  case Flight::Surface::Soft:
    return {R"(user\jackhumbert\effects\scrape_dust_soft.effect)", LTBF_TRAIL("scrape_trail_soft")};
  case Flight::Surface::Grass:
    return {R"(user\jackhumbert\effects\scrape_turf_grass.effect)", LTBF_TRAIL("scrape_trail_grass")};
  case Flight::Surface::None:
    break;
  }
  return {"", {"", "", ""}};
}
#undef LTBF_TRAIL

struct Live {
  Handle<IScriptable> sparks, trail;
  Flight::Surface surface = Flight::Surface::None;
  Flight::TrailDepth depth; // which trail it lays
  bool seen = false;
};
// per component, per scrape id
std::unordered_map<FlightComponent *, std::unordered_map<uint32_t, Live>> gLive;

struct Calls {
  RED4ext::CBaseFunction *spawn = nullptr, *update = nullptr, *breakLoop = nullptr;
  // gameFxInstance.SetBlackboardValue(CName, Float): an effect's input parameter (none: the effects as drawn)
  RED4ext::CBaseFunction *setValue = nullptr;
  bool ok = false;
};
const Calls &Resolve() {
  static Calls c = [] {
    Calls c;
    auto rtti = RED4ext::CRTTISystem::Get();
    auto system = rtti->GetClass("gameFxSystem");
    auto instance = rtti->GetClass("gameFxInstance");
    if (!system || !instance)
      return c;
    c.spawn = system->GetFunction("SpawnEffect");
    c.update = instance->GetFunction("UpdateTransform");
    c.breakLoop = instance->GetFunction("BreakLoop");
    c.setValue = instance->GetFunction("SetBlackboardValue");
    c.ok = c.spawn && c.update && c.breakLoop;
    return c;
  }();
  return c;
}

IScriptable *FxSystem() {
  auto gameInstance = RED4ext::CGameEngine::Get()->framework->gameInstance;
  auto rtti = RED4ext::CRTTISystem::Get();
  for (auto cls = rtti->GetClass("gameFxSystem"); cls && cls->GetName() != RED4ext::CName("IScriptable");
       cls = cls->parent)
    if (auto system = gameInstance->GetSystem(cls))
      return system;
  return nullptr;
}

// The thrusters' touches when the Drone skid is not finding them (every other mode): the same four rays, each
// thruster's load from how far it sinks into the ground (Flight::TouchLoad). Without it a thruster dragged along the
// road in any mode but Drone made no sound and no mark: its box is open at the bottom and takes no contact there.
void FeelThrusters(FlightComponent *aComponent) {
  auto stats = FlightStats::Of(aComponent);
  if (!stats)
    return;
  const float maxHover = GetFlightSettingsFloat(RED4ext::CRTTISystem::Get(), "hoverModeMaxHoverHeight", 7.0f);
  Flight::Hit hits[4];
  if (!FlightGroundNative::Find(aComponent, maxHover, nullptr, hits).found)
    return;
  if (!FlightGroundNative::Touches(aComponent, stats->State(), hits))
    return;
  const Flight::SkidScrapeParams params;
  for (Flight::SkidContact &c : aComponent->skidContacts)
    c.load = Flight::TouchLoad(c.clearance, params);
}

// The trail's ribbon spans its effect's up: spawned turned as the car is, it stood on edge and drew a line on the
// ground; a quarter turn about the car's forward lays it flat.
constexpr Flight::Quat kTrailTurn{0, 0.70710678f, 0, 0.70710678f};

// where a point on the car is in the world, turned as the car is (and then by aTurn, in the car's frame)
RED4ext::WorldTransform At(RED4ext::vehicle::BaseObject *aVehicle, const Flight::Vec3 &aLocal,
                           const Flight::Quat &aTurn = {0, 0, 0, 1}) {
  const auto &wt = aVehicle->worldTransform;
  const auto &q = wt.Orientation;
  const Flight::Quat car{q.i, q.j, q.k, q.r};
  const Flight::Vec3 offset = Flight::Rotate(car, aLocal);
  RED4ext::WorldTransform out = wt;
  const Flight::Quat turned = Flight::Mul(car, aTurn);
  out.Orientation = {turned.i, turned.j, turned.k, turned.r};
  out.Position.x.Bits += static_cast<int32_t>(offset.x * 131072.0f);
  out.Position.y.Bits += static_cast<int32_t>(offset.y * 131072.0f);
  out.Position.z.Bits += static_cast<int32_t>(offset.z * 131072.0f);
  return out;
}

// a point on the ground (world), the car's heading laid onto the ground: the car's turn tilted by the shortest arc
// from its up to the ground's normal (then by aTurn, in that frame)
RED4ext::WorldTransform OnGround(RED4ext::vehicle::BaseObject *aVehicle, const Flight::Vec3 &aGround,
                                 const Flight::Vec3 &aNormal, const Flight::Quat &aTurn = {0, 0, 0, 1}) {
  const auto &q = aVehicle->worldTransform.Orientation;
  const Flight::Quat car{q.i, q.j, q.k, q.r};
  const Flight::Vec3 up = Flight::Rotate(car, {0.0f, 0.0f, 1.0f});
  const Flight::Vec3 axis = Flight::Cross(up, aNormal);
  const float w = 1.0f + Flight::Dot(up, aNormal);
  const float n = std::sqrt(Flight::Dot(axis, axis) + w * w);
  const Flight::Quat arc = n > 1e-6f ? Flight::Quat{axis.x / n, axis.y / n, axis.z / n, w / n} : Flight::Quat{0, 0, 0, 1};
  const Flight::Quat turned = Flight::Mul(Flight::Mul(arc, car), aTurn);
  RED4ext::WorldTransform out;
  out.Orientation = {turned.i, turned.j, turned.k, turned.r};
  out.Position.x.Bits = static_cast<int32_t>(aGround.x * 131072.0f);
  out.Position.y.Bits = static_cast<int32_t>(aGround.y * 131072.0f);
  out.Position.z.Bits = static_cast<int32_t>(aGround.z * 131072.0f);
  return out;
}

Handle<IScriptable> Spawn(IScriptable *aSystem, const char *aPath, RED4ext::WorldTransform aAt) {
  Handle<IScriptable> out;
  if (!aPath || !*aPath)
    return out;
  RED4ext::game::FxResource resource;
  resource.effect.path = RED4ext::ResourcePath(aPath);
  bool ignoreTimeDilation = false;
  RED4ext::ExecuteFunction(aSystem, Resolve().spawn, &out, resource, aAt, ignoreTimeDilation);
  return out;
}

void Move(Handle<IScriptable> &aFx, RED4ext::WorldTransform aAt) {
  if (aFx)
    RED4ext::ExecuteFunction(aFx.GetPtr(), Resolve().update, nullptr, aAt);
}

// the scrape's inputs on its effect (the names tools/fx/scrape_effects.py gives them)
void Feed(Handle<IScriptable> &aFx, const Flight::ScrapeFxInputs &aIn) {
  if (!aFx || !Resolve().setValue)
    return;
  static const RED4ext::CName kForce("scrape_force"), kPress("scrape_press"), kSpeed("scrape_speed");
  for (auto [name, value] : {std::pair{kForce, aIn.force}, std::pair{kPress, aIn.press}, std::pair{kSpeed, aIn.speed}})
    RED4ext::ExecuteFunction(aFx.GetPtr(), Resolve().setValue, nullptr, name, value);
}

void End(Handle<IScriptable> &aFx) {
  if (aFx)
    RED4ext::ExecuteFunction(aFx.GetPtr(), Resolve().breakLoop, nullptr);
  aFx.Reset();
}
} // namespace

Flight::Surface ScrapeEffects::SurfaceOfMaterial(uint64_t aMaterial) {
  if (!aMaterial)
    return Flight::Surface::None;
  static std::mutex lock;
  static std::unordered_map<uint64_t, Flight::Surface> cache;
  std::lock_guard<std::mutex> _(lock);
  auto it = cache.find(aMaterial);
  if (it != cache.end())
    return it->second;
  const Flight::Surface s = Flight::SurfaceOf(RED4ext::CName(aMaterial).ToString());
  cache.emplace(aMaterial, s);
  return s;
}

Flight::Period ScrapeEffects::PeriodOfMaterial(uint64_t aMaterial) {
  if (!aMaterial)
    return {};
  static std::mutex lock;
  static std::unordered_map<uint64_t, Flight::Period> cache;
  std::lock_guard<std::mutex> _(lock);
  auto it = cache.find(aMaterial);
  if (it != cache.end())
    return it->second;
  const Flight::Period p = Flight::PeriodOf(RED4ext::CName(aMaterial).ToString());
  cache.emplace(aMaterial, p);
  return p;
}

void ScrapeEffects::Update(FlightComponent *aComponent, RED4ext::vehicle::BaseObject *aVehicle, float aDt) {
  const Flight::ScrapeParams params;
  auto it = gLive.find(aComponent);
  if (!aComponent->active || !aVehicle) {
    aComponent->scrapes.Clear();
    aComponent->skidScrapes.Clear();
    if (it != gLive.end()) {
      for (auto &[id, live] : it->second) {
        End(live.sparks);
        End(live.trail);
      }
      gLive.erase(it);
    }
    return;
  }
  if (const auto *body = aVehicle->physicsData) {
    const auto &q = body->currentTransform.Orientation;
    aComponent->scrapes.Follow({body->velocity.X, body->velocity.Y, body->velocity.Z},
                               {body->angularVelocity.X, body->angularVelocity.Y, body->angularVelocity.Z},
                               {q.i, q.j, q.k, q.r});
  }
  aComponent->scrapes.Step(aDt, params);
  aComponent->skidRanAge += aDt;
  if (aComponent->skidRanAge >= 0.25f)
    FeelThrusters(aComponent);
  // the thrusters' contacts (the skid may run less often than this, so they are kept a moment)
  aComponent->skidContactsAge += aDt;
  const bool skid = aComponent->skidContactsAge < 0.25f;
  static const Flight::SkidContact kNone[Flight::SkidScrapes::kThrusters];
  if (skid)
    aComponent->skidScrapes.Step(aComponent->skidContacts, aDt, {});
  else
    aComponent->skidScrapes.Step(kNone, aDt, {});
  if (!Resolve().ok) {
    LTBF_WARN_ONCE("[ScrapeEffects] gameFxSystem.SpawnEffect or gameFxInstance's functions not found: no scrape effects");
    return;
  }
  IScriptable *system = FxSystem();
  if (!system)
    return;
  auto &live = gLive[aComponent];
  for (auto &[id, l] : live)
    l.seen = false;
  auto follow = [&](uint32_t aId, Flight::Surface aSurface, uint64_t aMaterial, const RED4ext::WorldTransform &aAt,
                    const RED4ext::WorldTransform &aTrailAt, const Flight::ScrapeFxInputs &aIn) {
    const SurfaceFx fx = EffectsOf(aSurface);
    auto found = live.find(aId);
    if (found == live.end()) {
      Live l;
      l.surface = aSurface;
      l.sparks = Spawn(system, fx.sparks, aAt);
      l.depth.Step(aIn.press, aDt);
      const char *trail = fx.trail[l.depth.band];
      l.trail = Spawn(system, trail, aTrailAt);
      // one line a scrape, where it begins: which surface and physics material (none: the skid's ray had none, and
      // it scrapes as hard ground), and whether its effects spawned
      spdlog::info("[ScrapeFx] scrape {:#x} on {} ('{}'): sparks {}, trail {} (depth {})", aId,
                   Flight::SurfaceName(aSurface), aMaterial ? RED4ext::CName(aMaterial).ToString() : "none",
                   *fx.sparks ? (l.sparks ? "spawned" : "failed") : "none",
                   *trail ? (l.trail ? "spawned" : "failed") : "none", l.depth.band);
      found = live.emplace(aId, std::move(l)).first;
    } else {
      Live &l = found->second;
      Move(l.sparks, aAt);
      if (l.depth.Step(aIn.press, aDt)) {
        // pressed harder or lighter: this segment ends where it is and one of the new depth goes on from here
        End(l.trail);
        l.trail = Spawn(system, fx.trail[l.depth.band], aTrailAt);
      } else {
        Move(l.trail, aTrailAt);
      }
    }
    Feed(found->second.sparks, aIn);
    found->second.seen = true;
  };
  const Flight::SkidScrapeParams skidParams;
  // each touching thruster marks the ground right under it, with the effects of the ground its ray hit; a scrape's
  // effects show while it touches (a thruster bridging a short hop), and end where it left the ground
  for (const Flight::SkidScrape &s : aComponent->skidScrapes)
    if (s.active && s.since <= skidParams.effectsBridge)
      follow(s.id, s.surface, s.material, OnGround(aVehicle, s.ground, s.normal),
             OnGround(aVehicle, s.ground, s.normal, kTrailTurn),
             Flight::FxInputsOf(s.intensity, s.load, skidParams.fullLoad, s.slide, skidParams.fullSlide));
  for (const Flight::Scrape &s : aComponent->scrapes) {
    // the ground under the car is the thrusters' while the skid runs; the rest (kerbs, walls) stays the game's contacts.
    // Effects only while it touches: they sit on the car at the contact, and shown while the scrape faded out they rode
    // along in the air
    if (!s.active || !s.touching || (skid && s.normal.z > 0.7f))
      continue;
    follow(s.id, s.surface, s.material, At(aVehicle, s.local), At(aVehicle, s.local, kTrailTurn),
           Flight::FxInputsOf(s.intensity, s.load, params.fullLoad, s.slide, params.fullSlide));
  }
  for (auto l = live.begin(); l != live.end();) {
    if (!l->second.seen) {
      End(l->second.sparks);
      End(l->second.trail);
      l = live.erase(l);
    } else {
      ++l;
    }
  }
}
