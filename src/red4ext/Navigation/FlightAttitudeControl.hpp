#pragma once

// The attitude control of FlyAerobatic in C++ (Navigation/Aerobatics.hpp) for FlightModeAutopilot, which fills the
// params, the state and the inputs each frame, calls the eases and Step, and copies the state back.

#include "Aerobatics.hpp"
#include "Flight/FlightStats.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightAttitudeControl : RED4ext::IScriptable {
  // params (FlightModeAutopilot's, FlightSettings' and FlightMode's fields of the same name, defaults as
  // Navigation::EaseParams and Navigation::AttitudeControlParams)
  float raceRollFrequency = 2.5f;
  float raceRollRateMax = 1.0f;
  float raceRollAccelMax = 3.0f;
  float racePitchFrequency = 2.5f;
  float racePitchRateMax = 1.0f;
  float racePitchAccelMax = 3.0f;
  float attitudeFrequency = 5.0f;
  float attitudeDamping = 0.8f;
  float maxAngularAccel = 20.0f;
  float torqueGainPitch = 2.0f;
  float torqueGainRoll = 0.5f;
  float torqueGainYaw = 0.9f;
  float generalDampFactorAngular = 2.0f;
  float generalYawDampFactor = 1.0f;
  float generalDampFactorAngularMax = 3.5f;
  float standardModeSurgeFactor = 15.0f;
  float standardModeSwayFactor = 5.0f;
  float standardModeLiftFactor = 25.0f;
  float gravityFactor = 1.0f;

  // state, in and out
  RED4ext::Vector4 targetRight{};
  RED4ext::Vector4 targetForward{};
  RED4ext::Vector4 targetUp{};
  RED4ext::Vector4 omegaRef{};
  float bankAngle = 0.0f;
  float bankRateNow = 0.0f;
  float pitchAngle = 0.0f;
  float pitchRateNow = 0.0f;

  // inputs, this frame (inertiaX.W > 0.5: the world inertia rows are valid)
  float dt = 0.001f;
  bool shape = false;
  RED4ext::Vector4 accel{};
  RED4ext::Vector4 drag{};
  float dragAccel = 0.0f;
  float raceDecel = 0.0f;
  RED4ext::Vector4 thrusterTensor{1.0f, 1.0f, 1.0f, 0.0f};
  RED4ext::Vector4 inertiaX{};
  RED4ext::Vector4 inertiaY{};
  RED4ext::Vector4 inertiaZ{};
  RED4ext::Vector4 fwd{};
  RED4ext::Vector4 up{};

  // outputs
  RED4ext::Vector4 torque{};
  RED4ext::Vector4 thrust{};
  RED4ext::Vector4 rateRef{};
  float attitudeError = 0.0f;

  // the wanted up vector with its bank eased; updates bankAngle and bankRateNow
  RED4ext::Vector4 EaseBank(float aDt, RED4ext::Vector4 aFwd, RED4ext::Vector4 aUp);
  // the nose with its pitch eased; aTangent is the line's, read when aOnLine; updates pitchAngle and pitchRateNow
  RED4ext::Vector4 EasePitch(float aDt, RED4ext::Vector4 aFwd, bool aOnLine, RED4ext::Vector4 aTangent,
                             RED4ext::Vector4 aHeadingRef);
  // Navigation::AttitudeControl from the fields; sets the outputs and the state. Without stats the outputs are zero
  // and the state stays.
  void Step(RED4ext::Handle<FlightStats> aStats);

  RTTI_IMPL_TYPEINFO(FlightAttitudeControl);
  RTTI_IMPL_ALLOCATOR();

private:
  Navigation::EaseParams EaseParamsOf() const;
  Navigation::EaseState EaseStateOf() const;
  Navigation::AttitudeControlParams Params() const;
  Navigation::AttitudeControlState State() const;
  Navigation::AttitudeControlInput Input() const;
};

RTTI_DEFINE_CLASS(FlightAttitudeControl, {
  RTTI_PROPERTY(raceRollFrequency);
  RTTI_PROPERTY(raceRollRateMax);
  RTTI_PROPERTY(raceRollAccelMax);
  RTTI_PROPERTY(racePitchFrequency);
  RTTI_PROPERTY(racePitchRateMax);
  RTTI_PROPERTY(racePitchAccelMax);
  RTTI_PROPERTY(attitudeFrequency);
  RTTI_PROPERTY(attitudeDamping);
  RTTI_PROPERTY(maxAngularAccel);
  RTTI_PROPERTY(torqueGainPitch);
  RTTI_PROPERTY(torqueGainRoll);
  RTTI_PROPERTY(torqueGainYaw);
  RTTI_PROPERTY(generalDampFactorAngular);
  RTTI_PROPERTY(generalYawDampFactor);
  RTTI_PROPERTY(generalDampFactorAngularMax);
  RTTI_PROPERTY(standardModeSurgeFactor);
  RTTI_PROPERTY(standardModeSwayFactor);
  RTTI_PROPERTY(standardModeLiftFactor);
  RTTI_PROPERTY(gravityFactor);
  RTTI_PROPERTY(targetRight);
  RTTI_PROPERTY(targetForward);
  RTTI_PROPERTY(targetUp);
  RTTI_PROPERTY(omegaRef);
  RTTI_PROPERTY(bankAngle);
  RTTI_PROPERTY(bankRateNow);
  RTTI_PROPERTY(pitchAngle);
  RTTI_PROPERTY(pitchRateNow);
  RTTI_PROPERTY(dt);
  RTTI_PROPERTY(shape);
  RTTI_PROPERTY(accel);
  RTTI_PROPERTY(drag);
  RTTI_PROPERTY(dragAccel);
  RTTI_PROPERTY(raceDecel);
  RTTI_PROPERTY(thrusterTensor);
  RTTI_PROPERTY(inertiaX);
  RTTI_PROPERTY(inertiaY);
  RTTI_PROPERTY(inertiaZ);
  RTTI_PROPERTY(fwd);
  RTTI_PROPERTY(up);
  RTTI_PROPERTY(torque);
  RTTI_PROPERTY(thrust);
  RTTI_PROPERTY(rateRef);
  RTTI_PROPERTY(attitudeError);
  RTTI_METHOD(EaseBank);
  RTTI_METHOD(EasePitch);
  RTTI_METHOD(Step);
});
