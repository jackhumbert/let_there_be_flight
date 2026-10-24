#pragma once
#include <RED4ext/Common.hpp>

#ifdef __APPLE__
#include <cmath>
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>

// LTBF R41: math::Quaternion::ToMatrix (hash 2124484623) is inlined everywhere on macOS (R41b section 3). This is the same math, read
// from Windows 0x1DA684: normalize q (a zero quaternion stays zero), rows X, Y, Z from the usual rotation formula, W = (0, 0, 0, 1).
// macos/tests/matrix_test.cpp checks it against the identity and a 90 degree yaw.
inline RED4ext::Matrix *GetMatrixFromOrientation(RED4ext::Quaternion *q, RED4ext::Matrix *m) {
  float x = q->i, y = q->j, z = q->k, w = q->r;
  const float lengthSquared = x * x + y * y + z * z + w * w;
  if (lengthSquared > 0.0f) {
    const float inverse = 1.0f / std::sqrt(lengthSquared);
    x *= inverse;
    y *= inverse;
    z *= inverse;
    w *= inverse;
  }
  m->X = {1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y + z * w), 2.0f * (x * z - y * w), 0.0f};
  m->Y = {2.0f * (x * y - z * w), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z + x * w), 0.0f};
  m->Z = {2.0f * (x * z + y * w), 2.0f * (y * z - x * w), 1.0f - 2.0f * (x * x + y * y), 0.0f};
  m->W = {0.0f, 0.0f, 0.0f, 1.0f};
  return m;
}
#else
// math::Matrix math::Quaternion::ToMatrix(void) const
RED4ext::Matrix* __fastcall GetMatrixFromOrientation(RED4ext::Quaternion* q, RED4ext::Matrix* m) {
  RED4ext::UniversalRelocFunc<decltype(&GetMatrixFromOrientation)> call(2124484623);
  return call(q, m);
}
#endif
