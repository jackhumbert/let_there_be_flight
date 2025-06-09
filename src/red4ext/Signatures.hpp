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

// update with pid
// vehicle->sub_2A8
/// @hash 1414536155
// void __fastcall VehicleUpdateOrientationWithPID(RED4ext::vehicle::CarBaseObject *a1, RED4ext::Transform *, float, float);

/// @hash 2268865059
// void __fastcall FourWheelTorque(RED4ext::vehicle::WheeledPhysics *physics, unsigned __int8 rearWheelIndex,
                                // unsigned __int8 frontWheelIndex, float a4, RED4ext::Transform *transform);

/// @hash 1404578149
// RED4ext::CRTTIArrayType **__fastcall CreateCRTTIArrayTypeFromClass(RED4ext::CRTTIArrayType **a1, RED4ext::CBaseRTTIType *a2);

/// @hash 1992632754
// RED4ext::CRTTIHandleType **__fastcall CreateCRTTIHandleTypeFromClass(RED4ext::CRTTIHandleType **a1,
                                                                    //  RED4ext::CBaseRTTIType *a2);

/// @hash 682896186
// RED4ext::CRTTIWeakHandleType **__fastcall CreateCRTTIWeakHandleTypeFromClass(RED4ext::CRTTIWeakHandleType **a1,
                                                                            //  RED4ext::CBaseRTTIType *a2);

/// @hash 2481073725
// RED4ext::CRTTIResourceAsyncReferenceType **__fastcall CreateCRTTIRaRefTypeFromClass(RED4ext::CRTTIResourceAsyncReferenceType **a1,
                                                                              //  RED4ext::CBaseRTTIType *a2);

// struct ScriptData;

/// @hash 898639042
// bool __fastcall ProcessScriptTypes(uint32_t *version, ScriptData *scriptData, void *scriptLogger);

// 1.6  RVA: 0x204390
// 1.61 RVA: 0x204940
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 60 41 0F B6 D8 48 8B FA 48 8B F1 48 C7 44 24 20 00 00
// void * LoadResRefT(void *, void*, bool);

// 1.52 RVA: 0x9A4290 / 10109584
// 1.61hf1 RVA: 0x9AE2D0 / 10150608
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 81 EC A0 00 00 00 48 8D 4C 24 30 49 8B F0 48 8B FA E8
//using GetLocalizedTextScripts = void __fastcall (void *, RED4ext::CStackFrame *, RED4ext::CString *);

/// @hash 2680167661
// RED4ext::CString * __fastcall LookupLocalizedString(__int64, RED4ext::CString * result, char * locKey);

/// @hash 2229148842
// RED4ext::CString *__fastcall SetLocalizedTextString(__int64, RED4ext::CString *, RED4ext::CString *);

/// @hash 1989218322
// void __fastcall RollFactorTorqueThing(uint64_t *a1, float a2);