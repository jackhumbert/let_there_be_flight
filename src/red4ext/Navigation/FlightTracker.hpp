#pragma once

// The Autopilot's follower in C++ (Navigation/Tracker.hpp) for FlightModeAutopilot, which fills the
// params, the inputs and its own state each frame and copies the state back.

#include "FlightRoute.hpp"
#include "FlightTrajectory.hpp"
#include "FlightVehicleModel.hpp"
#include "Tracker.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightTracker : RED4ext::IScriptable {
  // params (defaults as Navigation::TrackerParams)
  float replanOffLine = 8.0f;
  float replanDistance = 30.0f;
  float cornerSpan = 15.0f;
  float cornerOvershoot = 10.0f;
  float collisionHorizon = 1.5f;
  float raceOffLine = 4.0f;
  float lineLead = 0.25f;
  float speedGain = 1.0f;
  float brakeAccel = 6.0f;
  float brakeMax = 9.0f;
  float velocityGain = 1.2f;
  float accelJerk = 20.0f;
  float raceJerk = 60.0f;
  float lineSpring = 1.0f;
  float lineVelocityGain = 2.5f;
  float lineSpan = 10.0f;
  float trajectoryCorrectionMax = 25.0f;
  float slack = 4.0f;
  float dwell = 0.3f;
  bool fullSurge = true;

  // inputs
  float dt = 0.0f;
  float speed = 0.0f;
  float replanTimer = 0.0f;
  RED4ext::Vector4 position{};
  RED4ext::Vector4 velocity{};
  RED4ext::Vector4 forward{0.0f, 1.0f, 0.0f, 0.0f};
  RED4ext::Vector4 aimOffset{};
  RED4ext::Vector4 overlayVelocity{};
  RED4ext::Vector4 target{};
  RED4ext::Vector4 rawAccel{};
  RED4ext::Vector4 rawDesiredVelocity{};
  float rawJerk = 20.0f;
  bool following = false;
  bool routeLine = true;
  bool planned = false;
  bool recovering = false;
  bool requestPending = false;
  bool surgeFree = false;

  // state in and out
  int32_t segment = 0;
  float closestS = 0.0f;
  float offRouteNow = 0.0f;
  float remaining = 0.0f;
  float aheadSmooth = 15.0f;
  float curvatureAhead = 0.0f;
  float blockedTimer = 0.0f;
  float targetSpeed = 0.0f;
  float speedCmd = 0.0f;
  float speedRate = 0.0f;
  RED4ext::Vector4 closestNow{};
  RED4ext::Vector4 pathCurvature{};
  RED4ext::Vector4 accelCmd{};
  bool blockedAhead = false;
  bool emergencyBrake = false;
  bool surgeHeld = false;
  float surgeChanging = 0.0f;

  // outputs
  RED4ext::Vector4 lookahead{};
  RED4ext::Vector4 desiredVelocity{};
  RED4ext::Vector4 accel{};
  bool replanLine = false;
  bool requestRoute = false;
  bool onLine = false;

  // from the car's place on the route through the emergency brake; sets target, the state and the replan flags
  void Track(RED4ext::Handle<FlightRoute> aRoute, RED4ext::Handle<FlightTrajectory> aTrajectory,
             RED4ext::Handle<FlightVehicleModel> aVehicle);
  // from the velocity error to the acceleration through the jerk limit, then the surge hold
  void Command(RED4ext::Handle<FlightRoute> aRoute, RED4ext::Handle<FlightTrajectory> aTrajectory);

  RTTI_IMPL_TYPEINFO(FlightTracker);
  RTTI_IMPL_ALLOCATOR();

private:
  Navigation::TrackerParams Params() const;
  Navigation::TrackerState State() const;
  void SetState(const Navigation::TrackerState &aState);
};

RTTI_DEFINE_CLASS(FlightTracker, {
  RTTI_PROPERTY(replanOffLine);
  RTTI_PROPERTY(replanDistance);
  RTTI_PROPERTY(cornerSpan);
  RTTI_PROPERTY(cornerOvershoot);
  RTTI_PROPERTY(collisionHorizon);
  RTTI_PROPERTY(raceOffLine);
  RTTI_PROPERTY(lineLead);
  RTTI_PROPERTY(speedGain);
  RTTI_PROPERTY(brakeAccel);
  RTTI_PROPERTY(brakeMax);
  RTTI_PROPERTY(velocityGain);
  RTTI_PROPERTY(accelJerk);
  RTTI_PROPERTY(raceJerk);
  RTTI_PROPERTY(lineSpring);
  RTTI_PROPERTY(lineVelocityGain);
  RTTI_PROPERTY(lineSpan);
  RTTI_PROPERTY(trajectoryCorrectionMax);
  RTTI_PROPERTY(slack);
  RTTI_PROPERTY(dwell);
  RTTI_PROPERTY(fullSurge);
  RTTI_PROPERTY(dt);
  RTTI_PROPERTY(speed);
  RTTI_PROPERTY(replanTimer);
  RTTI_PROPERTY(position);
  RTTI_PROPERTY(velocity);
  RTTI_PROPERTY(forward);
  RTTI_PROPERTY(aimOffset);
  RTTI_PROPERTY(overlayVelocity);
  RTTI_PROPERTY(target);
  RTTI_PROPERTY(rawAccel);
  RTTI_PROPERTY(rawDesiredVelocity);
  RTTI_PROPERTY(rawJerk);
  RTTI_PROPERTY(following);
  RTTI_PROPERTY(routeLine);
  RTTI_PROPERTY(planned);
  RTTI_PROPERTY(recovering);
  RTTI_PROPERTY(requestPending);
  RTTI_PROPERTY(surgeFree);
  RTTI_PROPERTY(segment);
  RTTI_PROPERTY(closestS);
  RTTI_PROPERTY(offRouteNow);
  RTTI_PROPERTY(remaining);
  RTTI_PROPERTY(aheadSmooth);
  RTTI_PROPERTY(curvatureAhead);
  RTTI_PROPERTY(blockedTimer);
  RTTI_PROPERTY(targetSpeed);
  RTTI_PROPERTY(speedCmd);
  RTTI_PROPERTY(speedRate);
  RTTI_PROPERTY(closestNow);
  RTTI_PROPERTY(pathCurvature);
  RTTI_PROPERTY(accelCmd);
  RTTI_PROPERTY(blockedAhead);
  RTTI_PROPERTY(emergencyBrake);
  RTTI_PROPERTY(surgeHeld);
  RTTI_PROPERTY(surgeChanging);
  RTTI_PROPERTY(lookahead);
  RTTI_PROPERTY(desiredVelocity);
  RTTI_PROPERTY(accel);
  RTTI_PROPERTY(replanLine);
  RTTI_PROPERTY(requestRoute);
  RTTI_PROPERTY(onLine);
  RTTI_METHOD(Track);
  RTTI_METHOD(Command);
});
