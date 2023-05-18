#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <RedLib.hpp>

struct FlightSettings : RED4ext::IScriptable {
  static RED4ext::Handle<FlightSettings> GetInstance();

  static float GetFloat(RED4ext::CString);
  static void SetFloat(RED4ext::CString, float);
  static RED4ext::Vector3 GetVector3(RED4ext::CString);
  static void SetVector3(RED4ext::CString, float, float, float);
  static void DebugBreak();

  template <typename T>
  static T GetProperty(RED4ext::CName name) {
    auto fs = GetInstance();
    auto prop = fs->GetNativeType()->GetProperty(name);
    return prop->GetValue<T>(fs);
  }

  RTTI_IMPL_TYPEINFO(FlightSettings);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightSettings, {
  RTTI_METHOD(GetInstance);
  RTTI_METHOD(GetFloat);
  RTTI_METHOD(SetFloat);
  RTTI_METHOD(GetVector3);
  RTTI_METHOD(SetVector3);
  RTTI_METHOD(DebugBreak);
});