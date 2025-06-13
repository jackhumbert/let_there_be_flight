#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/EInputKey.hpp>

// bool game::input::ManagerBackend::IsKey(uint16_t)
// REGISTER_FLIGHT_HOOK_HASH(bool, 989204148, ManagerBackend_IsKey, uint16_t key) {
//   bool result = ManagerBackend_IsKey_Original(key);
//   return result && (key - (uint16_t)RED4ext::EInputKey::IK_JoyX) > 3 && (key - (uint16_t)RED4ext::EInputKey::IK_JoyU) > 3;
// }