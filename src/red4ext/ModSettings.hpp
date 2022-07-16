#pragma once
#include <RED4ext/RED4ext.hpp>
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include "ModSettingsVariable.hpp"

struct ModSettings : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();
  static ModSettings *GetInstance();
  static void AddVariable(ModSettingsVariable *);
  static void WriteToFile();
  static void ReadFromFile();
  static RED4ext::user::RuntimeSettingsVar *CreateSettingVarFromBool(ScriptProperty *prop);
  static RED4ext::user::RuntimeSettingsVar *CreateSettingVarFromInt(ScriptProperty *prop);
  static RED4ext::user::RuntimeSettingsVar *CreateSettingVarFromFloat(ScriptProperty *prop);
  static RED4ext::user::RuntimeSettingsVar *CreateSettingVarFromEnum(ScriptProperty *prop);

  RED4ext::DynArray<ModSettingsVariable *> variables;
  RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>> variablesByMod;
  RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>> variablesByClass;
  RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<RED4ext::CName>> categoriesByMod;
  RED4ext::SharedMutex variables_lock;
};