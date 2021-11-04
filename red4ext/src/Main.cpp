#include <iostream>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/RTTITypes.hpp>

#include "Utils.hpp"
#include "stdafx.hpp"
#include "FlightAudio.hpp"
#include "FlightLog.hpp"
#include "FmodHelper.hpp"

RED4EXT_C_EXPORT void RED4EXT_CALL RegisterTypes()
{
    spdlog::info("Registering classes & types");
    FlightAudio::RegisterTypes();
    FlightLog::RegisterTypes();
}

RED4EXT_C_EXPORT void RED4EXT_CALL PostRegisterTypes()
{
    spdlog::info("Registering functions");
    FlightAudio::RegisterFunctions();
    FlightLog::RegisterFunctions();

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
