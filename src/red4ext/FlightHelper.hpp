#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Addresses.hpp>
#include <RED4ext/Relocation.hpp>
#include <RED4ext/ISerializable.hpp>
#include <RED4ext/DynArray.hpp>
#include <RED4ext/RTTITypes.hpp>
#include "DriveHelper.hpp"
#include "FlightHelperWrapper.hpp"

namespace vehicle {
namespace flight {

struct HelperWrapper;

struct Helper : RED4ext::vehicle::DriveHelper {
  virtual RED4ext::CClass *GetNativeType();
  virtual void PhysicsUpdate(RED4ext::vehicle::BaseObject *, float);
  static RED4ext::Handle<HelperWrapper> AddToDriverHelpers(RED4ext::DynArray<uintptr_t> *ra); //, RED4ext::ScriptInstance fc);

  HelperWrapper *wrapper;
};
RED4EXT_ASSERT_SIZE(Helper, 0x10);
 //char (*__kaboom)[sizeof(Helper)] = 1;

} // namespace flight
} // namespace vehicle