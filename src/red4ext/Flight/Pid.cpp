#include "Flight/Pid.hpp"

#include <algorithm>
#include <cmath>

namespace Flight {

namespace {
float ClampF(float x, float lo, float hi) { return std::min(std::max(x, lo), hi); }
} // namespace

float PidCorrection(PidState &s, const PidGains &g, float error, float dt) {
  float derivative = (error - s.lastError) / dt;
  s.integral = ClampF(error * dt + s.integral, -100.0f, 100.0f);
  s.lastError = error;
  return g.P * error + g.I * s.integral + g.D * derivative;
}

float PidCorrectionClamped(PidState &s, const PidGains &g, float error, float dt, float clamp) {
  return ClampF(PidCorrection(s, g, error, dt), -clamp, clamp);
}

float PidValue(PidState &s, const PidGains &g, float dt) {
  float error = s.input - s.value;
  s.value += PidCorrection(s, g, error, dt);
  return s.value;
}

void PidReset(PidState &s, float input) {
  s.input = input;
  s.integral = 0.0f;
  s.lastError = 0.0f;
}

float InputPidCorrection(const PidState &s, float P, float P_dec, float error) {
  if (std::fabs(s.input) > std::fabs(s.value) || s.input * s.value < 0.0f) {
    return P * error;
  }
  return P_dec * error;
}

float InputPidValue(PidState &s, float P, float P_dec) {
  float error = s.input - s.value;
  s.value += InputPidCorrection(s, P, P_dec, error);
  return s.value;
}

} // namespace Flight
