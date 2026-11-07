#include "Physics/ScrapeEffects.hpp"

#include "Flight/Component.hpp"
#include "Flight/Scrape.hpp"
#include "stdafx.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/WorldTransform.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/FxResource.hpp>
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace {
using RED4ext::Handle;
using RED4ext::IScriptable;

// the effects of a surface: sparks (or dust, splinters) and the trail decal; empty paths for none
struct SurfaceFx {
  const char *sparks, *trail;
};
SurfaceFx EffectsOf(Flight::Surface aSurface) {
  switch (aSurface) {
  case Flight::Surface::Hard:
    return {R"(user\jackhumbert\effects\scrape_sparks_hard.effect)", R"(user\jackhumbert\effects\scrape_trail_hard.effect)"};
  case Flight::Surface::Metal:
    return {R"(user\jackhumbert\effects\scrape_sparks_metal.effect)",
            R"(user\jackhumbert\effects\scrape_trail_metal.effect)"};
  case Flight::Surface::Glass:
    return {R"(user\jackhumbert\effects\scrape_sparks_glass.effect)", ""};
  case Flight::Surface::Wood:
    return {R"(base\fx\vehicles\_damage\scratches\car_scratches_wood.effect)", ""};
  case Flight::Surface::Soft:
    return {R"(user\jackhumbert\effects\scrape_dust_soft.effect)", R"(user\jackhumbert\effects\scrape_trail_soft.effect)"};
  case Flight::Surface::None:
    break;
  }
  return {"", ""};
}

struct Live {
  Handle<IScriptable> sparks, trail;
  bool seen = false;
};
// per component, per scrape id
std::unordered_map<FlightComponent *, std::unordered_map<uint32_t, Live>> gLive;

struct Calls {
  RED4ext::CBaseFunction *spawn = nullptr, *update = nullptr, *breakLoop = nullptr;
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
  aComponent->scrapes.Step(aDt, params);
  // the skid's contacts while it runs (it may run less often than this, so they are kept a moment)
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
  auto follow = [&](uint32_t aId, Flight::Surface aSurface, const RED4ext::WorldTransform &aAt,
                    const RED4ext::WorldTransform &aTrailAt) {
    auto found = live.find(aId);
    if (found == live.end()) {
      const SurfaceFx fx = EffectsOf(aSurface);
      Live l;
      l.sparks = Spawn(system, fx.sparks, aAt);
      l.trail = Spawn(system, fx.trail, aTrailAt);
      // one line a scrape, where it begins: which surface, and whether its effects spawned
      spdlog::info("[ScrapeFx] scrape {:#x} on {}: sparks {}, trail {}", aId, Flight::SurfaceName(aSurface),
                   *fx.sparks ? (l.sparks ? "spawned" : "failed") : "none",
                   *fx.trail ? (l.trail ? "spawned" : "failed") : "none");
      found = live.emplace(aId, std::move(l)).first;
    } else {
      Move(found->second.sparks, aAt);
      Move(found->second.trail, aTrailAt);
    }
    found->second.seen = true;
  };
  // each touching thruster marks the ground right under it, with the effects of the ground its ray hit
  for (const Flight::SkidScrape &s : aComponent->skidScrapes)
    if (s.active)
      follow(s.id, s.surface, OnGround(aVehicle, s.ground, s.normal),
             OnGround(aVehicle, s.ground, s.normal, kTrailTurn));
  for (const Flight::Scrape &s : aComponent->scrapes) {
    // the ground under the car is the thrusters' while the skid runs; the rest (kerbs, walls) stays the game's contacts
    if (!s.active || (skid && s.normal.z > 0.7f))
      continue;
    follow(s.id, s.surface, At(aVehicle, s.local), At(aVehicle, s.local, kTrailTurn));
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
