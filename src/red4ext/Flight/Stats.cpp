#include "Flight/Stats.hpp"

namespace Flight {

void UpdateDynamic(StatsState &s, const StatsInput &in) {
  s.d_lastOrientation = s.d_orientation;
  s.d_orientation = in.orientation;
  s.d_orientationChange = MulInverse(s.d_orientation, s.d_lastOrientation);

  Vec3 angularVelocity = Rotate(Conjugate(s.d_orientation), in.worldAngularVelocity);
  s.d_angularAcceleration = Sub(angularVelocity, s.d_angularVelocity);
  s.d_angularVelocity = angularVelocity;

  Axes(s.d_orientation, s.d_forward, s.d_right, s.d_up);
  s.d_forward2D = Normalize2D(s.d_forward);

  // Assumed equal to the entity position provider's result; see Stats.hpp.
  Vec3 position = Add(in.origin, Rotate(in.orientation, in.centerOfMass));

  Quat inverse = Conjugate(s.d_orientation);
  s.d_velocity = in.velocity;
  s.d_localVelocity = Rotate(inverse, s.d_velocity);
  s.d_localUp = Rotate(inverse, {0.0f, 0.0f, 1.0f});

  s.d_speed = Length(s.d_velocity);
  s.d_speedRatio = s.d_speed / 100.0f;
  s.d_speedRatioSquared = s.d_speedRatio * s.d_speedRatio;
  s.d_speed2D = Length2D(s.d_velocity);
  s.d_direction = Normalize(s.d_velocity);
  s.d_localDirection = Normalize(s.d_localVelocity);
  s.d_direction2D = Normalize2D(s.d_velocity);
  s.d_localDirection2D = Normalize2D(s.d_localVelocity);
  s.d_velocity2D = Mul(s.d_direction2D, s.d_speed2D);

  s.d_visualPosition = s.d_position;
  s.d_position = position;
}

} // namespace Flight
