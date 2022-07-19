#pragma once
#include <RED4ext/RED4ext.hpp>
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include <RED4ext/Scripting/Natives/Generated/user/RuntimeSettingsVar.hpp>

struct ModSettingsVariable {
  RED4ext::CName mod;
  RED4ext::CName typeName;
  RED4ext::CName className;
  RED4ext::CName category;
  RED4ext::user::RuntimeSettingsVar *settingsVar;
  RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>> listeners;
  RED4ext::SharedMutex listeners_lock;

  void UpdateValues();
  RED4ext::user::RuntimeSettingsVar *CreateSettingVarFromBool(ScriptProperty *prop);
  RED4ext::user::RuntimeSettingsVar *CreateSettingVarFromInt(ScriptProperty *prop);
  RED4ext::user::RuntimeSettingsVar *CreateSettingVarFromFloat(ScriptProperty *prop);
  RED4ext::user::RuntimeSettingsVar *CreateSettingVarFromEnum(ScriptProperty *prop);
};