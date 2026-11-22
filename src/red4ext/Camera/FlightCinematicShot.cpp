#include "FlightCinematicShot.hpp"

#include "Cinematic.hpp"
#include "Navigation/NavMap.hpp"
#include "Utils/Utils.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/WorldPosition.hpp>
#include <RED4ext/Scripting/Utils.hpp>

#include <algorithm>
#include <cmath>

namespace {
constexpr float kFixedPoint = 1.0f / 131072.0f;

// the game's WorldTransform.SetOrientationFromDir, resolved once
struct Natives {
  RED4ext::CBaseFunction *setOrientationFromDir = nullptr;
};

const Natives &Resolve() {
  static const Natives natives = [] {
    Natives n;
    if (auto cls = RED4ext::CRTTISystem::Get()->GetClass("WorldTransform")) {
      n.setOrientationFromDir = cls->GetFunction("SetOrientationFromDir");
    }
    if (!n.setOrientationFromDir) {
      LTBF_WARN_ONCE("[FlightCinematicShot] WorldTransform.SetOrientationFromDir was not found; the assumed "
                     "convention (Cinematic::OrientationFromDir) is in use");
    }
    return n;
  }();
  return natives;
}

Flight::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
Flight::Vec3 ToVec(const RED4ext::WorldPosition &aP) {
  return {aP.x.Bits * kFixedPoint, aP.y.Bits * kFixedPoint, aP.z.Bits * kFixedPoint};
}
Flight::Quat ToQuat(const RED4ext::Quaternion &aQ) { return {aQ.i, aQ.j, aQ.k, aQ.r}; }
RED4ext::Vector4 ToPoint(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z, 1.0f}; }

Cinematic::Pose ToPose(const RED4ext::WorldTransform &aXform) {
  return {ToVec(aXform.Position), ToQuat(aXform.Orientation)};
}

// global natives and static functions: no instance
const RED4ext::ScriptInstance kNone = nullptr;
} // namespace

RED4ext::WorldTransform FlightCinematicShot::AimAtCar(RED4ext::WorldTransform aXform, RED4ext::Vector4 aCentre,
                                                      RED4ext::Vector4 aVelocity, float aMinDistance) {
  const Natives &n = Resolve();
  // the game's orientation from a direction, through a copy of the camera's transform (the script calls it on the
  // transform itself, so whatever the native keeps of it is kept here too); the assumed one when it cannot be called
  const Cinematic::OrientFromDir orient = [&n, &aXform](const Flight::Vec3 &aDir) {
    if (n.setOrientationFromDir) {
      alignas(16) RED4ext::WorldTransform scratch = aXform;
      RED4ext::Vector4 dir = ToPoint(aDir);
      dir.W = 0.0f;
      if (RED4ext::ExecuteFunction(kNone, n.setOrientationFromDir, nullptr, scratch, dir)) {
        return ToQuat(scratch.Orientation);
      }
      LTBF_WARN_ONCE("[FlightCinematicShot] WorldTransform.SetOrientationFromDir failed; the assumed convention "
                     "(Cinematic::OrientationFromDir) is in use");
    }
    return Cinematic::OrientationFromDir(aDir);
  };

  const Cinematic::Pose before = ToPose(aXform);
  const Flight::Vec3 centre = ToVec(aCentre);
  const Flight::Vec3 aim = Cinematic::AimPoint(before.position, centre, ToVec(aVelocity));
  const Cinematic::Pose after =
      Cinematic::AimAtCarLead(before, centre, aim, orient, std::max(aMinDistance, Cinematic::kMinDistance));
  aXform.Orientation = {after.orientation.i, after.orientation.j, after.orientation.k, after.orientation.r};
  const bool moved = after.position.x != before.position.x || after.position.y != before.position.y ||
                     after.position.z != before.position.z;
  if (moved) {
    aXform.Position = RED4ext::WorldPosition(ToPoint(after.position));
  }
  return aXform;
}

float FlightCinematicShot::FitDistance(RED4ext::Vector4 aCamera, RED4ext::Vector4 aCentre, RED4ext::Quaternion aCar,
                                      RED4ext::Vector4 aHalf, float aFov, float aPrev, float aDt) {
  const float tanHalfV = std::tan(std::clamp(aFov, 5.0f, 150.0f) * 0.5f * 3.14159265f / 180.0f);
  const float target = Cinematic::FitDistanceAnyBank(ToVec(aCamera), ToVec(aCentre), ToQuat(aCar), ToVec(aHalf), tanHalfV);
  return Cinematic::EaseDistance(aPrev, target, aDt);
}

RED4ext::Vector4 FlightCinematicShot::LevelHeading(RED4ext::Vector4 aPrev, RED4ext::Quaternion aCar,
                                                   RED4ext::Vector4 aVelocity, float aDt) {
  const Flight::Vec3 h = Cinematic::LevelHeading(ToVec(aPrev), ToQuat(aCar), ToVec(aVelocity), aDt);
  return {h.x, h.y, h.z, 0.0f};
}

RED4ext::Handle<FlightCinematicFlyby> FlightCinematicShot::FlybyStraight(RED4ext::Vector4 aPosition,
                                                                         RED4ext::Vector4 aVelocity,
                                                                         RED4ext::Vector4 aOffset, int32_t aSide) {
  const Flight::Vec3 velocity = ToVec(aVelocity);
  return CinematicBinding::ToScript(Cinematic::PlanFlyby(Cinematic::StraightAhead(ToVec(aPosition), velocity),
                                                         ToVec(aOffset), Flight::Length(velocity),
                                                         aSide < 0 ? -1 : 1, CinematicBinding::Blocked));
}

bool CinematicBinding::Blocked(const Flight::Vec3 &aPoint) {
  const auto &map = Navigation::NavMap::Get();
  return map.IsLoaded() && !map.IsFree(aPoint.x, aPoint.y, aPoint.z);
}

RED4ext::Handle<FlightCinematicFlyby> CinematicBinding::ToScript(const Cinematic::Flyby &aFlyby) {
  auto out = RED4ext::MakeHandle<FlightCinematicFlyby>();
  out->ok = aFlyby.ok;
  out->start = ToPoint(aFlyby.start);
  out->velocity = {aFlyby.velocity.x, aFlyby.velocity.y, aFlyby.velocity.z, 0.0f};
  out->pass = aFlyby.pass;
  out->side = aFlyby.side;
  return out;
}

RED4ext::Quaternion FlightCinematicShot::OrientationFromDir(RED4ext::Vector4 aDir) {
  const Flight::Quat q = Cinematic::OrientationFromDir(ToVec(aDir));
  return {q.i, q.j, q.k, q.r};
}

RED4ext::Vector4 FlightCinematicShot::TransformPoint(RED4ext::WorldTransform aXform, RED4ext::Vector4 aPoint) {
  return ToPoint(Cinematic::TransformPoint(ToPose(aXform), ToVec(aPoint)));
}
