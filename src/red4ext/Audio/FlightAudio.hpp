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
  // each frame the player's car flies: its matrix, the echoes' level (0: off) and how far the view
  // is inside the cabin (Audio/Acoustics)
  void UpdateAcoustics(RED4ext::Matrix carMatrix, float gain, float inside);
  // the echoes' state as JSON (Acoustics::State), for tests
  RED4ext::CString AcousticsState();

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
  }
};