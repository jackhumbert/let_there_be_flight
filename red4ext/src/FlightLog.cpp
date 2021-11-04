#include <iostream>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
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

        flightLogInfo->AddParam("String", "value");
        flightLogWarn->AddParam("String", "value");
        flightLogError->AddParam("String", "value");

        flightLogInfo->flags = flags;
        flightLogWarn->flags = flags;
        flightLogError->flags = flags;

        flightLogCls.RegisterFunction(flightLogInfo);
        flightLogCls.RegisterFunction(flightLogWarn);
        flightLogCls.RegisterFunction(flightLogError);
    }
}
