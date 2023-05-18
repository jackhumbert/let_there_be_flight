// #include "Red/TypeInfo/Macros/Definition.hpp"
// #include <RED4ext/Common.hpp>
// #include <RED4ext/RTTISystem.hpp>
// #include <RED4ext/Scripting/Natives/Generated/vehicle/TPPCameraComponent.hpp>
// #include <RedLib.hpp>

// struct VehicleTPPCameraComponent : RED4ext::vehicle::TPPCameraComponent {
//     RTTI_IMPL_TYPEINFO(VehicleTPPCameraComponent);
//     RTTI_FWD_CONSTRUCTOR();
//     // RTTI_IMPL_ALLOCATOR();
// };

// RTTI_DEFINE_CLASS(RED4ext::vehicle::TPPCameraComponent, "vehicleTPPCameraComponent", {
//   // RTTI_ABSTRACT();
//   RTTI_PARENT(RED4ext::game::CameraComponent);
//   RTTI_PROPERTY(drivingDirectionCompensationSpeedCoef);
//   RTTI_PROPERTY(drivingDirectionCompensationAngleSmooth);
//   RTTI_PROPERTY(lockedCamera);
//   RTTI_PROPERTY(initialTransform);
//   RTTI_PROPERTY(pitch);
//   RTTI_PROPERTY(yaw);
//   RTTI_PROPERTY(data);
// });