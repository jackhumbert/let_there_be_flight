#include "FlightController.hpp"

#include "Utils.hpp"
#include "stdafx.hpp"

namespace FlightController {

RED4ext::TTypedClass<FlightController> flightControllerCls("FlightController");

RED4ext::CClass *FlightController::GetNativeType() { return &flightControllerCls; }

void RegisterTypes() {
  flightControllerCls.flags = {.isNative = true};
  RED4ext::CRTTISystem::Get()->RegisterType(&flightControllerCls);
}

//void PhysicsUpdate(float timeDelta) {

//}

void RegisterFunctions() {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto scriptable = rtti->GetClass("IScriptable");
  flightControllerCls.parent = scriptable;

  //auto physicsUpdate =
      //RED4ext::CClassStaticFunction(flightControllerCls, "PhysicsUpdate", "PhysicsUpdate", &PhysicsUpdate, {});
  //auto PhysicsUpdate =
      //RED4ext::CClassFunction::Create(flightControllerCls, "PhysicsUpdate", "PhysicsUpdate", NULL, {});
  //flightControllerCls.RegisterFunction(physicsUpdate);
}

} // namespace FlightController
