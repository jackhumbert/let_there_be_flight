#include "FlightWorld.hpp"

#include "Utils/Utils.hpp"
#include "stdafx.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/QueryFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/TraceResult.hpp>

namespace Navigation {

namespace {

struct Calls {
  RED4ext::CProperty *sqs = nullptr;
  RED4ext::CBaseFunction *byGroup = nullptr, *byFilter = nullptr, *addGroup = nullptr, *isValid = nullptr;
  RED4ext::physics::QueryFilter ground{};
  bool ok = false;
};

const Calls &Resolve() {
  static Calls c = [] {
    Calls c;
    auto rtti = RED4ext::CRTTISystem::Get();
    auto component = rtti->GetClass("FlightComponent");
    auto sqs = rtti->GetClass("gameSpatialQueriesSystem");
    auto filter = rtti->GetClass("physicsQueryFilter");
    auto trace = rtti->GetClass("physicsTraceResult");
    if (!component || !sqs || !filter || !trace)
      return c;
    c.sqs = component->GetProperty("sqs");
    c.byGroup = sqs->GetFunction("SyncRaycastByCollisionGroup");
    c.byFilter = sqs->GetFunction("SyncRaycastByQueryFilter");
    c.addGroup = filter->GetFunction("AddGroup");
    c.isValid = trace->GetFunction("IsValid");
    c.ok = c.sqs && c.byGroup && c.byFilter && c.addGroup && c.isValid;
    if (c.ok) {
      // SpotSupported's filter: QueryFilter.AddGroup(filter, n"Static"), n"Terrain", n"VehicleBlocker"
      const RED4ext::CName groups[] = {"Static", "Terrain", "VehicleBlocker"};
      for (RED4ext::CName group : groups)
        RED4ext::ExecuteFunction(RED4ext::ScriptInstance(nullptr), c.addGroup, nullptr, c.ground, group);
    }
    return c;
  }();
  return c;
}

// the component's spatial queries system, nullptr when it has none (warned once when the calls are missing)
RED4ext::IScriptable *Queries(const Calls &c, RED4ext::IScriptable *aComponent) {
  if (!c.ok) {
    LTBF_WARN_ONCE("[FlightWorld] a FlightComponent field or an engine ray function was not found: the Autopilot's "
                   "landing rays hit nothing");
    return nullptr;
  }
  if (!aComponent)
    return nullptr;
  auto sqs = c.sqs->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(aComponent);
  return sqs && *sqs ? sqs->GetPtr() : nullptr;
}

} // namespace

FlightWorld::FlightWorld() : MapWorld(NavMap::Get(), Helipads::Get().All(), Gates::Get().All()) {}

bool FlightWorld::VehicleRay(const Vec3 &aFrom, const Vec3 &aTo) const {
  const Calls &c = Resolve();
  auto *queries = Queries(c, component);
  if (!queries)
    return false;
  RED4ext::Vector4 from{aFrom.x, aFrom.y, aFrom.z, 1.0f}, to{aTo.x, aTo.y, aTo.z, 1.0f};
  RED4ext::CName group("Vehicle");
  RED4ext::physics::TraceResult result{};
  bool staticOnly = false, dynamicOnly = false, hit = false;
  RED4ext::ExecuteFunction(queries, c.byGroup, &hit, from, to, group, result, staticOnly, dynamicOnly);
  return hit;
}

bool FlightWorld::GroundRay(const Vec3 &aFrom, const Vec3 &aTo, Vec3 &aHit) const {
  const Calls &c = Resolve();
  auto *queries = Queries(c, component);
  if (!queries)
    return false;
  RED4ext::Vector4 from{aFrom.x, aFrom.y, aFrom.z, 1.0f}, to{aTo.x, aTo.y, aTo.z, 1.0f};
  RED4ext::physics::QueryFilter filter = c.ground;
  RED4ext::physics::TraceResult result{};
  bool staticOnly = false, dynamicOnly = false, any = false;
  RED4ext::ExecuteFunction(queries, c.byFilter, &any, from, to, filter, result, staticOnly, dynamicOnly);
  bool valid = false;
  RED4ext::ExecuteFunction(RED4ext::ScriptInstance(nullptr), c.isValid, &valid, result);
  if (!valid)
    return false;
  aHit = {result.position.X, result.position.Y, result.position.Z};
  return true;
}

} // namespace Navigation
