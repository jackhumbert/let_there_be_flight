// The chassis collision shape as pieces (Flight/ChassisShape.hpp).
#include "Flight/ChassisShape.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderBox.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderCapsule.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderConvex.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemBody.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/SystemResource.hpp>

namespace Flight {

namespace {

Quat ToQuat(const RED4ext::Quaternion &aQ) { return {aQ.i, aQ.j, aQ.k, aQ.r}; }
Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }

// a point in a frame (position, then orientation) into its parent
Vec3 Place(const Vec3 &aPoint, const Vec3 &aPosition, const Quat &aOrientation) {
  return Add(Rotate(aOrientation, aPoint), aPosition);
}

} // namespace

bool ChassisPieces(RED4ext::vehicle::ChassisComponent *aChassis, std::vector<ShapePiece> &aPieces, std::string &aWhy) {
  aPieces.clear();
  if (!aChassis) {
    aWhy = "no chassis component";
    return false;
  }
  auto &ref = aChassis->collisionResource;
  if (!ref.IsLoaded()) {
    if (ref.IsFailed()) {
      aWhy = "its collision resource failed to load";
      return false;
    }
    ref.LoadAsync();
    aWhy = "its collision resource is still loading";
    return false;
  }
  auto &resource = ref.Get();
  if (!resource) {
    aWhy = "no collision resource";
    return false;
  }
  auto rtti = RED4ext::CRTTISystem::Get();
  static auto *boxClass = rtti->GetClass("physicsColliderBox");
  static auto *convexClass = rtti->GetClass("physicsColliderConvex");
  static auto *sphereClass = rtti->GetClass("physicsColliderSphere");
  static auto *capsuleClass = rtti->GetClass("physicsColliderCapsule");

  const auto &chassisTransform = aChassis->localTransform;
  const Vec3 chassisPosition = ToVec(chassisTransform.Position.AsVector4());
  const Quat chassisOrientation = ToQuat(chassisTransform.Orientation);
  for (const auto &body : resource->bodies) {
    if (!body)
      continue;
    const Vec3 bodyPosition = ToVec(body->localToModel.position);
    const Quat bodyOrientation = ToQuat(body->localToModel.orientation);
    for (const auto &collider : body->collisionShapes) {
      if (!collider)
        continue;
      auto *type = collider->GetType();
      ShapePiece local;
      if (type->IsA(boxClass)) {
        const auto &h = static_cast<RED4ext::physics::ColliderBox *>(collider.GetPtr())->halfExtents;
        for (float x : {-1.0f, 1.0f})
          for (float y : {-1.0f, 1.0f})
            for (float z : {-1.0f, 1.0f})
              local.push_back({x * h.X, y * h.Y, z * h.Z});
      } else if (type->IsA(convexClass)) {
        for (const auto &v : static_cast<RED4ext::physics::ColliderConvex *>(collider.GetPtr())->vertices)
          local.push_back({v.X, v.Y, v.Z});
      } else if (type->IsA(sphereClass)) {
        local = RoundPiece({{}}, static_cast<RED4ext::physics::ColliderSphere *>(collider.GetPtr())->radius);
      } else if (type->IsA(capsuleClass)) {
        // CDPR's capsule lies along its own Z, height the distance between its end spheres' centres (aero.py)
        const auto *c = static_cast<RED4ext::physics::ColliderCapsule *>(collider.GetPtr());
        local = RoundPiece({{0.0f, 0.0f, 0.5f * c->height}, {0.0f, 0.0f, -0.5f * c->height}}, c->radius);
      }
      if (local.size() < 3)
        continue;
      const Vec3 shapePosition = ToVec(collider->localToBody.position);
      const Quat shapeOrientation = ToQuat(collider->localToBody.orientation);
      for (Vec3 &p : local)
        p = Place(Place(Place(p, shapePosition, shapeOrientation), bodyPosition, bodyOrientation), chassisPosition,
                  chassisOrientation);
      aPieces.push_back(std::move(local));
    }
  }
  if (aPieces.empty()) {
    aWhy = "its collision resource has no box, convex, sphere or capsule colliders";
    return false;
  }
  aWhy.clear();
  return true;
}

} // namespace Flight
