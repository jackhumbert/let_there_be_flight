#pragma once

// A manoeuvre's geometry in C++ (Navigation/Aerobatics.hpp) for FlightManeuver, which creates the shape once and
// asks it for the roll, barrel and pitch samples.

#include "Aerobatics.hpp"

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightManeuverShape : RED4ext::IScriptable {
  // the shape, as FlightManeuver's fields of the same name (kind is FlightManeuverKind's value)
  int32_t kind = 0;
  float side = 0.0f;
  float duration = 0.0f;
  float speed = 0.0f;
  float radius = 0.0f;
  float rollTime = 0.0f;
  RED4ext::Vector4 origin{};
  RED4ext::Vector4 dir{};
  RED4ext::Vector4 right{};

  // outputs of SampleAt and BarrelAt
  RED4ext::Vector4 pos{};
  RED4ext::Vector4 vel{};
  RED4ext::Vector4 acc{};
  RED4ext::Vector4 fwd{};
  RED4ext::Vector4 up{};
  RED4ext::Vector4 offset{};
  RED4ext::Vector4 offsetVelocity{};

  // Navigation::CreateManeuver; fills the shape fields
  void Create(int32_t aKind, float aSide, RED4ext::Vector4 aPosition, RED4ext::Vector4 aVelocity, float aAccel,
              float aRollTime);
  // pos, vel, acc, fwd and up of a pitch manoeuvre at aT
  void SampleAt(float aT);
  // offset, offsetVelocity and up of a barrel roll at aT
  void BarrelAt(float aT);
  // the up vector turned about aF by the aileron roll at aT
  RED4ext::Vector4 RollUpAt(float aT, RED4ext::Vector4 aF, RED4ext::Vector4 aU);

  RTTI_IMPL_TYPEINFO(FlightManeuverShape);
  RTTI_IMPL_ALLOCATOR();

private:
  Navigation::Maneuver Shape() const;
};

RTTI_DEFINE_CLASS(FlightManeuverShape, {
  RTTI_PROPERTY(kind);
  RTTI_PROPERTY(side);
  RTTI_PROPERTY(duration);
  RTTI_PROPERTY(speed);
  RTTI_PROPERTY(radius);
  RTTI_PROPERTY(rollTime);
  RTTI_PROPERTY(origin);
  RTTI_PROPERTY(dir);
  RTTI_PROPERTY(right);
  RTTI_PROPERTY(pos);
  RTTI_PROPERTY(vel);
  RTTI_PROPERTY(acc);
  RTTI_PROPERTY(fwd);
  RTTI_PROPERTY(up);
  RTTI_PROPERTY(offset);
  RTTI_PROPERTY(offsetVelocity);
  RTTI_METHOD(Create);
  RTTI_METHOD(SampleAt);
  RTTI_METHOD(BarrelAt);
  RTTI_METHOD(RollUpAt);
});
