#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>

namespace FlightController {
struct FlightController : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();
};
void RegisterTypes();
void RegisterFunctions();
} // namespace FlightController
