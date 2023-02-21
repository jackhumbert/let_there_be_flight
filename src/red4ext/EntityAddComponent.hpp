#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PhysicalMeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/VisualControllerComponent.hpp>

RED4ext::ent::PhysicalMeshComponent *CreateThrusterEngine(RED4ext::CName mesh, RED4ext::CName name,
                                                          RED4ext::CName slot,
                                                          RED4ext::CName bindName = "vehicle_slots");

void AddToController(RED4ext::ent::VisualControllerComponent *vcc, RED4ext::ent::MeshComponent *mc);