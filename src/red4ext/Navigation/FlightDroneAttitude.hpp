#pragma once

// The drone attitude in C++ (Navigation/DroneAttitude.hpp) for FlightModeAutopilot, which fills the
// params, the inputs and its own state each frame and copies the state back.

#include "DroneAttitude.hpp"
#include "FlightRoute.hpp"
#include "FlightTrajectory.hpp"
#include "FlightVehicleModel.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightDroneAttitude : RED4ext::IScriptable {
  // params (defaults as Navigation::AttitudeParams)
  float droneBodyShare = 0.5f;
  float droneTiltTime = 0.2f;
  float attitudeLead = 0.25f;
  float droneYawLead = 0.3f;
  float lineSpan = 10.0f;
  float droneYawFrequency = 3.0f;
  float droneYawRateMax = 1.5f;
  float droneYawAccelMax = 5.0f;
  float racePitchFrequency = 2.5f;
  float racePitchRateMax = 1.0f;
  float racePitchAccelMax = 3.0f;
  float raceRollFrequency = 2.5f;
  float raceRollRateMax = 1.0f;
  float raceRollAccelMax = 3.0f;
  float trajectoryAttitudeLead = 0.0f;
  float droneTakeoffTilt = 10.0f;
  float droneTakeoffIn = 5.0f;
  float droneTakeoffFull = 25.0f;
  float droneTakeoffOut = 50.0f;
  float droneTakeoffLength = 250.0f;
  bool dragFeedForward = true;

  // inputs
  float dt = 0.0f;
  float blend = 0.0f;
  float speed = 0.0f;
  float closestS = 0.0f;
  RED4ext::Vector4 accel{};
  RED4ext::Vector4 velocity{};
  RED4ext::Vector4 forward{0.0f, 1.0f, 0.0f, 0.0f};
  bool live = false;
  bool onLine = false;
  bool following = false;
  bool descending = false;

  // state in and out
  float droneYaw = 0.0f;
  float droneYawRate = 0.0f;
  float dronePitch = 0.0f;
  float dronePitchRate = 0.0f;
  float droneRoll = 0.0f;
  float droneRollRate = 0.0f;
  RED4ext::Vector4 attAccel{};
  RED4ext::Vector4 attRef{};
  RED4ext::Vector4 headingRef{1.0f, 0.0f, 0.0f, 0.0f};

  // outputs
  RED4ext::Vector4 fwd{};
  RED4ext::Vector4 up{};

  // the heading the nose is wanted at, before the landing turn's choice
  float HeadingYaw(RED4ext::Handle<FlightRoute> aRoute, RED4ext::Handle<FlightTrajectory> aTrajectory);
  // eases the state towards aWantedYaw and the tilt the thrust needs; sets fwd and up
  void Step(float aWantedYaw, RED4ext::Handle<FlightRoute> aRoute, RED4ext::Handle<FlightTrajectory> aTrajectory,
            RED4ext::Handle<FlightVehicleModel> aVehicle);

  RTTI_IMPL_TYPEINFO(FlightDroneAttitude);
  RTTI_IMPL_ALLOCATOR();

private:
  Navigation::AttitudeParams Params() const;
  Navigation::AttitudeState State() const;
  Navigation::AttitudeInput Input() const;
};

RTTI_DEFINE_CLASS(FlightDroneAttitude, {
  RTTI_PROPERTY(droneBodyShare);
  RTTI_PROPERTY(droneTiltTime);
  RTTI_PROPERTY(attitudeLead);
  RTTI_PROPERTY(droneYawLead);
  RTTI_PROPERTY(lineSpan);
  RTTI_PROPERTY(droneYawFrequency);
  RTTI_PROPERTY(droneYawRateMax);
  RTTI_PROPERTY(droneYawAccelMax);
  RTTI_PROPERTY(racePitchFrequency);
  RTTI_PROPERTY(racePitchRateMax);
  RTTI_PROPERTY(racePitchAccelMax);
  RTTI_PROPERTY(raceRollFrequency);
  RTTI_PROPERTY(raceRollRateMax);
  RTTI_PROPERTY(raceRollAccelMax);
  RTTI_PROPERTY(trajectoryAttitudeLead);
  RTTI_PROPERTY(droneTakeoffTilt);
  RTTI_PROPERTY(droneTakeoffIn);
  RTTI_PROPERTY(droneTakeoffFull);
  RTTI_PROPERTY(droneTakeoffOut);
  RTTI_PROPERTY(droneTakeoffLength);
  RTTI_PROPERTY(dragFeedForward);
  RTTI_PROPERTY(dt);
  RTTI_PROPERTY(blend);
  RTTI_PROPERTY(speed);
  RTTI_PROPERTY(closestS);
  RTTI_PROPERTY(accel);
  RTTI_PROPERTY(velocity);
  RTTI_PROPERTY(forward);
  RTTI_PROPERTY(live);
  RTTI_PROPERTY(onLine);
  RTTI_PROPERTY(following);
  RTTI_PROPERTY(descending);
  RTTI_PROPERTY(droneYaw);
  RTTI_PROPERTY(droneYawRate);
  RTTI_PROPERTY(dronePitch);
  RTTI_PROPERTY(dronePitchRate);
  RTTI_PROPERTY(droneRoll);
  RTTI_PROPERTY(droneRollRate);
  RTTI_PROPERTY(attAccel);
  RTTI_PROPERTY(attRef);
  RTTI_PROPERTY(headingRef);
  RTTI_PROPERTY(fwd);
  RTTI_PROPERTY(up);
  RTTI_METHOD(HeadingYaw);
  RTTI_METHOD(Step);
});
