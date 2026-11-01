// PID and InputPID as pure code (src/red4ext/Flight/Pid.hpp). Expected values are worked by hand from
// PID.reds; every value is a dyadic or short decimal, so the tolerance resolves every check.
#include "check.hpp"

#include "Flight/Pid.hpp"

using namespace Flight;

namespace {
constexpr double kTol = 1e-5;
} // namespace

TEST(pid_correction) {
  PidState s;
  PidGains g{1.0f, 0.5f, 0.25f};
  // derivative 2 / 0.5 = 4, integral 2 * 0.5 = 1: 2 + 0.5 + 1
  CHECK_NEAR(PidCorrection(s, g, 2.0f, 0.5f), 3.5, kTol);
  CHECK_NEAR(s.lastError, 2, kTol);
  CHECK_NEAR(s.integral, 1, kTol);
  // derivative (1 - 2) / 0.5 = -2, integral 0.5 + 1 = 1.5: 1 + 0.75 - 0.5
  CHECK_NEAR(PidCorrection(s, g, 1.0f, 0.5f), 1.25, kTol);
}

TEST(pid_integral_clamp) {
  PidState s;
  PidGains g{0.0f, 1.0f, 0.0f};
  CHECK_NEAR(PidCorrection(s, g, 1000.0f, 1.0f), 100, kTol);
  // clamp(-1000 + 100) = -100
  CHECK_NEAR(PidCorrection(s, g, -1000.0f, 1.0f), -100, kTol);
}

TEST(pid_correction_clamped) {
  PidState s;
  PidGains g{1.0f, 0.0f, 0.0f};
  CHECK_NEAR(PidCorrectionClamped(s, g, 30.0f, 1.0f, 10.0f), 10, kTol);
  CHECK_NEAR(PidCorrectionClamped(s, g, -30.0f, 1.0f, 10.0f), -10, kTol);
  // the state advanced
  CHECK_NEAR(s.lastError, -30, kTol);
}

TEST(pid_value) {
  PidState s;
  s.input = 1.0f;
  PidGains g{0.5f, 0.0f, 0.0f};
  CHECK_NEAR(PidValue(s, g, 0.1f), 0.5, kTol);
  // error 0.5
  CHECK_NEAR(PidValue(s, g, 0.1f), 0.75, kTol);
}

TEST(pid_reset) {
  PidState s;
  s.value = 0.75f;
  s.input = 1.0f;
  s.integral = 3.0f;
  s.lastError = 2.0f;
  PidReset(s, 0.3f);
  CHECK_NEAR(s.input, 0.3, kTol);
  CHECK_NEAR(s.integral, 0, kTol);
  CHECK_NEAR(s.lastError, 0, kTol);
  CHECK_NEAR(s.value, 0.75, kTol);
  // the input defaults to 0
  s.input = 0.6f;
  PidReset(s);
  CHECK_NEAR(s.input, 0, kTol);
}

TEST(pid_input_ramp) {
  const float P = 0.1f, Pd = 1.0f;
  PidState s;
  s.input = 1.0f;
  CHECK_NEAR(InputPidValue(s, P, Pd), 0.1, kTol);
  // error 0.9 at P
  CHECK_NEAR(InputPidValue(s, P, Pd), 0.19, kTol);
  // release toward zero: P_dec closes the whole error
  s.input = 0.0f;
  CHECK_NEAR(InputPidValue(s, P, Pd), 0, kTol);

  // across zero: P * (-1 - 0.5)
  PidState c;
  c.input = -1.0f;
  c.value = 0.5f;
  CHECK_NEAR(InputPidValue(c, P, Pd), 0.35, kTol);

  // same sign and smaller: P_dec * (0.2 - 0.5)
  PidState d;
  d.input = 0.2f;
  d.value = 0.5f;
  CHECK_NEAR(InputPidValue(d, P, Pd), 0.2, kTol);
  // across zero but smaller: the sign test alone picks P, 0.5 + 0.1 * -0.7 (P_dec would give -0.2)
  PidState e;
  e.input = -0.2f;
  e.value = 0.5f;
  CHECK_NEAR(InputPidValue(e, P, Pd), 0.43, kTol);

  CHECK_NEAR(s.integral, 0, kTol);
  CHECK_NEAR(s.lastError, 0, kTol);
  CHECK_NEAR(c.integral, 0, kTol);
  CHECK_NEAR(d.lastError, 0, kTol);
}

TEST(pid_derivative_and_order) {
  PidState s;
  PidGains g{2.0f, 1.0f, 0.5f};
  const float dt = 0.5f;
  // derivative 4, integral 1: 4 + 1 + 2
  CHECK_NEAR(PidCorrection(s, g, 2.0f, dt), 7, kTol);
  // derivative (4 - 2) / 0.5 = 4, integral 1 + 2 = 3: 8 + 3 + 2
  CHECK_NEAR(PidCorrection(s, g, 4.0f, dt), 13, kTol);
  // derivative (1 - 4) / 0.5 = -6, integral 3 + 0.5 = 3.5: 2 + 3.5 - 3
  CHECK_NEAR(PidCorrection(s, g, 1.0f, dt), 2.5, kTol);
  CHECK_NEAR(s.integral, 3.5, kTol);
  CHECK_NEAR(s.lastError, 1, kTol);
}
