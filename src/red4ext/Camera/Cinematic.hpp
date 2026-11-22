#pragma once

// The cinematic camera's shot maths (src/redscript/Extensions/vehicleCinematicCamera.reds): FlightCinematic.AimAtCar,
// the attached shots' level heading, the flybys' plan and the distance stop's limit, as pure functions over the
// script's own vector maths (Flight/ScriptMath.hpp). The game's WorldTransform.SetOrientationFromDir is a parameter,
// so the binding passes the game's own native; the C++ OrientationFromDir and TransformPoint assume a convention
// (Y forward, Z up, no roll) and serve the tests and the defaults. Whether the nav map has a point solid is the binding's `blocked`.

#include "Flight/ScriptMath.hpp"

#include <functional>

namespace Cinematic {

struct Pose {
  Flight::Vec3 position;
  Flight::Quat orientation{0.0f, 0.0f, 0.0f, 1.0f};
};

// m, closest a camera comes to the car's centre
constexpr float kMinDistance = 4.5f;
constexpr float kReachMargin = 1.25f; // the distance stop's margin over the camera's start distance

using OrientFromDir = std::function<Flight::Quat(const Flight::Vec3 &)>;

// WorldTransform.SetOrientationFromDir, assumed: dir is the Y axis, the X axis is dir x world up, the horizon level.
// Only called with |dir.z| / |dir| < 0.98.
Flight::Quat OrientationFromDir(const Flight::Vec3 &dir);

// WorldTransform.TransformPoint, assumed: position + orientation * v.
Flight::Vec3 TransformPoint(const Pose &p, const Flight::Vec3 &v);

// The camera aimed at the car with no lead: `com` is the car's centre in the world. Too close pushes the camera out
// along the line to the car; the orientation is set along that line unless it is near vertical (AimAtCarLead with
// the aim at `com`).
Pose AimAtCar(Pose camera, const Flight::Vec3 &com, const OrientFromDir &orient = OrientationFromDir);

// The distance stop's limit for a fixed shot in flight: the stop's own maximum or 1.25 times where the camera started.
float StopLimit(float maxDistanceFromCamera, float reach);

// Evaluate for a fixed shot in flight: the car is at least the limit from the camera.
bool DistanceStop(const Flight::Vec3 &vehicle, const Flight::Vec3 &camera, float maxDistanceFromCamera, float reach);

// --- The shots attached to the car: a level frame that turns with the car's heading ---

constexpr float kHeadingTime = 0.4f; // s, the heading's time constant (a turn is followed, a wobble is not)

// The level heading (a horizontal unit vector) a shot attached to the car is placed by: the car's forward made level
// and eased from `prev` with kHeadingTime over aDt. Its velocity, made level, when the car points up or down (forward
// within 17 degrees of vertical); `prev` when both are vertical. A `prev` of zero length (the shot's first frame)
// takes the target at once. The bank and pitch never move the camera: a shot that follows the car's whole attitude
// swings round a banking car.
Flight::Vec3 LevelHeading(const Flight::Vec3 &prev, const Flight::Quat &car, const Flight::Vec3 &velocity, float aDt);

// --- Framing: the car off centre, with room ahead of it ---

constexpr float kLeadAngle = 0.17f; // rad (10 degrees) the camera looks ahead of the car at full lead
constexpr float kLeadSpeed = 10.0f; // m/s across the line of sight for full lead

// The point the camera aims at for the car's centre `centre` moving at `velocity`: ahead of it along its motion
// across the line of sight, by kLeadAngle seen from the camera, scaled down below kLeadSpeed. The car then sits
// behind the frame's centre with room in front of it. `centre` itself when the car is (nearly) still across the line.
Flight::Vec3 AimPoint(const Flight::Vec3 &camera, const Flight::Vec3 &centre, const Flight::Vec3 &velocity);

// AimAtCar with the camera pushed out to `minDistance` (FitDistance) and the orientation set from there towards `aim`
// (AimPoint) instead of the centre.
Pose AimAtCarLead(Pose camera, const Flight::Vec3 &centre, const Flight::Vec3 &aim,
                  const OrientFromDir &orient = OrientationFromDir, float minDistance = kMinDistance);

// of the frame's height the car may fill at most, by the reported field of view, banked as far as it can go
// (FitDistanceAnyBank); level, it fills about half that
constexpr float kFillHeight = 0.65f;
constexpr float kFillWidth = 0.7f;    // and of its width (the lead moves the car a little off centre)
constexpr float kMaxFitDistance = 25.0f; // m, the furthest FitDistance pushes the camera
constexpr float kAspect = 16.0f / 9.0f;

// The least distance from the car's centre, along the line from it to `camera`, at which the car's box (half extents
// `half` in the car's frame `car`, around `centre`) fits within kFillHeight and kFillWidth of a level view looking at
// the centre, the vertical field of view's half-angle tangent `tanHalfV` (the camera's reported field of view; the
// game renders a little wider, so the car fills less). At least kMinDistance, at most kMaxFitDistance; kMinDistance
// when `half` is zero or the line is near vertical. The box as the car sits now: a banked car stands taller in a
// level view, and the end nearest the camera grows fastest as it comes close.
float FitDistance(const Flight::Vec3 &camera, const Flight::Vec3 &centre, const Flight::Quat &car,
                  const Flight::Vec3 &half, float tanHalfV);

// FitDistance for the car at any bank: the box turned with the car's heading only (its forward made level; as it is
// when the forward is near vertical), its width and height both the radius of its cross-section (sqrt(x^2 + z^2)),
// the circle it sweeps rolling about its length. The distance then holds while the car banks: fitted to the box as
// it sat, the camera pumped in and out with every turn.
float FitDistanceAnyBank(const Flight::Vec3 &camera, const Flight::Vec3 &centre, const Flight::Quat &car,
                         const Flight::Vec3 &half, float tanHalfV);

constexpr float kFitOutTime = 0.3f; // s, the time constant the camera backs off with when the car needs more room
constexpr float kFitInTime = 1.5f;  // s, and comes back in with when it needs less

// The fit distance eased from `prev` towards `target` over aDt: out with kFitOutTime, in with kFitInTime. `target` at
// once when `prev` is 0 (the shot's first frame).
float EaseDistance(float prev, float target, float aDt);

// --- Flybys: a camera beside the route ahead that the car passes ---

constexpr float kFlybyRelSpeed = 15.0f;  // m/s, the car's speed past the camera at most: faster, the camera drifts
constexpr float kFlybyPassTime = 3.0f;   // s from the shot's start to the pass, at most
constexpr float kFlybyMinPassTime = 1.5f; // s, the least the route ahead may leave
constexpr float kFlybyCloser = 1.0f;     // s sooner, the second pass time tried
// rad/s, the camera's turn at the pass as planned (relative speed over distance); the car strays from the planned
// line and speed, so it passes a little nearer than planned
constexpr float kFlybyMaxPan = 0.75f;
constexpr float kFlybyMinSide = 6.0f;    // m from the route at the pass, at least
constexpr float kFlybyMaxSide = 30.0f;   // and at most
constexpr float kFlybyRaise = 6.0f;      // m higher, the second try on each side
// m of each sight line nearest the car left out: the nav map has space blocked wherever the car would not fit, so
// round the car itself near a wall or the ground
constexpr float kFlybySightSkip = 6.0f;

// Where the car will be, as the binding knows it: the route distance it reaches aT seconds from now (the Autopilot's
// trajectory, or its speed along a straight line), and the point and the unit direction at a route distance. `end` is
// the last route distance there is.
struct Ahead {
  std::function<float(float)> distanceAfter;
  std::function<Flight::Vec3(float)> at;
  std::function<Flight::Vec3(float)> tangent;
  std::function<float(float)> speedAt;
  float end = 0.0f;
};

// A flyby: the camera starts at `start` and moves at `velocity` (zero when the car is no faster than kFlybyRelSpeed);
// the car passes it `pass` seconds after the start.
struct Flyby {
  bool ok = false;
  Flight::Vec3 start{}, velocity{};
  float pass = 0.0f;
  int side = 1;
};

// The flyby for a shot whose offset is `offset` (shot space: X right, Y ahead, Z up). The pass point is where the car
// will be after min(kFlybyPassTime, what the route leaves) seconds; the camera is beside it, level and square to the
// route, on side `side` (+1 right of the car's travel, -1 left) far enough that the car crosses the camera's view at
// no more than kFlybyMaxPan (|offset.x| at least, kFlybyMinSide to kFlybyMaxSide), at the offset's height. Faster than
// kFlybyRelSpeed, the camera drifts along the route's direction at the pass, so the car passes it at that speed. The
// camera's path over twice the pass time must be clear (`blocked` false for each point on it a metre apart) and so must its
// view of the car at four moments (`blocked` false along each sight line, its last kFlybySightSkip excepted).
// Tried on `side`, then raised by kFlybyRaise, then the same on the other side, then all four again with the pass
// kFlybyCloser sooner; not ok when none is clear or the route leaves less than kFlybyMinPassTime. The speed past the
// pass point is the route's (`speedAt`), or `speed` where that is zero.
Flyby PlanFlyby(const Ahead &ahead, const Flight::Vec3 &offset, float speed, int side,
                const std::function<bool(const Flight::Vec3 &)> &blocked);

// The camera of a flyby aT seconds after its start.
Flight::Vec3 FlybyAt(const Flyby &flyby, float aT);

// The straight line from `position` along `velocity` as an Ahead (no route: the car keeps on as it goes).
Ahead StraightAhead(const Flight::Vec3 &position, const Flight::Vec3 &velocity);

} // namespace Cinematic
