#pragma once

#include <RED4ext/RED4ext.hpp>
#include "Engine/RTTIClass.hpp"
#include <RED4ext/Scripting/Natives/gameIGameSystem.hpp>
#include "IFlightSystem.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include "Audio/FlightAudio.hpp"
#include "FlightComponent.hpp"

/// FlightSystem
class FlightSystem : public Engine::RTTIClass<FlightSystem, IFlightSystem> {
public:
  //virtual RED4ext::Memory::IAllocator* GetAllocator() override;
  virtual void OnRegisterUpdates(RED4ext::UpdateRegistrar *aRegistrar) override;
  virtual void OnWorldAttached(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void OnBeforeWorldDetach(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void OnWorldDetached(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void OnAfterWorldDetach() override;
  virtual uint32_t OnBeforeGameSave(const RED4ext::JobGroup& aJobGroup, void* a2) override;
  virtual void OnGameSave(void* aStream) override;
  virtual void OnAfterGameSave() override;
  virtual void OnGameLoad(const RED4ext::JobGroup& aJobGroup, bool& aSuccess, void* aStream) override;
  virtual bool OnGameRestored() override;
  virtual void OnGamePrepared() override;
  virtual void OnGamePaused() override;
  virtual void OnGameResumed() override;
  virtual void* IsSavingLocked(RED4ext::game::SaveLock* aLock, bool a2) override;
  virtual void OnStreamingWorldLoaded(RED4ext::world::RuntimeScene* aScene, uint64_t a2, const RED4ext::JobGroup& aJobGroup) override;
  virtual void sub_188() override;
  virtual void sub_190(RED4ext::IGameSystem::HighLow*) override;
  virtual void OnInitialize(const RED4ext::JobHandle& aJob) override;
  virtual void OnUninitialize() override;

  //static FlightSystem *GetInstance();
  static RED4ext::Handle<FlightSystem> GetInstance();

  void RegisterComponent(RED4ext::WeakHandle<FlightComponent>);
  void UnregisterComponent(RED4ext::WeakHandle<FlightComponent>);

  int32_t cameraIndex = 0;
  RED4ext::WeakHandle<RED4ext::ent::IPlacedComponent> soundListener;
  RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>> components;
  RED4ext::Handle<FlightAudio> audio;
  RED4ext::DynArray<RED4ext::WeakHandle<FlightComponent>> flightComponents;
  RED4ext::SharedMutex flightComponentsMutex;

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