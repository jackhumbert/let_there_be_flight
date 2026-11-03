
#include <Utils/FlightModule.hpp>
#ifdef __APPLE__
#include <cstdlib>
#include <spdlog/spdlog.h>
#include "MacShims.hpp"
#endif
#include <FlightController.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>

using namespace RED4ext;

// void vehicle::AreaSpeedLimiter::Update_PreMove(float,class THandle<class vehicle::BaseObject> const &)
REGISTER_FLIGHT_HOOK_HASH(void, 1836326624, AreaSpeedLimiter_Update, void * self, 
  float delta, Handle<vehicle::BaseObject> const & vehicle
) {
  auto fc = FlightController::FlightController::GetInstance();
#ifdef __APPLE__
  // LTBF R43 (macOS, diagnostics): log the limiter calls, about once a second, with whether flight skips them.
  static const bool s_macDiag = std::getenv("LTBF_MAC_DIAG") != nullptr;
  static uint32_t s_macCalls = 0;
  if (s_macDiag && (s_macCalls++ % 60) == 0) {
    const auto caller = reinterpret_cast<uintptr_t>(__builtin_return_address(0)) - LtbfMac::ImageBase();
    spdlog::info("[ltbf][phys] AreaSpeedLimiter call {} from game+0x{:X}, flight active {}", s_macCalls, caller,
                 fc ? static_cast<int>(fc->active) : -1);
  }
#endif
  if (fc && fc->active)
    return;
  return AreaSpeedLimiter_Update_Original(self, delta, vehicle);
}