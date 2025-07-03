#pragma once

#include <RED4ext/Common.hpp>
#include "Engine/RTTIClass.hpp"
#include <RED4ext/Scripting/Natives/gameIGameSystem.hpp>
#include "IFlightSystem.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/SharedSpinLock.hpp>
#include <RED4ext/Map.hpp>
#include "Audio/FlightAudio.hpp"
#include "Flight/Component.hpp"
#include <unordered_map>

using namespace RED4ext;

/// FlightSystem
class FlightSystem : public Engine::RTTIClass<FlightSystem, IFlightSystem> {
public:
  //virtual Memory::IAllocator* GetAllocator() override;
  virtual void OnRegisterUpdates(UpdateRegistrar *aRegistrar) override;
  virtual void OnWorldAttached(world::RuntimeScene *runtimeScene) override;
  virtual void OnBeforeWorldDetach(world::RuntimeScene *runtimeScene) override;
  virtual void OnWorldDetached(world::RuntimeScene *runtimeScene) override;
  virtual void OnAfterWorldDetach() override;
  virtual uint32_t OnBeforeGameSave(const JobGroup& aJobGroup, void* a2) override;
  virtual void OnGameSave(void* aStream) override;
  virtual void OnAfterGameSave() override;
  virtual void OnGameLoad(const JobGroup& aJobGroup, bool& aSuccess, void* aStream) override;
  virtual bool OnGameRestored() override;
  virtual void OnGamePrepared() override;
  virtual void OnGamePaused() override;
  virtual void OnGameResumed() override;
  virtual void* IsSavingLocked(game::SaveLock* aLock, bool a2) override;
  virtual void OnStreamingWorldLoaded(world::RuntimeScene* aScene, uint64_t a2, const JobGroup& aJobGroup) override;
  virtual void sub_180() override;
  virtual void sub_188() override;
  virtual void sub_190(void*) override;
  virtual void sub_198() override;
  virtual void OnInitialize(const JobHandle& aJob) override;
  virtual void OnUninitialize() override;

  //static FlightSystem *GetInstance();
  static Handle<FlightSystem> GetInstance();

  virtual void RegisterComponent(const Handle<FlightComponent> &) override;
  virtual void UnregisterComponent(const Handle<FlightComponent> &) override;

  int32_t cameraIndex = 0;
  WeakHandle<ent::IPlacedComponent> soundListener;
  Handle<FlightAudio> audio;
  std::unordered_map<uint64_t, WeakHandle<FlightComponent>> flightComponents;
  SharedSpinLock flightComponentsMutex;
  WeakHandle<FlightComponent> playerComponent;

private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType) {
    //aType->flags.b20000 = true;
    //aType->flags.isAlwaysTransient = true;
  }

  static void OnDescribe(Descriptor *aType, CRTTISystem *) {
    aType->AddFunction<&FlightSystem::RegisterComponent>("RegisterComponent");
    aType->AddFunction<&FlightSystem::UnregisterComponent>("UnregisterComponent");
    aType->AddFunction<&GetInstance>("GetInstance");
  }
};
RED4EXT_ASSERT_OFFSET(FlightSystem, cameraIndex, 0x48);
RED4EXT_ASSERT_OFFSET(FlightSystem, soundListener, 0x50);
RED4EXT_ASSERT_OFFSET(FlightSystem, audio, 0x60);
RED4EXT_ASSERT_OFFSET(FlightSystem, flightComponents, 0x70);
RED4EXT_ASSERT_OFFSET(FlightSystem, playerComponent, 0xB8);