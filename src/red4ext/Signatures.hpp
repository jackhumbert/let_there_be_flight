#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include "Addresses.hpp"

// right before components are processed for entites, and an appropriate time to insert our own
// can also look for string "Entity/InitializeComponents"
/// @pattern 48 89 54 24 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? FB FF FF 48 81 EC ? 05 00 00
/// @nth 0/2
using Entity_InitializeComponents = void (RED4ext::ent::Entity * entity, void * a2, void * a3);

// processes weapon firing for vehicles - we can check the cycleTimer value after to see if something was fired
// 1.5 added a byte in the middle of this pattern, which makes it hard to match with ?
/// @pattern 48 8B C4 55 56 41 54 41 55 41 56 41 57 48 8D A8
using VehicleProcessWeapons = void (RED4ext::vehicle::BaseObject *vehicle, float timeDelta, unsigned int shootIndex);

// 1.52 RVA: 0x184300 / 1590016
/// @pattern 48 83 EC 28 49 B9 25 23 22 84 E4 9C F2 CB 48 8D 05 E3 6B 47 03 41 B2 62 49 BB B3 01 00 00 00 01
using ReadFromGameSystemsStartFile = int();

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
RED4ext::Matrix *__fastcall GetMatrixFromOrientation(RED4ext::Quaternion *q, RED4ext::Matrix *m);

// 1.6 RVA: 0x1CF3C10
/// @pattern F3 0F 10 42 04 8B 02 F3 0F 10 4A 08 F3 0F 11 81 B4 02 00 00 F3 0F 11 89 B8 02 00 00 89 81 B0 02
using TPPCameraStatsUpdate = uintptr_t (RED4ext::vehicle::TPPCameraComponent *camera, uintptr_t data);


// 1.6 RVA: 0x17190A0
/// @pattern 48 8B C4 F3 0F 11 48 10 53 48 81 EC 30 01 00 00 80 B9 A0 04 00 00 00 48 8B D9 0F 29 70 D8 0F 28
using FPPCameraUpdate = char __fastcall(RED4ext::game::FPPCameraComponent *fpp, float deltaTime, float deltaYaw,
                                        float deltaPitch, float deltaYawExternal, float deltaPitchExternal, char a7);