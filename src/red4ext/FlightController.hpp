#pragma once

#include <RED4ext/Common.hpp>
// #include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RedLib.hpp>

#include "Flight/Controls.hpp"
#include "Flight/FlightPID.hpp"

struct FlightController : RED4ext::IScriptable {
  static RED4ext::Handle<FlightController> GetInstance();

  bool enabled;
  bool active;
  int32_t mode;

  // the eight input smoothers (FlightController.Initialize creates them)
  RED4ext::Handle<FlightInputPID> linearBrake, angularBrake, lift, surge, roll, pitch, yaw, sway;

  // the controls after smoothing and normalisation, filled by StepInputs
  float outSurge = 0, outLift = 0, outSway = 0, outYaw = 0, outRoll = 0, outPitch = 0, outLinearBrake = 0, outAngularBrake = 0;

  // steps the 8 InputPIDs and fills the out* fields with the normalised controls
  void StepInputs();

  // virtual bool CanBeDestructed() override { return false; };
  
  virtual ~FlightController() override {
    return;
  }
  
  RTTI_IMPL_TYPEINFO(FlightController);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightController, {
  RTTI_METHOD(GetInstance);
  RTTI_PROPERTY(enabled);
  RTTI_PROPERTY(active);
  RTTI_PROPERTY(mode);
  RTTI_PROPERTY(linearBrake);
  RTTI_PROPERTY(angularBrake);
  RTTI_PROPERTY(lift);
  RTTI_PROPERTY(surge);
  RTTI_PROPERTY(roll);
  RTTI_PROPERTY(pitch);
  RTTI_PROPERTY(yaw);
  RTTI_PROPERTY(sway);
  RTTI_PROPERTY(outSurge);
  RTTI_PROPERTY(outLift);
  RTTI_PROPERTY(outSway);
  RTTI_PROPERTY(outYaw);
  RTTI_PROPERTY(outRoll);
  RTTI_PROPERTY(outPitch);
  RTTI_PROPERTY(outLinearBrake);
  RTTI_PROPERTY(outAngularBrake);
  RTTI_METHOD(StepInputs);
});

RED4EXT_ASSERT_SIZE(FlightController, 0xE8);
RED4EXT_ASSERT_OFFSET(FlightController, enabled, 0x40);
RED4EXT_ASSERT_OFFSET(FlightController, active, 0x41);
RED4EXT_ASSERT_OFFSET(FlightController, mode, 0x44);
RED4EXT_ASSERT_OFFSET(FlightController, linearBrake, 0x48);
