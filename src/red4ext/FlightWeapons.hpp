#pragma once

#include <RED4ext/RED4ext.hpp>

namespace FlightWeapons {
void AddWeapons(RED4ext::vehicle::BaseObject *vehicle);
void AddWeaponSlots(RED4ext::ent::SlotComponent *sc);
} // namespace FlightWeapons