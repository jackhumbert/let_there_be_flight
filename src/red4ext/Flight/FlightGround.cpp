#include "FlightGround.hpp"

#include "Flight/Component.hpp"
#include "FlightConfiguration.hpp"
#include "Physics/ScrapeEffects.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"

#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/QueryFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/TraceResult.hpp>

float GetFlightSettingsFloat(RED4ext::CRTTISystem *rtti, RED4ext::CName aProperty, float aFallback);

namespace {

struct Calls {
  RED4ext::CProperty *tires[4] = {};
  RED4ext::CProperty *sqs = nullptr, *distance = nullptr;
  RED4ext::CBaseFunction *localToWorld = nullptr, *raycast = nullptr, *addGroup = nullptr, *isValid = nullptr;
  RED4ext::physics::QueryFilter filter{};
  bool ok = false;
};

const Calls &Resolve() {
  static Calls c = [] {
    Calls c;
    auto rtti = RED4ext::CRTTISystem::Get();
    auto component = rtti->GetClass("FlightComponent");
    auto placed = rtti->GetClass("entIPlacedComponent");
    auto sqs = rtti->GetClass("gameSpatialQueriesSystem");
    auto filter = rtti->GetClass("physicsQueryFilter");
    auto trace = rtti->GetClass("physicsTraceResult");
    if (!component || !placed || !sqs || !filter || !trace)
      return c;
    const char *tires[4] = {"fl_tire", "fr_tire", "bl_tire", "br_tire"};
    for (int i = 0; i < 4; ++i)
      c.tires[i] = component->GetProperty(tires[i]);
    c.sqs = component->GetProperty("sqs");
    c.distance = component->GetProperty("distance");
    c.localToWorld = placed->GetFunction("GetLocalToWorld");
    c.raycast = sqs->GetFunction("SyncRaycastByQueryFilter");
    c.addGroup = filter->GetFunction("AddGroup");
    c.isValid = trace->GetFunction("IsValid");
    c.ok = c.tires[0] && c.tires[1] && c.tires[2] && c.tires[3] && c.sqs && c.distance && c.localToWorld &&
           c.raycast && c.addGroup && c.isValid;
    if (c.ok) {
      // the script's filter: QueryFilter.AddGroup(filter, n"VehicleBlocker"), then n"Water"
      for (RED4ext::CName group : {RED4ext::CName("VehicleBlocker"), RED4ext::CName("Water")})
        RED4ext::ExecuteFunction(RED4ext::ScriptInstance(nullptr), c.addGroup, nullptr, c.filter, group);
    }
    return c;
  }();
  return c;
}

} // namespace

bool FlightGroundNative::Touches(FlightComponent *aComponent, const Flight::StatsState &aState,
                                 const Flight::Hit (&aHits)[4], Flight::SkidPoint *aPoints) {
  if (!aComponent || !aComponent->configuration || !aComponent->configuration->dragAreaMeasured)
    return false;
  auto vehicle = reinterpret_cast<RED4ext::vehicle::BaseObject *>(aComponent->entity);
  if (!vehicle)
    return false;
  const auto origin = vehicle->worldTransform.Position.AsVector4();
  const auto &o = vehicle->worldTransform.Orientation;
  const Flight::Vec3 up = Flight::Rotate(Flight::Quat{o.i, o.j, o.k, o.r}, {0.0f, 0.0f, 1.0f});
  const Flight::Vec3 o3{origin.X, origin.Y, origin.Z};
  const float bottom = std::min(aComponent->configuration->chassisBottom, aComponent->configuration->colliderBottom);
  const Flight::Quat toBody = Flight::Conjugate(aState.d_orientation);
  for (int i = 0; i < 4; ++i) {
    const Flight::Hit &h = aHits[i];
    Flight::SkidPoint p;
    const float overBottom = Flight::Dot(Flight::Sub(h.tire, o3), up) - bottom;
    p.offset = Flight::Rotate(toBody, Flight::Sub(h.tire, aState.d_position));
    p.clearance = h.tire.z - h.position.z - overBottom;
    p.normal = Flight::Normalize(h.normal);
    // where the thruster's scrape marks go (Physics/ScrapeEffects.cpp), and how fast it slides along the ground
    const Flight::Vec3 v = Flight::Add(aState.d_velocity,
                                       Flight::Rotate(aState.d_orientation, Flight::Cross(aState.d_angularVelocity, p.offset)));
    const Flight::Vec3 along = Flight::Sub(v, Flight::Mul(p.normal, Flight::Dot(v, p.normal)));
    // what the ground is, by its ray's material (a hit without one scrapes as hard ground)
    const Flight::Surface surface = h.material ? ScrapeEffects::SurfaceOfMaterial(h.material) : Flight::Surface::Hard;
    Flight::SkidContact &c = aComponent->skidContacts[i];
    const float load = c.load;
    c = {h.valid, h.position, p.normal, p.clearance, Flight::Length(along), surface};
    c.load = load;
    c.material = h.material;
    if (aPoints)
      aPoints[i] = p;
  }
  aComponent->skidContactsAge = 0.0f;
  return true;
}

Flight::Ground FlightGroundNative::Find(RED4ext::IScriptable *aComponent, float aMaxHoverHeight,
                                        Flight::Vec3 *aEmitters, Flight::Hit *aHits) {
  const Calls &c = Resolve();
  if (!c.ok || !aComponent) {
    LTBF_WARN_ONCE("[FlightGround] a FlightComponent field or an engine function was not found: no native ground");
    return {};
  }
  auto sqs = c.sqs->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(aComponent);
  if (!sqs || !*sqs)
    return {};
  float &distance = *c.distance->GetValuePtr<float>(aComponent);
  Flight::Hit hits[4];
  for (int i = 0; i < 4; ++i) {
    auto tire = c.tires[i]->GetValuePtr<RED4ext::Handle<RED4ext::IScriptable>>(aComponent);
    if (!tire || !*tire)
      return {};
    RED4ext::Matrix m{};
    RED4ext::ExecuteFunction(tire->GetPtr(), c.localToWorld, &m);
    const Flight::Vec3 origin{m.W.X, m.W.Y, m.W.Z};
    const Flight::Vec3 start = Flight::GroundRayStart(origin), end = Flight::GroundRayEnd(origin, aMaxHoverHeight);
    RED4ext::Vector4 start4{start.x, start.y, start.z, 1.0f}, end4{end.x, end.y, end.z, 1.0f};
    RED4ext::physics::TraceResult result{};
    // the static world only: started over a bike's emitters a ray otherwise hits the bike itself (Flight::GroundRayStart)
    bool staticOnly = true, dynamicOnly = false, hitAny = false;
    RED4ext::physics::QueryFilter filter = c.filter;
    RED4ext::ExecuteFunction(sqs->GetPtr(), c.raycast, &hitAny, start4, end4, filter, result, staticOnly, dynamicOnly);
    bool valid = false;
    RED4ext::ExecuteFunction(RED4ext::ScriptInstance(nullptr), c.isValid, &valid, result);
    hits[i] = {valid, origin, {result.position.X, result.position.Y, result.position.Z},
               {result.normal.X, result.normal.Y, result.normal.Z}, result.material.hash};
  }
  if (aEmitters) {
    *aEmitters = {};
    for (const Flight::Hit &h : hits)
      *aEmitters = Flight::Add(*aEmitters, Flight::Mul(h.tire, 0.25f));
  }
  if (aHits)
    for (int i = 0; i < 4; ++i)
      aHits[i] = hits[i];
  const Flight::Ground g = Flight::GroundFromHits(hits, 4, distance);
  if (g.found)
    distance = g.distance;
  return g;
}

RED4ext::Vector4 FlightGround::Find(RED4ext::Handle<RED4ext::IScriptable> aComponent) {
  const float maxHover = GetFlightSettingsFloat(RED4ext::CRTTISystem::Get(), "hoverModeMaxHoverHeight", 7.0f);
  const Flight::Ground g = FlightGroundNative::Find(aComponent.GetPtr(), maxHover);
  return g.found ? RED4ext::Vector4{g.normal.x, g.normal.y, g.normal.z, g.distance} : RED4ext::Vector4{0, 0, 0, -1};
}
