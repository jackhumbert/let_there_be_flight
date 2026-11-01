#include "FlightCinematicShot.hpp"

#include "Cinematic.hpp"
#include "Navigation/NavMap.hpp"
#include "Utils/Utils.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/WorldPosition.hpp>
#include <RED4ext/Scripting/Utils.hpp>

namespace {
constexpr float kFixedPoint = 1.0f / 131072.0f;

// the game's WorldTransform natives, resolved once (the script's WorldTransform.SetOrientationFromDir and
// WorldTransform.TransformPoint)
struct Natives {
  RED4ext::CBaseFunction *setOrientationFromDir = nullptr;
  RED4ext::CBaseFunction *transformPoint = nullptr;
};

const Natives &Resolve() {
  static const Natives natives = [] {
    Natives n;
    if (auto cls = RED4ext::CRTTISystem::Get()->GetClass("WorldTransform")) {
      n.setOrientationFromDir = cls->GetFunction("SetOrientationFromDir");
      n.transformPoint = cls->GetFunction("TransformPoint");
    }
    if (!n.setOrientationFromDir) {
      LTBF_WARN_ONCE("[FlightCinematicShot] WorldTransform.SetOrientationFromDir was not found; the assumed "
                     "convention (Cinematic::OrientationFromDir) is in use");
    }
    if (!n.transformPoint) {
      LTBF_WARN_ONCE("[FlightCinematicShot] WorldTransform.TransformPoint was not found; the assumed convention "
                     "(Cinematic::TransformPoint) is in use");
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

RED4ext::WorldTransform FlightCinematicShot::AimAtCar(RED4ext::WorldTransform aXform, RED4ext::Vector4 aCentreOfMass) {
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
  const Cinematic::Pose after = Cinematic::AimAtCar(before, ToVec(aCentreOfMass), orient);
  aXform.Orientation = {after.orientation.i, after.orientation.j, after.orientation.k, after.orientation.r};
  const bool moved = after.position.x != before.position.x || after.position.y != before.position.y ||
                     after.position.z != before.position.z;
  if (moved) {
    aXform.Position = RED4ext::WorldPosition(ToPoint(after.position));
  }
  return aXform;
}

float FlightCinematicShot::FitAhead(RED4ext::WorldTransform aShot, RED4ext::Vector4 aOffset, float aScale) {
  const Natives &n = Resolve();
  // the game's TransformPoint of the shot's own transform; the point's W is the script's
  const Cinematic::TransformPointFn transform = [&](const Cinematic::Pose &aPose, const Flight::Vec3 &aPoint) {
    if (n.transformPoint) {
      RED4ext::Vector4 point{aPoint.x, aPoint.y, aPoint.z, aOffset.W};
      RED4ext::WorldPosition out{};
      if (RED4ext::ExecuteFunction(kNone, n.transformPoint, &out, aShot, point)) {
        return ToVec(out);
      }
      LTBF_WARN_ONCE("[FlightCinematicShot] WorldTransform.TransformPoint failed; the assumed convention "
                     "(Cinematic::TransformPoint) is in use");
    }
    return Cinematic::TransformPoint(aPose, aPoint);
  };
  // the script's test: the nav map is loaded and the point is not free
  const auto blocked = [](const Flight::Vec3 &aPoint) {
    const auto &map = Navigation::NavMap::Get();
    return map.IsLoaded() && !map.IsFree(aPoint.x, aPoint.y, aPoint.z);
  };
  return Cinematic::FitAhead(ToPose(aShot), ToVec(aOffset), aScale, blocked, transform);
}

RED4ext::Quaternion FlightCinematicShot::OrientationFromDir(RED4ext::Vector4 aDir) {
  const Flight::Quat q = Cinematic::OrientationFromDir(ToVec(aDir));
  return {q.i, q.j, q.k, q.r};
}

RED4ext::Vector4 FlightCinematicShot::TransformPoint(RED4ext::WorldTransform aXform, RED4ext::Vector4 aPoint) {
  return ToPoint(Cinematic::TransformPoint(ToPose(aXform), ToVec(aPoint)));
}
