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
  RED4ext::DynArray<void *> unkC0;
  RED4ext::DynArray<ScriptClass *> classes;
  RED4ext::DynArray<void *> types;
  RED4ext::DynArray<RED4ext::CString> strings;
  RED4ext::HashMap<uint64_t, uint64_t> unk100;
  uint8_t unk60MUTX;
  void *unk138;
};


bool __fastcall ProcessScriptTypes(uint32_t *version, ScriptData *scriptData, void *scriptLogger);
constexpr uintptr_t ProcessScriptTypesAddr = 0x272560 + 0xC00;
decltype(&ProcessScriptTypes) ProcessScriptTypes_Original;

bool __fastcall ProcessScriptTypes(uint32_t *version, ScriptData *scriptData, void *scriptLogger) {
  for (const auto &scriptClass : scriptData->classes) {
    for (const auto &prop : scriptClass->properties) {
      if (prop->runtimeProperties.size) {
        auto offsetStr = prop->runtimeProperties.Get("offset");
        if (offsetStr) {
          auto cstr = offsetStr->c_str();
          char *p;
          auto offsetValue = strtoul(cstr, &p, 16);
          if (*p == 0) {
            spdlog::info("{}.{} at 0x{:X}", scriptClass->name.ToString(), prop->name.ToString(), offsetValue);
            if (prop->flags & prop_isNative) {
              auto rttiClass = RED4ext::CRTTISystem::Get()->GetClassByScriptName(scriptClass->name);
              auto rttiType = RED4ext::CRTTISystem::Get()->GetType(prop->type->name);
              rttiClass->props.PushBack(
                  RED4ext::CProperty::Create(rttiType, prop->name.ToString(), nullptr, offsetValue));
            } else {
              spdlog::warn("property is not native - nothing to register");
            }
          }
        }

        auto mod = prop->runtimeProperties.Get("modSettings.mod");
        if (mod) {
          ModSettingsVariable *variable = (ModSettingsVariable*)modSettingsVariable.AllocInstance();
          variable->mod = *mod;
          variable->scriptClass = scriptClass;
          variable->scriptProperty = prop;

          auto displayName = prop->runtimeProperties.Get("modSettings.displayName");
          if (displayName) {
            variable->displayName = *displayName;
          } else {
            variable->displayName = prop->name.ToString();
          }
          auto description = prop->runtimeProperties.Get("modSettings.description");
          if (description) {
            variable->description = *description;
          }
          ModSettingsVariables.EmplaceBack(*variable);
        }
      }
    }
  }
  return ProcessScriptTypes_Original(version, scriptData, scriptLogger);
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
    ModSettingsVariables = RED4ext::DynArray<ModSettingsVariable>(new RED4ext::Memory::DefaultAllocator());
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessScriptTypesAddr), &ProcessScriptTypes,
                                  reinterpret_cast<void **>(&ProcessScriptTypes_Original)))
      ;
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessScriptTypesAddr));
  }
};

REGISTER_FLIGHT_MODULE(RuntimePropertiesModule);