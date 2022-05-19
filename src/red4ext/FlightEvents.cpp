#include "FlightEvents.hpp"

namespace vehicle {

namespace flight {

//REGISTER_FLIGHT_MODULE(FlightEventsModule);

RED4ext::TTypedClass<Event> eventCls("vehicleFlightEvent");

RED4ext::CClass *Event::GetNativeType() { return &eventCls; }

RED4ext::TTypedClass<PhysicsUpdateEvent> physicsUpdateEventCls("vehicleFlightPhysicsUpdateEvent");

RED4ext::CClass *PhysicsUpdateEvent::GetNativeType() { return &physicsUpdateEventCls; }

void FlightEventsModule::RegisterTypes() {
  eventCls.flags = {.isNative = true};
  RED4ext::CRTTISystem::Get()->RegisterType(&eventCls);

  physicsUpdateEventCls.flags = {.isNative = true};
  RED4ext::CRTTISystem::Get()->RegisterType(&physicsUpdateEventCls);
}

void FlightEventsModule::PostRegisterTypes() {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto redEvent = rtti->GetClass("redEvent");
  eventCls.parent = redEvent;

  auto vehicleFlightEvent = rtti->GetClass("vehicleFlightEvent");
  physicsUpdateEventCls.parent = vehicleFlightEvent;

  auto vdtpe = rtti->GetClass("vehicleFlightPhysicsUpdateEvent");
  vdtpe->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Float"), "timeDelta", nullptr, 0x50));
}

} // namespace flight

} // namespace vehicle