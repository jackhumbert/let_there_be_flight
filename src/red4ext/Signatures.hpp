#pragma once
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
// #include "Addresses.hpp"
#include <RED4ext/Scripting/Natives/Generated/game/EffectSystem.hpp>
#include <RED4ext/Scripting/Natives/vehicleChassisComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/GameAudioSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/GameInstance.hpp>
//#include <RED4ext/Scripting/Natives/UpdateManager.hpp>

// 1.6  RVA: 0x2BE7400 / 46035968
// 1.62h1  RVA: 0x2C23B90 / 46283664
/// @pattern 40 55 56 57 41 56 B8 68 20 00 00 E8 ?  ?  E7 FF 48 2B E0 49 8B E9 49 8B F0 8B FA 4C 8B F1 F6 C2
// void __fastcall ExecuteCompileCommand(__int64 a1, char a2, RED4ext::CString *a3, RED4ext::CString *command);

// 1.6  RVA: 0x2BE75E0 / 46036448
// 1.62hf1  RVA: 0x2C23D70 / 46284144
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 40 48 8B FA 48 8B F1 48 8D 54 24 30 49 8B C9 49 8B D8
// __int64 __fastcall ExecuteCommand(void *scriptCompilation, RED4ext::CString *, RED4ext::CString *, __int64 currentDirectoryThing, char);

// update with pid
// 1.6  RVA: 0x1C9A750
// 1.61 RVA: 0x1C9A9B0
/// @pattern 48 8B C4 F3 0F 11 58 20 F3 0F 11 50 18 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 58 FC FF FF
void __fastcall VehicleUpdateOrientationWithPID(RED4ext::vehicle::CarBaseObject *a1, RED4ext::Transform *, float, float);

// something with 4 wheels
// 1.6  RVA: 0x1D3B030
// 1.61 RVA: 0x1D3B3F0
/// @pattern 40 53 48 81 EC A0 00 00 00 44 0F B6 D2 4C 8D 89 D0 05 00 00 41 0F B6 C0 48 8B D9 4D 69 C2 30 01
void __fastcall FourWheelTorque(RED4ext::vehicle::WheeledPhysics *physics, unsigned __int8 rearWheelIndex,
                                unsigned __int8 frontWheelIndex, float a4, RED4ext::Transform *transform);

// 1.61hf1 RVA: 0x200660 / 2098784
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 20 00 00 00 48 8D 4C 24 20 E8
/// @nth 0/2
RED4ext::CRTTIArrayType **__fastcall CreateCRTTIArrayTypeFromClass(RED4ext::CRTTIArrayType **a1, RED4ext::CBaseRTTIType *a2);

// 1.52 RVA: 0x1FBD20 / 2080032
// 1.6  RVA: 0x200050 / 2080032
// 1.61 RVA: 0x200600
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 28 00 00 00 48 8D 4C 24 20 E8
/// @nth 0/7
RED4ext::CRTTIHandleType **__fastcall CreateCRTTIHandleTypeFromClass(RED4ext::CRTTIHandleType **a1,
                                                                     RED4ext::CBaseRTTIType *a2);

// 1.52 RVA: 0x1FC0C0 / 2080960
// 1.6  RVA: 0x2003F0 / 2098160
// 1.61 RVA: 0x2009A0
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 28 00 00 00 48 8D 4C 24 20 E8
/// @nth 4/7
RED4ext::CRTTIWeakHandleType **__fastcall CreateCRTTIWeakHandleTypeFromClass(RED4ext::CRTTIWeakHandleType **a1,
                                                                             RED4ext::CBaseRTTIType *a2);

// 1.61hf1 RVA: 0x200920 / 2099488
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 28 00 00 00 48 8D 4C 24 20 E8
/// @nth 2/7
RED4ext::CRTTIResourceAsyncReferenceType **__fastcall CreateCRTTIRaRefTypeFromClass(RED4ext::CRTTIResourceAsyncReferenceType **a1,
                                                                               RED4ext::CBaseRTTIType *a2);

struct ScriptData;

// 1.52 RVA: 0x273160 / 2568544
// 1.6  RVA: 0x276F30 / 2584368
// 1.61 RVA: 0x2774E0
/// @pattern 48 8B C4 4C 89 40 18 48 89 48 08 55 53 48 8D 68 A1 48 81 EC A8 00 00 00 48 89 70 10 48 8B DA 48
bool __fastcall ProcessScriptTypes(uint32_t *version, ScriptData *scriptData, void *scriptLogger);

// 1.6  RVA: 0x204390
// 1.61 RVA: 0x204940
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 60 41 0F B6 D8 48 8B FA 48 8B F1 48 C7 44 24 20 00 00
void * LoadResRefT(void *, void*, bool);

// 1.52 RVA: 0x9A4290 / 10109584
// 1.61hf1 RVA: 0x9AE2D0 / 10150608
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 81 EC A0 00 00 00 48 8D 4C 24 30 49 8B F0 48 8B FA E8
//using GetLocalizedTextScripts = void __fastcall (void *, RED4ext::CStackFrame *, RED4ext::CString *);

// 1.61hf1 RVA: 0x6FA540 / 7316800
/// @pattern 40 53 48 83 EC 30 F2 41 0F 10 00 48 8B DA 41 8B 40 08 4C 8D 44 24 20 44 0F B6 89 39 04 00 00 F2
RED4ext::CString * __fastcall LookupLocalizedString(__int64, RED4ext::CString * result, char * locKey);

// 1.61hf1 RVA: 0x6FA5B0 / 7316912
/// @pattern 40 53 48 83 EC 30 F2 41 0F 10 00 48 8B DA 41 8B 40 08 48 8D 54 24 20 F2 0F 11 44 24 20 89 44 24
RED4ext::CString *__fastcall SetLocalizedTextString(__int64, RED4ext::CString *, RED4ext::CString *);

// 1.6 RVA: 0x1D5B9A0 / 30783904
/// @pattern 40 55 41 54 41 55 41 57 48 8D 6C 24 88 48 81 EC 78 01 00 00 4C 8B F9 44 0F 29 A4 24 00 01 00 00
void __fastcall RollFactorTorqueThing(uint64_t *a1, float a2);