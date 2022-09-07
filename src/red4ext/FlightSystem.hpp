#pragma once


#include <RED4ext/RED4ext.hpp>
#include "Engine/RTTIClass.hpp"
#include <RED4ext/Scripting/Natives/gameIGameSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include "FlightAudio.hpp"
#include "IFlightSystem.hpp"
#include "FlightComponent.hpp"

class FlightSystem : public Engine::RTTIClass<FlightSystem, IFlightSystem> {
public:
  virtual void RegisterUpdates(RED4ext::UpdateManagerHolder *holder) override;
  virtual bool WorldAttached(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void WorldPendingDetach(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void WorldDetached(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void sub_130() override;
  virtual uint32_t sub_138(uint64_t, uint64_t) override;
  virtual void sub_140(uint64_t) override;
  virtual void sub_148() override;
  virtual void OnGameLoad(void *, uint64_t, uint64_t) override;
  virtual bool sub_158() override;
  virtual void OnGamePrepared() override;
  virtual void sub_168() override;
  virtual void sub_170() override;
  virtual void sub_178(uintptr_t a1, bool a2) override;
  virtual void OnStreamingWorldLoaded(uint64_t, bool isGameLoaded, uint64_t) override;
  virtual void sub_188() override;
  virtual void sub_190(HighLow *) override;
  virtual void Initialize(void **unkThing) override;
  virtual void sub_1A0() override;

  //static FlightSystem *GetInstance();
  static RED4ext::Handle<FlightSystem> GetInstance();

  void RegisterComponent(RED4ext::WeakHandle<FlightComponent>);
  void UnregisterComponent(RED4ext::WeakHandle<FlightComponent>);

  int32_t cameraIndex = 0;
  RED4ext::WeakHandle<RED4ext::ent::IPlacedComponent> soundListener;
  RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>> components;
  RED4ext::Handle<FlightAudio> audio;
  RED4ext::DynArray<RED4ext::WeakHandle<FlightComponent>> flightComponents;

private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType) {
    //aType->flags.b20000 = true;
    //aType->flags.isAlwaysTransient = true;
  }

  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&FlightSystem::RegisterComponent>("RegisterComponent");
    aType->AddFunction<&FlightSystem::UnregisterComponent>("UnregisterComponent");
    aType->AddFunction<&GetInstance>("GetInstance");
  }
};
RED4EXT_ASSERT_OFFSET(FlightSystem, cameraIndex, 0x48);
RED4EXT_ASSERT_OFFSET(FlightSystem, soundListener, 0x50);
RED4EXT_ASSERT_OFFSET(FlightSystem, audio, 0x70);
RED4EXT_ASSERT_OFFSET(FlightSystem, flightComponents, 0x80);