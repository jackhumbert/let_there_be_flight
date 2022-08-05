#pragma once

#include <RED4ext/Scripting/Natives/gameIGameSystem.hpp>

namespace FlightSystem {

extern RED4ext::CClass *classPointer;

struct IFlightSystem : RED4ext::game::IGameSystem {
  virtual RED4ext::CClass *GetNativeType() override; // 00
};

struct FlightSystem : IFlightSystem {
  virtual RED4ext::CClass *GetNativeType() override;
  virtual void RegisterUpdates(RED4ext::UpdateManagerHolder *holder) override; // 110
  virtual bool sub_118(void *) override;
  virtual void sub_120(void *runtimeScene) override;
  virtual void sub_128(void *runtimeScene) override;
  virtual void sub_130();
  virtual void sub_138();
  virtual void sub_140(); 
  virtual void sub_148();
  // 150, OnGameLoad
  virtual void sub_150(void *, uint64_t, uint64_t);
  // ReturnOne - should probably always return 1
  virtual bool sub_158();
  virtual void sub_160(); 
  // might be called from GameInstance->Systems168o170
  virtual void sub_168();
  // might be called from GameInstance->Systems168o170
  virtual void sub_170(); 
  // something with a CString @ 0x08
  virtual void sub_178(uintptr_t a1, bool a2);
  virtual void sub_180(uint64_t, bool isGameLoaded, uint64_t); 
  virtual void sub_188();
  // called from GameInstance->sub_20
  virtual void sub_190(HighLow *); // 190
  // some systems load tweaks - might be a setup, called from GameInstance->sub_20
  virtual void sub_198(void *); // 198
  virtual void sub_1A0();       // 1A0


  static FlightSystem *GetInstance();
  RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>> components;
};
//RED4EXT_ASSERT_OFFSET(FlightSystem, components, 0x48);

}  // namespace FlightSystem