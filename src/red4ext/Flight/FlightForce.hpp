#pragma once

// The force tail (Flight/Force.hpp) for FlightComponent, which fills the params from FlightSettings and the
// inputs from its fields each frame, calls Step and copies the outputs back.

#include "Flight/FlightStats.hpp"
#include "Flight/Force.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightForce : RED4ext::IScriptable {
  // params (FlightSettings' fields of the same name, defaults as Flight::ForceParams)
  bool physxFlightPhysics = true;
  float generalDampFactorAngular = 2.0f;
  float brakeFactorAngular = 1.0f;
  float generalYawDampFactor = 1.0f;
  float dampStabilityK = 0.19f;
  float dampMaxPerFrame = 1.0f;
  float generalDampFactorAngularMax = 3.5f;
  float generalDampFactorLinear = 0.0f;
  float brakeFactorLinear = 1.0f;
  float generalSwayWithYaw = 0.0f;
  float generalYawDirectionalityFactor = 5.0f;
  float generalPitchDirectionalityFactor = 15.0f;
  float generalYawAeroFactor = 0.0f;
  float generalPitchAeroFactor = 0.0f;
  float collisionRecoveryDelay = 0.8f;
  float collisionRecoveryDuration = 0.8f;

  // inputs, this frame
  float dt = 0.0f;
  RED4ext::Vector4 force{};
  RED4ext::Vector4 torque{};
  bool applyPhysics = true;
  float pitch = 0.0f;
  float roll = 0.0f;
  float yaw = 0.0f;
  float linearBrake = 0.0f;
  float angularBrake = 0.0f;
  RED4ext::Vector4 thrusterTensor{};
  RED4ext::Vector4 contactSpin{};
  float collisionTimer = 0.0f; // in and out

  // outputs
  RED4ext::Vector4 physicsForce{};
  RED4ext::Vector4 physicsTorque{};
  float linearDamp = 0.0f;
  RED4ext::Vector4 worldForce{};
  RED4ext::Vector4 worldTorque{};

  // Flight::ForceTail from the fields; sets the outputs and collisionTimer. Without stats the outputs are zero.
  void Step(RED4ext::Handle<FlightStats> aStats);

  RTTI_IMPL_TYPEINFO(FlightForce);
  RTTI_IMPL_ALLOCATOR();

private:
  Flight::ForceParams Params() const;
  Flight::ForceInput Input() const;
};

RTTI_DEFINE_CLASS(FlightForce, {
  RTTI_PROPERTY(physxFlightPhysics);
  RTTI_PROPERTY(generalDampFactorAngular);
  RTTI_PROPERTY(brakeFactorAngular);
  RTTI_PROPERTY(generalYawDampFactor);
  RTTI_PROPERTY(dampStabilityK);
  RTTI_PROPERTY(dampMaxPerFrame);
  RTTI_PROPERTY(generalDampFactorAngularMax);
  RTTI_PROPERTY(generalDampFactorLinear);
  RTTI_PROPERTY(brakeFactorLinear);
  RTTI_PROPERTY(generalSwayWithYaw);
  RTTI_PROPERTY(generalYawDirectionalityFactor);
  RTTI_PROPERTY(generalPitchDirectionalityFactor);
  RTTI_PROPERTY(generalYawAeroFactor);
  RTTI_PROPERTY(generalPitchAeroFactor);
  RTTI_PROPERTY(collisionRecoveryDelay);
  RTTI_PROPERTY(collisionRecoveryDuration);
  RTTI_PROPERTY(dt);
  RTTI_PROPERTY(force);
  RTTI_PROPERTY(torque);
  RTTI_PROPERTY(applyPhysics);
  RTTI_PROPERTY(pitch);
  RTTI_PROPERTY(roll);
  RTTI_PROPERTY(yaw);
  RTTI_PROPERTY(linearBrake);
  RTTI_PROPERTY(angularBrake);
  RTTI_PROPERTY(thrusterTensor);
  RTTI_PROPERTY(contactSpin);
  RTTI_PROPERTY(collisionTimer);
  RTTI_PROPERTY(physicsForce);
  RTTI_PROPERTY(physicsTorque);
  RTTI_PROPERTY(linearDamp);
  RTTI_PROPERTY(worldForce);
  RTTI_PROPERTY(worldTorque);
  RTTI_METHOD(Step);
});
