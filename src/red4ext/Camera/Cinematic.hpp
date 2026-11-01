#pragma once

// The cinematic camera's shot maths (src/redscript/Extensions/vehicleCinematicCamera.reds): FlightCinematic.AimAtCar,
// the fixed shot's fit loop and initial scale, and the distance stop's limit, as pure functions over the script's own
// vector maths (Flight/ScriptMath.hpp), with the script's literals and operand order. The game's
// WorldTransform.SetOrientationFromDir and WorldTransform.TransformPoint are parameters, so the binding passes the
// game's own natives; the C++ versions here assume a convention (Y forward, Z up, no roll) and serve the tests and
// the defaults. Whether the nav map is loaded and free is the binding's `blocked`.

#include "Flight/ScriptMath.hpp"

#include <functional>

namespace Cinematic {

struct Pose {
  Flight::Vec3 position;
  Flight::Quat orientation{0.0f, 0.0f, 0.0f, 1.0f};
};

constexpr float kMinDistance = 4.5f; // FlightCinematic.MinDistance
constexpr float kRoadSpeed = 20.0f;  // FlightCinematic.RoadSpeed
constexpr float kMaxScale = 5.0f;    // the clamp's top in GetRootTransform
constexpr float kShrink = 0.8f;      // the fit loop's step
constexpr float kReachMargin = 1.25f; // the distance stop's margin over the camera's start distance

using OrientFromDir = std::function<Flight::Quat(const Flight::Vec3 &)>;
using TransformPointFn = std::function<Flight::Vec3(const Pose &, const Flight::Vec3 &)>;

// WorldTransform.SetOrientationFromDir, assumed: dir is the Y axis, the X axis is dir x world up, the horizon level.
// Only called with |dir.z| / |dir| < 0.98.
Flight::Quat OrientationFromDir(const Flight::Vec3 &dir);

// WorldTransform.TransformPoint, assumed: position + orientation * v.
Flight::Vec3 TransformPoint(const Pose &p, const Flight::Vec3 &v);

// FlightCinematic.AimAtCar: `com` is the car's centre of mass in the world. Too close pushes the camera out along the
// line to the car; the orientation is set from the line as it was before that, unless it is near vertical.
Pose AimAtCar(Pose camera, const Flight::Vec3 &com, const OrientFromDir &orient = OrientationFromDir);

// The fixed shot's first scale: |speed| / kRoadSpeed clamped to [1, kMaxScale].
float InitialScale(float speed);

// The fit loop of cameraShotRoot_FixedShot.GetRootTransform: shrinks the scale while the point `offset` with its Y
// scaled lands where `blocked` says (the binding's IsLoaded && !IsFree). Returns the scale.
float FitAhead(const Pose &shot, const Flight::Vec3 &offset, float scale,
               const std::function<bool(const Flight::Vec3 &)> &blocked, const TransformPointFn &transform = TransformPoint);

// The distance stop's limit for a fixed shot in flight: the stop's own maximum or 1.25 times where the camera started.
float StopLimit(float maxDistanceFromCamera, float reach);

// Evaluate for a fixed shot in flight: the car is at least the limit from the camera.
bool DistanceStop(const Flight::Vec3 &vehicle, const Flight::Vec3 &camera, float maxDistanceFromCamera, float reach);

} // namespace Cinematic
