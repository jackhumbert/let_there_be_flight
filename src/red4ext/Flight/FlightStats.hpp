#pragma once

// The script class FlightStats (src/redscript/Flight/FlightStats.reds): the fields the plugin reads and fills
// live here, the vehicle, record and position-provider handles stay script fields.

#include "Flight/Stats.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightStats : RED4ext::IScriptable {
  // set by UpdateStatic
  float s_mass = 0.0f;
  float s_forwardWeightTransferFactor = 0.0f;
  float s_brakingFrictionFactor = 0.0f;
  float s_airResistanceFactor = 0.0f;
  RED4ext::Vector3 s_centerOfMass{};
  RED4ext::Vector4 s_momentOfInertia{};

  // set by UpdateDynamic
  RED4ext::Vector4 d_position{};
  RED4ext::Vector4 d_visualPosition{};
  RED4ext::Quaternion d_orientation{0.0f, 0.0f, 0.0f, 0.0f};
  RED4ext::Quaternion d_lastOrientation{0.0f, 0.0f, 0.0f, 0.0f};
  RED4ext::Vector4 d_forward{};
  RED4ext::Vector4 d_right{};
  RED4ext::Vector4 d_up{};
  RED4ext::Vector4 d_localUp{};
  RED4ext::Vector4 d_forward2D{};
  RED4ext::Quaternion d_orientationChange{0.0f, 0.0f, 0.0f, 0.0f};
  RED4ext::Vector4 d_angularVelocity{};
  RED4ext::Vector4 d_angularAcceleration{};
  RED4ext::Vector4 d_velocity{};
  RED4ext::Vector4 d_localVelocity{};
  RED4ext::Vector4 d_velocity2D{};
  float d_speed = 0.0f;
  float d_speedRatio = 0.0f;
  float d_speedRatioSquared = 0.0f;
  float d_speed2D = 0.0f;
  RED4ext::Vector4 d_direction{};
  RED4ext::Vector4 d_localDirection{};
  RED4ext::Vector4 d_direction2D{};
  RED4ext::Vector4 d_localDirection2D{};

  // true when the plugin updates the dynamic half before FlightComponent.OnUpdate this frame (FlightComponent.nativeStats)
  bool nativeUpdate = false;

  // Flight::UpdateDynamic from the vehicle's state, read through the natives the script's UpdateDynamic calls (the
  // current pose). If one cannot be called it warns once, leaves the state and clears nativeUpdate.
  void UpdateNative(RED4ext::vehicle::BaseObject *aVehicle);

  // the d_ fields as plain values, and back (a Vector4's W is left as it is)
  Flight::StatsState State() const;
  // the s_ fields as plain values
  Flight::StatsStatic Static() const;
  void SetState(const Flight::StatsState &aState);

  // the stats of a FlightComponent (its script field), nullptr when it has none
  static FlightStats *Of(RED4ext::IScriptable *aComponent);

  RTTI_IMPL_TYPEINFO(FlightStats);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightStats, {
  RTTI_PROPERTY(s_mass);
  RTTI_PROPERTY(s_centerOfMass);
  RTTI_PROPERTY(s_momentOfInertia);
  RTTI_PROPERTY(s_forwardWeightTransferFactor);
  RTTI_PROPERTY(s_brakingFrictionFactor);
  RTTI_PROPERTY(s_airResistanceFactor);
  RTTI_PROPERTY(d_position);
  RTTI_PROPERTY(d_visualPosition);
  RTTI_PROPERTY(d_orientation);
  RTTI_PROPERTY(d_lastOrientation);
  RTTI_PROPERTY(d_forward);
  RTTI_PROPERTY(d_right);
  RTTI_PROPERTY(d_up);
  RTTI_PROPERTY(d_localUp);
  RTTI_PROPERTY(d_forward2D);
  RTTI_PROPERTY(d_orientationChange);
  RTTI_PROPERTY(d_angularVelocity);
  RTTI_PROPERTY(d_angularAcceleration);
  RTTI_PROPERTY(d_velocity);
  RTTI_PROPERTY(d_localVelocity);
  RTTI_PROPERTY(d_velocity2D);
  RTTI_PROPERTY(d_speed);
  RTTI_PROPERTY(d_speedRatio);
  RTTI_PROPERTY(d_speedRatioSquared);
  RTTI_PROPERTY(d_speed2D);
  RTTI_PROPERTY(d_direction);
  RTTI_PROPERTY(d_localDirection);
  RTTI_PROPERTY(d_direction2D);
  RTTI_PROPERTY(d_localDirection2D);
  RTTI_PROPERTY(nativeUpdate);
});
