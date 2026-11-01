#pragma once

// PID.reds as pure code, for the native PID classes.
//
// The script's PID and InputPID share the fields of PidState; the gains live beside it. There is no dt
// guard, as in the script: dt 0 gives inf or NaN through the derivative. Callers guarantee dt > 0:
// FlightComponent.OnUpdate returns at timeDelta <= 0 before the controller and the modes run.

namespace Flight {

// The script's valueFloat, inputFloat, integralFloat and lastErrorFloat.
struct PidState {
  float value = 0, input = 0, integral = 0, lastError = 0;
};

struct PidGains {
  float P = 0, I = 0, D = 0;
};

struct Pid {
  PidState state;
  PidGains gains;
};

// PID.GetCorrection: advances the integral (clamped to +-100) and lastError.
float PidCorrection(PidState &s, const PidGains &g, float error, float dt);

// PID.GetCorrectionClamped: the correction limited to +-clamp; the state still advances.
float PidCorrectionClamped(PidState &s, const PidGains &g, float error, float dt, float clamp);

// PID.GetValue(dt): moves the value toward the input by the correction.
float PidValue(PidState &s, const PidGains &g, float dt);

// PID.Reset: sets the input and clears the integral and lastError. The value is not touched.
void PidReset(PidState &s, float input = 0.0f);

// InputPID.GetCorrection: P while the input is larger than the value or on the other side of zero, P_dec otherwise.
// Reads no dt and changes no state.
float InputPidCorrection(const PidState &s, float P, float P_dec, float error);

// InputPID.GetValue(dt).
float InputPidValue(PidState &s, float P, float P_dec);


} // namespace Flight
