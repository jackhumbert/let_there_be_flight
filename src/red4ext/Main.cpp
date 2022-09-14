#include <RED4ext/InstanceType.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/EffectDesc.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/EffectSpawnerComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/VehicleTPPCameraParams_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/BoxBlurEffect.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/EBlurDimension.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/IEffect.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ImageWidget.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/TextureAtlas.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemBody.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/ChassisComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <iostream>

#include "Engine/RTTIClass.hpp"
#include "FlightAudio.hpp"
#include "Utils.hpp"
#include "stdafx.hpp"
//#include "FlightHUDGameController.hpp"
#include "FlightLog.hpp"
//#include "FlightStats_Record.hpp"
#include "FlightSystem.hpp"
#include "FmodHelper.hpp"
#include "VehicleSpeedUnlimiter.hpp"
#include "FlightHelper.hpp"
#include "FlightController.hpp"
#include "FlightEvents.hpp"
#include "FlightCamera.hpp"

RED4EXT_C_EXPORT void RED4EXT_CALL RegisterTypes() {
  spdlog::info("Registering classes & types");
  FlightModuleFactory::GetInstance().RegisterTypes();
}

void SetAtlasResource(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
  RED4ext::red::ResourceReferenceScriptToken value;
  RED4ext::GetParameter(aFrame, &value);
  aFrame->code++; // skip ParamEnd
  auto rtti = RED4ext::CRTTISystem::Get();

  auto redResourceReferenceScriptToken = rtti->GetClass("redResourceReferenceScriptToken");
  auto IsValid = redResourceReferenceScriptToken->GetFunction("IsValid");
  bool valid;
  RED4ext::ExecuteFunction(redResourceReferenceScriptToken, IsValid, &valid, &value);
  if (valid) {
    auto inkMaskWidget = rtti->GetClass("inkMaskWidget");
    inkMaskWidget->GetProperty("textureAtlas")->SetValue(aContext, value.resource);
    if (aOut != nullptr) {
      *aOut = true;
    }
  } else {
    if (aOut != nullptr) {
      *aOut = false;
    }
  }
}

void CreateEffect(RED4ext::IScriptable *apContext, RED4ext::CStackFrame *apFrame, void *apOut, int64_t a4) {
  RED4ext::CName typeName;
  RED4ext::CName effectName;

  RED4ext::GetParameter(apFrame, &typeName);
  RED4ext::GetParameter(apFrame, &effectName);
  apFrame->code++; // skip ParamEnd

  auto pRtti = RED4ext::CRTTISystem::Get();

  auto pEffectClass = pRtti->GetClass(typeName);
  auto pEffectInstance = reinterpret_cast<RED4ext::ISerializable *>(pEffectClass->AllocInstance());
  auto pEffectHandle = RED4ext::Handle<RED4ext::ISerializable>(pEffectInstance);

  pEffectClass->GetProperty("effectName")->SetValue(pEffectInstance, effectName);

  auto pWidgetClass = pRtti->GetClass("inkWidget");
  auto pEffectsProp = pWidgetClass->GetProperty("effects");
  auto pEffectsType = reinterpret_cast<RED4ext::CRTTIArrayType *>(pEffectsProp->type);
  auto pEffectsArray = pEffectsProp->GetValue<RED4ext::DynArray<void *> *>(apContext);

  auto lastIndex = pEffectsType->GetLength(pEffectsArray);

  pEffectsType->InsertAt(pEffectsArray, lastIndex);

  auto pLastElement = pEffectsType->GetElement(pEffectsArray, lastIndex);

  pEffectsType->GetInnerType()->Assign(pLastElement, &pEffectHandle);
}

void SetBlurDimension(RED4ext::IScriptable *apContext, RED4ext::CStackFrame *apFrame, bool *apOut, int64_t a4) {
  RED4ext::CName effectName;
  RED4ext::ink::EBlurDimension blurDimension;
  RED4ext::GetParameter(apFrame, &effectName);
  RED4ext::GetParameter(apFrame, &blurDimension);
  apFrame->code++; // skip ParamEnd

  auto pRtti = RED4ext::CRTTISystem::Get();

  auto pEffectClass = pRtti->GetClass("inkBoxBlurEffect");
  auto pGenericEffectClass = pRtti->GetClass("inkIEffect");

  auto pWidgetClass = pRtti->GetClass("inkWidget");
  auto pEffectsProp = pWidgetClass->GetProperty("effects");
  auto pEffectsType = reinterpret_cast<RED4ext::CRTTIArrayType *>(pEffectsProp->type);
  auto pEffectsArray = pEffectsProp->GetValue<RED4ext::DynArray<void *> *>(apContext);

  auto pEffectsArraySize = pEffectsType->GetLength(pEffectsArray);

  bool found = false;

  for (int i = 0; i < pEffectsArraySize; i++) {
    auto pEffect = (RED4ext::Handle<RED4ext::ISerializable> *)pEffectsType->GetElement(pEffectsArray, i);
    RED4ext::CName pEffectName =
        pGenericEffectClass->GetProperty("effectName")->GetValue<RED4ext::CName>(pEffect->instance);
    if (pEffectName == effectName) {
      pEffectClass->GetProperty("blurDimension")->SetValue(pEffect->instance, blurDimension);
      found = true;
      break;
    }
  }

  if (apOut) {
    *apOut = found;
  }
}

void SetShapeResource(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
  RED4ext::red::ResourceReferenceScriptToken value;
  RED4ext::GetParameter(aFrame, &value);
  aFrame->code++; // skip ParamEnd
  auto rtti = RED4ext::CRTTISystem::Get();

  // auto inkImageWidget = rtti->GetClass("inkImageWidget");
  // auto setAtlasResource = inkImageWidget->GetFunction("SetAtlasResource");
  // RED4ext::StackArgs_t args;
  // args.emplace_back(nullptr, &value); // or value, I don't remember how it
  // should be passed. RED4ext::ExecuteFunction(aContext, setAtlasResource,
  // aOut, args);

  auto inkShapeWidget = rtti->GetClass("inkShapeWidget");
  // uint64_t resource =
  // RED4ext::FNV1a64("base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
  inkShapeWidget->GetProperty("shapeResource")->SetValue(aContext, value.resource);
}

void ChassisGetComOffset(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Transform *aOut,
                         int64_t a4) {
  aFrame->code++; // skip ParamEnd

  auto rtti = RED4ext::CRTTISystem::Get();
  auto vccClass = rtti->GetClass("vehicleChassisComponent");
  auto crProp = vccClass->GetProperty("collisionResource");
  auto cr = crProp->GetValue<RED4ext::Ref<RED4ext::physics::SystemResource>>(aContext);
  RED4ext::Handle<RED4ext::physics::SystemBody> hpsb = cr.Fetch().GetPtr()->bodies[0];
  auto params = hpsb->params;

  if (aOut) {
    *aOut = params.comOffset;
  }
}

#include "LoadResRef.hpp"
#include <RED4ext/Scripting/Natives/Generated/world/Effect.hpp>

void EffectSpawnerAddEffect(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
  // RED4ext::red::ResourceReferenceScriptToken value;
  // RED4ext::CName name;
  // bool valid;
  // RED4ext::GetParameter(aFrame, &value);
  // RED4ext::GetParameter(aFrame, &name);
  aFrame->code++; // skip ParamEnd

  spdlog::info("[EffectSpawnerComponent] Adding New Effect");

  auto rtti = RED4ext::CRTTISystem::Get();

  // auto rrst = rtti->GetClass("redResourceReferenceScriptToken");
  // auto IsValid = rrst->GetFunction("IsValid");
  // RED4ext::ExecuteFunction(rrst, IsValid, &valid, &value);

  // if (valid) {
  auto esc = reinterpret_cast<RED4ext::ent::EffectSpawnerComponent *>(aContext);

  auto effectDescCls = rtti->GetClass("entEffectDesc");
  auto ceei = rtti->GetClass("worldCompiledEffectEventInfo");
  auto cepi = rtti->GetClass("worldCompiledEffectPlacementInfo");
  auto v3 = rtti->GetClass("Vector3");
  auto q = rtti->GetClass("Quaternion");
  auto ed = reinterpret_cast<RED4ext::ent::EffectDesc *>(effectDescCls->AllocInstance());

  ed->effectName = "test_effect";
  ed->id.unk00 = 2738936757675336192; // 2738936757675335680 changed a bit
  //auto existingEffect = (RED4ext::ent::EffectDesc *)esc->effectDescs[0].instance;
  //ed->effect.ref = existingEffect->effect.ref;

  //auto wrapper = new RED4ext::ResourceWrapper<RED4ext::world::Effect>();
  //uint64_t hash = 2413621465439885870;
  //LoadResRef<RED4ext::world::Effect>(&hash, wrapper, true);
  //ed->effect.ref = wrapper->self->resource;

  //ed->effect.ref = 3990659875028156682;
  ed->effect.path = 2413621465439885870; // base\fx\vehicles\flight.effect

  {
    auto eventInfo = reinterpret_cast<RED4ext::world::CompiledEffectEventInfo *>(ceei->AllocInstance());
    eventInfo->eventRUID.unk00 = 2738876334750015488;
    eventInfo->placementIndexMask = 15;
    eventInfo->flags = 1;
    ed->compiledEffectInfo.eventsSortedByRUID.EmplaceBack(*eventInfo);
  }

  {
    auto eventInfo = reinterpret_cast<RED4ext::world::CompiledEffectEventInfo *>(ceei->AllocInstance());
    eventInfo->eventRUID.unk00 = 2738876938696237056;
    eventInfo->placementIndexMask = 15;
    eventInfo->flags = 1;
    ed->compiledEffectInfo.eventsSortedByRUID.EmplaceBack(*eventInfo);
  }

  {
    auto eventInfo = reinterpret_cast<RED4ext::world::CompiledEffectEventInfo *>(ceei->AllocInstance());
    eventInfo->eventRUID.unk00 = 2738888051890561024;
    eventInfo->placementIndexMask = 15;
    eventInfo->flags = 1;
    ed->compiledEffectInfo.eventsSortedByRUID.EmplaceBack(*eventInfo);
  }

  {
    auto eventInfo = reinterpret_cast<RED4ext::world::CompiledEffectEventInfo *>(ceei->AllocInstance());
    eventInfo->eventRUID.unk00 = 2738893332334747648;
    eventInfo->placementIndexMask = 15;
    eventInfo->flags = 1;
    ed->compiledEffectInfo.eventsSortedByRUID.EmplaceBack(*eventInfo);
  }

  {
    auto eventInfo = reinterpret_cast<RED4ext::world::CompiledEffectEventInfo *>(ceei->AllocInstance());
    eventInfo->eventRUID.unk00 = 2738893580167782400;
    eventInfo->placementIndexMask = 15;
    eventInfo->flags = 1;
    ed->compiledEffectInfo.eventsSortedByRUID.EmplaceBack(*eventInfo);
  }

  {
    auto eventInfo = reinterpret_cast<RED4ext::world::CompiledEffectEventInfo *>(ceei->AllocInstance());
    eventInfo->eventRUID.unk00 = 2738893796308656128;
    eventInfo->placementIndexMask = 15;
    eventInfo->flags = 1;
    ed->compiledEffectInfo.eventsSortedByRUID.EmplaceBack(*eventInfo);
  }

  {
    auto placementInfo = reinterpret_cast<RED4ext::world::CompiledEffectPlacementInfo *>(cepi->AllocInstance());
    placementInfo->flags = 5;
    placementInfo->placementTagIndex = 0;
    placementInfo->relativePositionIndex = 0;
    placementInfo->relativeRotationIndex = 0;
    ed->compiledEffectInfo.placementInfos.EmplaceBack(*placementInfo);
  }

  {
    auto placementInfo = reinterpret_cast<RED4ext::world::CompiledEffectPlacementInfo *>(cepi->AllocInstance());
    placementInfo->flags = 5;
    placementInfo->placementTagIndex = 1;
    placementInfo->relativePositionIndex = 0;
    placementInfo->relativeRotationIndex = 0;
    ed->compiledEffectInfo.placementInfos.EmplaceBack(*placementInfo);
  }

  {
    auto placementInfo = reinterpret_cast<RED4ext::world::CompiledEffectPlacementInfo *>(cepi->AllocInstance());
    placementInfo->flags = 5;
    placementInfo->placementTagIndex = 2;
    placementInfo->relativePositionIndex = 0;
    placementInfo->relativeRotationIndex = 0;
    ed->compiledEffectInfo.placementInfos.EmplaceBack(*placementInfo);
  }

  {
    auto placementInfo = reinterpret_cast<RED4ext::world::CompiledEffectPlacementInfo *>(cepi->AllocInstance());
    placementInfo->flags = 5;
    placementInfo->placementTagIndex = 3;
    placementInfo->relativePositionIndex = 0;
    placementInfo->relativeRotationIndex = 0;
    ed->compiledEffectInfo.placementInfos.EmplaceBack(*placementInfo);
  }

  ed->compiledEffectInfo.placementTags.EmplaceBack("fx_holo_corner_fl");
  ed->compiledEffectInfo.placementTags.EmplaceBack("fx_holo_corner_fr");
  ed->compiledEffectInfo.placementTags.EmplaceBack("fx_holo_corner_bl");
  ed->compiledEffectInfo.placementTags.EmplaceBack("fx_holo_corner_br");

  auto vector = reinterpret_cast<RED4ext::Vector3 *>(v3->AllocInstance());
  ed->compiledEffectInfo.relativePositions.EmplaceBack(*vector);

  auto quaternion = reinterpret_cast<RED4ext::Quaternion *>(q->AllocInstance());
  quaternion->r = 1.0;
  ed->compiledEffectInfo.relativeRotations.EmplaceBack(*quaternion);

  auto edHandle = RED4ext::Handle<RED4ext::ent::EffectDesc>(ed);

  esc->effectDescs.EmplaceBack(edHandle);

  spdlog::info("[EffectSpawnerComponent] New Effect Added");
  //}

  // if (aOut) {
  //  *aOut = valid;
  //}
}

RED4EXT_C_EXPORT void RED4EXT_CALL PostRegisterTypes() {
  spdlog::info("Registering functions");

  FlightModuleFactory::GetInstance().PostRegisterTypes();

  RED4ext::CRTTISystem::Get()->RegisterScriptName("entBaseCameraComponent", "BaseCameraComponent");
  // RED4ext::CRTTISystem::Get()->RegisterScriptName("entColliderComponent",
  // "ColliderComponent"); FlightStats_Record::RegisterFunctions();

  auto rtti = RED4ext::CRTTISystem::Get();

  auto inkMaskWidget = rtti->GetClass("inkMaskWidget");
  auto setAtlasTextureFunc = RED4ext::CClassFunction::Create(inkMaskWidget, "SetAtlasResource", "SetAtlasResource",
                                                             &SetAtlasResource, {.isNative = true});
  inkMaskWidget->RegisterFunction(setAtlasTextureFunc);

  auto inkShapeWidget = rtti->GetClass("inkShapeWidget");
  auto setShapeResourceFunc = RED4ext::CClassFunction::Create(inkShapeWidget, "SetShapeResource", "SetShapeResource",
                                                              &SetShapeResource, {.isNative = true});
  inkShapeWidget->RegisterFunction(setShapeResourceFunc);

  auto inkWidget = rtti->GetClass("inkWidget");

  auto createEffectFunc =
      RED4ext::CClassFunction::Create(inkWidget, "CreateEffect", "CreateEffect", &CreateEffect, {.isNative = true});
  inkWidget->RegisterFunction(createEffectFunc);

  auto setBlurDimensionFunc = RED4ext::CClassFunction::Create(inkWidget, "SetBlurDimension", "SetBlurDimension",
                                                              &SetBlurDimension, {.isNative = true});
  inkWidget->RegisterFunction(setBlurDimensionFunc);

  // auto getEffectFunc = RED4ext::CClassFunction::Create(inkWidget,
  // "GetEffect", "GetEffect", &GetEffect, { .isNative = true });
  // getEffectFunc->AddParam("CName", "effectName");
  // getEffectFunc->SetReturnType("inkIEffect");
  // inkWidget->RegisterFunction(getEffectFunc);

  auto gamePSMVehicleEnum = rtti->GetEnum("gamePSMVehicle");
  gamePSMVehicleEnum->hashList.PushBack("Flight");
  gamePSMVehicleEnum->valueList.PushBack(8);

  auto UIGameContextEnum = rtti->GetEnum("UIGameContext");
  UIGameContextEnum->hashList.PushBack("VehicleFlight");
  UIGameContextEnum->valueList.PushBack(10);

  auto NavGenAgentSizeEnum = rtti->GetEnum("NavGenAgentSize");
  NavGenAgentSizeEnum->hashList.PushBack("Vehicle");
  NavGenAgentSizeEnum->valueList.PushBack(1);

  // auto UIGameContextEnum = rtti->GetEnum("HUDActorType");
  // UIGameContextEnum->hashList.PushBack("FLIGHT");
  // UIGameContextEnum->valueList.PushBack(7);

  // RED4ext::CEnum::Flags flags = {};
  // RED4ext::CEnum gamePSMVehicleEnum = RED4ext::CEnum::CEnum("gamePSMVehicle",
  // 10, flags);

  // rtti->CreateScriptedEnum("gamePSMVehicle", 10, &gamePSMVehicleEnum);

  /*   auto cc = rtti->GetClass("vehicleTPPCameraComponent");
     cc->props.PushBack(RED4ext::CProperty::Create(
          rtti->GetType("Bool"), "isInAir", nullptr, 0x2E0));*/
  auto cc = rtti->GetClass("vehicleTPPCameraComponent");
  cc->props.PushBack(
      RED4ext::CProperty::Create(rtti->GetType("Float"), "drivingDirectionCompensationSpeedCoef", nullptr, 0x4E0));
  cc->props.PushBack(
      RED4ext::CProperty::Create(rtti->GetType("Float"), "drivingDirectionCompensationAngleSmooth", nullptr, 0x4E8));
  cc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Bool"), "lockedCamera", nullptr, 0x48A));
  cc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("WorldPosition"), "worldPosition", nullptr, 0x320));
  cc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("WorldTransform"), "worldTransform2", nullptr, 0x2B0));
  cc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Float"), "pitch", nullptr, 0x380));
  cc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Float"), "yaw", nullptr, 0x384));
  cc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Float"), "yawDelta", nullptr, 0x2D0));
  cc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Float"), "pitchDelta", nullptr, 0x2D4));

  auto vcc = rtti->GetClass("vehicleChassisComponent");
  auto getComOffsetFunc =
      RED4ext::CClassFunction::Create(vcc, "GetComOffset", "GetComOffset", &ChassisGetComOffset, {.isNative = true});
  vcc->RegisterFunction(getComOffsetFunc);

  auto vdtpe = rtti->GetClass("vehicleDriveToPointEvent");
  vdtpe->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Vector3"), "targetPos", nullptr, 0x40));
  vdtpe->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Bool"), "useTraffic", nullptr, 0x50));
  vdtpe->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Float"), "speedInTraffic", nullptr, 0x54));

  RED4ext::CRTTISystem::Get()->RegisterScriptName("entEffectSpawnerComponent", "EffectSpawnerComponent");
  auto eesc = rtti->GetClass("entEffectSpawnerComponent");
  auto eescAddEffect =
      RED4ext::CClassFunction::Create(eesc, "AddEffect", "AddEffect", &EffectSpawnerAddEffect, {.isNative = true});
  eesc->RegisterFunction(eescAddEffect);

  auto ecc = rtti->GetClass("entColliderComponent");
  ecc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Float"), "mass", nullptr, 0x150));
  ecc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Float"), "massOverride", nullptr, 0x14C));
  ecc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Vector3"), "inertia", nullptr, 0x158));
  ecc->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Transform"), "comOffset", nullptr, 0x170));

  // using func_t = bool (*)(RED4ext::CBaseRTTIType*, int64_t,
  // RED4ext::ScriptInstance); RED4ext::RelocFunc<func_t> func(0x1400000);

  // 0x120 + 0x40 (ptr120, float*);
  // 0x14342E6C0
}

bool GetVFTRVA(RED4ext::CGameApplication * app) {

 auto rtti = RED4ext::CRTTISystem::Get();

//// auto types = RED4ext::DynArray<RED4ext::CBaseRTTIType*>(new RED4ext::Memory::DefaultAllocator());
//// rtti->GetNativeTypes(types);
 auto classes = RED4ext::DynArray<RED4ext::CClass *>(new RED4ext::Memory::DefaultAllocator());
 rtti->GetClasses(nullptr, classes);

 for (const auto &cls : classes) {
   if (cls && cls->name != "None" && !cls->flags.isAbstract) {
     if (cls->name == "inkInputKeyIconManager")
       continue;
     auto name = cls->name.ToString();
     auto instance = cls->AllocMemory();
     cls->ConstructCls(instance);
     if (instance) {
       auto va = *reinterpret_cast<uintptr_t *>(instance);
       auto rva = va - RED4ext::RelocBase::GetImageBase();
       if (va > RED4ext::RelocBase::GetImageBase() && rva < 0x4700000) {
         spdlog::info("#define {}_VFT_RVA 0x{:X}", name, rva);
       }
     }
   }
 }
 return true;
}


RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason,
                                        const RED4ext::Sdk *aSdk) {
  switch (aReason) {
  case RED4ext::EMainReason::Load: {
    // Attach hooks, register RTTI types, add custom states or initalize your
    // application. DO NOT try to access the game's memory at this point, it
    // is not initalized yet.

    Utils::CreateLogger();
    spdlog::info("Starting up Let There Be Flight v0.1.1");
    auto ptr = GetModuleHandle(nullptr);
    spdlog::info("Base address: {}", fmt::ptr(ptr));
    auto modPtr = aHandle;
    spdlog::info("Mod address: {}", fmt::ptr(modPtr));

    RED4ext::RTTIRegistrator::Add(RegisterTypes, PostRegisterTypes);
    Engine::RTTIRegistrar::RegisterPending();

    RED4ext::GameState initState;
    initState.OnEnter = nullptr;
    initState.OnUpdate = nullptr;
    initState.OnExit = &FlightAudio::Load;

    aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Initialization, &initState);

    //initState.OnEnter = &GetVFTRVA;
    //initState.OnUpdate = nullptr;
    //initState.OnExit = nullptr;
    //aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &initState);

    RED4ext::GameState shutdownState;
    shutdownState.OnEnter = nullptr;
    shutdownState.OnUpdate = &FlightAudio::Unload;
    shutdownState.OnExit = nullptr;

    aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Shutdown, &shutdownState);

    FlightModuleFactory::GetInstance().Load(aSdk, aHandle);

    break;
  }
  case RED4ext::EMainReason::Unload: {
    // Free memory, detach hooks.
    // The game's memory is already freed, to not try to do anything with it.

    spdlog::info("Shutting down");
    FlightModuleFactory::GetInstance().Unload(aSdk, aHandle);
    spdlog::shutdown();
    break;
  }
  }

  return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo *aInfo) {
  aInfo->name = L"Let There Be Flight";
  aInfo->author = L"Jack Humbert";
  aInfo->version = RED4EXT_SEMVER(0, 1, 1);
  aInfo->runtime = RED4EXT_RUNTIME_LATEST;
  aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports() { return RED4EXT_API_VERSION_LATEST; }
