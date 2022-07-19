#include <RED4ext/RED4ext.hpp>
#include "FlightModule.hpp"
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include "ScriptDefinitions/ScriptHost.hpp"
#include "stdafx.hpp"

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

      }
    }
  }
  auto og = ProcessScriptTypes_Original(version, scriptData, scriptLogger);
  return og;
}

struct RuntimePropertiesModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessScriptTypesAddr), &ProcessScriptTypes,
                                  reinterpret_cast<void **>(&ProcessScriptTypes_Original)))
      ;
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessScriptTypesAddr));  }
};

REGISTER_FLIGHT_MODULE(RuntimePropertiesModule);