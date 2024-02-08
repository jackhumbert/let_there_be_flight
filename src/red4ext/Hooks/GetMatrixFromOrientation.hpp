#pragma once
#include <RED4ext/Common.hpp>

/// @hash 2124484623
RED4ext::Matrix* __fastcall GetMatrixFromOrientation(RED4ext::Quaternion* q, RED4ext::Matrix* m) {
  RED4ext::RelocFunc<decltype(&GetMatrixFromOrientation)> call(GetMatrixFromOrientation_Addr);
  return call(q, m);
}