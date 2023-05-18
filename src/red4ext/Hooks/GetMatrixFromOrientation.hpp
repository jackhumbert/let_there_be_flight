#pragma once
#include <RED4ext/Common.hpp>

// 1.52 RVA: 0x1C58B0 / 1857712
// 1.6 RVA: 0x1C9A30 / 1874480
/// @pattern 4C 8B DC 48 81 EC B8 00 00 00 0F 10 21 41 0F 29 73 E8 0F 28 DC 0F 59 DC C7 44 24 0C 00 00 00 00
RED4ext::Matrix* __fastcall GetMatrixFromOrientation(RED4ext::Quaternion* q, RED4ext::Matrix* m) {
  RED4ext::RelocFunc<decltype(&GetMatrixFromOrientation)> call(GetMatrixFromOrientation_Addr);
  return call(q, m);
}