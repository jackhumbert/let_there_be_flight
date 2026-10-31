#pragma once

#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include "Engine/RTTIClass.hpp"
#include "FmodHelper.hpp"
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>

class FlightAudio : public Engine::RTTIClass<FlightAudio, RED4ext::IScriptable> {
public:
  static bool Load(RED4ext::CGameApplication *aApp);
  static bool Unload(RED4ext::CGameApplication *aApp);

  void UpdateVolume();
  void Pause();
  void Resume();

  static FlightAudio* Get();

  // A listener the scripts set each frame in place of the player's soundListener component: the
  // cinematic camera's (its shots are around the car, the component stays with V, FlightComponent).
  void SetCameraListener(RED4ext::Vector4 aPosition, RED4ext::Vector4 aForward, RED4ext::Vector4 aUp);
  // false when the scripts have not set it lately (then the component is used)
  static bool GetCameraListener(RED4ext::Matrix &aMatrix);

  void UpdateListenerMatrix(RED4ext::Matrix matrix);
  void UpdateEventMatrix(RED4ext::CString emitterName, RED4ext::Matrix matrix);
  void UpdateParameter(RED4ext::CString name, float value);
  void Play(RED4ext::CString eventName);
  void Start(RED4ext::CString emitterName, RED4ext::CString eventName);
  void StartWithPitch(RED4ext::CString emitterName, RED4ext::CString eventName, float pitch);
  void Stop(RED4ext::CString emitterName);
  void UpdateEvent(RED4ext::CString emitterName, RED4ext::Matrix matrix, float eventVolume,
                     RED4ext::Handle<RED4ext::IScriptable> update);
  // each frame the player's car flies: its matrix, the echoes' level (0: off), how far the view is
  // inside the cabin and who casts the near rays (Acoustics::RaySource: 0 none, 1 the scripts, 2 PhysX)
  void UpdateAcoustics(RED4ext::Matrix carMatrix, float gain, float inside, int32_t raySource);
  // the echoes' state as JSON (Acoustics::State), for tests
  RED4ext::CString AcousticsState();
  // the near rays' directions for the car (Acoustics::Probe::NearRays), for the scripts to cast
  RED4ext::DynArray<RED4ext::Vector4> AcousticsRays(RED4ext::Matrix carMatrix);
  // what they hit: distance (< 0: nothing) and physics material, cast from origin
  void AcousticsHits(RED4ext::Vector4 origin, RED4ext::DynArray<float> distances,
                     RED4ext::DynArray<RED4ext::CName> materials);
  // the nav map's and the live distances side by side, a row a probe, to
  // red4ext/logs/ltbf_acoustics_<time>.csv (on) or no more (off)
  void AcousticsLog(bool on);
  // LTBF's FMOD sound as stems (Acoustics::Stems) to red4ext/logs/ltbf_stems_<time>_*.wav: the base
  // path, "" when stopped or the files could not be opened
  RED4ext::CString AcousticsStems(bool on);
  // a test sound in place of LTBF's FMOD sound (Acoustics::TestSource: 0 off, 1 clicks, 2 pink noise, 3 a 440 Hz sine)
  void AcousticsTestSource(int32_t source);

private:
  friend Descriptor;

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&FlightAudio::UpdateListenerMatrix>("UpdateListenerMatrix");
    aType->AddFunction<&FlightAudio::SetCameraListener>("SetCameraListener");
    aType->AddFunction<&FlightAudio::UpdateEventMatrix>("UpdateEventMatrix");
    aType->AddFunction<&FlightAudio::UpdateParameter>("UpdateParameter");
    aType->AddFunction<&FlightAudio::Play>("Play");
    aType->AddFunction<&FlightAudio::Start>("Start");
    aType->AddFunction<&FlightAudio::StartWithPitch>("StartWithPitch");
    aType->AddFunction<&FlightAudio::Stop>("Stop");
    aType->AddFunction<&FlightAudio::UpdateEvent>("UpdateEvent");
    aType->AddFunction<&FlightAudio::Pause>("Pause");
    aType->AddFunction<&FlightAudio::Resume>("Resume");
    aType->AddFunction<&FlightAudio::UpdateAcoustics>("UpdateAcoustics");
    aType->AddFunction<&FlightAudio::AcousticsState>("AcousticsState");
    aType->AddFunction<&FlightAudio::AcousticsRays>("AcousticsRays");
    aType->AddFunction<&FlightAudio::AcousticsHits>("AcousticsHits");
    aType->AddFunction<&FlightAudio::AcousticsLog>("AcousticsLog");
    aType->AddFunction<&FlightAudio::AcousticsStems>("AcousticsStems");
    aType->AddFunction<&FlightAudio::AcousticsTestSource>("AcousticsTestSource");
  }
};