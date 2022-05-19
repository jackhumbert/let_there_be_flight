#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>

namespace FlightController {

struct FlightController : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();

  static FlightController *GetInstance();

  bool enabled;
  bool active;
};

RED4EXT_ASSERT_SIZE(FlightController, 0x48);
RED4EXT_ASSERT_OFFSET(FlightController, enabled, 0x40);
RED4EXT_ASSERT_OFFSET(FlightController, active, 0x41);

} // namespace FlightController
