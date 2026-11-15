#pragma once

// The audio inputs from the game (docs/design/audio.md, Inputs): once a frame per flight component,
// after its script's OnUpdate, Component() reads the vehicle and the script's fields through RTTI into
// Inputs::Frame, works Inputs::Values out (Inputs.hpp, the rules of the old FlightComponent.UpdateAudioParams
// and the audio half of IFlightThruster.Update), and hands them to the thrusters' sound, the player's
// echoes and wind.
//
// Game thread.

#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Utils.hpp>

#include <cstdint>
#include <utility>

struct FlightComponent; // the other headers forward-declare it as a struct: the keyword is part of the mangled name

namespace Gather {

// The component's thrusters, its player's echoes and wind: skipped while the photo mode is on.
void Component(FlightComponent *c, float deltaTime);
// The component's entity is gone: its values are dropped.
void Forget(uint64_t aEntity);

// A game system by its RTTI class name (or the nearest base class the game keeps it under); nullptr if it has none.
RED4ext::IScriptable *System(const char *aClass);

// aFunc of the game system aSystem with aArgs, the result into aOut; false if the game has no such system or function.
template <typename... Args>
bool Call(const char *aSystem, const char *aFunc, void *aOut, Args &&...aArgs) {
  auto cls = RED4ext::CRTTISystem::Get()->GetClass(aSystem);
  auto func = cls ? cls->GetFunction(aFunc) : nullptr;
  auto system = func ? System(aSystem) : nullptr;
  return system && RED4ext::ExecuteFunction(system, func, aOut, std::forward<Args>(aArgs)...);
}

} // namespace Gather
