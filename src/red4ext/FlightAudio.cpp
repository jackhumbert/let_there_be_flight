#include <iostream>

#include "FlightAudio.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/RTTITypes.hpp>

#include "Utils.hpp"
#include "stdafx.hpp"
#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>

#include "stdafx.hpp"

#define ERRCHECK(_result) ERRCHECK_fn(_result, __FILE__, __LINE__)

void ERRCHECK_fn(FMOD_RESULT result, const char* file, int line)
{
    if (result != FMOD_OK)
    {
        spdlog::error("{0}({1}): FMOD error {2} - {3}", file, line, result, FMOD_ErrorString(result));
    }
}

namespace FlightAudio {

    static std::vector<char*> gPathList;
    void* extraDriverData;
    FMOD::Studio::System* fmod_system;
    FMOD::System* coreSystem;
    FMOD::Studio::Bank* soundBank;
    FMOD::Studio::Bank* stringsBank;
    std::unordered_map<std::string, FMOD::Studio::EventInstance*> eventMap;

    struct FlightAudio : RED4ext::IScriptable
    {
        RED4ext::CClass* GetNativeType();
    };

    RED4ext::TTypedClass<FlightAudio> flightAudioCls("FlightAudio");

    RED4ext::CClass* FlightAudio::GetNativeType()
    {
        return &flightAudioCls;
    }

    void Load() {

        ERRCHECK(FMOD::Studio::System::create(&fmod_system));

        // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
        ERRCHECK(fmod_system->getCoreSystem(&coreSystem));
        ERRCHECK(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));

        // Due to a bug in WinSonic on Windows, FMOD initialization may fail on some machines.
        // If you get the error "FMOD error 51 - Error initializing output device", try using
        // a different output type such as FMOD_OUTPUTTYPE_AUTODETECT
        ERRCHECK(coreSystem->setOutput(FMOD_OUTPUTTYPE_AUTODETECT));

        std::string bank_path = (Utils::GetRootDir() / "red4ext" / "plugins" / "flight_control" / "base_sounds.bank").string();
        std::string strings_path = (Utils::GetRootDir() / "red4ext" / "plugins" / "flight_control" / "base_sounds.strings.bank").string();
        const char* bank = bank_path.c_str();
        const char* strings = strings_path.c_str();
        ERRCHECK(fmod_system->initialize(1024, FMOD_STUDIO_INIT_LIVEUPDATE, FMOD_INIT_NORMAL, extraDriverData));
        ERRCHECK(fmod_system->loadBankFile(bank, FMOD_STUDIO_LOAD_BANK_NORMAL, &soundBank));
        ERRCHECK(fmod_system->loadBankFile(strings, FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank));

        spdlog::info("FMOD loaded");
    }

    void Unload() {
        stringsBank->unload();
        soundBank->unload();
        fmod_system->release();
    }


    void Start(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
    {
        RED4ext::CString eventName;
        RED4ext::GetParameter(aFrame, &eventName);
        aFrame->code++; // skip ParamEnd
        spdlog::info(fmt::format("Starting sound: {}", eventName.c_str()));
        FMOD::Studio::EventDescription* eventDescription;
        //std::string path = "event:/";
        //ERRCHECK(fmod_system->getEvent(path.append(eventName.c_str()).c_str(), &eventDescription));
        ERRCHECK(fmod_system->getEvent(fmt::format("event:/{}", eventName.c_str()).c_str(), &eventDescription));
        FMOD::Studio::EventInstance* eventInstance;
        ERRCHECK(eventDescription->createInstance(&eventInstance));
        ERRCHECK(eventInstance->start());
        eventMap[eventName.c_str()] = eventInstance;
        ERRCHECK(fmod_system->update());
    }

    void Stop(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
    {
        int32_t count;
        RED4ext::CString eventName;
        RED4ext::GetParameter(aFrame, &eventName);
        aFrame->code++; // skip ParamEnd
        if (eventMap.find(eventName.c_str()) != eventMap.end()) {
            spdlog::info(fmt::format("Stopping sound: {}", eventName.c_str()));
            ERRCHECK(eventMap[eventName.c_str()]->stop(FMOD_STUDIO_STOP_IMMEDIATE));
            ERRCHECK(fmod_system->update());
            eventMap[eventName.c_str()]->release();
            eventMap.erase(eventName.c_str());
        } else {
            spdlog::warn(fmt::format("Sound is not playing: {}", eventName.c_str()));
        }
    }

    void Update(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
    {
        aFrame->code++; // skip ParamEnd

        auto rtti = RED4ext::CRTTISystem::Get();
        auto flightAudioCls = rtti->GetClass("FlightAudio");

        RED4ext::Vector3 playerPosition = flightAudioCls->GetProperty("playerPosition")->GetValue<RED4ext::Vector3>(aContext);
        RED4ext::Vector3 playerUp = flightAudioCls->GetProperty("playerUp")->GetValue<RED4ext::Vector3>(aContext);
        RED4ext::Vector3 playerForward = flightAudioCls->GetProperty("playerForward")->GetValue<RED4ext::Vector3>(aContext);
        RED4ext::Vector3 cameraPosition = flightAudioCls->GetProperty("cameraPosition")->GetValue<RED4ext::Vector3>(aContext);
        RED4ext::Vector3 cameraUp = flightAudioCls->GetProperty("cameraUp")->GetValue<RED4ext::Vector3>(aContext);
        RED4ext::Vector3 cameraForward = flightAudioCls->GetProperty("cameraForward")->GetValue<RED4ext::Vector3>(aContext);

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

        for (const std::pair<std::string, FMOD::Studio::EventInstance*>& eventInstance : eventMap) {
            ERRCHECK(eventInstance.second->set3DAttributes(&attributes));
            ERRCHECK(eventInstance.second->setVolume(flightAudioCls->GetProperty("volume")->GetValue<float>(aContext)));
            eventInstance.second->setParameterByName("YawDiff", flightAudioCls->GetProperty("yawDiff")->GetValue<float>(aContext));
            eventInstance.second->setParameterByName("PitchDiff", flightAudioCls->GetProperty("pitchDiff")->GetValue<float>(aContext));
            eventInstance.second->setParameterByName("Speed", flightAudioCls->GetProperty("speed")->GetValue<float>(aContext));
            eventInstance.second->setParameterByName("Surge", flightAudioCls->GetProperty("surge")->GetValue<float>(aContext));
            eventInstance.second->setParameterByName("Lift", flightAudioCls->GetProperty("lift")->GetValue<float>(aContext));
            eventInstance.second->setParameterByName("Yaw", flightAudioCls->GetProperty("yaw")->GetValue<float>(aContext));
            eventInstance.second->setParameterByName("Brake", flightAudioCls->GetProperty("brake")->GetValue<float>(aContext));
        }
        ERRCHECK(fmod_system->update());
    }

    void RegisterTypes() {
        flightAudioCls.flags = { .isNative = true };
        RED4ext::CRTTISystem::Get()->RegisterType(&flightAudioCls, 456815);
    }

    void RegisterFunctions() {
        auto rtti = RED4ext::CRTTISystem::Get();
        auto scriptable = rtti->GetClass("IScriptable");
        flightAudioCls.parent = scriptable;

        RED4ext::CBaseFunction::Flags n_flags = { .isNative = true };
;
        auto startSound = RED4ext::CClassFunction::Create(&flightAudioCls, "Start", "Start", &Start);
        auto stopSound = RED4ext::CClassFunction::Create(&flightAudioCls, "Stop", "Stop", &Stop);
        auto setParams = RED4ext::CClassFunction::Create(&flightAudioCls, "Update", "Update", &Update);

        startSound->flags = n_flags;
        stopSound->flags = n_flags;
        setParams->flags = n_flags;

        flightAudioCls.RegisterFunction(startSound);
        flightAudioCls.RegisterFunction(stopSound);
        flightAudioCls.RegisterFunction(setParams);
    }
}
