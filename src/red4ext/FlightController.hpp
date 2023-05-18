#pragma once

#include <RED4ext/Common.hpp>
// #include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RedLib.hpp>

struct FlightController : RED4ext::IScriptable {
  static FlightController *GetInstance();

  bool enabled;
  bool active;
  int32_t mode;
  
  RTTI_IMPL_TYPEINFO(FlightController);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightController, {
  RTTI_METHOD(GetInstance);
  RTTI_PROPERTY(enabled);
  RTTI_PROPERTY(active);
  RTTI_PROPERTY(mode);
});

RED4EXT_ASSERT_SIZE(FlightController, 0x48);
RED4EXT_ASSERT_OFFSET(FlightController, enabled, 0x40);
RED4EXT_ASSERT_OFFSET(FlightController, active, 0x41);
RED4EXT_ASSERT_OFFSET(FlightController, mode, 0x44);
