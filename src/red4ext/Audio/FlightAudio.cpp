#include <iostream>

#include "FlightAudio.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>

#include "Utils/Utils.hpp"
#include "stdafx.hpp"
#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_studio.hpp>
#include "FlightSystem.hpp"

#define ERRCHECK(_result) ERRCHECK_fn(_result, __FILE__, __LINE__)

void ERRCHECK_fn(FMOD_RESULT result, const char *file, int line) {
  if (result != FMOD_OK) {
    spdlog::error("{0}({1}): FMOD error {2} - {3}", file, line, result, FMOD_ErrorString(result));
  }
}

static std::vector<char *> gPathList;
void *extraDriverData;
FMOD::Studio::System *fmod_system;
FMOD::System *coreSystem;
FMOD::Studio::Bank *soundBank;
FMOD::Studio::Bank *stringsBank;
FMOD::Studio::Bus *engine;
FMOD::Studio::Bus *wind;
FMOD::Studio::Bus *warning;
std::unordered_map<std::string, FMOD::Studio::EventInstance *> eventMap;
FMOD_3D_ATTRIBUTES listenerAttributes = {{0}};
FMOD_3D_ATTRIBUTES eventAttributes = {{0}, {0}, {.x = 0, .y = 0, .z = -1}, {.x = 0, .y = 1, .z = 0}};

FlightAudio* FlightAudio::Get() {
  return FlightSystem::FlightSystem::GetInstance()->audio;
}

bool FlightAudio::Load(RED4ext::CGameApplication *aApp) {
  ERRCHECK(FMOD::Studio::System::create(&fmod_system));

  // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
  ERRCHECK(fmod_system->getCoreSystem(&coreSystem));
  ERRCHECK(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));

  // Due to a bug in WinSonic on Windows, FMOD initialization may fail on some machines.
  // If you get the error "FMOD error 51 - Error initializing output device", try using
  // a different output type such as FMOD_OUTPUTTYPE_AUTODETECT
  ERRCHECK(coreSystem->setOutput(FMOD_OUTPUTTYPE_AUTODETECT));

  std::string bank_path =
      (Utils::GetRootDir() / "red4ext" / "plugins" / "let_there_be_flight" / "base_sounds.bank").string();
  std::string strings_path =
      (Utils::GetRootDir() / "red4ext" / "plugins" / "let_there_be_flight" / "base_sounds.strings.bank").string();
  const char *bank = bank_path.c_str();
  const char *strings = strings_path.c_str();
  //ERRCHECK(fmod_system->initialize(1024, FMOD_STUDIO_INIT_LIVEUPDATE,
  //                                 FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_PROFILE_ENABLE, extraDriverData));
  ERRCHECK(fmod_system->initialize(1024, FMOD_STUDIO_INIT_LIVEUPDATE,
                                   FMOD_INIT_3D_RIGHTHANDED, extraDriverData));
  ERRCHECK(fmod_system->loadBankFile(bank, FMOD_STUDIO_LOAD_BANK_NORMAL, &soundBank));
  ERRCHECK(fmod_system->loadBankFile(strings, FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank));

  spdlog::info("FMOD loaded");

  return true;
}

bool FlightAudio::Unload(RED4ext::CGameApplication *aApp) {
  stringsBank->unload();
  soundBank->unload();
  fmod_system->release();

  return true;
}

void FlightAudio::UpdateVolume() {
  if (!engine) {
    fmod_system->getBus("bus:/Engine", &engine);
  }
  if (!wind) {
    fmod_system->getBus("bus:/Wind", &wind);
  }
  if (!warning) {
    fmod_system->getBus("bus:/Warning", &warning);
  }
  float volume;
  auto stack = RED4ext::CStack(this);
  auto result = RED4ext::CStackType(RED4ext::CRTTISystem::Get()->GetType("Float"), &volume);
  stack.result = &result;
  FlightAudio::GetRTTIType()->GetFunction("GetGameVolume")->Execute(&stack);
  auto gameVolume = volume;
  FlightAudio::GetRTTIType()->GetFunction("GetEngineVolume")->Execute(&stack);
  engine->setVolume(volume * gameVolume);
  FlightAudio::GetRTTIType()->GetFunction("GetWindVolume")->Execute(&stack);
  wind->setVolume(volume * gameVolume);
  FlightAudio::GetRTTIType()->GetFunction("GetWarningVolume")->Execute(&stack);
  warning->setVolume(volume * gameVolume);
  ERRCHECK(fmod_system->update());
}

void FlightAudio::UpdateParameter(RED4ext::CString  name, float value) {
  ERRCHECK(fmod_system->setParameterByName(name.c_str(), value));
  ERRCHECK(fmod_system->update());
}

void FlightAudio::Pause() {
  for (auto const &evt : eventMap) {
    ERRCHECK(evt.second->setPaused(true));
    //evt.second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
    ERRCHECK(fmod_system->update());
  }
}

void FlightAudio::Resume() {
  for (auto const &evt : eventMap) {
    ERRCHECK(evt.second->setPaused(false));
    // evt.second->start();
    ERRCHECK(fmod_system->update());
  }
}

void FlightAudio::Start(RED4ext::CString emitterName, RED4ext::CString eventName) {
  //if (emitterName && eventName) {
    spdlog::info(fmt::format("[FlightAudio] Starting sound: {}", emitterName.c_str()));
    FMOD::Studio::EventDescription *eventDescription;
    // std::string path = "event:/";
    // ERRCHECK(fmod_system->getEvent(path.append(eventName.c_str()).c_str(), &eventDescription));
    ERRCHECK(fmod_system->getEvent(fmt::format("event:/{}", eventName.c_str()).c_str(), &eventDescription));
    FMOD::Studio::EventInstance *eventInstance;
    ERRCHECK(eventDescription->createInstance(&eventInstance));
    ERRCHECK(eventInstance->start());
    // ERRCHECK(eventInstance->setTimelinePosition(rand()));
    // float pitch = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    // ERRCHECK(eventInstance->setPitch(1.0 + pitch * 0.01));
    eventMap[emitterName.c_str()] = eventInstance;
    ERRCHECK(fmod_system->update());
  //}
}

void FlightAudio::StartWithPitch(RED4ext::CString emitterName, RED4ext::CString eventName, float pitch) {
  //if (emitterName && eventName) {
    spdlog::info(fmt::format("[FlightAudio] Starting sound: {} with pitch: {}", emitterName.c_str(), pitch));
    FMOD::Studio::EventDescription *eventDescription;
    // std::string path = "event:/";
    // ERRCHECK(fmod_system->getEvent(path.append(eventName.c_str()).c_str(), &eventDescription));
    ERRCHECK(fmod_system->getEvent(fmt::format("event:/{}", eventName.c_str()).c_str(), &eventDescription));
    FMOD::Studio::EventInstance *eventInstance;
    ERRCHECK(eventDescription->createInstance(&eventInstance));
    ERRCHECK(eventInstance->start());
    ERRCHECK(eventInstance->setTimelinePosition(rand()));
    ERRCHECK(eventInstance->setPitch(pitch));
    eventMap[emitterName.c_str()] = eventInstance;
    ERRCHECK(fmod_system->update());
  //}
}

void FlightAudio::Play(RED4ext::CString eventName) {
  //if (eventName) {
    spdlog::info(fmt::format("[FlightAudio] Playing event: {}", eventName.c_str()));
    FMOD::Studio::EventDescription *eventDescription;
    ERRCHECK(fmod_system->getEvent(fmt::format("event:/{}", eventName.c_str()).c_str(), &eventDescription));
    FMOD::Studio::EventInstance *eventInstance;
    ERRCHECK(eventDescription->createInstance(&eventInstance));
    ERRCHECK(eventInstance->start());
    ERRCHECK(eventInstance->release());
    ERRCHECK(fmod_system->update());
  //}
}

void FlightAudio::Stop(RED4ext::CString emitterName) {
  //if (emitterName) {
    if (eventMap.find(emitterName.c_str()) != eventMap.end()) {
      spdlog::info(fmt::format("[FlightAudio] Stopping sound: {}", emitterName.c_str()));
      ERRCHECK(eventMap[emitterName.c_str()]->stop(FMOD_STUDIO_STOP_IMMEDIATE));
      ERRCHECK(fmod_system->update());
      eventMap[emitterName.c_str()]->release();
      eventMap.erase(emitterName.c_str());
    } else {
      spdlog::warn(fmt::format("[FlightAudio] Sound is not playing: {}", emitterName.c_str()));
    }
  //}
}

void FlightAudio::UpdateListenerMatrix(RED4ext::Matrix matrix) {
  listenerAttributes.position = {.x = matrix.W.X, .y = matrix.W.Z, .z = -matrix.W.Y};
  listenerAttributes.up = {.x = matrix.Z.X, .y = matrix.Z.Z, .z = -matrix.Z.Y};
  listenerAttributes.forward = {.x = matrix.Y.X, .y = matrix.Y.Z, .z = -matrix.Y.Y};
  ERRCHECK(fmod_system->setListenerAttributes(0, &listenerAttributes));
  ERRCHECK(fmod_system->update());
}

void FlightAudio::UpdateEventMatrix(RED4ext::CString emitterName, RED4ext::Matrix matrix) {
  eventAttributes.position = {.x = matrix.W.X, .y = matrix.W.Z, .z = -matrix.W.Y};
  eventAttributes.up = {.x = matrix.Z.X, .y = matrix.Z.Z, .z = -matrix.Z.Y};
  eventAttributes.forward = {.x = matrix.Y.X, .y = matrix.Y.Z, .z = -matrix.Y.Y};

  ERRCHECK(eventMap[emitterName.c_str()]->set3DAttributes(&eventAttributes));
  ERRCHECK(fmod_system->update());
}

void FlightAudio::UpdateEvent(RED4ext::CString emitterName, RED4ext::Matrix matrix, float eventVolume, RED4ext::Handle<RED4ext::IScriptable> update) {
  if (eventMap.contains(emitterName.c_str())) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto flightAudioUpdateCls = rtti->GetClass("FlightAudioUpdate");

    eventAttributes.position = {.x = matrix.W.X, .y = matrix.W.Z, .z = -matrix.W.Y};
    eventAttributes.up = {.x = matrix.Z.X, .y = matrix.Z.Z, .z = -matrix.Z.Y};
    eventAttributes.forward = {.x = matrix.Y.X, .y = matrix.Y.Z, .z = -matrix.Y.Y};

    ERRCHECK(eventMap[emitterName.c_str()]->set3DAttributes(&eventAttributes));
    ERRCHECK(eventMap[emitterName.c_str()]->setVolume(eventVolume));

    for (int i = 0; i < flightAudioUpdateCls->allProps.size; i++) {
      auto pParameterName = flightAudioUpdateCls->allProps[i]->name.ToString();
      eventMap[emitterName.c_str()]->setParameterByName(
          pParameterName, flightAudioUpdateCls->GetProperty(pParameterName)->GetValue<float>(update));
    }
    ERRCHECK(fmod_system->update());
  }
}
