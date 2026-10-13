#pragma once
// LTBF R41 (macOS): the fork's vehicleChassisComponent.hpp redefines vehicle::ChassisComponent (with ITransformAttachable as a second
// base and the physics proxy fields). On macOS LTBF uses the SDK's generated ChassisComponent (reflected layout: collisionResource
// 0x128, optionalPlayerOnlyCollisionResource 0x140, size 0x190); the only fields LTBF reads beyond it (proxyID 0x160, sharedMutex
// 0x16C, R41b) are used by the thruster colliders, which are skipped on macOS (macos/NOT_YET.md).

#include <RED4ext/Scripting/Natives/Generated/vehicle/ChassisComponent.hpp>
