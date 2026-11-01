#include "FlightController.hpp"
#include "Utils/FlightModule.hpp"

#include "Utils/Utils.hpp"
#include "stdafx.hpp"
#include <RED4ext/RTTISystem.hpp>

RED4ext::Handle<FlightController> handle;

RED4ext::Handle<FlightController> FlightController::GetInstance() { 
  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightController Instance");
    auto instance = reinterpret_cast<FlightController *>(RED4ext::CRTTISystem::Get()->GetClass("FlightController")->CreateInstance(true));
    handle = RED4ext::Handle<FlightController>(instance);
  }
  
  return handle;
}

namespace {

Flight::InputAxis AxisOf(const RED4ext::Handle<FlightInputPID> &aPid) {
  if (!aPid) {
    return {};
  }
  return {aPid->State(), aPid->P, aPid->P_dec};
}

void Store(const RED4ext::Handle<FlightInputPID> &aPid, const Flight::InputAxis &aAxis) {
  if (aPid) {
    aPid->SetState(aAxis.state);
  }
}

} // namespace

void FlightController::StepInputs() {
  Flight::ControlAxes axes{AxisOf(surge), AxisOf(lift), AxisOf(sway), AxisOf(yaw),
                           AxisOf(roll),  AxisOf(pitch), AxisOf(linearBrake), AxisOf(angularBrake)};
  const Flight::Controls c = Flight::StepControls(axes);
  Store(surge, axes.surge);
  Store(lift, axes.lift);
  Store(sway, axes.sway);
  Store(yaw, axes.yaw);
  Store(roll, axes.roll);
  Store(pitch, axes.pitch);
  Store(linearBrake, axes.linearBrake);
  Store(angularBrake, axes.angularBrake);
  outSurge = c.surge;
  outLift = c.lift;
  outSway = c.sway;
  outYaw = c.yaw;
  outRoll = c.roll;
  outPitch = c.pitch;
  outLinearBrake = c.linearBrake;
  outAngularBrake = c.angularBrake;
}
