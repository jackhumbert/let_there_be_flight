#include <RED4ext/RED4ext.hpp>
#include "Utils/FlightModule.hpp"
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include "ScriptDefinitions/ScriptHost.hpp"
#include "stdafx.hpp"
#include "Addresses.hpp"

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

RED4ext::CRTTIHandleType **__fastcall CreateCRTTIArrayTypeFromClass(RED4ext::CRTTIArrayType **a1, RED4ext::CBaseRTTIType *a2) {
  RED4ext::RelocFunc<decltype(&CreateCRTTIArrayTypeFromClass)> call(CreateCRTTIArrayTypeFromClass_Addr);
  return call(a1, a2);
}

RED4ext::CRTTIWeakHandleType ** __fastcall CreateCRTTIWeakHandleTypeFromClass(RED4ext::CRTTIWeakHandleType **a1, RED4ext::CBaseRTTIType *a2) {
RED4ext::RelocFunc<decltype(&CreateCRTTIWeakHandleTypeFromClass)> call(CreateCRTTIWeakHandleTypeFromClass_Addr);
  return call(a1, a2);
}

RED4ext::CRTTIHandleType **__fastcall CreateCRTTIHandleTypeFromClass(RED4ext::CRTTIHandleType **a1, RED4ext::CBaseRTTIType *a2) {
  RED4ext::RelocFunc<decltype(&CreateCRTTIHandleTypeFromClass)> call(CreateCRTTIHandleTypeFromClass_Addr);
  return call(a1, a2);
}

RED4ext::CRTTIResourceAsyncReferenceType **__fastcall CreateCRTTIRaRefTypeFromClass(RED4ext::CRTTIResourceAsyncReferenceType **a1, RED4ext::CBaseRTTIType *a2) {
  RED4ext::RelocFunc<decltype(&CreateCRTTIRaRefTypeFromClass)> call(CreateCRTTIRaRefTypeFromClass_Addr);
  return call(a1, a2);
}

REGISTER_FLIGHT_HOOK(bool __fastcall, ProcessScriptTypes, uint32_t* version, ScriptData* scriptData, void* scriptLogger) {
  for (const auto& scriptClass : scriptData->classes) {
    for (const auto& prop : scriptClass->properties) {
      if (prop->runtimeProperties.size) {
        auto offsetStr = prop->runtimeProperties.Get("offset");
        if (offsetStr) {
          auto cstr = offsetStr->c_str();
          char* p;
          auto offsetValue = strtoul(cstr, &p, 16);
          RED4ext::CName typeName = prop->type->name;
          if (*p == 0) {
            spdlog::info("{}.{} at 0x{:X} {}", scriptClass->name.ToString(), prop->name.ToString(), offsetValue, prop->type->name.ToString());
            if (prop->flags.isNative) {
              auto customTypeStr = prop->runtimeProperties.Get("type");
              if (customTypeStr) {
                spdlog::info("Custom type: {}", customTypeStr->c_str());
                typeName = RED4ext::CName(customTypeStr->c_str());
                //prop->type->name = typeName;
              }
              auto rtti = RED4ext::CRTTISystem::Get();
              auto rttiClass = rtti->GetClassByScriptName(scriptClass->name);
              RED4ext::CBaseRTTIType *rttiType = rtti->GetClassByScriptName(typeName);
              if (!rttiType) {
                rttiType = rtti->GetType(typeName);
              }
              if (!rttiType) {
                spdlog::info("  script type not registered - breaking down '{}'", typeName.ToString());
                std::string typeStr(typeName.ToString());
                auto del = typeStr.find(":");
                if (del != -1) {
                  auto outerTypeStr = typeStr.substr(0, del);
                  auto innerTypeStr = typeStr.substr(del + 1, typeStr.length());
                  //spdlog::info("{} & {}", outerTypeStr, innerTypeStr);
                  RED4ext::CBaseRTTIType * innerType = rtti->GetClassByScriptName(innerTypeStr.c_str());
                  if (!innerType) {
                    auto del = innerTypeStr.find(":");
                    if (del != -1) {
                      auto innerInnerTypeStr = innerTypeStr.substr(del + 1, innerTypeStr.length());
                      auto innerInnerType = rtti->GetClassByScriptName(innerInnerTypeStr.c_str());
                      if (innerTypeStr.starts_with("wref")) {
                        RED4ext::CRTTIWeakHandleType *whType;
                        CreateCRTTIWeakHandleTypeFromClass(&whType, innerInnerType);
                        rtti->RegisterType(whType);
                        innerType = whType;
                      } else if (innerTypeStr.starts_with("ref")) {
                        RED4ext::CRTTIHandleType *whType;
                        CreateCRTTIHandleTypeFromClass(&whType, innerInnerType);
                        rtti->RegisterType(whType);
                        innerType = whType;
                      } else if (innerTypeStr.starts_with("array")) {
                        RED4ext::CRTTIArrayType *whType;
                        CreateCRTTIArrayTypeFromClass(&whType, innerInnerType);
                        rtti->RegisterType(whType);
                        innerType = whType;
                      } else if (innerTypeStr.starts_with("raRef")) {
                        RED4ext::CRTTIResourceAsyncReferenceType *whType;
                        CreateCRTTIRaRefTypeFromClass(&whType, innerInnerType);
                        rtti->RegisterType(whType);
                        innerType = whType;
                      }
                    }
                  }
                  if (innerType) {
                    //if (customTypeStr) {
                    //  //innerScriptType = ScriptType();
                    //  ScriptClass *innerScriptCls = (ScriptClass*)malloc(0x90);
                    //  innerScriptCls->rttiType = (RED4ext::CClass*)innerType;
                    //  innerScriptCls->name = RED4ext::CName(innerTypeStr.c_str());
                    //  prop->type->innerType = innerScriptCls;
                    //}
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
                    } else if (outerTypeStr.starts_with("array")) {
                      RED4ext::CRTTIArrayType *whType;
                      CreateCRTTIArrayTypeFromClass(&whType, innerType);
                      rtti->RegisterType(whType);
                      rttiType = whType;
                    } else if (innerTypeStr.starts_with("raRef")) {
                      RED4ext::CRTTIResourceAsyncReferenceType *whType;
                      CreateCRTTIRaRefTypeFromClass(&whType, innerType);
                      rtti->RegisterType(whType);
                      innerType = whType;
                    }
                  } else {
                    spdlog::warn("could not find inner type '{}' in '{}'", innerTypeStr, typeStr);
                  }
                }
              }
              //if (customTypeStr) {
              //  prop->type->rttiType = rttiType;
              //}
              if (rttiType) {
                rttiClass->props.PushBack(
                    RED4ext::CProperty::Create(rttiType, prop->name.ToString(), nullptr, offsetValue));
              } else {
                spdlog::warn("could not find type '{}' for property: {}", prop->type->name.ToString(), prop->name.ToString());
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