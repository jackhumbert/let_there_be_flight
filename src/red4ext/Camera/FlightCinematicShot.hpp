#pragma once

// The cinematic camera's shot maths in C++ (Camera/Cinematic.hpp) for vehicleCinematicCamera.reds: FlightCinematic.
// AimAtCar and the fixed shot's fit loop. The orientation from a direction and the transform of a point are the
// game's own WorldTransform natives, resolved once; the pure C++ versions are only the fallback and the defaults
// the tests use.

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/WorldTransform.hpp>
#include <RedLib.hpp>

struct FlightCinematicShot : RED4ext::IScriptable {
  // Cinematic::AimAtCar: the camera pushed out to the minimum distance from aCentreOfMass (in the world) and aimed
  // at it. The position is written back only when it changed.
  static RED4ext::WorldTransform AimAtCar(RED4ext::WorldTransform aXform, RED4ext::Vector4 aCentreOfMass);
  // Cinematic::FitAhead: the scale shrunk until the shot's point aOffset, its Y scaled, is in open air.
  static float FitAhead(RED4ext::WorldTransform aShot, RED4ext::Vector4 aOffset, float aScale);
  // the pure C++ defaults, for the bridge's comparison with the game's natives
  static RED4ext::Quaternion OrientationFromDir(RED4ext::Vector4 aDir);
  static RED4ext::Vector4 TransformPoint(RED4ext::WorldTransform aXform, RED4ext::Vector4 aPoint);

  RTTI_IMPL_TYPEINFO(FlightCinematicShot);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightCinematicShot, {
  RTTI_METHOD(AimAtCar);
  RTTI_METHOD(FitAhead);
  RTTI_METHOD(OrientationFromDir);
  RTTI_METHOD(TransformPoint);
});
