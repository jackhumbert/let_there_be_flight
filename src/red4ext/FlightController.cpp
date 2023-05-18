#include "FlightController.hpp"
#include "Utils/FlightModule.hpp"

#include "Utils/Utils.hpp"
#include "stdafx.hpp"
#include <RED4ext/RTTISystem.hpp>

RED4ext::Handle<FlightController> handle;

FlightController* FlightController::GetInstance() { 
  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightController Instance");
    auto instance = reinterpret_cast<FlightController *>(RED4ext::CRTTISystem::Get()->GetClass("FlightController")->CreateInstance());
    handle = RED4ext::Handle<FlightController>(instance);
  }
  
  return (FlightController*)handle.instance;
}
