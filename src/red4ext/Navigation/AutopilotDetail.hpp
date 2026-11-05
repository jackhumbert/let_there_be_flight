#pragma once

// Helpers the Autopilot's .cpp files share (not part of its interface): the two vector types back and
// forth, and the script's Vector4 and float natives the Autopilot calls, with their edge cases.

#include "Planner.hpp"

#include "Flight/ScriptMath.hpp"

#include <algorithm>
#include <cmath>

namespace Navigation::AutopilotDetail {

constexpr float kTurn = 6.2831853f;
constexpr float kHalfTurn = 3.1415927f;

inline Vec3 N(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z}; }
inline Flight::Vec3 F(const Vec3 &aV) { return {aV.x, aV.y, aV.z}; }

// ClampF, MinF and MaxF as the script has them
inline float Clamp(float aV, float aLo, float aHi) { return std::min(std::max(aV, aLo), aHi); }
inline float Deg2Rad(float aDeg) { return aDeg * kHalfTurn / 180.0f; }
inline float Rad2Deg(float aRad) { return aRad * 180.0f / kHalfTurn; }

inline Vec3 Add(const Vec3 &a, const Vec3 &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 Sub(const Vec3 &a, const Vec3 &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 Mul(const Vec3 &v, float k) { return {v.x * k, v.y * k, v.z * k}; }
inline float Dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float Length(const Vec3 &v) { return std::sqrt(Dot(v, v)); }
inline float Length2D(const Vec3 &v) { return std::sqrt(v.x * v.x + v.y * v.y); }
inline float Distance(const Vec3 &a, const Vec3 &b) { return Length(Sub(a, b)); }
inline float Distance2D(const Vec3 &a, const Vec3 &b) { return Length2D(Sub(a, b)); }
inline float DistanceSquared2D(const Vec3 &a, const Vec3 &b) {
  const float dx = a.x - b.x, dy = a.y - b.y;
  return dx * dx + dy * dy;
}
// Vector4.Normalize: a zero vector stays zero, no threshold
inline Vec3 Normalize(const Vec3 &v) { return N(Flight::Normalize(F(v))); }
// Vector4.Normalize2D: x and y over the horizontal length, Z kept
inline Vec3 Normalize2D(const Vec3 &v) { return N(Flight::Normalize2D(F(v))); }
// FlightModeAutopilot.Flat: the horizontal unit vector (zero if v is vertical)
inline Vec3 Flat(const Vec3 &v) {
  const float length = std::sqrt(v.x * v.x + v.y * v.y);
  return length > 0.0001f ? Vec3{v.x / length, v.y / length, 0.0f} : Vec3{0.0f, 0.0f, 0.0f};
}

} // namespace Navigation::AutopilotDetail
