#pragma once

//#ifdef RED4EXT_STATIC_LIB
#include "DriveHelper.hpp"
//#endif

//RED4ext::TTypedClass<RED4ext::vehicle::DriveHelper> helperCls("vehicleDriveHelper");

//RED4EXT_INLINE RED4ext::CClass *RED4ext::vehicle::DriveHelper::GetNativeType() { return &helperCls; }

RED4EXT_INLINE RED4ext::CClass * RED4ext::vehicle::DriveHelper::GetNativeType() {
  RED4ext::RelocFunc<RED4ext::CClass* (*)()> func(RED4ext::vehicle::DriveHelper_GetNativeType);
  return func();
}

RED4EXT_INLINE uintptr_t RED4ext::vehicle::DriveHelper::dtor(char a2) {
  RED4ext::RelocFunc<uintptr_t (*)(DriveHelper*, char)> func(RED4ext::vehicle::DriveHelper_dtor);
  return func(this, a2);
}

RED4EXT_INLINE void RED4ext::vehicle::DriveHelper::sub_18() {}