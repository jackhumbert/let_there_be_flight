#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>

namespace FlightSettings {

struct FlightSettings : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();
  static FlightSettings *GetInstance();

  //static float GetFloat(RED4ext::CName);
};

} // namespace FlightSettings