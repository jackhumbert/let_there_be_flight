#include "WheelRest.hpp"

#include "Flight/Component.hpp"
#include "Flight/FlightGround.hpp"
#include "Flight/FlightStats.hpp"
#include "FlightConfiguration.hpp"
#include "stdafx.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/gamedataTweakDBRecord.hpp>
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace WheelRest {

namespace {

// FlightGroundNative::Find's hover height: its rays reach 10 m further, 12 m under the emitters
constexpr float kReach = 2.0f;

// a car never measured: the origin over the ground as a multiple of the chassis bottom's depth under it, and without one
constexpr float kEstimateFactor = 2.2f;
constexpr float kEstimateDefault = 0.4f;

std::mutex lock;
std::unordered_map<uint64_t, float> heights; // by vehicle record

// the car's vehicle record, from its FlightStats' s_record (0 when there is none)
uint64_t RecordOf(RED4ext::IScriptable *aComponent) {
  auto *stats = FlightStats::Of(aComponent);
  if (!stats)
    return 0;
  static RED4ext::CProperty *prop = [] {
    auto *cls = RED4ext::CRTTISystem::Get()->GetClass("FlightStats");
    return cls ? cls->GetProperty("s_record") : nullptr;
  }();
  if (!prop)
    return 0;
  auto *record = prop->GetValuePtr<RED4ext::WeakHandle<RED4ext::IScriptable>>(stats);
  if (!record || !record->instance || record->Expired())
    return 0;
  return reinterpret_cast<RED4ext::game::data::TweakDBRecord *>(record->instance)->recordID.value;
}

bool Origin(RED4ext::IScriptable *aComponent, Flight::Vec3 &aOrigin, float &aUpZ) {
  auto *component = static_cast<FlightComponent *>(aComponent);
  auto *vehicle = component ? reinterpret_cast<RED4ext::vehicle::BaseObject *>(component->entity) : nullptr;
  if (!vehicle)
    return false;
  const auto p = vehicle->worldTransform.Position.AsVector4();
  const auto &o = vehicle->worldTransform.Orientation;
  aOrigin = {p.X, p.Y, p.Z};
  aUpZ = Flight::Rotate(Flight::Quat{o.i, o.j, o.k, o.r}, {0.0f, 0.0f, 1.0f}).z;
  return true;
}

} // namespace

Ground Under(RED4ext::IScriptable *aComponent) {
  Flight::Hit hits[4];
  FlightGroundNative::Find(aComponent, kReach, nullptr, hits);
  Ground g;
  float low = 1e9f, high = -1e9f, sum = 0.0f;
  for (const auto &h : hits) {
    if (!h.valid)
      return {};
    low = std::min(low, h.position.z);
    high = std::max(high, h.position.z);
    sum += h.position.z;
  }
  g.valid = true;
  g.z = sum * 0.25f;
  g.spread = high - low;
  return g;
}

void Measure(FlightComponent *aComponent) {
  Flight::Vec3 origin;
  float upZ = 0.0f;
  if (!aComponent || !Origin(aComponent, origin, upZ))
    return;
  const Ground g = Under(aComponent);
  const float over = origin.z - g.z;
  const float bottom = aComponent->configuration ? aComponent->configuration->chassisBottom : 0.0f;
  const float colliders = aComponent->configuration ? aComponent->configuration->colliderBottom : 1e9f;
  if (!g.valid || upZ < 0.985f || g.spread > 0.08f || std::fabs(over) > 0.6f) {
    spdlog::info("[WheelRest] not on its wheels at flight on (ground {}, spread {:.2f} m, up {:.3f}, origin {:.2f} m "
                 "over it): not measured",
                 g.valid ? "found" : "not found", g.spread, upZ, over);
    return;
  }
  const uint64_t record = RecordOf(aComponent);
  {
    std::lock_guard _(lock);
    heights[record] = over;
  }
  spdlog::info("[WheelRest] resting on its wheels: origin {:.3f} m over the ground (spread {:.3f} m, chassis bottom "
               "{:.3f} m, thrusters' bottom {:.3f} m in the car's frame), record {:#x}",
               over, g.spread, bottom, colliders, record);
}

float OriginHeight(RED4ext::IScriptable *aComponent, bool *aMeasured) {
  const uint64_t record = RecordOf(aComponent);
  {
    std::lock_guard _(lock);
    auto it = heights.find(record);
    if (aMeasured)
      *aMeasured = it != heights.end();
    if (it != heights.end())
      return it->second;
  }
  auto *component = static_cast<FlightComponent *>(aComponent);
  const auto *config = component ? component->configuration.GetPtr() : nullptr;
  if (config && config->dragAreaMeasured && config->chassisBottom < 0.0f)
    return kEstimateFactor * -config->chassisBottom;
  return kEstimateDefault;
}

float RestHeight(RED4ext::IScriptable *aComponent, float aZ) {
  Flight::Vec3 origin;
  float upZ = 0.0f;
  if (!aComponent || !Origin(aComponent, origin, upZ))
    return 0.0f;
  return OriginHeight(aComponent) + aZ - origin.z;
}

} // namespace WheelRest
