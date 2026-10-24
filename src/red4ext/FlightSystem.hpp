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
#ifdef __APPLE__
  // LTBF R42d (macOS): the game returns the SaveLock by value through x8 (macos/sdk-overlay gameIGameSystem.hpp).
  virtual game::SaveLock IsSavingLocked(bool a2) override;
#else
  virtual void* IsSavingLocked(game::SaveLock* aLock, bool a2) override;
#endif
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
#ifdef __APPLE__
  // LTBF R42f (macOS): libc++'s unordered_map is 0x28 bytes, MSVC's 0x40. The scripts address playerComponent at the Windows offset
  // 0xB8 (@runtimeProperty("offset", "0xB8") in FlightSystem.reds), so without this pad they read and wrote 0x18 bytes past it, past
  // the end of the object (Boot C/C2/C3 heap corruption in world).
  uint8_t macUnorderedMapPad[0x40 - sizeof(std::unordered_map<uint64_t, WeakHandle<FlightComponent>>)];
#endif
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
LTBF_FORCE_RTTI_REGISTRAR(FlightSystem) // LTBF R41: macOS, see Engine/RTTIRegistrar.hpp

RED4EXT_ASSERT_OFFSET(FlightSystem, cameraIndex, 0x48);
RED4EXT_ASSERT_OFFSET(FlightSystem, soundListener, 0x50);
RED4EXT_ASSERT_OFFSET(FlightSystem, audio, 0x60);
RED4EXT_ASSERT_OFFSET(FlightSystem, flightComponents, 0x70);
RED4EXT_ASSERT_OFFSET(FlightSystem, playerComponent, 0xB8);