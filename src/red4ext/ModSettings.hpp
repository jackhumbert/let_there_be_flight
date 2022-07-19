#pragma once
#include <RED4ext/RED4ext.hpp>
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include "ModSettingsVariable.hpp"
#include "INIReader.h"

struct ModSettings : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();

  static ModSettings *GetInstance();
  static bool GetSettingString(RED4ext::CName, RED4ext::CName, RED4ext::CString *);
  static void AddVariable(ModSettingsVariable *);
  static void WriteToFile();
  static void ReadFromFile();

  INIReader reader;
  RED4ext::DynArray<ModSettingsVariable *> variables;
  RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>> variablesByMod;
  RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>> variablesByClass;
  RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<RED4ext::CName>> categoriesByMod;
  RED4ext::SharedMutex variables_lock;
};