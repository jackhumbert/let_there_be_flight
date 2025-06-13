#pragma once
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/state/MachineStateContext.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/state/MachineActionParameterBool.hpp>

// game::stateMachine::ActionParameterBool game::stateMachine::StateContext::GetPermanentBoolParameter(CName) const
RED4ext::game::state::MachineActionParameterBool __fastcall GetPermanentBoolParameter(
    RED4ext::game::state::MachineStateContext * context, 
    RED4ext::game::state::MachineActionParameterBool * param, 
    RED4ext::CName name
  ) {
  RED4ext::UniversalRelocFunc<decltype(&GetPermanentBoolParameter)> call(1497441376);
  return call(context, param, name);
}