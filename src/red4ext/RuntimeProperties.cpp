#include <RED4ext/RED4ext.hpp>
#include "FlightModule.hpp"
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include "ScriptDefinitions/ScriptHost.hpp"
#include "stdafx.hpp"
#include "ModSettings.hpp"

struct ScriptRTTIContainer {
  RED4ext::CRTTISystem *rtti;
  void *scriptContainerVft;
  ScriptHost *host;
};

struct ScriptData {
  RED4ext::HashMap<uint64_t, uint64_t> unk00;
  RED4ext::HashMap<uint64_t, uint64_t> unk30;
  RED4ext::HashMap<uint64_t, uint64_t> unk60;
  RED4ext::DynArray<void*> files;
  RED4ext::DynArray<void *> functions;
  RED4ext::DynArray<void *> enums;
RED4ext::DynArray<void*> unkC0;
RED4ext::DynArray<ScriptClass*> classes;
RED4ext::DynArray<void*> types;
RED4ext::DynArray<RED4ext::CString> strings;
RED4ext::HashMap<uint64_t, uint64_t> unk100;
uint8_t unk60MUTX;
void* unk138;
};


bool __fastcall ProcessScriptTypes(uint32_t* version, ScriptData* scriptData, void* scriptLogger);
constexpr uintptr_t ProcessScriptTypesAddr = 0x272560 + 0xC00;
decltype(&ProcessScriptTypes) ProcessScriptTypes_Original;

bool __fastcall ProcessScriptTypes(uint32_t* version, ScriptData* scriptData, void* scriptLogger) {
  for (const auto& scriptClass : scriptData->classes) {
    for (const auto& prop : scriptClass->properties) {
      if (prop->runtimeProperties.size) {
        auto offsetStr = prop->runtimeProperties.Get("offset");
        if (offsetStr) {
          auto cstr = offsetStr->c_str();
          char* p;
          auto offsetValue = strtoul(cstr, &p, 16);
          if (*p == 0) {
            spdlog::info("{}.{} at 0x{:X}", scriptClass->name.ToString(), prop->name.ToString(), offsetValue);
            if (prop->flags.isNative) {
              auto rttiClass = RED4ext::CRTTISystem::Get()->GetClassByScriptName(scriptClass->name);
              auto rttiType = RED4ext::CRTTISystem::Get()->GetType(prop->type->name);
              rttiClass->props.PushBack(
                RED4ext::CProperty::Create(rttiType, prop->name.ToString(), nullptr, offsetValue));
            }
            else {
              spdlog::warn("property is not native - nothing to register");
            }
          }
        }
        //    }
        //  }
        //}
        //for (const auto &scriptClass : scriptData->classes) {
        //  for (const auto &prop : scriptClass->properties) {
        //    if (prop->runtimeProperties.size) {
        auto mod = prop->runtimeProperties.Get("ModSettings.mod");
        if (mod) {
          auto variable = (ModSettingsVariable*)RED4ext::CRTTISystem::Get()->GetClass("ModSettings")->AllocInstance(true);
          variable->mod = RED4ext::CNamePool::Add(mod->c_str());
          variable->typeName = prop->type->name;
          variable->className = scriptClass->name;

          auto category = prop->runtimeProperties.Get("ModSettings.category");
          if (category) {
            variable->category = RED4ext::CNamePool::Add(category->c_str());
          }
          else {
            variable->category = "None";
          }

          RED4ext::user::RuntimeSettingsVar* settingsVar = NULL;

          if (prop->type->name == "Bool") {
            settingsVar = ModSettings::CreateSettingVarFromBool(prop);
          }
          else if (prop->type->name == "Float") {
            settingsVar = ModSettings::CreateSettingVarFromFloat(prop);
          }
          else if (prop->type->name == "Int32" || prop->type->name == "Uint32") {
            settingsVar = ModSettings::CreateSettingVarFromInt(prop);
          }
          else if (RED4ext::CRTTISystem::Get()->GetType(prop->type->name)->GetType() == RED4ext::ERTTIType::Enum) {
            settingsVar = ModSettings::CreateSettingVarFromEnum(prop);
          }

          if (settingsVar) {
            settingsVar->name = prop->name;
            settingsVar->groupPath =
              RED4ext::CNamePool::Add("/mods/" + *scriptClass->name.ToString() + *"/" + *prop->name.ToString());

            auto displayName = prop->runtimeProperties.Get("ModSettings.displayName");
            if (displayName) {
              settingsVar->displayName = RED4ext::CNamePool::Add(displayName->c_str());
            }
            else {
              settingsVar->displayName = prop->name;
            }

            auto description = prop->runtimeProperties.Get("ModSettings.description");
            if (description) {
              settingsVar->description = RED4ext::CNamePool::Add(description->c_str());
            }

            variable->settingsVar = settingsVar;

            ModSettings::AddVariable(variable);
          }
        }
      }
    }
  }
  auto og = ProcessScriptTypes_Original(version, scriptData, scriptLogger);
  return og;
}


uintptr_t __fastcall ScriptHost_LoadScripts(ScriptHost* scriptHost, RED4ext::CString* scriptLocation, uintptr_t scriptLoader);
constexpr uintptr_t ScriptHost_LoadScriptsAddr = 0x268DD0 + 0xC00;
decltype(&ScriptHost_LoadScripts) ScriptHost_LoadScripts_Original;

uintptr_t __fastcall ScriptHost_LoadScripts(ScriptHost* scriptHost, RED4ext::CString* scriptLocation, uintptr_t scriptLoader){
  auto og = ScriptHost_LoadScripts_Original(scriptHost, scriptLocation, scriptLoader);
  ModSettings::ReadFromFile();
  return og;
}

//bool __fastcall ProcessScriptClass(ScriptRTTIContainer *rttiCont, ScriptClass *scriptClass, uintptr_t logger);
//constexpr uintptr_t ProcessScriptClassAddr = 0x274610 + 0xC00;
//decltype(&ProcessScriptClass) ProcessScriptClass_Original;

//bool __fastcall ProcessScriptClass(ScriptRTTIContainer *rttiCont, ScriptClass *scriptClass, uintptr_t logger) {
//  for (const auto &prop : scriptClass->properties) {
//    if (prop->runtimeProperties.size) {
//      auto offsetStr = prop->runtimeProperties.Get("offset");
//      if (offsetStr) {
//        auto cstr = offsetStr->c_str();
//        char *p;
//        auto offsetValue = strtoul(cstr, &p, 16);
//        if (*p == 0) {
//          spdlog::info("{}.{} at 0x{:X}", scriptClass->name.ToString(), prop->name.ToString(), offsetValue);
//          if (prop->flags & prop_isNative) {
//            scriptClass->rttiType->props.PushBack(
//                RED4ext::CProperty::Create(prop->type->rttiType, prop->name.ToString(), nullptr, offsetValue));
//          } else {
//            spdlog::warn("not native");
//          }
//        }
//      }
//    }
//  }
//  return ProcessScriptClass_Original(rttiCont, scriptClass, logger);
//}

struct RuntimePropertiesModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessScriptTypesAddr), &ProcessScriptTypes,
                                  reinterpret_cast<void **>(&ProcessScriptTypes_Original)))
      ;
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ScriptHost_LoadScriptsAddr), &ScriptHost_LoadScripts,
                                  reinterpret_cast<void **>(&ScriptHost_LoadScripts_Original)))
      ;
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessScriptTypesAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ScriptHost_LoadScriptsAddr));
  }
};

REGISTER_FLIGHT_MODULE(RuntimePropertiesModule);