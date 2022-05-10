#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>

namespace RED4ext {
namespace vehicle {

// E9 FB C3 65 FF CC CC CC
constexpr uintptr_t DriveHelper_GetNativeType = 0x141D12EF0 - RED4ext::Addresses::ImageBase;

// 40 53 48 83 EC 20 48 8D  05 8B 28 70 01 48 8B D9
constexpr uintptr_t DriveHelper_dtor = 0x141D30500 - RED4ext::Addresses::ImageBase;

struct DriveHelper {
  virtual RED4ext::CClass *GetNativeType();
  virtual uintptr_t dtor(char a2);
  virtual void PhysicsUpdate(RED4ext::vehicle::BaseObject *, float) = 0;
  virtual void sub_18();
};
RED4EXT_ASSERT_SIZE(DriveHelper, 0x8);
// char (*__kaboom)[sizeof(Helper)] = 1;

} // namespace vehicle
} // namespace RED4ext

#ifdef RED4EXT_HEADER_ONLY
#include "DriveHelper-inl.hpp"
#endif