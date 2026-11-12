#pragma once

// The developer settings (Flight/DeveloperSettings.hpp) for scripts and tests: Set changes one at run time.
// LoadFile reads developer.ini from the plugin folder at plugin load.

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RedLib.hpp>
#include <filesystem>

struct FlightDeveloperSettings : RED4ext::IScriptable {
  // sets one by name (Flight::SetDeveloperSetting) and logs it; false for an unknown name or a value that does not parse
  static bool Set(RED4ext::CString aName, RED4ext::CString aValue);
  // the value as text, "" for an unknown name
  static RED4ext::CString Get(RED4ext::CString aName);
  // "name = value" for each setting off its code default, one per line ("" when none)
  static RED4ext::CString Describe();

  // reads aFile when it exists, logging each line refused or clamped and each setting off its default
  static void LoadFile(const std::filesystem::path &aFile);

  RTTI_IMPL_TYPEINFO(FlightDeveloperSettings);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightDeveloperSettings, {
  RTTI_METHOD(Set);
  RTTI_METHOD(Get);
  RTTI_METHOD(Describe);
});
