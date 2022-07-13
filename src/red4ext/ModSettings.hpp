#pragma once
#include <RED4ext/RED4ext.hpp>
#include "ScriptDefinitions/ScriptDefinitions.hpp"

struct ModSettingsVariable : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();

  RED4ext::CString mod;
  ScriptClass *scriptClass;
  ScriptProperty *scriptProperty;
  RED4ext::CString displayName;
  RED4ext::CString description;
};

RED4ext::TTypedClass<ModSettingsVariable> modSettingsVariable("ModSettingsVariable");

RED4ext::CClass *ModSettingsVariable::GetNativeType() { return &modSettingsVariable; }

struct ModSettings : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();

};

RED4ext::TTypedClass<ModSettings> modSettings("ModSettings");

RED4ext::CClass *ModSettings::GetNativeType() { return &modSettings; }

RED4ext::DynArray<ModSettingsVariable> ModSettingsVariables;