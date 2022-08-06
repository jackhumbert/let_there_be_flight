#pragma once

#include <RED4ext/Scripting/Natives/gameIGameSystem.hpp>

namespace FlightSystem {

extern RED4ext::CClass *classPointer;

struct IFlightSystem : RED4ext::game::IGameSystem {
  //virtual RED4ext::CClass *GetNativeType() override; // 00
};

struct FlightSystem : IFlightSystem {
  virtual RED4ext::CClass *GetNativeType() override;
  virtual void RegisterUpdates(RED4ext::UpdateManagerHolder *holder) override;
  virtual bool sub_118(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void sub_120(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void sub_128(RED4ext::world::RuntimeScene *runtimeScene) override;
  virtual void sub_130() override;
  virtual bool sub_138() override;
  virtual void sub_140() override; 
  virtual void sub_148() override;
  virtual void sub_150(void *, uint64_t, uint64_t) override;
  virtual bool sub_158() override;
  virtual void sub_160() override; 
  virtual void sub_168() override;
  virtual void sub_170() override; 
  virtual void sub_178(uintptr_t a1, bool a2) override;
  virtual void sub_180(uint64_t, bool isGameLoaded, uint64_t) override; 
  virtual void sub_188() override;
  virtual void sub_190(HighLow *) override;
  virtual void ** sub_198(void ** unkThing) override; 
  virtual void sub_1A0() override;


  static FlightSystem *GetInstance();
  int32_t cameraIndex = 0;
  RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>> components;
};
RED4EXT_ASSERT_OFFSET(FlightSystem, cameraIndex, 0x48);

}  // namespace FlightSystem