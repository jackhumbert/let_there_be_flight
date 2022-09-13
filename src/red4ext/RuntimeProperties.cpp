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

// 1.52 RVA: 0x1FC0C0 / 2080960
// 1.6 RVA: 0x2003F0 / 2098160
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 28 00 00 00 48 8D 4C 24 20 E8
/// @nth 4/7
RED4ext::CRTTIWeakHandleType ** __fastcall CreateCRTTIWeakHandleTypeFromClass(RED4ext::CRTTIWeakHandleType **a1, RED4ext::CBaseRTTIType *a2) {
  RED4ext::RelocFunc<decltype(&CreateCRTTIWeakHandleTypeFromClass)> call(0x2003F0);
  return call(a1, a2);
}

// 1.52 RVA: 0x1FBD20 / 2080032
// 1.6 RVA: 0x200050 / 2080032
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B FA 48 8B F1 BA 28 00 00 00 48 8D 4C 24 20 E8
/// @nth 0/7
RED4ext::CRTTIHandleType **__fastcall CreateCRTTIHandleTypeFromClass(RED4ext::CRTTIHandleType **a1, RED4ext::CBaseRTTIType *a2) {
  RED4ext::RelocFunc<decltype(&CreateCRTTIHandleTypeFromClass)> call(0x200050);
  return call(a1, a2);
}

// 1.52 RVA: 0x273160 / 2568544
// 1.6 RVA: 0x276F30 / 2584368
/// @pattern 48 8B C4 4C 89 40 18 48 89 48 08 55 53 48 8D 68 A1 48 81 EC A8 00 00 00 48 89 70 10 48 8B DA 48
bool __fastcall ProcessScriptTypes(uint32_t* version, ScriptData* scriptData, void* scriptLogger);
constexpr uintptr_t ProcessScriptTypesAddr = 0x276F30;
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
            spdlog::info("{}.{} at 0x{:X} {}", scriptClass->name.ToString(), prop->name.ToString(), offsetValue, prop->type->name.ToString());
            if (prop->flags.isNative) {
              auto rtti = RED4ext::CRTTISystem::Get();
              auto rttiClass = rtti->GetClassByScriptName(scriptClass->name);
              auto rttiType = rtti->GetType(prop->type->name);
              if (!rttiType) {
                std::string typeStr(prop->type->name.ToString());
                auto del = typeStr.find(":");
                if (del != -1) {
                  auto outerTypeStr = typeStr.substr(0, del);
                  auto innerTypeStr = typeStr.substr(del + 1, typeStr.length());
                  //spdlog::info("{} & {}", outerTypeStr, innerTypeStr);
                  auto innerType = rtti->GetClassByScriptName(innerTypeStr.c_str());
                  if (innerType) {
                    if (outerTypeStr.starts_with("wref")) {
                      RED4ext::CRTTIWeakHandleType *whType;
                      CreateCRTTIWeakHandleTypeFromClass(&whType, innerType);
                      rtti->RegisterType(whType);
                      rttiType = whType;
                    } else if (outerTypeStr.starts_with("ref")) {
                      RED4ext::CRTTIHandleType *whType;
                      CreateCRTTIHandleTypeFromClass(&whType, innerType);
                      rtti->RegisterType(whType);
                      rttiType = whType;
                    }
                  } else {
                    spdlog::warn("could not find inner type: {}", innerTypeStr);
                  }
                }
              }
              if (rttiType) {
                rttiClass->props.PushBack(
                    RED4ext::CProperty::Create(rttiType, prop->name.ToString(), nullptr, offsetValue));
              } else {
                spdlog::warn("could not find type for property: {}", prop->name.ToString());
              }
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