#include "FlightPID.hpp"

Flight::PidState FlightPID::State() const { return {valueFloat, inputFloat, integralFloat, lastErrorFloat}; }

void FlightPID::SetState(const Flight::PidState &aState) {
  valueFloat = aState.value;
  inputFloat = aState.input;
  integralFloat = aState.integral;
  lastErrorFloat = aState.lastError;
}

Flight::PidGains FlightPID::Gains() const { return {P, I, D}; }

float FlightPID::NativeCorrection(float aError, float aTimeDelta) {
  Flight::PidState state = State();
  float correction = Flight::PidCorrection(state, Gains(), aError, aTimeDelta);
  SetState(state);
  return correction;
}

void FlightPID::NativeReset(float aInput) {
  Flight::PidState state = State();
  Flight::PidReset(state, aInput);
  SetState(state);
}

float FlightInputPID::NativeInputCorrection(float aError) {
  return Flight::InputPidCorrection(State(), P, P_dec, aError);
}
