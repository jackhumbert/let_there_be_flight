#pragma once

// The game's script natives on Vector4 and Quaternion, reproduced with their edge cases so native code
// computes what the script computes. The edge cases come from the game's code (each function is found by
// the SHA-256 of its mangled name in bin/x64/cyberpunk2077_addresses.json).
// Natives covered:
//   Vector4.Normalize, Vector4.Normalize2D, Vector4.GetAngleBetween, Vector4.GetAngleDegAroundAxis
//   (-> MathUtils::VectorUtils::GetAngleRadAroundAxis), Vector4 add / sub / scale / negate / dot / cross,
//   Quaternion * Quaternion, Quaternion * Vector4, Quaternion.MulInverse, Quaternion.GetAxes,
//   and the math acos at 0x20fcf0 (relative to the module base).
// All of them use xyz only (the dot products are dpps with mask 0x7f); W is never read or written.
// Assumed from one reading of the code and not yet compared with a value from the game: the acos clamp,
// the 57.2957763671875 degree constant, the 1e-8 projection guard and the sign of GetAngleDegAroundAxis.
// Use these where native code must match a script result; Navigation::V (Vec3Ops.hpp) is the
// Navigation code's own maths, with its own thresholds.

#include <cmath>

namespace Flight {

struct Vec3 {
  float x = 0, y = 0, z = 0;
};

// RED4ext::Quaternion layout. All zero like a script default, not identity.
struct Quat {
  float i = 0, j = 0, k = 0, r = 0;
};

inline Vec3 Add(const Vec3 &a, const Vec3 &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 Sub(const Vec3 &a, const Vec3 &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 Mul(const Vec3 &v, float k) { return {v.x * k, v.y * k, v.z * k}; }
inline Vec3 Neg(const Vec3 &v) { return {-v.x, -v.y, -v.z}; }
inline float Dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(const Vec3 &a, const Vec3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float Length(const Vec3 &v) { return std::sqrt(Dot(v, v)); }
inline float Length2D(const Vec3 &v) { return std::sqrt(v.x * v.x + v.y * v.y); }

// Vector4.Normalize: a zero vector comes back unchanged, and there is no small-length threshold
// (unlike Navigation::V::Normalize).
inline Vec3 Normalize(const Vec3 &v) {
  float l = Length(v);
  if (l == 0.0f)
    return v;
  return {v.x / l, v.y / l, v.z / l};
}

// Vector4.Normalize2D: divides x and y by the horizontal length and keeps Z; unchanged when that length is 0.
inline Vec3 Normalize2D(const Vec3 &v) {
  float l = Length2D(v);
  if (l == 0.0f)
    return v;
  return {v.x / l, v.y / l, v.z};
}

inline Quat Conjugate(const Quat &q) { return {-q.i, -q.j, -q.k, q.r}; }

// Quaternion * Quaternion, Hamilton product.
inline Quat Mul(const Quat &a, const Quat &b) {
  return {a.r * b.i + a.i * b.r + a.j * b.k - a.k * b.j, a.r * b.j - a.i * b.k + a.j * b.r + a.k * b.i,
          a.r * b.k + a.i * b.j - a.j * b.i + a.k * b.r, a.r * b.r - a.i * b.i - a.j * b.j - a.k * b.k};
}

// Quaternion.MulInverse, a * conjugate(b). It only feeds FlightStats.d_orientationChange, which nothing
// reads; the game's operand order is assumed.
inline Quat MulInverse(const Quat &a, const Quat &b) { return Mul(a, Conjugate(b)); }

// Quaternion * Vector4: v + 2 r (u x v) + u x (2 (u x v)) with u the quaternion's xyz.
inline Vec3 Rotate(const Quat &q, const Vec3 &v) {
  Vec3 u{q.i, q.j, q.k};
  Vec3 t = Mul(Cross(u, v), 2.0f);
  return Add(Add(v, Mul(t, q.r)), Cross(u, t));
}

// Quaternion.GetAxes: the rotated Y (forward), X (right) and Z (up).
inline void Axes(const Quat &q, Vec3 &aForward, Vec3 &aRight, Vec3 &aUp) {
  aForward = Rotate(q, {0.0f, 1.0f, 0.0f});
  aRight = Rotate(q, {1.0f, 0.0f, 0.0f});
  aUp = Rotate(q, {0.0f, 0.0f, 1.0f});
}

// The math acos: |d| < 1 is acos(d); d >= 1 gives 0 and d <= -1 gives pi (as a float).
inline float AcosClamped(float d) {
  if (std::fabs(d) < 1.0f)
    return std::acos(d);
  if (d > 0.0f)
    return 0.0f;
  return 3.14159274f;
}

// The constant the natives multiply radians by.
constexpr float kRadToDeg = 57.2957763671875f;

// Vector4.GetAngleBetween, in degrees. A zero vector normalizes to itself, so its dot is 0 and the angle 90.
inline float AngleBetweenDeg(const Vec3 &a, const Vec3 &b) {
  return AcosClamped(Dot(Normalize(a), Normalize(b))) * kRadToDeg;
}

// v's part along n. The game returns a constant for a near-zero n; axes are unit, so it is never hit here.
inline Vec3 ProjectOnAxis(const Vec3 &v, const Vec3 &n) {
  float nn = Dot(n, n);
  if (nn < 1e-8f)
    return {0.0f, 0.0f, 0.0f};
  return Mul(n, Dot(v, n) / nn);
}

// Vector4.GetAngleDegAroundAxis: both vectors are projected onto the plane of the axis, and the result is
// the angle from a to b, right-handed about the axis (negative when Dot(axis, ap x bp) < 0; 0 counts as
// positive).
inline float AngleDegAroundAxis(const Vec3 &a, const Vec3 &b, const Vec3 &aAxis) {
  Vec3 ap = Sub(a, ProjectOnAxis(a, aAxis));
  Vec3 bp = Sub(b, ProjectOnAxis(b, aAxis));
  float angle = AcosClamped(Dot(Normalize(ap), Normalize(bp)));
  if (Dot(aAxis, Cross(ap, bp)) < 0.0f)
    angle = -angle;
  return angle * kRadToDeg;
}

} // namespace Flight
