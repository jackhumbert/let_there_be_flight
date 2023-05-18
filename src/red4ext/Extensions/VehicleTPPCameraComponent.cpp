#include "Red/TypeInfo/Macros/Definition.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
#include <RedLib.hpp>

struct VehicleTPPCameraComponent : RED4ext::vehicle::TPPCameraComponent {
    RTTI_IMPL_TYPEINFO(VehicleTPPCameraComponent);
    RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(VehicleTPPCameraComponent, "vehicleTPPCameraComponent", {
  // RTTI_ABSTRACT();
  RTTI_PARENT(RED4ext::game::CameraComponent);
  RTTI_PROPERTY(VehicleTPPCameraComponent::drivingDirectionCompensationSpeedCoef);
  RTTI_PROPERTY(VehicleTPPCameraComponent::drivingDirectionCompensationAngleSmooth);
  RTTI_PROPERTY(VehicleTPPCameraComponent::lockedCamera);
  RTTI_PROPERTY(VehicleTPPCameraComponent::initialTransform);
  RTTI_PROPERTY(VehicleTPPCameraComponent::pitch);
  RTTI_PROPERTY(VehicleTPPCameraComponent::yaw);
  RTTI_PROPERTY(VehicleTPPCameraComponent::data);
});