#pragma once

// Image sources (docs/design/audio-native.md D.9): a surface is a plane, the echo off it is heard from
// the car's mirror image behind the plane, and its path is source to image to listener. Pure maths
// for Targets() (Reflections.cpp) and the echo voices' positions; no game or FMOD types.

#include "Reflections.hpp"

namespace Acoustics {

// A surface: a point on it and its unit normal, pointing from the surface toward the car.
struct Plane {
  Vec3 point, normal;
};

// p mirrored in the plane.
Vec3 Mirror(const Vec3 &aPoint, const Plane &aSurface);

// The echo of aSource heard at aListener off aSurface: delay over the direct sound, the rate that
// delay changes with aVelocity (the car and listener moving past a still surface), and the gain
// against the dry sound at Tuning::nearDistance (0 when not aInRange).
Echo EchoFor(const Plane &aSurface, const Vec3 &aSource, const Vec3 &aListener, const Vec3 &aVelocity,
             float aReflectivity, const Tuning &aTuning, bool aInRange);

// The surface a probe found aDistance from aSource along aOutward (a unit vector), seen square on.
Plane SquareOn(const Vec3 &aSource, const Vec3 &aOutward, float aDistance);

} // namespace Acoustics
