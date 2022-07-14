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

        auto mod = prop->runtimeProperties.Get("ModSettings.mod");
        if (mod) {
          auto variable = (ModSettingsVariable *)RED4ext::CRTTISystem::Get()->GetClass("ModSettings")->AllocInstance(true);
          variable->mod = RED4ext::CNamePool::Add(mod->c_str());
          variable->scriptClass = scriptClass;
          variable->className = scriptClass->name;

          auto category = prop->runtimeProperties.Get("ModSettings.category");
          if (category) {
            variable->category = RED4ext::CNamePool::Add(category->c_str());
          } else {
            variable->category = "None";
          }

          auto defaultValue = 0.0;
          if (prop->defaultValues.size) {
            std::string valueStr(prop->defaultValues[0].c_str());
            defaultValue = std::stof(valueStr);
          }

          auto rtm = (new RED4ext::Memory::DefaultAllocator())
                         ->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarFloat), 8);
          memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarFloat));
          auto rt = new RED4ext::user::RuntimeSettingsVarFloat(*(RED4ext::user::RuntimeSettingsVarFloat *)rtm.memory);

          rt->minValue = 0.0;
          rt->maxValue = 10.0;
          rt->stepValue = 0.5;
          rt->valueInput = defaultValue;
          rt->defaultValue = defaultValue;
          rt->valueValidated = defaultValue;
          rt->valueWrittenToFile = defaultValue;
          rt->type = RED4ext::user::EConfigVarType::Float;

          
          rt->name = prop->name;
          rt->displayNameKeys = RED4ext::DynArray<RED4ext::CName>(new RED4ext::Memory::DefaultAllocator());
          rt->groupPath =
              RED4ext::CNamePool::Add("/mods/" + *scriptClass->name.ToString() + *"/" + *prop->name.ToString());
          rt->updatePolicy = RED4ext::user::EConfigVarUpdatePolicy::Immediately;
          rt->unk44 = 0xFF;
          rt->unk45 = 0xFF;
          rt->bitfield.isInPreGame = true;
          rt->bitfield.isInGame = true;
          rt->bitfield.isVisible = true;
          rt->bitfield.isInitialized = true;
          rt->bitfield.isDisabled = false;
          rt->bitfield.canBeRestoredToDefault = true;

          variable->settingsVar = rt;

          auto displayName = prop->runtimeProperties.Get("ModSettings.displayName");
          if (displayName) {
            rt->displayName = RED4ext::CNamePool::Add(displayName->c_str());
          } else {
            rt->displayName = prop->name;
          }

          auto description = prop->runtimeProperties.Get("ModSettings.description");
          if (description) {
            rt->description = RED4ext::CNamePool::Add(description->c_str());
          }

          auto step = prop->runtimeProperties.Get("ModSettings.step");
          if (step) {
            std::string valueStr(step->c_str());
            rt->stepValue = std::stof(valueStr);
          }

          auto min = prop->runtimeProperties.Get("ModSettings.min");
          if (min) {
            std::string valueStr(min->c_str());
            rt->minValue = std::stof(valueStr);
          }

          auto max = prop->runtimeProperties.Get("ModSettings.max");
          if (max) {
            std::string valueStr(max->c_str());
            rt->maxValue = std::stof(valueStr);
          }

          ModSettings::AddVariable(variable);
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
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessScriptTypesAddr), &ProcessScriptTypes,
                                  reinterpret_cast<void **>(&ProcessScriptTypes_Original)))
      ;
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessScriptTypesAddr));
  }
};

REGISTER_FLIGHT_MODULE(RuntimePropertiesModule);