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

struct ScriptData {
#ifdef __APPLE__
  void *unk00_25; // game 2.5: 8 new bytes at the start (definitionsByName +8, classes +0xD8)
#endif
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
#ifdef __APPLE__
static_assert(offsetof(ScriptData, classes) == 0xD8);
#endif

#ifdef __APPLE__
// LTBF R41: the four CreateCRTTI*TypeFromClass functions have no standalone macOS copy. The game creates these types by name:
// CRTTISystem::GetType walks the prefix-factory table ("array:", "handle:", "whandle:", "raRef:") and registers the new type
// itself (macos/ADDRESS_LEFTOVERS.md section 3), so the explicit RegisterType below is skipped on macOS.
// R42b: the name goes into the game's CName pool first. GetType looks the hash up, and only when the type does not exist yet does it
// turn the hash back into text (CName::ToString) to find the prefix factory; a CName built from text alone (hash only) has no text in
// the pool, so every type that did not exist yet came back null (Boot B2: 8 "Missing native property" link errors).
static RED4ext::CBaseRTTIType *MacCompositeType(const char *aPrefix, RED4ext::CBaseRTTIType *aInner) {
  if (!aInner)
    return nullptr;
  const std::string name = std::string(aPrefix) + aInner->GetName().ToString();
  auto type = RED4ext::CRTTISystem::Get()->GetType(RED4ext::CNamePool::Add(name.c_str()));
  if (!type)
    spdlog::error("[ltbf] RTTI type '{}' could not be created (CRTTISystem::GetType returned null)", name);
  return type;
}

RED4ext::CRTTIHandleType **CreateCRTTIArrayTypeFromClass(RED4ext::CRTTIArrayType **a1, RED4ext::CBaseRTTIType *a2) {
  *a1 = reinterpret_cast<RED4ext::CRTTIArrayType *>(MacCompositeType("array:", a2));
  return reinterpret_cast<RED4ext::CRTTIHandleType **>(a1);
}

RED4ext::CRTTIWeakHandleType **CreateCRTTIWeakHandleTypeFromClass(RED4ext::CRTTIWeakHandleType **a1, RED4ext::CBaseRTTIType *a2) {
  *a1 = reinterpret_cast<RED4ext::CRTTIWeakHandleType *>(MacCompositeType("whandle:", a2));
  return a1;
}

RED4ext::CRTTIHandleType **CreateCRTTIHandleTypeFromClass(RED4ext::CRTTIHandleType **a1, RED4ext::CBaseRTTIType *a2) {
  *a1 = reinterpret_cast<RED4ext::CRTTIHandleType *>(MacCompositeType("handle:", a2));
  return a1;
}

RED4ext::CRTTIResourceAsyncReferenceType **CreateCRTTIRaRefTypeFromClass(RED4ext::CRTTIResourceAsyncReferenceType **a1,
                                                                         RED4ext::CBaseRTTIType *a2) {
  *a1 = reinterpret_cast<RED4ext::CRTTIResourceAsyncReferenceType *>(MacCompositeType("raRef:", a2));
  return a1;
}

// GetType already registered the type.
#define LTBF_REGISTER_COMPOSITE_TYPE(rtti, type) ((void)(rtti), (void)(type))
#else
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

#define LTBF_REGISTER_COMPOSITE_TYPE(rtti, type) (rtti)->RegisterType(type)
#endif

// bool CScriptDataValidator::Validate(CScriptedDataEnvironment const &, IScriptDataErrorReporter &) const
REGISTER_FLIGHT_HOOK_HASH(bool __fastcall, 898639042, ProcessScriptTypes, uint32_t* version, const ScriptData* scriptData, void* scriptLogger) {
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
#ifdef __APPLE__
              if (!rttiClass) {
                spdlog::error("[ltbf] native property {}.{}: the class has no RTTI type; property not added",
                              scriptClass->name.ToString(), prop->name.ToString());
                continue;
              }
#endif
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
                        LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType);
                        innerType = whType;
                      } else if (innerTypeStr.starts_with("ref")) {
                        RED4ext::CRTTIHandleType *whType;
                        CreateCRTTIHandleTypeFromClass(&whType, innerInnerType);
                        LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType);
                        innerType = whType;
                      } else if (innerTypeStr.starts_with("array")) {
                        RED4ext::CRTTIArrayType *whType;
                        CreateCRTTIArrayTypeFromClass(&whType, innerInnerType);
                        LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType);
                        innerType = whType;
                      } else if (innerTypeStr.starts_with("raRef")) {
                        RED4ext::CRTTIResourceAsyncReferenceType *whType;
                        CreateCRTTIRaRefTypeFromClass(&whType, innerInnerType);
                        LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType);
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
                      LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType);
                      rttiType = whType;
                    } else if (outerTypeStr.starts_with("ref")) {
                      RED4ext::CRTTIHandleType *whType;
                      CreateCRTTIHandleTypeFromClass(&whType, innerType);
                      LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType);
                      rttiType = whType;
                    } else if (outerTypeStr.starts_with("array")) {
                      RED4ext::CRTTIArrayType *whType;
                      CreateCRTTIArrayTypeFromClass(&whType, innerType);
                      LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType);
                      rttiType = whType;
                    } else if (innerTypeStr.starts_with("raRef")) {
                      RED4ext::CRTTIResourceAsyncReferenceType *whType;
                      CreateCRTTIRaRefTypeFromClass(&whType, innerType);
                      LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType);
                      innerType = whType;
                    }
                  } else {
#ifdef __APPLE__
                    spdlog::error("[ltbf] native property {}.{}: could not find inner type '{}' in '{}'",
                                  scriptClass->name.ToString(), prop->name.ToString(), innerTypeStr, typeStr);
#else
                    spdlog::warn("could not find inner type '{}' in '{}'", innerTypeStr, typeStr);
#endif
                  }
                }
              }
              //if (customTypeStr) {
              //  prop->type->rttiType = rttiType;
              //}
#ifdef __APPLE__
              // LTBF R42g (macOS): some of the scripts' @runtimeProperty fields re-declare a property the game class already has
              // (PhysicalMeshComponent.visibilityAnimationParam / startInactive, IComponent.name, MeshComponent.visualScale, ...) at
              // the Windows offset. A second CProperty with the same name is not harmless on macOS: where the layouts differ
              // (PhysicalMeshComponent: game 0x210 / 0x222, script 0x228 / 0x23A) anything that resolves the property by name writes
              // 0x18 bytes past the real field into whatever the macOS layout keeps there, for every such component the game builds
              // (Boots C to F2: heap corruption while entities stream in). The game's own property already gives the scripts the
              // field, so it is used instead of a duplicate.
              if (rttiType) {
                RED4ext::CProperty *existing = nullptr;
                for (auto *cls = rttiClass; cls && !existing; cls = cls->parent) {
                  for (auto *gameProp : cls->props) {
                    if (gameProp && gameProp->name == prop->name) {
                      existing = gameProp;
                      break;
                    }
                  }
                }
                if (existing) {
                  LtbfMac::Log("[ltbf] native property %s.%s: the game class already has it at 0x%X (script offset 0x%lX); the "
                               "game's property is used, no duplicate added",
                               scriptClass->name.ToString(), prop->name.ToString(), existing->valueOffset, offsetValue);
                  continue;
                }
              }
#endif
              if (rttiType) {
#ifdef __APPLE__
                auto property = RED4ext::CProperty::Create(rttiType, prop->name.ToString(), nullptr, offsetValue);
                if (property) {
                  rttiClass->props.PushBack(property);
                } else {
                  spdlog::error("[ltbf] native property {}.{} ({}): CProperty::Create failed; property not added",
                                scriptClass->name.ToString(), prop->name.ToString(), typeName.ToString());
                }
#else
                rttiClass->props.PushBack(
                    RED4ext::CProperty::Create(rttiType, prop->name.ToString(), nullptr, offsetValue));
#endif
              } else {
#ifdef __APPLE__
                spdlog::error("[ltbf] native property {}.{}: could not find type '{}' (script type '{}'); property not added",
                              scriptClass->name.ToString(), prop->name.ToString(), typeName.ToString(),
                              prop->type->name.ToString());
#else
                spdlog::warn("could not find type '{}' for property: {}", prop->type->name.ToString(), prop->name.ToString());
#endif
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