#include <chrono>
#include <iostream>
#include <mutex>

#include "FlightAudio.hpp"
#include "Acoustics/Acoustics.hpp"
#include "Acoustics/Stems.hpp"
#include "Acoustics/Probe.hpp"

#include <algorithm>
#include <array>
#include <ctime>
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
  ERRCHECK(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_STEREO, 0));

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

  // the flying sound's echoes off the city: on the master bus for what mixes through the buses, and
  // in each thruster event before its Object Spatializer (Start), which hands the rest to the
  // output past every bus
  Acoustics::Attach(fmod_system, {"bus:/"});

  return true;
}

bool FlightAudio::Unload(RED4ext::CGameApplication *aApp) {
  Acoustics::Detach();
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
    Acoustics::AttachToEvent(fmod_system, eventInstance);
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
    Acoustics::AttachToEvent(fmod_system, eventInstance);
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
      Acoustics::DetachFromEvent(eventMap[emitterName.c_str()]);
      ERRCHECK(eventMap[emitterName.c_str()]->stop(FMOD_STUDIO_STOP_IMMEDIATE));
      ERRCHECK(fmod_system->update());
      eventMap[emitterName.c_str()]->release();
      eventMap.erase(emitterName.c_str());
    } else {
      spdlog::warn(fmt::format("[FlightAudio] Sound is not playing: {}", emitterName.c_str()));
    }
  //}
}

static std::mutex cameraListenerLock;
static RED4ext::Matrix cameraListener;
static std::chrono::steady_clock::time_point cameraListenerTime;

void FlightAudio::SetCameraListener(RED4ext::Vector4 aPosition, RED4ext::Vector4 aForward, RED4ext::Vector4 aUp) {
  std::lock_guard guard(cameraListenerLock);
  auto &m = cameraListener;
  m.Y = {aForward.X, aForward.Y, aForward.Z, 0.0f};
  m.Z = {aUp.X, aUp.Y, aUp.Z, 0.0f};
  m.X = {aForward.Y * aUp.Z - aForward.Z * aUp.Y, aForward.Z * aUp.X - aForward.X * aUp.Z,
         aForward.X * aUp.Y - aForward.Y * aUp.X, 0.0f};
  m.W = {aPosition.X, aPosition.Y, aPosition.Z, 1.0f};
  cameraListenerTime = std::chrono::steady_clock::now();
}

bool FlightAudio::GetCameraListener(RED4ext::Matrix &aMatrix) {
  std::lock_guard guard(cameraListenerLock);
  if (std::chrono::steady_clock::now() - cameraListenerTime > std::chrono::milliseconds(200)) {
    return false;
  }
  aMatrix = cameraListener;
  return true;
}

void FlightAudio::UpdateListenerMatrix(RED4ext::Matrix matrix) {
  listenerAttributes.position = {.x = matrix.W.X, .y = matrix.W.Z, .z = -matrix.W.Y};
  listenerAttributes.up = {.x = matrix.Z.X, .y = matrix.Z.Z, .z = -matrix.Z.Y};
  listenerAttributes.forward = {.x = matrix.Y.X, .y = matrix.Y.Z, .z = -matrix.Y.Y};
  ERRCHECK(fmod_system->setListenerAttributes(0, &listenerAttributes));
  ERRCHECK(fmod_system->update());
  Acoustics::SetListener({matrix.W.X, matrix.W.Y, matrix.W.Z});
}

void FlightAudio::UpdateAcoustics(RED4ext::Matrix carMatrix, float gain, float inside, int32_t raySource) {
  Acoustics::SetCar({carMatrix.W.X, carMatrix.W.Y, carMatrix.W.Z}, {carMatrix.Y.X, carMatrix.Y.Y, carMatrix.Y.Z},
                    {carMatrix.Z.X, carMatrix.Z.Y, carMatrix.Z.Z}, gain, inside,
                    static_cast<Acoustics::RaySource>(raySource));
}

RED4ext::DynArray<RED4ext::Vector4> FlightAudio::AcousticsRays(RED4ext::Matrix carMatrix) {
  RED4ext::DynArray<RED4ext::Vector4> out;
  for (const auto &d : Acoustics::Probe::NearRays({carMatrix.Y.X, carMatrix.Y.Y, carMatrix.Y.Z},
                                                  {carMatrix.Z.X, carMatrix.Z.Y, carMatrix.Z.Z})) {
    out.EmplaceBack(d.x, d.y, d.z, 0.0f);
  }
  return out;
}

void FlightAudio::AcousticsHits(RED4ext::Vector4 origin, RED4ext::DynArray<float> distances,
                                RED4ext::DynArray<RED4ext::CName> materials) {
  if (distances.size != Acoustics::kNearRays || materials.size != Acoustics::kNearRays) {
    return;
  }
  std::array<float, Acoustics::kNearRays> distance;
  std::array<std::string, Acoustics::kNearRays> material;
  for (uint32_t k = 0; k < Acoustics::kNearRays; ++k) {
    distance[k] = distances[k];
    material[k] = distances[k] >= 0 ? materials[k].ToString() : "";
  }
  Acoustics::SetLiveHits({origin.X, origin.Y, origin.Z}, distance, material, Acoustics::RaySource::Scripts);
}

namespace {
// red4ext/logs/<aPrefix><local time>
std::string LogPath(const char *aPrefix) {
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm local{};
  localtime_s(&local, &now);
  char stamp[32];
  std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &local);
  return (Utils::GetRootDir() / "red4ext" / "logs" / (std::string(aPrefix) + stamp)).string();
}
} // namespace

void FlightAudio::AcousticsLog(bool on) {
  Acoustics::SetLog(on ? LogPath("ltbf_acoustics_") + ".csv" : "");
}

void FlightAudio::AcousticsTestSource(int32_t source) {
  Acoustics::SetTestSource(static_cast<Acoustics::TestSource>(std::clamp(source, 0, 3)));
}

RED4ext::CString FlightAudio::AcousticsStems(bool on) {
  if (!on) {
    Acoustics::Stems::Stop();
    return "";
  }
  const auto base = LogPath("ltbf_stems_");
  return Acoustics::Stems::Start(base) ? base.c_str() : "";
}

RED4ext::CString FlightAudio::AcousticsState() {
  // and what FMOD is playing: channels (real ones mixed), and each event's state, volume as set
  // and as heard, and audibility
  int channels = 0, real = 0;
  coreSystem->getChannelsPlaying(&channels, &real);
  std::string events;
  for (auto const &[name, instance] : eventMap) {
    FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
    float volume = 0, finalVolume = 0, audibility = -1;
    bool paused = false, virtualized = false;
    instance->getPlaybackState(&state);
    instance->getVolume(&volume, &finalVolume);
    instance->getPaused(&paused);
    instance->isVirtual(&virtualized);
    FMOD::ChannelGroup *group = nullptr;
    if (instance->getChannelGroup(&group) == FMOD_OK && group) {
      group->getAudibility(&audibility);
    }
    events += fmt::format("{}\"{}\":[{},{:.3f},{:.3f},{:.4f},{},{}]", events.empty() ? "" : ",", name, int(state), volume,
                          finalVolume, audibility, paused, virtualized);
  }
  auto state = Acoustics::State();
  state.pop_back();
  state += fmt::format(",\"fmodChannels\":{},\"fmodReal\":{},\"events\":{{{}}}}}", channels, real, events);
  return {state.c_str()};
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
