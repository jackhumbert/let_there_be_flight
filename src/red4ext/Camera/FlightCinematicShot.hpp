#pragma once

// The cinematic camera's shot maths in C++ (Camera/Cinematic.hpp) for vehicleCinematicCamera.reds: FlightCinematic.
// AimAtCar, the level heading of the shots attached to the car and the flybys' plan. The orientation from a direction and the transform of a point are the
// game's own WorldTransform natives, resolved once; the pure C++ versions are only the fallback and the defaults
// the tests use.

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/WorldTransform.hpp>
#include <RedLib.hpp>

#include "Camera/Cinematic.hpp"

// A flyby's plan (Cinematic::PlanFlyby) for script: the camera starts at `start` (world) and moves at `velocity` (m/s);
// the car passes it `pass` s after the start, on `side` (+1 right of its travel, -1 left). `ok` false: no clear spot.
struct FlightCinematicFlyby : RED4ext::IScriptable {
  bool ok = false;
  RED4ext::Vector4 start{}, velocity{};
  float pass = 0.0f;
  int32_t side = 1;

  RTTI_IMPL_TYPEINFO(FlightCinematicFlyby);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightCinematicFlyby, {
  RTTI_PROPERTY(ok);
  RTTI_PROPERTY(start);
  RTTI_PROPERTY(velocity);
  RTTI_PROPERTY(pass);
  RTTI_PROPERTY(side);
});

struct FlightCinematicShot : RED4ext::IScriptable {
  // Cinematic::AimAtCarLead: the camera pushed out to aMinDistance from aCentre (the middle of the car's box, in the
  // world) and aimed ahead of it along aVelocity (Cinematic::AimPoint), so the car has room in front. The position is
  // written back only when it changed.
  static RED4ext::WorldTransform AimAtCar(RED4ext::WorldTransform aXform, RED4ext::Vector4 aCentre,
                                          RED4ext::Vector4 aVelocity, float aMinDistance);
  // The distance from aCentre the camera at aCamera keeps (Cinematic::FitDistanceAnyBank for the box of half extents
  // aHalf, the car's orientation aCar, the vertical field of view aFov degrees), eased from aPrev over aDt seconds
  // (Cinematic::EaseDistance; aPrev 0 on the shot's first frame).
  static float FitDistance(RED4ext::Vector4 aCamera, RED4ext::Vector4 aCentre, RED4ext::Quaternion aCar,
                           RED4ext::Vector4 aHalf, float aFov, float aPrev, float aDt);
  // Cinematic::LevelHeading: the level unit heading (W 0) a shot attached to the car is placed by, eased from aPrev
  // (zero on the shot's first frame) towards the car's level forward over aDt seconds.
  static RED4ext::Vector4 LevelHeading(RED4ext::Vector4 aPrev, RED4ext::Quaternion aCar, RED4ext::Vector4 aVelocity,
                                       float aDt);
  // Cinematic::PlanFlyby along the straight line from aPosition at aVelocity (no route to follow), for a shot offset
  // aOffset (shot space), preferring side aSide (+1 right, -1 left)
  static RED4ext::Handle<FlightCinematicFlyby> FlybyStraight(RED4ext::Vector4 aPosition, RED4ext::Vector4 aVelocity,
                                                             RED4ext::Vector4 aOffset, int32_t aSide);
  // the pure C++ versions of the game's natives, so a test can compare the two
  static RED4ext::Quaternion OrientationFromDir(RED4ext::Vector4 aDir);
  static RED4ext::Vector4 TransformPoint(RED4ext::WorldTransform aXform, RED4ext::Vector4 aPoint);

  RTTI_IMPL_TYPEINFO(FlightCinematicShot);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightCinematicShot, {
  RTTI_METHOD(AimAtCar);
  RTTI_METHOD(FitDistance);
  RTTI_METHOD(LevelHeading);
  RTTI_METHOD(FlybyStraight);
  RTTI_METHOD(OrientationFromDir);
  RTTI_METHOD(TransformPoint);
});

// shared with FlightModeAutopilot's route flyby
namespace CinematicBinding {
// the nav map is loaded and has aPoint solid (Cinematic::PlanFlyby's `blocked`)
bool Blocked(const Flight::Vec3 &aPoint);
// a plan as script reads it
RED4ext::Handle<FlightCinematicFlyby> ToScript(const Cinematic::Flyby &aFlyby);
} // namespace CinematicBinding
