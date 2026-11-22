#include <RED4ext/Common.hpp>
#include "Utils/FlightModule.hpp"
#include "ScriptDefinitions/ScriptDefinitions.hpp"
#include "ScriptDefinitions/ScriptHost.hpp"
#include "stdafx.hpp"


struct ScriptRTTIContainer {
  RED4ext::CRTTISystem *rtti;
  void *scriptContainerVft;
  ScriptHost *host;
};

RED4ext::CRTTIHandleType **__fastcall CreateCRTTIArrayTypeFromClass(RED4ext::CRTTIArrayType **a1, RED4ext::CBaseRTTIType *a2) {
  RED4ext::UniversalRelocFunc<decltype(&CreateCRTTIArrayTypeFromClass)> call(1404578149);
  return call(a1, a2);
}

RED4ext::CRTTIWeakHandleType ** __fastcall CreateCRTTIWeakHandleTypeFromClass(RED4ext::CRTTIWeakHandleType **a1, RED4ext::CBaseRTTIType *a2) {
RED4ext::UniversalRelocFunc<decltype(&CreateCRTTIWeakHandleTypeFromClass)> call(682896186);
  return call(a1, a2);
}

RED4ext::CRTTIHandleType **__fastcall CreateCRTTIHandleTypeFromClass(RED4ext::CRTTIHandleType **a1, RED4ext::CBaseRTTIType *a2) {
  RED4ext::UniversalRelocFunc<decltype(&CreateCRTTIHandleTypeFromClass)> call(1992632754);
  return call(a1, a2);
}

RED4ext::CRTTIResourceAsyncReferenceType **__fastcall CreateCRTTIRaRefTypeFromClass(RED4ext::CRTTIResourceAsyncReferenceType **a1, RED4ext::CBaseRTTIType *a2) {
  RED4ext::UniversalRelocFunc<decltype(&CreateCRTTIRaRefTypeFromClass)> call(2481073725);
  return call(a1, a2);
}

// The RTTI type `<wrapper>:<inner>` (whandle, handle, array, raRef) for a native property's
// script type. The game's own type of that name is used when there is one: a second type with
// the same name made here would not be the one native code puts in a Variant, so the script's
// casts of that Variant (e.g. PlayerStateMachine.TrackedTarget, a whandle:entIPlacedComponent)
// would fail. Only a type the game does not have is created and registered.
static RED4ext::CBaseRTTIType *WrappedType(RED4ext::CRTTISystem *rtti, const std::string &wrapper,
                                           RED4ext::CBaseRTTIType *inner) {
  if (!inner) {
    return nullptr;
  }
  const char *prefix = nullptr;
  if (wrapper.starts_with("wref")) {
    prefix = "whandle:";
  } else if (wrapper.starts_with("ref")) {
    prefix = "handle:";
  } else if (wrapper.starts_with("array")) {
    prefix = "array:";
  } else if (wrapper.starts_with("raRef")) {
    prefix = "raRef:";
  } else {
    return nullptr;
  }
  std::string name = std::string(prefix) + inner->GetName().ToString();
  if (auto existing = rtti->GetType(RED4ext::CName(name.c_str()))) {
    return existing;
  }
  RED4ext::CBaseRTTIType *created = nullptr;
  if (prefix[0] == 'w') {
    RED4ext::CRTTIWeakHandleType *t;
    CreateCRTTIWeakHandleTypeFromClass(&t, inner);
    created = t;
  } else if (prefix[0] == 'h') {
    RED4ext::CRTTIHandleType *t;
    CreateCRTTIHandleTypeFromClass(&t, inner);
    created = t;
  } else if (prefix[0] == 'a') {
    RED4ext::CRTTIArrayType *t;
    CreateCRTTIArrayTypeFromClass(&t, inner);
    created = t;
  } else {
    RED4ext::CRTTIResourceAsyncReferenceType *t;
    CreateCRTTIRaRefTypeFromClass(&t, inner);
    created = t;
  }
  if (created) {
    spdlog::info("  created type '{}'", name);
    rtti->RegisterType(created);
  }
  return created;
}

// bool CScriptDataValidator::Validate(CScriptedDataEnvironment const &, IScriptDataErrorReporter &) const
// Struct layouts are 2.5 (ScriptDefinitions.hpp); the first argument is the validator (this).
REGISTER_FLIGHT_HOOK_HASH(bool __fastcall, 898639042, CScriptDataValidator_Validate, uint32_t* version, const CScriptedDataEnvironment* scriptData, void* scriptLogger) {
  for (const auto& scriptClass : scriptData->classes) {
    for (const auto& prop : scriptClass->properties) {
      if (prop->attributes.size) {
        auto offsetStr = prop->attributes.Get("offset");
        if (offsetStr) {
          auto cstr = offsetStr->c_str();
          char* p;
          auto offsetValue = strtoul(cstr, &p, 16);
          RED4ext::CName typeName = prop->type->name;
          if (*p == 0) {
            spdlog::info("{}.{} at 0x{:X} {}", scriptClass->name.ToString(), prop->name.ToString(), offsetValue, prop->type->name.ToString());
            if (prop->flags.isImported) {
              auto customTypeStr = prop->attributes.Get("type");
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
                      innerType = WrappedType(rtti, innerTypeStr, innerInnerType);
                    }
                  }
                  if (innerType) {
                    rttiType = WrappedType(rtti, outerTypeStr, innerType);
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
  auto og = CScriptDataValidator_Validate_Original(version, scriptData, scriptLogger);

  return og;
}