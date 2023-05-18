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

  void UpdateListenerMatrix(RED4ext::Matrix matrix);
  void UpdateEventMatrix(RED4ext::CString emitterName, RED4ext::Matrix matrix);
  void UpdateParameter(RED4ext::CString name, float value);
  void Play(RED4ext::CString eventName);
  void Start(RED4ext::CString emitterName, RED4ext::CString eventName);
  void StartWithPitch(RED4ext::CString emitterName, RED4ext::CString eventName, float pitch);
  void Stop(RED4ext::CString emitterName);
  void UpdateEvent(RED4ext::CString emitterName, RED4ext::Matrix matrix, float eventVolume,
                     RED4ext::Handle<RED4ext::IScriptable> update);

private:
  friend Descriptor;

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&FlightAudio::UpdateListenerMatrix>("UpdateListenerMatrix");
    aType->AddFunction<&FlightAudio::UpdateEventMatrix>("UpdateEventMatrix");
    aType->AddFunction<&FlightAudio::UpdateParameter>("UpdateParameter");
    aType->AddFunction<&FlightAudio::Play>("Play");
    aType->AddFunction<&FlightAudio::Start>("Start");
    aType->AddFunction<&FlightAudio::StartWithPitch>("StartWithPitch");
    aType->AddFunction<&FlightAudio::Stop>("Stop");
    aType->AddFunction<&FlightAudio::UpdateEvent>("UpdateEvent");
  }
};