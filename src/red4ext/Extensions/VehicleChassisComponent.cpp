// #include "Red/TypeInfo/Macros/Definition.hpp"
// #include <RED4ext/Common.hpp>
// #include <RED4ext/RTTISystem.hpp>
// #include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
// #include <RED4ext/Scripting/Natives/Generated/vehicle/ChassisComponent.hpp>
// #include <RED4ext/Scripting/Natives/Generated/physics/SystemBody.hpp>
// #include <RED4ext/Scripting/Natives/Generated/physics/SystemBodyParams.hpp>
// #include <RedLib.hpp>

// struct VehicleChassisComponent : RED4ext::vehicle::ChassisComponent {
//   RED4ext::Transform GetComOffset() {
//     auto hpsb = this->collisionResource.Fetch().GetPtr()->bodies[0];
//     return hpsb->params.comOffset;
//   }
//   RTTI_IMPL_TYPEINFO(VehicleChassisComponent);
//   RTTI_FWD_CONSTRUCTOR();
//   // RTTI_IMPL_ALLOCATOR();
// };

// RTTI_DEFINE_CLASS(VehicleChassisComponent, "vehicleChassisComponent", {
//   // RTTI_ABSTRACT();
//   RTTI_PARENT(RED4ext::ent::IPlacedComponent);
//   RTTI_METHOD(GetComOffset);
// });