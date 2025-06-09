#pragma once
#include <RED4ext/Common.hpp>

// math::Matrix math::Quaternion::ToMatrix(void) const
RED4ext::Matrix* __fastcall GetMatrixFromOrientation(RED4ext::Quaternion* q, RED4ext::Matrix* m) {
  RED4ext::UniversalRelocFunc<decltype(&GetMatrixFromOrientation)> call(2124484623);
  return call(q, m);
}