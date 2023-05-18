#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/Event.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include "Red/TypeInfo/Macros/Definition.hpp"
#include "Utils/FlightModule.hpp"
#include <RedLib.hpp>

namespace RED4ext::vehicle::flight {

struct Event : RED4ext::red::Event {
  RED4ext::Handle<RED4ext::vehicle::BaseObject> vehicle;

  RTTI_IMPL_TYPEINFO(Event);
  RTTI_IMPL_ALLOCATOR();
};

struct PhysicsUpdateEvent : Event {
  float timeDelta = 0;

  RTTI_IMPL_TYPEINFO(PhysicsUpdateEvent);
  RTTI_IMPL_ALLOCATOR();
};
RED4EXT_ASSERT_OFFSET(PhysicsUpdateEvent, timeDelta, 0x50);
RED4EXT_ASSERT_SIZE(PhysicsUpdateEvent, 0x58);
//char (*__kaboom)[sizeof(PhysicsUpdateEvent)] = 1;
//char (*__kaboom2)[offsetof(PhysicsUpdateEvent, timeDelta)] = 1;

} // namespace vehicle::flight

RTTI_DEFINE_CLASS(RED4ext::vehicle::flight::Event, "vehicleFlightEvent", {
  RTTI_PARENT(RED4ext::red::Event);
  RTTI_PROPERTY(vehicle);
});

RTTI_DEFINE_CLASS(RED4ext::vehicle::flight::PhysicsUpdateEvent, "vehicleFlightPhysicsUpdateEvent", {
  RTTI_PARENT(vehicle::flight::Event);
  RTTI_PROPERTY(timeDelta);
});