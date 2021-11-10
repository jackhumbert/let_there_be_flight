#include <iostream>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ImageWidget.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/TextureAtlas.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/EBlurDimension.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/BoxBlurEffect.hpp>
#include <RED4ext/InstanceType.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/IEffect.hpp>

#include "Utils.hpp"
#include "stdafx.hpp"
#include "FlightAudio.hpp"
#include "FlightLog.hpp"
#include "FlightStats_Record.hpp"
#include "FmodHelper.hpp"

RED4EXT_C_EXPORT void RED4EXT_CALL RegisterTypes()
{
    spdlog::info("Registering classes & types");
    FlightAudio::RegisterTypes();
    FlightLog::RegisterTypes();
    //FlightStats_Record::RegisterTypes();
}

void SetAtlasResource(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4ext::red::ResourceReferenceScriptToken value;
    RED4ext::GetParameter(aFrame, &value);
    aFrame->code++; // skip ParamEnd
    auto rtti = RED4ext::CRTTISystem::Get();

    //auto inkImageWidget = rtti->GetClass("inkImageWidget");
    //auto setAtlasResource = inkImageWidget->GetFunction("SetAtlasResource");
    //RED4ext::StackArgs_t args;
    //args.emplace_back(nullptr, &value); // or value, I don't remember how it should be passed.
    //RED4ext::ExecuteFunction(aContext, setAtlasResource, aOut, args);

    auto redResourceReferenceScriptToken = rtti->GetClass("redResourceReferenceScriptToken");
    auto IsValid = redResourceReferenceScriptToken->GetFunction("IsValid");
    bool valid;
    RED4ext::ExecuteFunction(redResourceReferenceScriptToken, IsValid, &valid, &value);
    if (valid) {
        auto inkMaskWidget = rtti->GetClass("inkMaskWidget");
        //uint64_t resource = RED4ext::FNV1a64("base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
        inkMaskWidget->GetProperty("textureAtlas")->SetValue(aContext, value.resource);
        if (aOut != nullptr) {
        *aOut = true;
        }
    }
    else {
        if (aOut != nullptr) {
            *aOut = false;
        }
    }
    
}

void CreateEffect(RED4ext::IScriptable* apContext, RED4ext::CStackFrame* apFrame, void* apOut, int64_t a4)
{
    RED4ext::CName typeName;
    RED4ext::CName effectName;

    RED4ext::GetParameter(apFrame, &typeName);
    RED4ext::GetParameter(apFrame, &effectName);
    apFrame->code++; // skip ParamEnd

    auto pRtti = RED4ext::CRTTISystem::Get();

    auto pEffectClass = pRtti->GetClass(typeName);
    auto pEffectInstance = reinterpret_cast<RED4ext::ISerializable*>(pEffectClass->AllocInstance());
    auto pEffectHandle = RED4ext::Handle<RED4ext::ISerializable>(pEffectInstance);

    pEffectClass->GetProperty("effectName")->SetValue(pEffectInstance, effectName);

    auto pWidgetClass = pRtti->GetClass("inkWidget");
    auto pEffectsProp = pWidgetClass->GetProperty("effects");
    auto pEffectsType = reinterpret_cast<RED4ext::CRTTIArrayType*>(pEffectsProp->type);
    auto pEffectsArray = pEffectsProp->GetValue<RED4ext::DynArray<void*>*>(apContext);

    auto lastIndex = pEffectsType->GetLength(pEffectsArray);

    pEffectsType->InsertAt(pEffectsArray, lastIndex);

    auto pLastElement = pEffectsType->GetElement(pEffectsArray, lastIndex);

    pEffectsType->GetInnerType()->Assign(pLastElement, &pEffectHandle);
}

void SetBlurDimension(RED4ext::IScriptable* apContext, RED4ext::CStackFrame* apFrame, bool* apOut, int64_t a4) {

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
    auto pEffectsType = reinterpret_cast<RED4ext::CRTTIArrayType*>(pEffectsProp->type);
    auto pEffectsArray = pEffectsProp->GetValue<RED4ext::DynArray<void*>*>(apContext);

    auto pEffectsArraySize = pEffectsType->GetLength(pEffectsArray);

    bool found = false;

    for (int i = 0; i < pEffectsArraySize; i++) {
        auto pEffect = (RED4ext::Handle<RED4ext::ISerializable>*)pEffectsType->GetElement(pEffectsArray, i);
        RED4ext::CName pEffectName = pGenericEffectClass->GetProperty("effectName")->GetValue<RED4ext::CName>(pEffect->instance);
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

void SetShapeResource(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    RED4ext::red::ResourceReferenceScriptToken value;
    RED4ext::GetParameter(aFrame, &value);
    aFrame->code++; // skip ParamEnd
    auto rtti = RED4ext::CRTTISystem::Get();

    //auto inkImageWidget = rtti->GetClass("inkImageWidget");
    //auto setAtlasResource = inkImageWidget->GetFunction("SetAtlasResource");
    //RED4ext::StackArgs_t args;
    //args.emplace_back(nullptr, &value); // or value, I don't remember how it should be passed.
    //RED4ext::ExecuteFunction(aContext, setAtlasResource, aOut, args);

    auto inkShapeWidget = rtti->GetClass("inkShapeWidget");
    //uint64_t resource = RED4ext::FNV1a64("base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
    inkShapeWidget->GetProperty("shapeResource")->SetValue(aContext, value.resource);
}

RED4EXT_C_EXPORT void RED4EXT_CALL PostRegisterTypes()
{
    spdlog::info("Registering functions");
    FlightAudio::RegisterFunctions();
    FlightLog::RegisterFunctions();
    RED4ext::CRTTISystem::Get()->RegisterScriptName("entBaseCameraComponent", "BaseCameraComponent");
    //RED4ext::CRTTISystem::Get()->RegisterScriptName("entColliderComponent", "ColliderComponent");
    //FlightStats_Record::RegisterFunctions();

    auto rtti = RED4ext::CRTTISystem::Get();

    auto inkMaskWidget = rtti->GetClass("inkMaskWidget");
    auto setAtlasTextureFunc = RED4ext::CClassFunction::Create(inkMaskWidget, "SetAtlasResource", "SetAtlasResource", &SetAtlasResource, { .isNative = true });
    setAtlasTextureFunc->AddParam("redResourceReferenceScriptToken", "atlasResourcePath");
    setAtlasTextureFunc->SetReturnType("Bool");
    inkMaskWidget->RegisterFunction(setAtlasTextureFunc);

    auto inkShapeWidget = rtti->GetClass("inkShapeWidget");
    auto setShapeResourceFunc = RED4ext::CClassFunction::Create(inkShapeWidget, "SetShapeResource", "SetShapeResource", &SetShapeResource, { .isNative = true });
    setShapeResourceFunc->AddParam("redResourceReferenceScriptToken", "shapeResourcePath");
    inkShapeWidget->RegisterFunction(setShapeResourceFunc);


    auto inkWidget = rtti->GetClass("inkWidget");

    auto createEffectFunc = RED4ext::CClassFunction::Create(inkWidget, "CreateEffect", "CreateEffect", &CreateEffect, { .isNative = true });
    createEffectFunc->AddParam("CName", "typeName");
    createEffectFunc->AddParam("CName", "effectName");
    inkWidget->RegisterFunction(createEffectFunc);

    auto setBlurDimensionFunc = RED4ext::CClassFunction::Create(inkWidget, "SetBlurDimension", "SetBlurDimension", &SetBlurDimension, { .isNative = true });
    setBlurDimensionFunc->AddParam("CName", "effectName");
    setBlurDimensionFunc->AddParam("inkEBlurDimension", "blurDimension");
    setBlurDimensionFunc->SetReturnType("Bool");
    inkWidget->RegisterFunction(setBlurDimensionFunc);

    //auto getEffectFunc = RED4ext::CClassFunction::Create(inkWidget, "GetEffect", "GetEffect", &GetEffect, { .isNative = true });
    //getEffectFunc->AddParam("CName", "effectName");
    //getEffectFunc->SetReturnType("inkIEffect");
    //inkWidget->RegisterFunction(getEffectFunc);


}

BOOL APIENTRY DllMain(HMODULE aModule, DWORD aReason, LPVOID aReserved)
{
    switch (aReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(aModule);
        RED4ext::RTTIRegistrator::Add(RegisterTypes, PostRegisterTypes);

        Utils::CreateLogger();
        spdlog::info("Starting up");

        break;
    }
    case DLL_PROCESS_DETACH:
    {
        spdlog::info("Shutting down");
        spdlog::shutdown();

        break;
    }
    }

    return TRUE;
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Load(RED4ext::PluginHandle aHandle, const RED4ext::IRED4ext* aInterface)
{
    FlightAudio::Load();
    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Unload()
{
    FlightAudio::Unload();
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo* aInfo)
{
    aInfo->name = L"Flight Control";
    aInfo->author = L"Jack Humbert";
    aInfo->version = RED4EXT_SEMVER(0, 0, 1);
    aInfo->runtime = RED4EXT_RUNTIME_LATEST;
    aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_LATEST;
}
