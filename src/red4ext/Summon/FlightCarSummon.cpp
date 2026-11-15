#include "Summon/FlightCarSummon.hpp"

#include <Windows.h>
#include <spdlog/spdlog.h>

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/entEntityID.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Transform.hpp>
#include <RED4ext/TweakDB.hpp>

// game::VehicleSystem::RequestNewVehicle(WorldTransform const&, TweakDBID,
// red::FixedSizeFunction<void(EntityStub*),64,8> const&), a virtual bool on 2.5 (the vtable's
// entry there; its neighbours hold RestoreVehicle, then DespawnVehicle). This binary's offset
// only seeds the live vtable scan (see FlightCarSummon.hpp).
constexpr uintptr_t kRequestNewVehicleOffset = 0x21AD480;

namespace {
// The red::FixedSizeFunction the spawn takes: 64 bytes of storage plus the erased dispatcher
// (the table of handlers) at +64. The machinery reads that slot unconditionally and calls its
// [0] with (the wrapper's capture, the EntityStub*'s location) when the stub materializes, its
// [8] when copying the callable (dest, this) and its [24] to destroy (the buffer) : a
// zeroed callback puts null there, and the materialize's call through it crashes the game (docs/design/vehicle-summon.md section 3). The table's
// unfilled entries point at the destroy no-op: a call through any slot lands somewhere safe.
RED4ext::ent::EntityID g_lastSpawnedEntity;

void __fastcall SpawnDispatchInvoke(void* /*aCapture*/, void** aStubPtr) {
  auto* stub = aStubPtr ? *aStubPtr : nullptr;
  if (stub) {
    // the stub's entity id, the field the machinery's own broadcast reads
    g_lastSpawnedEntity =
        *reinterpret_cast<RED4ext::ent::EntityID*>(reinterpret_cast<uintptr_t>(stub) + 64);
  }
}

void __fastcall SpawnDispatchCopy(void* aDest, void* aSrc) {
  auto* dest = reinterpret_cast<void**>(aDest);
  memset(dest, 0, 64);
  dest[8] = aSrc ? *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(aSrc) + 64) : nullptr;
}

void __fastcall SpawnDispatchNothing(void* /*aBuffer*/) {}

void* g_spawnDispatcher[8] = {
    SpawnDispatchInvoke, SpawnDispatchCopy, SpawnDispatchNothing, SpawnDispatchNothing,
    SpawnDispatchNothing, SpawnDispatchNothing, SpawnDispatchNothing, SpawnDispatchNothing,
};
} // namespace

bool FlightCarSummon::SpawnVehicleAt(RED4ext::TweakDBID aRecord, float aX, float aY, float aZ,
                                     float aYaw) {
  spdlog::info("[FlightCarSummon] SpawnVehicleAt: record {:016x} at ({:0.1f}, {:0.1f}, {:0.1f}) yaw {:0.0f}",
               aRecord.value, aX, aY, aZ, aYaw);
  ZeroMemory(&g_lastSpawnedEntity, sizeof(g_lastSpawnedEntity)); // no stale capture from an earlier summon
  auto* rtti = RED4ext::CRTTISystem::Get();
  // the RTTI type's name is "gameVehicleSystem"; the script-visible "VehicleSystem" is only
  // its alias and the hash lookup here finds nothing for it
  auto* type = rtti ? rtti->GetType("gameVehicleSystem") : nullptr;
  if (!type) {
    spdlog::warn("[FlightCarSummon] no gameVehicleSystem type in the RTTI - no spawn");
    return false;
  }
  auto* engine = RED4ext::CGameEngine::Get();
  auto* system = engine && engine->framework && engine->framework->gameInstance
                     ? engine->framework->gameInstance->GetSystem(type)
                     : nullptr;
  if (!system || !aRecord.value) {
    spdlog::warn("[FlightCarSummon] no VehicleSystem system or a zero record - no spawn");
    return false;
  }
  // the record must be in TweakDB at all - through the C++ API, not the script native: the
  // GetVehicleRecord wrapper's native binding can be unbound in a modded launch and the script
  // call crashes no matter whose frame it is (docs/design/vehicle-summon.md section 3)
  auto* tweaks = RED4ext::TweakDB::Get();
  RED4ext::Handle<RED4ext::IScriptable> vehicleRecord;
  if (!tweaks || !tweaks->TryGetRecord(aRecord, vehicleRecord) || !vehicleRecord) {
    spdlog::warn("[FlightCarSummon] record {:016x} is not a TweakDB record - no spawn", aRecord.value);
    return false;
  }
  auto* vtable = *reinterpret_cast<void***>(system);
  if (!vtable) {
    return false;
  }
  const auto expected = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr)) + kRequestNewVehicleOffset;
  for (uintptr_t i = 0; i < 256; ++i) {
    auto* entry = vtable[i];
    if (reinterpret_cast<uintptr_t>(entry) != expected) {
      continue;
    }
    spdlog::info("[FlightCarSummon] RequestNewVehicle at the VehicleSystem vtable slot {}", i);
    // (WorldTransform const&: three int32 of the fixed-point position, the float read as the
    // value scaled by 2^17 (math::WorldTransform::_ToXForm multiplies by 2^-17), then the
    // quaternion; TweakDBID by value; our FixedSizeFunction const& - storage plus the
    // dispatcher at +64.) Verified against the 2.5 binary (the summon machine's own spawn, and
    // a spawn that passed floats here landed 8 km up); a vtable that no longer holds the offset never
    // gets here.
    struct alignas(16) WorldTransform {
      int32_t x;
      int32_t y;
      int32_t z;
      int32_t zero;
      RED4ext::Quaternion orientation; // 10
    };
    const auto yawRad = aYaw * 0.0174532925f;
    WorldTransform at{};
    at.x = static_cast<int32_t>(aX * 131072.0f);
    at.y = static_cast<int32_t>(aY * 131072.0f);
    at.z = static_cast<int32_t>(aZ * 131072.0f);
    at.orientation =
        RED4ext::Quaternion(0.0f, 0.0f, std::sin(yawRad * 0.5f), std::cos(yawRad * 0.5f));
    alignas(8) struct {
      uint8_t storage[64];
      void* dispatcher;
    } callback = {};
    callback.dispatcher = g_spawnDispatcher;
    using RequestFn = bool(__fastcall*)(void*, const void*, uint64_t, const void*);
    return reinterpret_cast<RequestFn>(entry)(system, &at, aRecord.value, &callback);
  }
  spdlog::warn(
      "[FlightCarSummon] RequestNewVehicle is not where 2.5 had it - the pad summon yields to vanilla");
  return false;
}

RED4ext::ent::EntityID FlightCarSummon::LastSpawnEntity() {
  return g_lastSpawnedEntity;
}