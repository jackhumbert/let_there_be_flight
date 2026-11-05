#include "FlightForce.hpp"

#include "Flight/DeveloperSettings.hpp"

namespace {
Flight::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
RED4ext::Vector4 ToDirection(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }
} // namespace

Flight::ForceParams FlightForce::Params() const {
  Flight::ForceParams p;
  p.physxFlightPhysics = physxFlightPhysics;
  p.shapeDrag = Flight::Developer().shapeDrag;
  p.dragK = Flight::Developer().dragK;
  p.generalDampFactorAngular = generalDampFactorAngular;
  p.brakeFactorAngular = brakeFactorAngular;
  p.generalYawDampFactor = generalYawDampFactor;
  p.dampStabilityK = dampStabilityK;
  p.dampMaxPerFrame = dampMaxPerFrame;
  p.generalDampFactorAngularMax = generalDampFactorAngularMax;
  p.generalDampFactorLinear = generalDampFactorLinear;
  p.brakeFactorLinear = brakeFactorLinear;
  p.generalSwayWithYaw = generalSwayWithYaw;
  p.generalYawDirectionalityFactor = generalYawDirectionalityFactor;
  p.generalPitchDirectionalityFactor = generalPitchDirectionalityFactor;
  p.generalYawAeroFactor = generalYawAeroFactor;
  p.generalPitchAeroFactor = generalPitchAeroFactor;
  p.collisionRecoveryDelay = collisionRecoveryDelay;
  p.collisionRecoveryDuration = collisionRecoveryDuration;
  return p;
}

Flight::ForceInput FlightForce::Input() const {
  Flight::ForceInput i;
  i.dt = dt;
  i.force = ToVec(force);
  i.torque = ToVec(torque);
  i.applyPhysics = applyPhysics;
  i.pitch = pitch;
  i.roll = roll;
  i.yaw = yaw;
  i.linearBrake = linearBrake;
  i.angularBrake = angularBrake;
  i.thrusterTensor = ToVec(thrusterTensor);
  i.contactSpin = ToVec(contactSpin);
  i.collisionTimer = collisionTimer;
  return i;
}

void FlightForce::Step(RED4ext::Handle<FlightStats> aStats) {
  if (!aStats) {
    physicsForce = physicsTorque = worldForce = worldTorque = ToDirection({});
    linearDamp = 0.0f;
    return;
  }
  const Flight::ForceOutput out = Flight::ForceTail(Params(), aStats->Static(), aStats->State(), Input());
  physicsForce = ToDirection(out.physicsForce);
  physicsTorque = ToDirection(out.physicsTorque);
  linearDamp = out.linearDamp;
  worldForce = ToDirection(out.worldForce);
  worldTorque = ToDirection(out.worldTorque);
  collisionTimer = out.collisionTimer;
}
