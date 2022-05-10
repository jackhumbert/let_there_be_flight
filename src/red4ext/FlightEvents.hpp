#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/Event.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>

namespace vehicle {

namespace flight {

struct Event : RED4ext::red::Event {
  RED4ext::CClass *GetNativeType();
  RED4ext::Handle<RED4ext::vehicle::BaseObject> vehicle;
};

struct PhysicsUpdateEvent : Event {
  RED4ext::CClass *GetNativeType();
  float timeDelta;
};
RED4EXT_ASSERT_OFFSET(PhysicsUpdateEvent, timeDelta, 0x50);
RED4EXT_ASSERT_SIZE(PhysicsUpdateEvent, 0x58);
//char (*__kaboom)[sizeof(PhysicsUpdateEvent)] = 1;
//char (*__kaboom2)[offsetof(PhysicsUpdateEvent, timeDelta)] = 1;

void RegisterTypes();
void RegisterFunctions();

} // namespace flight

} // namespace vehicle