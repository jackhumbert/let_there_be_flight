#include "FlightController.hpp"
#include "FlightEvents.hpp"
#include "FlightHelperWrapper.hpp"
#include "stdafx.hpp"
#include <RED4ext/Common.hpp>

namespace vehicle {
namespace flight {

REGISTER_FLIGHT_MODULE(HelperWrapperModule);

RED4ext::TTypedClass<HelperWrapper> helperCls("vehicleFlightHelper");

RED4ext::CClass *HelperWrapper::GetNativeType() { return &helperCls; }

void HelperWrapperModule::RegisterTypes() {
  helperCls.flags = {.isNative = true};
  RED4ext::CRTTISystem::Get()->RegisterType(&helperCls);
}

void HelperWrapperModule::PostRegisterTypes() {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto is = rtti->GetClass("IScriptable");
  helperCls.parent = is;

  auto vdtpe = rtti->GetClass("vehicleFlightHelper");
  vdtpe->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Vector4"), "force", nullptr, 0x48));
  vdtpe->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Vector4"), "torque", nullptr, 0x58));
}

} // namespace flight
} // namespace vehicle