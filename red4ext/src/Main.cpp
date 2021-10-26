#include <iostream>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>

#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>
#include "Utils.hpp"
#include "stdafx.hpp"

static std::vector<char*> gPathList;
void* extraDriverData = NULL;
FMOD::Studio::System* fmod_system = NULL;
FMOD::System* coreSystem = NULL;
FMOD::Studio::Bank* masterBank = NULL;
FMOD::Studio::Bank* stringsBank = NULL;
FMOD::Studio::EventDescription* ltbfDescription = NULL;
FMOD::Studio::EventInstance* ltbfInstance = NULL;

// RED4EXT_C_EXPORT void RED4EXT_CALL RegisterTypes() {

// }

// RED4EXT_C_EXPORT void RED4EXT_CALL PostRegisterTypes() {
    
// }

#define ERRCHECK(_result) ERRCHECK_fn(_result, __FILE__, __LINE__)
void ERRCHECK_fn(FMOD_RESULT result, const char *file, int line)
{
    if (result != FMOD_OK)
    {
        spdlog::error("{0}({1}): FMOD error {2} - {3}", file, line, result, FMOD_ErrorString(result));
    }
}

BOOL APIENTRY DllMain(HMODULE aModule, DWORD aReason, LPVOID aReserved)
{
    switch (aReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(aModule);
        // RED4ext::RTTIRegistrator::Add(RegisterTypes, PostRegisterTypes);

        Utils::CreateLogger();

        break;
    }
    case DLL_PROCESS_DETACH:
    {
        spdlog::shutdown();

        break;
    }
    }

    return TRUE;
}

void StartSnd(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    spdlog::info("Starting sound");
    ERRCHECK(ltbfInstance->start());
}

void StopSnd(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    spdlog::info("Stopping sound");
    ERRCHECK(ltbfInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT));
}

void SetParams(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    float volume;
    RED4ext::Vector4 playerPosition;
    RED4ext::Vector4 playerUp;
    RED4ext::Vector4 playerForward;
    RED4ext::Vector4 cameraPosition;
    RED4ext::Vector4 cameraUp;
    RED4ext::Vector4 cameraForward;
    float speed;
    float surge;
    float yawDiff;

    RED4ext::GetParameter(aFrame, &volume);
    RED4ext::GetParameter(aFrame, &playerPosition);
    RED4ext::GetParameter(aFrame, &playerUp);
    RED4ext::GetParameter(aFrame, &playerForward);
    RED4ext::GetParameter(aFrame, &cameraPosition);
    RED4ext::GetParameter(aFrame, &cameraUp);
    RED4ext::GetParameter(aFrame, &cameraForward);
    RED4ext::GetParameter(aFrame, &speed);
    RED4ext::GetParameter(aFrame, &surge);
    RED4ext::GetParameter(aFrame, &yawDiff);
    aFrame->code++; // skip ParamEnd

    ERRCHECK(ltbfInstance->setVolume(volume));

    FMOD_3D_ATTRIBUTES attributes = { { 0 } };
    attributes.position.x = cameraPosition.X;
    attributes.position.y = cameraPosition.Y;
    attributes.position.z = cameraPosition.Z;        
    attributes.forward.x = cameraForward.X;
    attributes.forward.y = cameraForward.Y;
    attributes.forward.z = cameraForward.Z;        
    attributes.up.x = cameraUp.X;
    attributes.up.y = cameraUp.Y;
    attributes.up.z = cameraUp.Z;        
    ERRCHECK(fmod_system->setListenerAttributes(0, &attributes));

    attributes.position.x = playerPosition.X;
    attributes.position.y = playerPosition.Y;
    attributes.position.z = playerPosition.Z;        
    attributes.forward.x = playerForward.X;
    attributes.forward.y = playerForward.Y;
    attributes.forward.z = playerForward.Z;        
    attributes.up.x = playerUp.X;
    attributes.up.y = playerUp.Y;
    attributes.up.z = playerUp.Z;        
    ERRCHECK(ltbfInstance->set3DAttributes(&attributes));

    // spdlog::info("Updating Params: {0}, {1}, {2}", speed, surge, yawDiff);
    ERRCHECK(ltbfInstance->setParameterByName("Speed", speed));
    ERRCHECK(ltbfInstance->setParameterByName("Surge", surge));
    ERRCHECK(ltbfInstance->setParameterByName("YawDiff", yawDiff));
    ERRCHECK(fmod_system->update());
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Load(RED4ext::PluginHandle aHandle, const RED4ext::IRED4ext* aInterface)
{
    ERRCHECK(FMOD::Studio::System::create(&fmod_system));

    // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
    ERRCHECK(fmod_system->getCoreSystem(&coreSystem));
    ERRCHECK(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));
    
    // Due to a bug in WinSonic on Windows, FMOD initialization may fail on some machines.
    // If you get the error "FMOD error 51 - Error initializing output device", try using
    // a different output type such as FMOD_OUTPUTTYPE_AUTODETECT
    ERRCHECK(coreSystem->setOutput(FMOD_OUTPUTTYPE_AUTODETECT));

    std::string bank_path = (Utils::GetRootDir() / "bin" / "x64" / "plugins" / "flight_control" / "vehicle1.bank").string();
    std::string strings_path = (Utils::GetRootDir() / "bin" / "x64" / "plugins" / "flight_control" / "vehicle1.strings.bank").string();
    const char* bank = bank_path.c_str();
    const char* strings = strings_path.c_str();
    ERRCHECK(fmod_system->initialize(1024, FMOD_STUDIO_INIT_LIVEUPDATE, FMOD_INIT_NORMAL, extraDriverData));
    ERRCHECK(fmod_system->loadBankFile(bank, FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank));
    ERRCHECK(fmod_system->loadBankFile(strings, FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank));

    // Get the Looping Engine Noise
    ERRCHECK(fmod_system->getEvent("event:/vehicle1", &ltbfDescription));

    ERRCHECK(ltbfDescription->createInstance(&ltbfInstance));

    spdlog::info("FMOD loaded");

    auto rtti = RED4ext::CRTTISystem::Get();

    {
        auto flightControlCls = rtti->GetClass("FlightControl");

        auto startSound = RED4ext::CClassFunction::Create(flightControlCls, "StartSnd", "StartSnd", &StartSnd);
        auto stopSound = RED4ext::CClassFunction::Create(flightControlCls, "StopSnd", "StopSnd", &StopSnd);
        auto setParams = RED4ext::CClassFunction::Create(flightControlCls, "SetParams", "SetParams", &SetParams);
        setParams->AddParam("Float", "volume");
        setParams->AddParam("Vector4", "playerPositon");
        setParams->AddParam("Vector4", "playerUp");
        setParams->AddParam("Vector4", "playerForward");
        setParams->AddParam("Vector4", "cameraPositon");
        setParams->AddParam("Vector4", "cameraUp");
        setParams->AddParam("Vector4", "cameraForward");
        setParams->AddParam("Float", "speed");
        setParams->AddParam("Float", "surge");
        setParams->AddParam("Float", "yawDiff");

        flightControlCls->RegisterFunction(startSound);
        flightControlCls->RegisterFunction(stopSound);
        flightControlCls->RegisterFunction(setParams);
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Unload()
{
    stringsBank->unload();
    masterBank->unload();
    fmod_system->release();
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo* aInfo)
{
    aInfo->name = L"FlightControl";
    aInfo->author = L"Jack Humbert";
    aInfo->version = RED4EXT_SEMVER(1, 0, 0);
    aInfo->runtime = RED4EXT_RUNTIME_LATEST;
    aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_LATEST;
}
