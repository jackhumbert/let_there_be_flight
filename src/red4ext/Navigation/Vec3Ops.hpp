#pragma once

// Vector helpers on Planner.hpp's Vec3 for the follower's pure code (the script's Vector4 ops, with
// the same edge cases).

#include "Planner.hpp"

#include <cmath>

namespace Navigation::V {

inline Vec3 Add(const Vec3 &a, const Vec3 &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 Sub(const Vec3 &a, const Vec3 &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 Mul(const Vec3 &v, float k) { return {v.x * k, v.y * k, v.z * k}; }
inline float Dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float Len(const Vec3 &v) { return std::sqrt(Dot(v, v)); }
inline Vec3 Cross(const Vec3 &a, const Vec3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
// zero for a vector shorter than 1e-6
inline Vec3 Normalize(const Vec3 &v) {
  float l = Len(v);
  return l < 1e-6f ? Vec3{0.0f, 0.0f, 0.0f} : Mul(v, 1.0f / l);
}
// The horizontal unit vector of v (zero if v is vertical), as FlightModeAutopilot.Flat.
inline Vec3 Flat(const Vec3 &v) {
  float length = std::sqrt(v.x * v.x + v.y * v.y);
  return length > 0.0001f ? Vec3{v.x / length, v.y / length, 0.0f} : Vec3{0.0f, 0.0f, 0.0f};
}
// u without its part along the unit vector f (FlightAttitude.Across)
inline Vec3 Across(const Vec3 &u, const Vec3 &f) { return Sub(u, Mul(f, Dot(u, f))); }

} // namespace Navigation::V
