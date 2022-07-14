#pragma once
#include <RED4ext/RED4ext.hpp>
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarFloat.hpp>

struct ModSettingsVariable : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();

  RED4ext::CName mod;
  ScriptClass *scriptClass;
  RED4ext::CName className;
  RED4ext::CName category;
  RED4ext::CName propertyName;
  RED4ext::CName displayName;
  RED4ext::CName description;
  float defaultValue;
  RED4ext::user::RuntimeSettingsVar *settingsVar;
};

struct ModSettings : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();
  static ModSettings *GetInstance();
  static void AddVariable(ModSettingsVariable*);

  int32_t isAccessingModspace;
  RED4ext::DynArray<ModSettingsVariable *> variables;
  RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>> variablesByMod;
  RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<RED4ext::CName>> categoriesByMod;
};