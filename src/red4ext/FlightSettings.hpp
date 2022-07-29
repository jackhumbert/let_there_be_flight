#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>

namespace FlightSettings {

struct FlightSettings : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();
  static FlightSettings *GetInstance();

};

float GetFloat(RED4ext::CString);
RED4ext::Vector3 GetVector3(RED4ext::CString);

template <typename T> static T GetProperty(RED4ext::CName name) {
  auto fs = FlightSettings::GetInstance();
  auto prop = fs->GetNativeType()->GetProperty(name);
  return prop->GetValue<T>(fs);
}

} // namespace FlightSettings