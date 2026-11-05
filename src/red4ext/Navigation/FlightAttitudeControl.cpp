#include "FlightAttitudeControl.hpp"

namespace {
Flight::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
RED4ext::Vector4 ToDirection(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }
} // namespace

Navigation::EaseParams FlightAttitudeControl::EaseParamsOf() const {
  Navigation::EaseParams p;
  p.raceRollFrequency = raceRollFrequency;
  p.raceRollRateMax = raceRollRateMax;
  p.raceRollAccelMax = raceRollAccelMax;
  p.racePitchFrequency = racePitchFrequency;
  p.racePitchRateMax = racePitchRateMax;
  p.racePitchAccelMax = racePitchAccelMax;
  return p;
}

Navigation::EaseState FlightAttitudeControl::EaseStateOf() const {
  Navigation::EaseState s;
  s.bankAngle = bankAngle;
  s.bankRateNow = bankRateNow;
  s.pitchAngle = pitchAngle;
  s.pitchRateNow = pitchRateNow;
  return s;
}

Navigation::AttitudeControlParams FlightAttitudeControl::Params() const {
  Navigation::AttitudeControlParams p;
  p.attitudeFrequency = attitudeFrequency;
  p.attitudeDamping = attitudeDamping;
  p.maxAngularAccel = maxAngularAccel;
  p.torqueGainPitch = torqueGainPitch;
  p.torqueGainRoll = torqueGainRoll;
  p.torqueGainYaw = torqueGainYaw;
  p.generalDampFactorAngular = generalDampFactorAngular;
  p.generalYawDampFactor = generalYawDampFactor;
  p.generalDampFactorAngularMax = generalDampFactorAngularMax;
  p.dampStabilityK = dampStabilityK;
  p.dampMaxPerFrame = dampMaxPerFrame;
  p.standardModeSurgeFactor = standardModeSurgeFactor;
  p.standardModeSwayFactor = standardModeSwayFactor;
  p.standardModeLiftFactor = standardModeLiftFactor;
  p.gravityFactor = gravityFactor;
  return p;
}

Navigation::AttitudeControlState FlightAttitudeControl::State() const {
  Navigation::AttitudeControlState s;
  s.targetRight = ToVec(targetRight);
  s.targetForward = ToVec(targetForward);
  s.targetUp = ToVec(targetUp);
  s.omegaRef = ToVec(omegaRef);
  s.bankAngle = bankAngle;
  s.bankRateNow = bankRateNow;
  return s;
}

Navigation::AttitudeControlInput FlightAttitudeControl::Input() const {
  Navigation::AttitudeControlInput i;
  i.dt = dt;
  i.fwd = ToVec(fwd);
  i.up = ToVec(up);
  i.shape = shape;
  i.accel = ToVec(accel);
  i.drag = ToVec(drag);
  i.dragAccel = dragAccel;
  i.raceDecel = raceDecel;
  i.thrusterTensor = ToVec(thrusterTensor);
  i.inertia.valid = inertiaX.W > 0.5f;
  i.inertia.x = ToVec(inertiaX);
  i.inertia.y = ToVec(inertiaY);
  i.inertia.z = ToVec(inertiaZ);
  return i;
}

RED4ext::Vector4 FlightAttitudeControl::EaseBank(float aDt, RED4ext::Vector4 aFwd, RED4ext::Vector4 aUp) {
  Navigation::EaseState state = EaseStateOf();
  const Flight::Vec3 out = Navigation::EaseBank(state, EaseParamsOf(), aDt, ToVec(aFwd), ToVec(aUp));
  bankAngle = state.bankAngle;
  bankRateNow = state.bankRateNow;
  return ToDirection(out);
}

RED4ext::Vector4 FlightAttitudeControl::EasePitch(float aDt, RED4ext::Vector4 aFwd, bool aOnLine,
                                                  RED4ext::Vector4 aTangent, RED4ext::Vector4 aHeadingRef) {
  Navigation::EaseState state = EaseStateOf();
  const Flight::Vec3 out = Navigation::EasePitch(state, EaseParamsOf(), aDt, ToVec(aFwd), aOnLine, ToVec(aTangent),
                                                 ToVec(aHeadingRef));
  pitchAngle = state.pitchAngle;
  pitchRateNow = state.pitchRateNow;
  return ToDirection(out);
}

void FlightAttitudeControl::Step(RED4ext::Handle<FlightStats> aStats) {
  if (!aStats) {
    torque = thrust = rateRef = ToDirection({});
    attitudeError = 0.0f;
    return;
  }
  Navigation::AttitudeControlState state = State();
  const Navigation::AttitudeControlOutput out =
      Navigation::AttitudeControl(state, Params(), aStats->Static(), aStats->State(), Input());
  targetRight = ToDirection(state.targetRight);
  targetForward = ToDirection(state.targetForward);
  targetUp = ToDirection(state.targetUp);
  omegaRef = ToDirection(state.omegaRef);
  bankAngle = state.bankAngle;
  bankRateNow = state.bankRateNow;
  torque = ToDirection(out.torque);
  thrust = ToDirection(out.thrust);
  rateRef = ToDirection(out.rateRef);
  attitudeError = out.attitudeError;
}
