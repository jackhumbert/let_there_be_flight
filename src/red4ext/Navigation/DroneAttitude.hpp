#pragma once

// The drone style's attitude target (FlightModeAutopilot.DroneAttitude): the nose level along the
// flight path, the body tilted on each axis towards the thrust the commanded acceleration and gravity
// need (DroneBodyAngle), eased like the plane's pitch and bank. Pure code over the script's fields.

#include "Route.hpp"
#include "Trajectory.hpp"
#include "Vehicle.hpp"

namespace Navigation {

// FlightModeAutopilot's fields of the same name
struct AttitudeParams {
  float droneBodyShare = 0.5f, droneTiltTime = 0.2f, attitudeLead = 0.25f, droneYawLead = 0.3f, lineSpan = 10.0f;
  float droneYawFrequency = 3.0f, droneYawRateMax = 1.5f, droneYawAccelMax = 5.0f;
  float racePitchFrequency = 2.5f, racePitchRateMax = 1.0f, racePitchAccelMax = 3.0f;
  float raceRollFrequency = 2.5f, raceRollRateMax = 1.0f, raceRollAccelMax = 3.0f;
  float trajectoryAttitudeLead = 0.0f;
  float droneTakeoffTilt = 10.0f, droneTakeoffIn = 5.0f, droneTakeoffFull = 25.0f, droneTakeoffOut = 50.0f,
        droneTakeoffLength = 250.0f;
  bool dragFeedForward = true; // AutopilotSurge.dragFeedForward
};

// what carries over from one frame to the next
struct AttitudeState {
  float droneYaw = 0.0f, droneYawRate = 0.0f, dronePitch = 0.0f, dronePitchRate = 0.0f, droneRoll = 0.0f,
        droneRollRate = 0.0f;
  Vec3 attAccel{}, attRef{}, headingRef{1.0f, 0.0f, 0.0f};
};

// this frame
struct AttitudeInput {
  float dt = 0.0f;
  Vec3 accel{};                   // the commanded acceleration
  float blend = 0.0f;             // how much the nose follows the path rather than the hover heading
  bool live = false;              // false: the state is set to the target (the first frame)
  Vec3 velocity{};
  float speed = 0.0f;
  Vec3 forward{0.0f, 1.0f, 0.0f}; // the car's nose, where the route has no direction
  bool onLine = false;
  float closestS = 0.0f;
  bool following = false;         // a trajectory is followed and the status is "following"
  bool descending = false;        // the status is "descending" or "landed"
};

struct AttitudeOutput {
  Vec3 forward{}, up{};
};

// into (-pi, pi] by steps of 2 pi
float WrapAngle(float aAngle);
// An angle eased towards aWanted: critically damped at frequency aF, its rate and acceleration limited.
// Returns the new angle, the new rate in aRateOut.
float EaseAngle(float aDt, float aWanted, float aAngle, float aRateIn, float aF, float aRateMax, float aAccelMax,
                float &aRateOut);
// The drag the car loses at its velocity, as acceleration along it, to be thrust back (zero when off or
// below 0.5 m/s).
Vec3 DragFeedForward(const VehicleSpec &aSpec, const Vec3 &aVelocity, bool aOn);
// The heading the nose is wanted at: along the line a moment ahead (turned into the bend by the drift),
// off it along the velocity, held where there is no direction. aRoute and aTrajectory may be null.
float DroneHeadingYaw(const AttitudeState &aState, const AttitudeParams &aParams, const AttitudeInput &aInput,
                      const Route *aRoute, const Trajectory *aTrajectory);
// Eases the state towards aWantedYaw (the landing turn's choice stays with the caller) and the tilt the
// thrust needs, and returns the nose and top directions.
AttitudeOutput DroneAttitude(AttitudeState &aState, const AttitudeParams &aParams, const VehicleSpec &aSpec,
                             const AttitudeInput &aInput, float aWantedYaw, const Route *aRoute,
                             const Trajectory *aTrajectory);

} // namespace Navigation
