#include "FlightManeuverShape.hpp"

namespace {
Flight::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
RED4ext::Vector4 ToDirection(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }
// a position: W 1, as the script's origin (the caller's position) and the samples built on it
RED4ext::Vector4 ToPoint(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z, 1.0f}; }
} // namespace

Navigation::Maneuver FlightManeuverShape::Shape() const {
  Navigation::Maneuver m;
  m.kind = static_cast<Navigation::ManeuverKind>(kind);
  m.side = side;
  m.duration = duration;
  m.origin = ToVec(origin);
  m.dir = ToVec(dir);
  m.right = ToVec(right);
  m.speed = speed;
  m.radius = radius;
  m.rollTime = rollTime;
  return m;
}

void FlightManeuverShape::Create(int32_t aKind, float aSide, RED4ext::Vector4 aPosition, RED4ext::Vector4 aVelocity,
                                 float aAccel, float aRollTime) {
  const Navigation::Maneuver m = Navigation::CreateManeuver(static_cast<Navigation::ManeuverKind>(aKind), aSide,
                                                            ToVec(aPosition), ToVec(aVelocity), aAccel, aRollTime);
  kind = static_cast<int32_t>(m.kind);
  side = m.side;
  duration = m.duration;
  origin = ToPoint(m.origin);
  dir = ToDirection(m.dir);
  right = ToDirection(m.right);
  speed = m.speed;
  radius = m.radius;
  rollTime = m.rollTime;
}

void FlightManeuverShape::SampleAt(float aT) {
  Flight::Vec3 p, v, a, f, u;
  Navigation::SampleManeuver(Shape(), aT, p, v, a, f, u);
  pos = ToPoint(p);
  vel = ToDirection(v);
  acc = ToDirection(a);
  fwd = ToDirection(f);
  up = ToDirection(u);
}

void FlightManeuverShape::BarrelAt(float aT) {
  Flight::Vec3 o, ov, u;
  Navigation::BarrelOffset(Shape(), aT, o, ov, u);
  offset = ToDirection(o);
  offsetVelocity = ToDirection(ov);
  up = ToDirection(u);
}

RED4ext::Vector4 FlightManeuverShape::RollUpAt(float aT, RED4ext::Vector4 aF, RED4ext::Vector4 aU) {
  return ToDirection(Navigation::RollUp(Shape(), aT, ToVec(aF), ToVec(aU)));
}
