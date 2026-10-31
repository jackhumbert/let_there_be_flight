#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include "Engine/RTTIClass.hpp"
#include "Buses.hpp"
#include "FmodHelper.hpp"
#include "Inputs/Inputs.hpp"
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>

#include <string>

namespace FMOD {
class ChannelGroup;
} // namespace FMOD

class FlightAudio : public Engine::RTTIClass<FlightAudio, RED4ext::IScriptable> {
public:
  static bool Load(RED4ext::CGameApplication *aApp);
  static bool Unload(RED4ext::CGameApplication *aApp);

  // each frame, from FlightSystem's PrePhysics: the buses' volumes (the mod's settings against the game's, which
  // it reads 4 times a second) and the one coreSystem->update() of the frame
  void UpdateVolume();
  void Pause();
  void Resume();

  static FlightAudio* Get();
  // a bus's channel group in the plugin's own FMOD Core system (docs/design/audio-native.md B.5)
  static FMOD::ChannelGroup *Group(Buses::Bus aBus);

  // A listener Inputs::Gather sets each frame in place of the player's soundListener component: the
  // cinematic camera's (its shots are around the car, the component stays with V, FlightComponent).
  void SetCameraListener(RED4ext::Vector4 aPosition, RED4ext::Vector4 aForward, RED4ext::Vector4 aUp);
  // false when it has not been set lately (then the component is used)
  static bool GetCameraListener(RED4ext::Matrix &aMatrix);

  // the listener, each frame from FlightSystem's PrePhysics
  void UpdateListenerMatrix(RED4ext::Matrix matrix);
  // the water low-pass's input of the engine synth: 1 under water, 0 in the air
  void SetWater(float value);
  // LTBF's own samples on FMOD Core channels at the car (Samples, docs/design/audio-native.md A.3): a
  // sound by its event's name that plays once, or loops under an id until StopLoop
  void PlayOneShot(RED4ext::CString name, RED4ext::Handle<RED4ext::vehicle::BaseObject> vehicle);
  void StartLoop(RED4ext::CString id, RED4ext::CString name, RED4ext::Handle<RED4ext::vehicle::BaseObject> vehicle);
  void StopLoop(RED4ext::CString id);
  // a thruster's engine synth (EngineSynth::Channels): started with the car's pitch, the car's entity
  // hash (an NPC car's thrusters fold into one voice) and whether it is the player's
  void StartEngine(RED4ext::CString emitterName, float pitch, uint64_t car, bool player);
  void StopEngine(RED4ext::CString emitterName);
  // Each frame for each attached thruster (Inputs::Gather): the synth channel of that id gets the thruster's
  // place and cone axes, volume and the inputs
  static void UpdateThruster(const std::string &aId, const Inputs::Values &aValues, float aVolume,
                             const Inputs::Vec3 &aPosition, const Inputs::Axes &aCone);
  // the Autopilot's cinematic camera is on (FlightModeAutopilot.OnCinematicCameraChanged): the player's car
  // is heard from outside, and from the camera
  void SetCinematic(bool on);
  static bool Cinematic();
  // the synth channels and their cost as JSON, for tests
  RED4ext::CString EngineState();
  // which of the old FMOD event's effects the synth keeps (EngineSynth::ParseCharacter: "fmod", "clean" or
  // "cone=0,distortion=1,darkness=0,grit=1"); returns the state after, or why not
  RED4ext::CString EngineCharacter(RED4ext::CString text);
  RED4ext::CString EngineSound(RED4ext::CString name);
  // the voice budget (Voices::SetBudget): Windows Sonic objects and thruster synth voices, each 1 to
  // 64; returns the audio summary, or why not ("error: ...")
  RED4ext::CString VoicesBudget(int32_t maxObjects, int32_t maxSynth);
  // the voice pool as JSON (Voices::State), for tests
  RED4ext::CString VoicesState();
  // the wind as the listener hears it (Wind, docs/design/audio-native.md A.4): each frame the player's car
  // flies (Inputs::Gather), its matrix, world velocity, the angles between its nose and its way (degrees) and
  // the view (0 TPP, 1 FPP, 2 the cinematic camera)
  void UpdateWind(RED4ext::Matrix carMatrix, RED4ext::Vector4 velocity, int32_t view);
  // the car's front windows (world positions), kept in the car's own axes for the first person wind
  void SetWindowSlots(RED4ext::Handle<RED4ext::vehicle::BaseObject> vehicle, RED4ext::Vector4 left,
                      RED4ext::Vector4 right);
  void WindOn(bool on);
  // the wind's state as JSON (Wind::State), for tests
  RED4ext::CString WindState();
  // air absorption on the echoes (Acoustics::SetAirAbsorption)
  void AcousticsAir(bool on);
  // echoes off planes fitted to the live hits (on) or off square-on surfaces (off)
  void AcousticsPlanes(bool on);
  // NPC cars heard through buildings: muffled and quieter when static geometry is between (on by default)
  void AcousticsOcclusion(bool on);
  // one line for people: the thrusters' synth (or silence), its character, and the echoes
  // (Acoustics::Summary), as they are now
  RED4ext::CString AudioSummary();
  // each frame the player's car flies (Inputs::Gather): its matrix, the echoes' level (0: off), how far the
  // view is inside the cabin and who casts the near rays (Acoustics::RaySource: 0 none, 2 PhysX)
  void UpdateAcoustics(RED4ext::Matrix carMatrix, float gain, float inside, int32_t raySource);
  // the echoes' state as JSON (Acoustics::State), for tests
  RED4ext::CString AcousticsState();
  // the nav map's and the live distances side by side, a row a probe, to
  // red4ext/logs/ltbf_acoustics_<time>.csv (on) or no more (off)
  void AcousticsLog(bool on);
  // LTBF's sound as stems (Acoustics::Stems) to red4ext/logs/ltbf_stems_<time>_*.wav: the base
  // path, "" when stopped or the files could not be opened
  RED4ext::CString AcousticsStems(bool on);
  // a test sound in place of LTBF's sound (Acoustics::TestSource: 0 off, 1 clicks, 2 pink noise, 3 a 440 Hz sine)
  void AcousticsTestSource(int32_t source);

private:
  friend Descriptor;

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&FlightAudio::SetWater>("SetWater");
    aType->AddFunction<&FlightAudio::PlayOneShot>("PlayOneShot");
    aType->AddFunction<&FlightAudio::StartLoop>("StartLoop");
    aType->AddFunction<&FlightAudio::StopLoop>("StopLoop");
    aType->AddFunction<&FlightAudio::Pause>("Pause");
    aType->AddFunction<&FlightAudio::Resume>("Resume");
    aType->AddFunction<&FlightAudio::AcousticsState>("AcousticsState");
    aType->AddFunction<&FlightAudio::AcousticsLog>("AcousticsLog");
    aType->AddFunction<&FlightAudio::AcousticsStems>("AcousticsStems");
    aType->AddFunction<&FlightAudio::AcousticsTestSource>("AcousticsTestSource");
    aType->AddFunction<&FlightAudio::StartEngine>("StartEngine");
    aType->AddFunction<&FlightAudio::StopEngine>("StopEngine");
    aType->AddFunction<&FlightAudio::EngineState>("EngineState");
    aType->AddFunction<&FlightAudio::EngineCharacter>("EngineCharacter");
    aType->AddFunction<&FlightAudio::EngineSound>("EngineSound");
    aType->AddFunction<&FlightAudio::AcousticsAir>("AcousticsAir");
    aType->AddFunction<&FlightAudio::AcousticsPlanes>("AcousticsPlanes");
    aType->AddFunction<&FlightAudio::AcousticsOcclusion>("AcousticsOcclusion");
    aType->AddFunction<&FlightAudio::AudioSummary>("AudioSummary");
    aType->AddFunction<&FlightAudio::VoicesBudget>("VoicesBudget");
    aType->AddFunction<&FlightAudio::VoicesState>("VoicesState");
    aType->AddFunction<&FlightAudio::SetCinematic>("SetCinematic");
    aType->AddFunction<&FlightAudio::SetWindowSlots>("SetWindowSlots");
    aType->AddFunction<&FlightAudio::WindOn>("WindOn");
    aType->AddFunction<&FlightAudio::WindState>("WindState");
  }
};