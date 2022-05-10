#pragma once

#include "FlightHelper.hpp"
#include <RED4ext/Addresses.hpp>
#include <RED4ext/DynArray.hpp>
#include <RED4ext/ISerializable.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Relocation.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/VehiclePhysicsStruct.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>

namespace vehicle {
namespace flight {

struct Helper;

struct HelperWrapper : RED4ext::IScriptable {
  virtual RED4ext::CClass *GetNativeType();

  static void RegisterTypes();
  static void RegisterFunctions();

  Helper *helper;
  RED4ext::Vector4 force;
  RED4ext::Vector4 torque;
};
RED4EXT_ASSERT_SIZE(HelperWrapper, 0x68);
//char (*__kaboom)[sizeof(HelperWrapper)] = 1;

} // namespace flight
} // namespace vehicle