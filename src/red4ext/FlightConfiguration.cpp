#include "FlightConfiguration.hpp"
#include <RED4ext/Memory/Allocators.hpp>

void IFlightConfiguration::Setup(RED4ext::vehicle::BaseObject * vehicle) {
  this->thrusters = RED4ext::DynArray<RED4ext::Handle<IFlightThruster>>(new RED4ext::Memory::DefaultAllocator());
  auto onInit = GetType()->GetFunction("OnSetup");
  if (onInit) {
    auto rtti = RED4ext::CRTTISystem::Get();
    RED4ext::CStackType args[1];
    auto handle = RED4ext::Handle<RED4ext::vehicle::BaseObject>(vehicle);
    args[0] = RED4ext::CStackType(rtti->GetType("handle:vehicleBaseObject"), &handle);
    auto stack = RED4ext::CStack(this, args, 1, nullptr, 0);
    onInit->Execute(&stack);
  }
}

void IFlightConfiguration::AddSlots(RED4ext::ent::SlotComponent *slotComponent) {
  auto rtti = RED4ext::CRTTISystem::Get();

  auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
  slot->boneName = this->flightCameraBone;
  slot->slotName = "CustomFlightCamera";
  slot->relativePosition = this->flightCameraOffset;
  slotComponent->slots.EmplaceBack(*slot);
  slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);

  //for (auto thruster : thrusters) {
  //  auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
  //  slot->boneName = thruster->boneName;
  //  slot->slotName = thruster->slotName;
  //  slot->relativePosition = thruster->relativePosition;
  //  slot->relativeRotation = thruster->relativeRotation;
  //  //slot->relativeRotation = RED4ext::Quaternion(0.0, 0.0, 0.0, 1.0);
  //  slotComponent->slots.EmplaceBack(*slot);
  //  slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);
  //}
}

void IFlightConfiguration::AddMeshes(RED4ext::ent::Entity *entity, RED4ext::ent::VisualControllerComponent *vcc) {
  for (auto thruster : thrusters) {
    auto mesh = CreateThrusterEngine(thruster->meshPath, thruster->meshName, thruster->slotName);
    thruster->meshComponent = RED4ext::Handle<RED4ext::ent::MeshComponent>(mesh);
    entity->componentsStorage.components.EmplaceBack(thruster->meshComponent);
    AddToController(vcc, mesh);
  }
}