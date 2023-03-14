#pragma once
#include "FlightModule.hpp"

namespace FlightLog {
struct FlightLog : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();
};
}
