#pragma once

#include <RED4ext/Scripting/Natives/Generated/game/IGameSystem.hpp>

namespace FlightSystem {

struct IFlightSystem : RED4ext::game::IGameSystem {
  //RED4ext::CClass *GetNativeType(); // 00
};

struct FlightSystem : IFlightSystem {
  RED4ext::CClass *GetNativeType() override;
  bool sub_118(void *) override;
  bool sub_120() override;
  void *RegisterUpdate(uintptr_t lookup) override; // 110

  static FlightSystem *GetInstance();
  RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>> components;
};
//RED4EXT_ASSERT_OFFSET(FlightSystem, components, 0x48);

}  // namespace FlightSystem