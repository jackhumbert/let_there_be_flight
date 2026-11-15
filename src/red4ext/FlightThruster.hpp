#pragma once

#include <RED4ext/Common.hpp>
#include "Engine/RTTIClass.hpp"
#include "Flight/Component.hpp"
#include "Flight/Thruster.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>

#include <array>

struct FlightComponent;

class IFlightThruster : public Engine::RTTIClass<IFlightThruster, RED4ext::IScriptable> {
public:
  virtual ~IFlightThruster() override;

  RED4ext::WeakHandle<FlightComponent> flightComponent;
  RED4ext::CName boneName;
  RED4ext::CName slotName;
  RED4ext::Vector3 relativePosition = RED4ext::Vector3(0.0, 0.0, 0.0);
  alignas(0x10) RED4ext::Quaternion relativeRotation = RED4ext::Quaternion(0.0, 0.0, 0.0, 1.0);
  RED4ext::CName meshPath = "user\\jackhumbert\\meshes\\engine_corpo.mesh";
  RED4ext::CName meshName = "Thruster";
  RED4ext::Handle<RED4ext::ent::MeshComponent> meshComponent;
  RED4ext::WeakHandle<RED4ext::vehicle::BaseObject> vehicle;
  bool attached = true;

  // Not called at the moment: the first UpdateNative in game crashed the game (2026-11-15) and the script animates
  // the thrusters (IFlightThruster.Update) until that is found. Resolves the game's natives and the script fields
  // once, fills the thruster's parameters and finds the animation controller. False (logged once) when anything is
  // missing or the effects do not fit.
  bool SetupMotion();

  // IFlightThruster.Update in C++ (Flight::Thruster*), for the script to call each frame once it works. False without
  // doing anything when SetupMotion has not succeeded.
  bool UpdateNative(RED4ext::Vector4 aForce, RED4ext::Vector4 aTorque);

  // JSON of what SetupMotion resolved: {"ready","multiply","findOnEntity","fx","anim"}
  RED4ext::CString MotionState();

  // not RTTI properties: C++-only members appended to the struct after `attached`. The scripts never see them; the
  // fields the script declares itself live after sizeof(IFlightThruster)
  struct FxSlot {
    Flight::ThrusterFxKind kind = Flight::ThrusterFxKind::Base;
    RED4ext::WeakHandle<RED4ext::IScriptable> instance;
  };
  static constexpr size_t kMaxFx = 4;
  Flight::ThrusterParams params;
  std::array<FxSlot, kMaxFx> fxSlots;
  uint32_t fxCount = 0;
  RED4ext::WeakHandle<RED4ext::IScriptable> animationController;
  bool motionReady = false;

private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType) {
    aType->flags.isAbstract = true;
    // aType->flags.b20000 = true;
    // aType->flags.isAlwaysTransient = true;
  }

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&IFlightThruster::SetupMotion>("SetupMotion");
    aType->AddFunction<&IFlightThruster::UpdateNative>("UpdateNative");
    aType->AddFunction<&IFlightThruster::MotionState>("MotionState");
  }
};
RED4EXT_ASSERT_OFFSET(IFlightThruster, flightComponent, 0x40);
RED4EXT_ASSERT_OFFSET(IFlightThruster, boneName, 0x50);
RED4EXT_ASSERT_OFFSET(IFlightThruster, slotName, 0x58);
RED4EXT_ASSERT_OFFSET(IFlightThruster, relativePosition, 0x60);
RED4EXT_ASSERT_OFFSET(IFlightThruster, relativeRotation, 0x70);
RED4EXT_ASSERT_OFFSET(IFlightThruster, meshPath, 0x80);
RED4EXT_ASSERT_OFFSET(IFlightThruster, meshName, 0x88);
RED4EXT_ASSERT_OFFSET(IFlightThruster, meshComponent, 0x90);
RED4EXT_ASSERT_OFFSET(IFlightThruster, vehicle, 0xA0);
RED4EXT_ASSERT_OFFSET(IFlightThruster, attached, 0xB0);