#include <iostream>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ImageWidgetReference.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/InstanceType.hpp>
#include <RED4ext/RTTITypes.hpp>

#include "stdafx.hpp"
#include "FlightLog.hpp"

namespace FlightLog {

    struct FlightLog : RED4ext::IScriptable
    {
        RED4ext::CClass* GetNativeType();
    };

    RED4ext::TTypedClass<FlightLog> flightLogCls("FlightLog");

    RED4ext::CClass* FlightLog::GetNativeType()
    {
        return &flightLogCls;
    }

    void Info(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
    {
        RED4ext::CString value;
        RED4ext::GetParameter(aFrame, &value);
        aFrame->code++; // skip ParamEnd
        spdlog::info(value.c_str());
    }

    void Warn(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
    {
        RED4ext::CString value;
        RED4ext::GetParameter(aFrame, &value);
        aFrame->code++; // skip ParamEnd
        spdlog::warn(value.c_str());
    }

    void Error(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
    {
        RED4ext::CString value;
        RED4ext::GetParameter(aFrame, &value);
        aFrame->code++; // skip ParamEnd
        spdlog::error(value.c_str());
    }

    void Probe(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
    {
        RED4ext::Handle<RED4ext::IScriptable> image;
        RED4ext::red::ResourceReferenceScriptToken value;
        RED4ext::GetParameter(aFrame, &image);
        RED4ext::GetParameter(aFrame, &value);
        aFrame->code++; // skip ParamEnd

        auto rtti = RED4ext::CRTTISystem::Get();

        auto inkImageWidget = rtti->GetClass("inkImageWidget");
        auto setAtlasResource = inkImageWidget->GetFunction("SetAtlasResource");
        RED4ext::StackArgs_t args;
        args.emplace_back(nullptr, &value);
        //image->ExecuteFunction("SetAtlasResource", args);
        RED4ext::ExecuteFunction(image, setAtlasResource, aOut, args);

    }

    void RegisterTypes() {
        flightLogCls.flags = { .isNative = true };
        RED4ext::CRTTISystem::Get()->RegisterType(&flightLogCls, 456816);
    }

    void RegisterFunctions() {
        auto rtti = RED4ext::CRTTISystem::Get();
        auto scriptable = rtti->GetClass("IScriptable");
        flightLogCls.parent = scriptable;

        RED4ext::CBaseFunction::Flags flags = { .isNative = true, .isStatic = true };

        auto flightLogInfo = RED4ext::CClassStaticFunction::Create(&flightLogCls, "Info", "Info", &Info);
        auto flightLogWarn = RED4ext::CClassStaticFunction::Create(&flightLogCls, "Warn", "Warn", &Warn);
        auto flightLogError = RED4ext::CClassStaticFunction::Create(&flightLogCls, "Error", "Error", &Error);
        auto flightLogProbe = RED4ext::CClassStaticFunction::Create(&flightLogCls, "Probe", "Probe", &Probe);

        flightLogInfo->flags = flags;
        flightLogWarn->flags = flags;
        flightLogError->flags = flags;
        flightLogProbe->flags = flags;

        flightLogCls.RegisterFunction(flightLogInfo);
        flightLogCls.RegisterFunction(flightLogWarn);
        flightLogCls.RegisterFunction(flightLogError);
        flightLogCls.RegisterFunction(flightLogProbe);
    }
}
