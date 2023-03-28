#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include "Addresses.hpp"
#include <RED4ext/Scripting/Natives/Generated/game/EffectSystem.hpp>
#include <RED4ext/Scripting/Natives/vehicleChassisComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/GameAudioSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/GameInstance.hpp>
//#include <RED4ext/Scripting/Natives/UpdateManager.hpp>


// right before components are processed for entities, and an appropriate time to insert our own
// can also look for string "Entity/InitializeComponents"
/// @pattern 48 89 54 24 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? FB FF FF 48 81 EC ? 05 00 00
/// @nth 0/2
using Entity_InitializeComponents = void (RED4ext::ent::Entity * entity, void * a2, void * a3);

// processes weapon firing for vehicles - we can check the cycleTimer value after to see if something was fired
// 1.5 added a byte in the middle of this pattern, which makes it hard to match with ?
/// @pattern 48 8B C4 55 56 41 54 41 55 41 56 41 57 48 8D A8
using VehicleProcessWeapons = void (RED4ext::vehicle::BaseObject *vehicle, float timeDelta, unsigned int shootIndex);

// not used anymore
// 1.52 RVA: 0x184300 / 1590016
/// @pattern 48 83 EC 28 49 B9 25 23 22 84 E4 9C F2 CB 48 8D 05 E3 6B 47 03 41 B2 62 49 BB B3 01 00 00 00 01
//using ReadFromGameSystemsStartFile = int();

// look for base\\systems\\game_systems_startup.csv
// 1.52
// 40 53 48 83 EC 20 48 8B D9 48 8D 4C 24 30 E8 FD F9 48 FD 48 8B D0 48 8B CB E8 12 29 4A FD 48 8D
// 1.6 RVA: 0x2D5BCD0 / 47561936
// 40 53 48 83 EC 20 48 8B D9 48 8D 4C 24 30 E8 7D A7 43 FD 48 8B D0 48 8B CB E8 92 D6 44 FD 48 8D
/// @pattern 40 53 48 83 EC 20 48 8B D9 48 8D 4C 24 30 E8 ?  ?  ?  FD 48 8B D0 48 8B CB E8 ?  ?  ?  FD 48 8D
/// @nth 6/7
using GetGameSystemsData = RED4ext::DynArray<RED4ext::GameSystemData> *(RED4ext::DynArray<RED4ext::GameSystemData> *gameSystemsData);

// 1.52 RVA: 0x1C58B0 / 1857712
// 1.6 RVA: 0x1C9A30 / 1874480
/// @pattern 4C 8B DC 48 81 EC B8 00 00 00 0F 10 21 41 0F 29 73 E8 0F 28 DC 0F 59 DC C7 44 24 0C 00 00 00 00
using GetMatrixFromOrientation = RED4ext::Matrix *__fastcall(RED4ext::Quaternion *q, RED4ext::Matrix *m);

// 1.6 RVA: 0x1CF3C10
/// @pattern F3 0F 10 42 04 8B 02 F3 0F 10 4A 08 F3 0F 11 81 B4 02 00 00 F3 0F 11 89 B8 02 00 00 89 81 B0 02
using TPPCameraStatsUpdate = uintptr_t (RED4ext::vehicle::TPPCameraComponent *camera, uintptr_t data);


// 1.6 RVA: 0x17190A0
/// @pattern 48 8B C4 F3 0F 11 48 10 53 48 81 EC 30 01 00 00 80 B9 A0 04 00 00 00 48 8B D9 0F 29 70 D8 0F 28
using FPPCameraUpdate = char __fastcall (RED4ext::game::FPPCameraComponent *fpp, float deltaTime, float deltaYaw,
                                        float deltaPitch, float deltaYawExternal, float deltaPitchExternal, char a7);

// main vehicle physics update
// 1.6  RVA: 0x1D3C5D0
// 1.61 RVA: 0x1D3C990
/// @pattern F3 0F 11 4C 24 10 55 53 57 41 54 41 55 41 56 48 8D AC 24 98 FD FF FF 48 81 EC 68 03 00 00 48 8B
using VehiclePhysicsUpdate = uintptr_t __fastcall (RED4ext::vehicle::Physics *, float);

// where driverHelpers are processed
// vehicleWheeledPhysics::sub_58
// 1.6  RVA: 0x1D3EB10
// 1.61 RVA: 0x1D3EED0
/// @pattern 48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 41 56 48 81 EC B0 00 00 00 0F 29 70 E8 4C 8B F1 0F
uintptr_t __fastcall VehicleHelperUpdate(RED4ext::vehicle::WheeledPhysics *, float);

// 1.6  RVA: 0x1D3AD50
// 1.61 RVA: 0x1D3B110
/// @pattern 48 8B C4 53 48 81 EC A0 00 00 00 0F 29 70 E8 48 8B D9 0F 29 78 D8 44 0F 29 40 C8 44 0F 29 48 B8
void __fastcall ProcessAirResistance(RED4ext::vehicle::WheeledPhysics *a1, float deltaTime);

// update with pid
// 1.6  RVA: 0x1C9A750
// 1.61 RVA: 0x1C9A9B0
/// @pattern 48 8B C4 F3 0F 11 58 20 F3 0F 11 50 18 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 58 FC FF FF
using VehicleUpdateOrientationWithPID = void __fastcall(RED4ext::vehicle::CarBaseObject *a1, RED4ext::Transform *, float, float);

// something with 4 wheels
// 1.6  RVA: 0x1D3B030
// 1.61 RVA: 0x1D3B3F0
/// @pattern 40 53 48 81 EC A0 00 00 00 44 0F B6 D2 4C 8D 89 D0 05 00 00 41 0F B6 C0 48 8B D9 4D 69 C2 30 01
using FourWheelTorque = void __fastcall (RED4ext::vehicle::WheeledPhysics *physics, unsigned __int8 rearWheelIndex,
                                unsigned __int8 frontWheelIndex, float a4, RED4ext::Transform *transform);

// 1.52 RVA : 0x1478200
// 1.6  RVA: 0x148ED00 / 21556480
// 1.61 RVA: 0x148F3F0
/// @pattern 48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 48 89 4C 24 08 55 41 54 41 55 41 56 41 57 48 8D 6C 24 D0 48 81 EC 30 01 00 00 4C 8B AD 80 00 00 00
using CreateStaticEffect = uintptr_t (RED4ext::game::EffectSystem *, uintptr_t, uint64_t, uint64_t, uintptr_t, uintptr_t);

// 1.6  RVA: 0x1D0E180 / 30466432
// 1.61 RVA: 0x1D0E540
/// @pattern 40 53 48 81 EC 80 00 00 00 F3 0F 10 41 40 48 8B D9 F3 0F 10 51 08 0F 28 C8 F3 0F 59 09 0F 29 74
using PhysicsStructUpdate = short (RED4ext::vehicle::PhysicsData *ps);

// 1.52 RVA: 0x1CE0FC0 / 30281664
// 1.6  RVA: 0x1D0D770 / 30463856
// 1.61 RVA: 0x1D0DB30
/// @pattern 48 89 5C 24 08 57 48 83 EC 30 0F 29 74 24 20 48 8B DA 0F 10 32 48 8B F9 66 0F 3A 40 F6 7F 0F 28
using PhysicsUnkStructVelocityUpdate = short (RED4ext::vehicle::PhysicsData *ps, RED4ext::Vector3 *);

// 1.62 RVA: 0x200660 / 2098784
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 20 00 00 00 48 8D 4C 24 20 E8
/// @nth 0/2
using CreateCRTTIArrayTypeFromClass = RED4ext::CRTTIArrayType **__fastcall(RED4ext::CRTTIArrayType **a1, RED4ext::CBaseRTTIType *a2);

// 1.52 RVA: 0x1FBD20 / 2080032
// 1.6  RVA: 0x200050 / 2080032
// 1.61 RVA: 0x200600
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 28 00 00 00 48 8D 4C 24 20 E8
/// @nth 0/7
using CreateCRTTIHandleTypeFromClass = RED4ext::CRTTIHandleType **__fastcall(RED4ext::CRTTIHandleType **a1,
                                                                     RED4ext::CBaseRTTIType *a2);

// 1.52 RVA: 0x1FC0C0 / 2080960
// 1.6  RVA: 0x2003F0 / 2098160
// 1.61 RVA: 0x2009A0
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 28 00 00 00 48 8D 4C 24 20 E8
/// @nth 4/7
using CreateCRTTIWeakHandleTypeFromClass = RED4ext::CRTTIWeakHandleType **__fastcall (RED4ext::CRTTIWeakHandleType **a1,
                                                                             RED4ext::CBaseRTTIType *a2);

// 1.62 RVA: 0x200920 / 2099488
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 28 00 00 00 48 8D 4C 24 20 E8
/// @nth 2/7
using CreateCRTTIRaRefTypeFromClass = RED4ext::CRTTIResourceAsyncReferenceType **__fastcall (RED4ext::CRTTIResourceAsyncReferenceType **a1,
                                                                               RED4ext::CBaseRTTIType *a2);

struct ScriptData;

// 1.52 RVA: 0x273160 / 2568544
// 1.6  RVA: 0x276F30 / 2584368
// 1.61 RVA: 0x2774E0
/// @pattern 48 8B C4 4C 89 40 18 48 89 48 08 55 53 48 8D 68 A1 48 81 EC A8 00 00 00 48 89 70 10 48 8B DA 48
using ProcessScriptTypes = bool __fastcall (uint32_t *version, ScriptData *scriptData, void *scriptLogger);

// 1.6  RVA: 0x204390
// 1.61 RVA: 0x204940
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 60 41 0F B6 D8 48 8B FA 48 8B F1 48 C7 44 24 20 00 00
using LoadResRefT = void * (void *, void*, bool);

// 1.52 RVA: 0x9A4290 / 10109584
// 1.62 RVA: 0x9AE2D0 / 10150608
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 81 EC A0 00 00 00 48 8D 4C 24 30 49 8B F0 48 8B FA E8
//using GetLocalizedTextScripts = void __fastcall (void *, RED4ext::CStackFrame *, RED4ext::CString *);

// 1.62 RVA: 0x6FA540 / 7316800
/// @pattern 40 53 48 83 EC 30 F2 41 0F 10 00 48 8B DA 41 8B 40 08 4C 8D 44 24 20 44 0F B6 89 39 04 00 00 F2
using LookupLocalizedString = RED4ext::CString * __fastcall (__int64, RED4ext::CString * result, char * locKey);

// 1.62 RVA: 0x6FA5B0 / 7316912
/// @pattern 40 53 48 83 EC 30 F2 41 0F 10 00 48 8B DA 41 8B 40 08 48 8D 54 24 20 F2 0F 11 44 24 20 89 44 24
using SetLocalizedTextString = RED4ext::CString *__fastcall (__int64, RED4ext::CString *, RED4ext::CString *);

// 1.6 RVA: 0x1D5B9A0 / 30783904
/// @pattern 40 55 41 54 41 55 41 57 48 8D 6C 24 88 48 81 EC 78 01 00 00 4C 8B F9 44 0F 29 A4 24 00 01 00 00
using RollFactorTorqueThing = void __fastcall (uint64_t *a1, float a2);