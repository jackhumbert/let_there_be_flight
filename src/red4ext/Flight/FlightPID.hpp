#pragma once

// PID.reds' PID and InputPID as native classes (Flight/Pid.hpp holds the maths). The fields keep the script's names;
// the script methods that build, update and read them stay in Utils/PID.reds and call the natives below.

#include "Flight/Pid.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RedLib.hpp>

struct FlightPID : RED4ext::IScriptable {
  float valueFloat = 0.0f;
  float inputFloat = 0.0f;
  float P = 0.0f;
  float I = 0.0f;
  float D = 0.0f;
  float integralFloat = 0.0f;
  float lastErrorFloat = 0.0f;

  Flight::PidState State() const;
  void SetState(const Flight::PidState &aState);
  Flight::PidGains Gains() const;

  // Flight::PidCorrection on the fields: advances the integral and lastErrorFloat.
  float NativeCorrection(float aError, float aTimeDelta);
  // Flight::PidReset on the fields: sets the input, clears the integral and lastErrorFloat.
  void NativeReset(float aInput);

  RTTI_IMPL_TYPEINFO(FlightPID);
  RTTI_IMPL_ALLOCATOR();
};

struct FlightInputPID : FlightPID {
  float P_dec = 0.0f;

  // Flight::InputPidCorrection on the fields; reads no dt and changes no state.
  float NativeInputCorrection(float aError);

  RTTI_IMPL_TYPEINFO(FlightInputPID);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightPID, "PID", {
  RTTI_PROPERTY(valueFloat);
  RTTI_PROPERTY(inputFloat);
  RTTI_PROPERTY(P);
  RTTI_PROPERTY(I);
  RTTI_PROPERTY(D);
  RTTI_PROPERTY(integralFloat);
  RTTI_PROPERTY(lastErrorFloat);
  RTTI_METHOD(NativeCorrection);
  RTTI_METHOD(NativeReset);
});

RTTI_DEFINE_CLASS(FlightInputPID, "InputPID", {
  RTTI_PARENT(FlightPID);
  RTTI_PROPERTY(P_dec);
  RTTI_METHOD(NativeInputCorrection);
});
