#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <string_view>
#include <unordered_map>

#include "FlightAudio.hpp"
#include "Acoustics/Acoustics.hpp"
#include "Acoustics/Engine.hpp"
#include "Acoustics/Stems.hpp"
#include "Buses.hpp"
#include "EngineSynth/Channels.hpp"
#include "EngineSynth/ScrapeChannels.hpp"
#include "Inputs/Gather.hpp"
#include "Inputs/Inputs.hpp"
#include "Out/Backend.hpp"
#include "Out/Out.hpp"
#include "Samples/Samples.hpp"
#include "Voices/Voices.hpp"
#include "Wind/Wind.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <ctime>
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>

#include "Flight/DeveloperSettings.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"
#include "FlightSystem.hpp"

// the water low-pass's input (SetWater), for the engine synth
static float water = 0.0f;

FlightAudio* FlightAudio::Get() {
  return FlightSystem::FlightSystem::GetInstance()->audio;
}

bool FlightAudio::Load(RED4ext::CGameApplication *aApp) {
  // the backend for the session (docs/design/audio.md, Backends); Wwise falls back to FMOD when it cannot install
  if (!Out::Init(Flight::Developer().wwiseAudio ? Out::Backend::Wwise : Out::Backend::Fmod)) {
    return false;
  }
  Samples::Load(Utils::GetRootDir() / "red4ext" / "plugins" / "let_there_be_flight" / "audio");
  Wind::Init();
  if (Out::Current() == Out::Backend::Fmod) {
    // on the master group: the stems' bus track, and the test source with its echoes (the rest has its echoes from
    // the car's Engine already); the master's limiter goes after it on the first frame
    Acoustics::Attach(Out::FmodSystem(), Out::FmodMaster());
  } else {
    Acoustics::AttachVoice();
  }
  return true;
}

bool FlightAudio::Unload(RED4ext::CGameApplication *aApp) {
  // every voice before the backend
  EngineSynth::Channels::StopAll();
  EngineSynth::ScrapeChannels::StopAll();
  Samples::Unload();
  Wind::Shutdown();
  Acoustics::Detach();
  Out::Shutdown();
  return true;
}

// The game's own volume state (UpdateVolume): the time dilations of the hub menu, the world map and the radial
// menu, and the master and sfx sliders (percent). Read through the game's natives at most 4 times a second.
static bool dilationHub = false, dilationMap = false, dilationRadial = false;
static int volumeMaster = 100, volumeSfx = 100;

static bool TimeDilation(const char *aName) {
  bool active = false;
  Gather::Call("gameTimeSystem", "IsTimeDilationActive", &active, RED4ext::CName(aName));
  return active;
}

// /audio/volume's integer setting, left as it was if the game does not answer
static void ReadVolumeSetting(const char *aName, int &aValue) {
  RED4ext::Handle<RED4ext::IScriptable> var;
  if (!Gather::Call("userSettingsUserSettings", "GetVar", &var, RED4ext::CName("/audio/volume"), RED4ext::CName(aName)) ||
      !var) {
    return;
  }
  auto getValue = var->GetType()->GetFunction("GetValue");
  int32_t value = 0;
  if (getValue && RED4ext::ExecuteFunction(var.GetPtr(), getValue, &value)) {
    aValue = value;
  }
}

void FlightAudio::UpdateVolume() {
  using Clock = std::chrono::steady_clock;
  static Clock::time_point lastRefresh{};
  const auto now = Clock::now();
  if (now - lastRefresh >= std::chrono::milliseconds(250)) {
    lastRefresh = now;
    dilationHub = TimeDilation("HubMenu");
    dilationMap = TimeDilation("WorldMap");
    dilationRadial = TimeDilation("radialMenu");
    ReadVolumeSetting("MasterVolume", volumeMaster);
    ReadVolumeSetting("SfxVolume", volumeSfx);
  }
  // Not TimeSystem.IsPausedState() for the game volume: that flag is the in-game clock pause that
  // quests and races set (Chipping In, act 2 transitions, race starts) and sometimes never clear,
  // which left flight audio silent for the rest of the session. The real pause menu is covered by
  // isInMenu and by FlightSystem::OnGamePaused/OnGameResumed on the native side.
  auto cls = FlightAudio::GetRTTIType();
  auto setting = [&](const char *aName) { return *cls->GetProperty(aName)->GetValuePtr<float>(this); };
  const bool inMenu = *cls->GetProperty("isInMenu")->GetValuePtr<bool>(this);
  // under Wwise the game's master and SFX volumes act on LTBF's voices already (they are under its SFX bus)
  const bool included = Out::GameVolumeIncluded();
  const float gameVolume = Inputs::GameVolume(inMenu, dilationHub, dilationMap, dilationRadial,
                                              included ? 100 : volumeMaster, included ? 100 : volumeSfx);
  const Buses::Volumes volumes{gameVolume, setting("engineVolume"), setting("windVolume"), setting("warningVolume")};
  for (auto bus : {Buses::Bus::Engine, Buses::Bus::Wind, Buses::Bus::Warning, Buses::Bus::Scrape}) {
    Out::SetBusVolume(bus, Buses::BusVolume(bus, volumes));
  }
  // the backend's frame: FMOD's one update, Wwise's voices started and stopped
  Out::Update();
}

void FlightAudio::SetWater(float value) {
  water = value;
  spdlog::info("[FlightAudio] the player's car {} the water", value > 0.5f ? "is in" : "is out of");
}

void FlightAudio::Pause() {
  Out::SetPaused(true);
  Voices::Pause(true);
}

void FlightAudio::Resume() {
  Out::SetPaused(false);
  Voices::Pause(false);
}

// the listener in the game's world (UpdateListenerMatrix), for the synth channels' spatializer
static EngineSynth::Channels::Frame listenerFrame;
// its velocity, smoothed (UpdateListenerMatrix)
static Voices::Vec3 listenerVelocity;
// the car's front windows in its own axes (SetWindowSlots)
static Voices::Vec3 windowLeft, windowRight;

// FMOD's Event Cone Angle: degrees between the event's forward and the way to the listener (0 facing
// it, 180 facing away), in the game's world
static float ConeAngle(const Voices::Vec3 &aPosition, const Voices::Vec3 &aForward, const Voices::Vec3 &aListener) {
  const float x = aListener.x - aPosition.x, y = aListener.y - aPosition.y, z = aListener.z - aPosition.z;
  const float length = std::sqrt(x * x + y * y + z * z);
  if (length < 1e-4f) {
    return 0.0f;
  }
  const auto &f = aForward;
  const float c = (x * f.x + y * f.y + z * f.z) / length;
  return std::acos(std::clamp(c, -1.0f, 1.0f)) * 57.29578f;
}

// each thruster's start pitch (StartEngine), for the stems' parameter log
static std::unordered_map<std::string, float> startPitch;

void FlightAudio::PlayOneShot(RED4ext::CString name, RED4ext::Handle<RED4ext::vehicle::BaseObject> vehicle) {
  Samples::PlayOneShot(name.c_str(), vehicle);
}

void FlightAudio::StartLoop(RED4ext::CString id, RED4ext::CString name, RED4ext::Handle<RED4ext::vehicle::BaseObject> vehicle) {
  Samples::StartLoop(id.c_str(), name.c_str(), vehicle);
}

void FlightAudio::StopLoop(RED4ext::CString id) {
  Samples::StopLoop(id.c_str());
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

static std::atomic<bool> cinematic{false};

void FlightAudio::SetCinematic(bool on) {
  cinematic.store(on);
}

bool FlightAudio::Cinematic() {
  return cinematic.load();
}

void FlightAudio::UpdateListenerMatrix(RED4ext::Matrix matrix) {
  listenerFrame = {{matrix.W.X, matrix.W.Y, matrix.W.Z},
                   {matrix.X.X, matrix.X.Y, matrix.X.Z},
                   {matrix.Y.X, matrix.Y.Y, matrix.Y.Z},
                   {matrix.Z.X, matrix.Z.Y, matrix.Z.Z}};
  static std::chrono::steady_clock::time_point lastFrame;
  const auto now = std::chrono::steady_clock::now();
  const float dt = lastFrame == std::chrono::steady_clock::time_point{}
                       ? 0.0f
                       : std::clamp(std::chrono::duration<float>(now - lastFrame).count(), 0.0f, 0.1f);
  lastFrame = now;
  // the listener's velocity, for the wind and the Doppler shift (Inputs::ListenerVelocity: a jump keeps it)
  static Voices::Vec3 lastListener;
  static bool haveListener = false;
  if (haveListener) {
    const Voices::Vec3 &p = listenerFrame.position;
    const Inputs::Vec3 v = Inputs::ListenerVelocity({listenerVelocity.x, listenerVelocity.y, listenerVelocity.z},
                                                    {lastListener.x, lastListener.y, lastListener.z}, {p.x, p.y, p.z}, dt);
    listenerVelocity = {v.x, v.y, v.z};
  }
  lastListener = listenerFrame.position;
  haveListener = true;
  listenerFrame.velocity = listenerVelocity;
  Out::SetListener(listenerFrame);
  Samples::Frame(dt);
  Wind::Tick();
  // the echo voices go to their image sources before the pool ranks them
  Acoustics::Engine::Frame(listenerFrame.position);
  Voices::Frame(listenerFrame, dt);
  Acoustics::SetListener({matrix.W.X, matrix.W.Y, matrix.W.Z});
}

void FlightAudio::UpdateAcoustics(RED4ext::Matrix carMatrix, float gain, float inside, int32_t raySource) {
  Acoustics::SetCar({carMatrix.W.X, carMatrix.W.Y, carMatrix.W.Z}, {carMatrix.Y.X, carMatrix.Y.Y, carMatrix.Y.Z},
                    {carMatrix.Z.X, carMatrix.Z.Y, carMatrix.Z.Z}, gain, inside,
                    raySource == static_cast<int32_t>(Acoustics::RaySource::PhysX) ? Acoustics::RaySource::PhysX
                                                                                    : Acoustics::RaySource::None);
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
  // and what the backend is playing (FMOD's channels; Wwise's voices and relay)
  auto state = Acoustics::State();
  state.pop_back();
  std::string out = Out::State();
  state += ",\"out\":" + out;
  if (out.find("\"fmodChannels\"") != std::string::npos) {
    // the FMOD figures where tests read them before
    out.erase(0, out.find("\"fmodChannels\""));
    out.pop_back();
    state += "," + out;
  }
  return {(state + "}").c_str()};
}

void FlightAudio::UpdateThruster(const std::string &aId, const Inputs::Values &aValues, float aVolume,
                                 const Inputs::Vec3 &aPosition, const Inputs::Axes &aCone) {
  const float cone = ConeAngle({aPosition.x, aPosition.y, aPosition.z}, {aCone.forward.x, aCone.forward.y, aCone.forward.z},
                               listenerFrame.position);
  if (!EngineSynth::Channels::Playing(aId)) {
    return;
  }
  const EngineSynth::Channels::Frame thruster{{aPosition.x, aPosition.y, aPosition.z},
                                              {aCone.right.x, aCone.right.y, aCone.right.z},
                                              {aCone.forward.x, aCone.forward.y, aCone.forward.z},
                                              {aCone.up.x, aCone.up.y, aCone.up.z}};
  EngineSynth::Channels::Update(aId, thruster, Inputs::ToSynth(aValues, aVolume, water, cone));
  if (Acoustics::Stems::Recording()) {
    std::vector<std::pair<std::string, float>> values;
    for (int i = 0; i < Inputs::kValueCount; ++i) {
      values.emplace_back(Inputs::kNames[i], Inputs::Get(aValues, i));
    }
    values.emplace_back("cone", cone);
    const float position[3] = {aPosition.x, aPosition.y, aPosition.z};
    auto pitch = startPitch.find(aId);
    Acoustics::Stems::WriteParams(aId, pitch != startPitch.end() ? pitch->second : 1.0f, aVolume, position, values);
  }
}

void FlightAudio::StartEngine(RED4ext::CString emitterName, float pitch, uint64_t car, bool player) {
  spdlog::info("[FlightAudio] Starting the engine synth: {} with pitch: {}", emitterName.c_str(), pitch);
  startPitch[emitterName.c_str()] = pitch;
  EngineSynth::Channels::Start(emitterName.c_str(), pitch, car, player);
  // the player's thrusters scrape in voices of their own
  if (player)
    EngineSynth::ScrapeChannels::Start(emitterName.c_str());
}

void FlightAudio::StopEngine(RED4ext::CString emitterName) {
  EngineSynth::Channels::Stop(emitterName.c_str());
  EngineSynth::ScrapeChannels::Stop(emitterName.c_str());
}

RED4ext::CString FlightAudio::EngineState() {
  return EngineSynth::Channels::State().c_str();
}

RED4ext::CString FlightAudio::EngineCharacter(RED4ext::CString text) {
  EngineSynth::Character c = EngineSynth::Channels::GetCharacter();
  if (!EngineSynth::ParseCharacter(text.c_str(), c)) {
    return fmt::format("error: \"{}\" not understood: fmod, clean, or cone, distortion, darkness, grit =0 or =1 "
                       "with commas between",
                       text.c_str())
        .c_str();
  }
  EngineSynth::Channels::SetCharacter(c);
  spdlog::info("[FlightAudio] engine character {}", text.c_str());
  return AudioSummary();
}

RED4ext::CString FlightAudio::EngineSound(RED4ext::CString name) {
  EngineSynth::Sound sound;
  if (!EngineSynth::ParseSound(name.c_str(), sound)) {
    return fmt::format("error: \"{}\" not understood: drone, bullroarer or fan", name.c_str()).c_str();
  }
  EngineSynth::Channels::SetSound(sound);
  spdlog::info("[FlightAudio] engine sound {}", name.c_str());
  return AudioSummary();
}

RED4ext::CString FlightAudio::VoicesBudget(int32_t maxObjects, int32_t maxSynth) {
  if (maxObjects < 1 || maxObjects > 64 || maxSynth < 1 || maxSynth > 64) {
    return fmt::format("error: objects {} and synth voices {} must each be 1 to 64", maxObjects, maxSynth).c_str();
  }
  Voices::SetBudget(maxObjects, maxSynth);
  spdlog::info("[FlightAudio] voice budget {} objects, {} synth voices", maxObjects, maxSynth);
  return AudioSummary();
}

RED4ext::CString FlightAudio::VoicesState() {
  return Voices::State().c_str();
}

void FlightAudio::UpdateWind(RED4ext::Matrix carMatrix, RED4ext::Vector4 velocity, int32_t view) {
  Wind::Inputs in;
  in.carPosition = {carMatrix.W.X, carMatrix.W.Y, carMatrix.W.Z};
  in.carRight = {carMatrix.X.X, carMatrix.X.Y, carMatrix.X.Z};
  in.carForward = {carMatrix.Y.X, carMatrix.Y.Y, carMatrix.Y.Z};
  in.carUp = {carMatrix.Z.X, carMatrix.Z.Y, carMatrix.Z.Z};
  in.carVelocity = {velocity.X, velocity.Y, velocity.Z};
  in.view = static_cast<Wind::View>(std::clamp(view, 0, 2));
  in.windowLeft = windowLeft;
  in.windowRight = windowRight;
  in.listenerPosition = listenerFrame.position;
  in.listenerVelocity = listenerVelocity;
  Acoustics::Features f;
  if (Acoustics::LastFeatures(f)) {
    in.haveProbe = true;
    in.openSky = 1.0f - f.enclosure;
    in.floorDistance = f.floor;
    in.wallDistance = std::min(f.left, f.right);
  }
  Wind::Update(in);
}

void FlightAudio::SetWindowSlots(RED4ext::Handle<RED4ext::vehicle::BaseObject> vehicle, RED4ext::Vector4 left,
                                 RED4ext::Vector4 right) {
  if (!vehicle) {
    return;
  }
  const auto &p = vehicle->worldTransform.Position;
  const float px = p.x.Bits * (1.0f / 131072.0f), py = p.y.Bits * (1.0f / 131072.0f), pz = p.z.Bits * (1.0f / 131072.0f);
  const auto &q = vehicle->worldTransform.Orientation;
  // the world offset turned by the orientation's inverse (its conjugate): v + 2r(u x v) + 2u x (u x v), u = -(i, j, k)
  auto local = [&](const RED4ext::Vector4 &w) {
    const float vx = w.X - px, vy = w.Y - py, vz = w.Z - pz;
    const float ux = -q.i, uy = -q.j, uz = -q.k;
    const float tx = 2.0f * (uy * vz - uz * vy), ty = 2.0f * (uz * vx - ux * vz), tz = 2.0f * (ux * vy - uy * vx);
    return Voices::Vec3{vx + q.r * tx + (uy * tz - uz * ty), vy + q.r * ty + (uz * tx - ux * tz),
                        vz + q.r * tz + (ux * ty - uy * tx)};
  };
  windowLeft = local(left);
  windowRight = local(right);
}

void FlightAudio::WindOn(bool on) {
  Wind::on.store(on);
  spdlog::info("[FlightAudio] wind {}", on ? "on" : "off");
}

RED4ext::CString FlightAudio::WindState() {
  return Wind::State().c_str();
}

void FlightAudio::AcousticsAir(bool on) {
  Acoustics::SetAirAbsorption(on);
}

void FlightAudio::AcousticsPlanes(bool on) {
  Acoustics::SetPlanes(on);
}

void FlightAudio::AcousticsOcclusion(bool on) {
  Acoustics::SetOcclusion(on);
}

RED4ext::CString FlightAudio::AudioSummary() {
  const std::string synth = EngineSynth::Channels::Summary();
  const std::string thrusters = synth.rfind("synth on 0 ", 0) == 0
                                    ? "thrusters: silent, not flying (" + synth.substr(synth.find("character")) + ")"
                                    : "thrusters: " + synth;
  return (thrusters + "; " + Acoustics::Summary() + "; " + Voices::Summary() + "; " + Wind::Summary() + "; " +
          Out::Summary())
      .c_str();
}
