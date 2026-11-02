#pragma once

// FlightCarSummon: the VehicleSystem's own record spawn, for the pad summon
// (src/redscript/Summon/FlightSummonSystem.reds, docs/design/vehicle-summon.md). The vanilla
// summon cannot deliver a car at a landing pad: its spawn-point search is tied to road-side
// summon spots. The garage's other path - the quest node's summon-to-transform call
// (quest::SpawnPlayerVehicle_NodeType::OnExecute, the VehicleSystem vtable slot at +616 with
// a resolved GarageVehicleID, drive-in flag and three zeros filled after) - only feeds the
// garage machine, which drives on traffic lanes (none at a pad). And the script-side helpers
// this feature could lean on are unreliable in a modded launch: the TweakDB natives' runtime
// wrappers (TweakDBInterface.GetVehicleRecord, TDBID.Create, ...) carry unbound native
// pointers and crash the game at the call, and GarageVehicleID.Resolve leaves recordID zero
// when its own record check (GetRecordHandle) misses (docs section 4).
//
// So the spawn is game::VehicleSystem::RequestNewVehicle(WorldTransform const&, TweakDBID,
// red::FixedSizeFunction<void(EntityStub*),64,8> const&): it prepares an entity id and hands the
// record to the stub set through the VehicleSystem's own wrapper. The record comes from the
// player's unlocked vehicles (the garage's own list), and the callback - 64 bytes of storage
// plus the erased dispatcher at +64 - is this class's own (FlightCarSummon.cpp): at stub
// materialization the machinery calls the dispatcher's [0] unconditionally (a zeroed callback
// crashed that way) and [0] is where the spawned entity's id lands.
//
// The vtable entry is not in the shipped address DB, so the call scans the live vtable for the
// 2.5 image offset of the function; no entry matches = false and the summon stays vanilla
// (which refuses at a pad rather than crash).
#include <RED4ext/Common.hpp>
#include <RED4ext/NativeTypes.hpp>
#include <RED4ext/Scripting/Natives/entEntityID.hpp>
#include <RedLib.hpp>

struct FlightCarSummon : RED4ext::IScriptable {
  // Spawns the vehicle aRecord stands for at (aX, aY, aZ), yaw in degrees - the pads' heading
  // convention, +Y turned by it, CCW. False when the system is not there, the record is not a
  // TweakDB record or the vtable slot is missing. The spawn's entity is not returned here:
  // LastSpawnEntity hands it to the summon machine's tick when the stub materializes.
  static bool SpawnVehicleAt(RED4ext::TweakDBID aRecord, float aX, float aY, float aZ, float aYaw);

  // The entity id of the last vehicle this class spawned, from its callback's stub; empty
  // (zero) until one materializes. The summon machine's tick reads it to find its vehicle.
  static RED4ext::ent::EntityID LastSpawnEntity();
  RTTI_IMPL_TYPEINFO(FlightCarSummon);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightCarSummon, { RTTI_METHOD(SpawnVehicleAt); RTTI_METHOD(LastSpawnEntity); });