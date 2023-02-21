#include "FlightConfiguration.hpp"
#include <RED4ext/Memory/Allocators.hpp>

void IFlightConfiguration::Setup(FlightComponent * component) {
  this->component = RED4ext::Handle<FlightComponent>(component);
  this->thrusters = RED4ext::DynArray<RED4ext::Handle<IFlightThruster>>(new RED4ext::Memory::DefaultAllocator());
  auto onInit = GetType()->GetFunction("OnSetup");
  if (onInit) {
    auto stack = RED4ext::CStack(this, nullptr, 0, nullptr, 0);
    onInit->Execute(&stack);
  }
}

void IFlightConfiguration::AddSlots(RED4ext::ent::SlotComponent *slotComponent) {
  for (auto thruster : thrusters) {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto slot = reinterpret_cast<RED4ext::ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance());
    slot->boneName = thruster->boneName;
    slot->slotName = thruster->slotName;
    slotComponent->slots.EmplaceBack(*slot);
    slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);
  }
}

void IFlightConfiguration::AddMeshes(RED4ext::ent::Entity *entity, RED4ext::ent::VisualControllerComponent *vcc) {
  for (auto thruster : thrusters) {
    auto mesh = CreateThrusterEngine(thruster->meshPath, thruster->meshName, thruster->slotName);
    thruster->meshComponent = RED4ext::Handle<RED4ext::ent::MeshComponent>(mesh);
    entity->componentsStorage.components.EmplaceBack(thruster->meshComponent);
    AddToController(vcc, mesh);
  }
}