#pragma once

// LTBF R41 (macOS only). Field access for the game structs whose macOS layout differs from the jackhumbert fork's Windows layout.
//
// The fork's vehicle physics structs (vehicle::Physics, WheeledPhysics, BikePhysics, ...) do not even pass their own size asserts
// under clang's Itanium layout, and the macOS game has some of their fields at other offsets. So on macOS LTBF never reads those
// fields through the fork structs: it goes through the accessors below, whose offsets come from macos/ADDRESS_SURVEY.md (R40,
// "Struct layouts LTBF uses": each one read from a Windows function and its macOS twin).
//
// R41b (macos/ADDRESS_LEFTOVERS.md) located the fields R40 left UNSURE. The one it could not check (the FPP anim feature's camera weight)
// keeps a placeholder, LTBF_MAC_TODO_R41b_<field>: while it is LTBF_MAC_UNKNOWN, LTBF sets that value through the reflected property
// or skips it with a one-time log line (LtbfMac::SkipOnce).
// Fields that are the same on both platforms and whose fork struct passes its offset asserts under clang (BaseObject.physics,
// physicsData, input, actionInterface; PhysicsData force/torque/velocity/vehicle/centerOfMass) are read through the fork structs.

#include <cstddef>
#include <cstdint>

#include "MacShims.hpp"

#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Transform.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>

#define LTBF_MAC_UNKNOWN (-1)

// ---- R41b results (macos/ADDRESS_LEFTOVERS.md). The two items R41b left unverified keep a fail-safe and a log line. -------------
// The game's dynamic CRUID accessor: uint64_t() returning (old + 1) * 4 | 1 on the int64 global at 0x8DC7EB0 (Windows hash
// 2027097961, inline `lock xadd` there). CRUID::Next on macOS calls it.
#define LTBF_MAC_CRUID_NEXT_RVA 0xF3559C
#define LTBF_MAC_CRUID_COUNTER_RVA 0x99DBE00
// FPPCameraComponent.animFeature's vehicleProceduralCameraWeight (Windows +0xEC inside the anim feature): not checked on macOS.
// LTBF sets it through the reflected property instead of the offset; this placeholder stays until the offset is read.
#ifndef LTBF_MAC_TODO_R41b_FPPAnimFeature_vehicleProceduralCameraWeight
#define LTBF_MAC_TODO_R41b_FPPAnimFeature_vehicleProceduralCameraWeight LTBF_MAC_UNKNOWN
#endif

namespace LtbfMac
{
namespace Layout
{
// ---- verified in R40 (Windows -> macOS) ----------------------------------------------------------------------------------------
// vehicle::Physics
constexpr std::ptrdiff_t Physics_parent = 0x50; // W 0x60 (Physics::GetVehicle M 0x1360EE8)
// vehicle::WheeledPhysics
constexpr std::ptrdiff_t WheeledPhysics_wheels = 0x5D0;          // W 0x5E0 ("insert2" in the fork), stride 0x190 on both
constexpr std::ptrdiff_t WheeledPhysics_wheelStride = 0x190;
constexpr std::ptrdiff_t WheeledPhysics_wheelCount = 0xC10;      // W 0xC20
constexpr std::ptrdiff_t WheeledPhysics_driveHelpers = 0xD98;    // W 0xDA8 (DynArray: entries +0, capacity +8, size +0xC)
constexpr std::ptrdiff_t WheeledPhysics_vehicle = 0xF0;          // W 0x108 (pointer the helper loop and the drag block use)
constexpr std::ptrdiff_t WheeledPhysics_airDragCoefficient = 0xC80; // W 0xC90
// vehicle::BikePhysics
constexpr std::ptrdiff_t BikePhysics_vehicle = 0xDC8;            // W 0xDE0
// The global flag byte ApplyAirResistance tests (W .data 0x32FE130; M __bss 0x8FEEA51, written at runtime)
constexpr uintptr_t AirResistanceFlagRva = 0x9CD0561;

// ---- R41b (macos/ADDRESS_LEFTOVERS.md sections 1 and "new layout findings") ------------------------------------------------------
// vehicle::BikePhysics: turnRate is a WheeledPhysics member (W 0xF4); the bike block moves by -0x18 (W 0xE18/0xE1C/0xE20)
constexpr std::ptrdiff_t WheeledPhysics_turnRate = 0xDC;
constexpr std::ptrdiff_t BikePhysics_negTurnRate = 0xE00;
constexpr std::ptrdiff_t BikePhysics_turnInput = 0xE04;
constexpr std::ptrdiff_t BikePhysics_tiltControlEnabled = 0xE08;
// vehicle::PhysicsData (same as Windows): vehicle back pointer, centerOfMass (Vector3), worldPosition (Vector4)
constexpr std::ptrdiff_t PhysicsData_vehicle = 0x180;
constexpr std::ptrdiff_t PhysicsData_centerOfMass = 0x1D4;
constexpr std::ptrdiff_t PhysicsData_worldPosition = 0x170;
// vehicle::TPPCameraComponent: the vehicle Handle instance is at 0x318 (W 0x320; R40's 0x320 was the refcount); the rest is the
// same as Windows
constexpr std::ptrdiff_t TPPCamera_vehicle = 0x318;
constexpr std::ptrdiff_t TPPCamera_data = 0x330;          // CameraPreset copy, 0x330..0x38A
constexpr std::ptrdiff_t TPPCamera_cameraPitch = 0x4D8;
constexpr std::ptrdiff_t TPPCamera_cameraDirection = 0x4DC; // Vector3
constexpr std::ptrdiff_t TPPCamera_speedCoef = 0x578;     // drivingDirectionCompensationSpeedCoef
constexpr std::ptrdiff_t TPPCamera_angleSmooth = 0x580;   // drivingDirectionCompensationAngleSmooth
// game::FPPCameraComponent (same as Windows)
constexpr std::ptrdiff_t FPPCamera_yawOffset = 0x42C;
constexpr std::ptrdiff_t FPPCamera_animFeature = 0x360;   // Handle instance
constexpr std::ptrdiff_t FPPAnimFeature_vehicleProceduralCameraWeight =
    LTBF_MAC_TODO_R41b_FPPAnimFeature_vehicleProceduralCameraWeight;
// vehicle::Effects.vehicle (same)
constexpr std::ptrdiff_t Effects_vehicle = 0x0;
// vehicle::ChassisComponent and physics::ProxyHelper (same)
constexpr std::ptrdiff_t ChassisComponent_proxyID = 0x160;
constexpr std::ptrdiff_t ChassisComponent_sharedMutex = 0x16C;
constexpr std::ptrdiff_t ProxyHelper_mutex = 0x18;
// vehicle::BaseObject.actionInterface (same)
constexpr std::ptrdiff_t BaseObject_actionInterface = 0x3B8;
// ent::SlotComponent.slotIndexLookup (same; R41b: GetSlotIndex reads [+0x150..+0x16C] inline)
constexpr std::ptrdiff_t SlotComponent_slotIndexLookup = 0x150;
// ent::VisualControllerComponent.resourcePaths: INFERRED, not read from a macOS function. The class has the same size (0x128) and the
// same reflected offsets on macOS (meshProxy 0x90, appearanceDependency 0xA8, cookedAppearanceData 0xB8, forcedLodDistance 0xF0,
// layout/reflection-2026-10-08.tsv); the fork's resourcePaths sits between them at 0xC8, and a class's own members are laid out the
// same way by MSVC and clang (macos/NOT_YET.md).
constexpr std::ptrdiff_t VisualControllerComponent_resourcePaths = 0xC8;
// physics::ColliderSphere: radius 0x88, size 0x90 on macOS (W 0x90, 0xA0): set it through the reflected property
constexpr std::ptrdiff_t ColliderSphere_radius = 0x88;

// The fork's camera helper structs (deps/red4ext.sdk vehicleTPPCameraComponent.hpp). The macOS SDK's TPPCameraComponent is the
// reflected layout only; LTBF reaches these through the offsets above.
struct TppCameraPreset // TPPCameraComponent::CameraPreset, the `data` block at 0x330
{
    RED4ext::Vector4 linearVelocity;
    RED4ext::Quaternion orientation;
    float yawDelta;
    float pitchDelta;
    uint8_t isUsingMouse;
    uint8_t unk29[3];
    float unk2C;
    uint8_t isInAir;
    uint8_t unk31[7];
    float unk38;
    float unk3C;
    RED4ext::Vector4 angularVelocity;
    float affectsPitch;
    uint8_t unk54[4];
    void* cameraSystem;
};
static_assert(offsetof(TppCameraPreset, yawDelta) == 0x20 && offsetof(TppCameraPreset, isInAir) == 0x30, "CameraPreset");

struct TppCameraUpdate // the update context UpdatePitch gets (Windows r9, macOS x3)
{
    void* preset;
    float anotherDeltaTime;
    float deltaTime;
    RED4ext::Transform transform;
    RED4ext::Vector4 locationFromOffset;
};
static_assert(offsetof(TppCameraUpdate, locationFromOffset) == 0x30, "TPPCameraUpdate");

// A typed pointer to the field at aOffset, or null when the offset is still a placeholder (the caller logs once and skips).
template<typename T>
inline T* Field(const void* aObject, std::ptrdiff_t aOffset)
{
    if (!aObject || aOffset < 0)
    {
        return nullptr;
    }
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(aObject) + static_cast<uintptr_t>(aOffset));
}

// Same, for a verified offset: logs nothing, never null for a non-null object.
template<typename T>
inline T& At(const void* aObject, std::ptrdiff_t aOffset)
{
    return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(aObject) + static_cast<uintptr_t>(aOffset));
}

// A placeholder field, or a one-time skip line naming the placeholder.
#define LTBF_MAC_FIELD(type, object, offsetName, placeholder, feature)                                                           \
    ([&]() -> type* {                                                                                                            \
        auto* _field = ::LtbfMac::Layout::Field<type>((object), ::LtbfMac::Layout::offsetName);                                  \
        if (!_field && (object))                                                                                                 \
        {                                                                                                                        \
            ::LtbfMac::SkipOnce(placeholder, feature);                                                                           \
        }                                                                                                                        \
        return _field;                                                                                                           \
    }())
} // namespace Layout
} // namespace LtbfMac
