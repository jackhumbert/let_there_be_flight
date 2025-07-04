#include "Utils/FlightModule.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SoundListenerComponent.hpp>
#include <RED4ext/Scripting/Natives/Box.hpp>

using namespace RED4ext;

static bool skip = false;

// virtual void ent::SoundListenerComponent::OnTransformUpdated(struct Box &)
// REGISTER_FLIGHT_HOOK_HASH(void, 562370452, OnTransformUpdated, ent::SoundListenerComponent *self, Box & box) {
//   if (!skip) {
//     OnTransformUpdated_Original(self, box);
//   }
// }

// void audio::SoundSystem::SetListenerVectors(struct Vector4 const &,struct Vector4 const &,struct Vector4 const &)
// REGISTER_FLIGHT_HOOK_HASH(void, 3465549286, SetListenerVectors, void *self, Vector4 & position, Vector4 & zAxis, Vector4 & yAxis) {
//   if (!skip) {
//     SetListenerVectors_Original(self, position, zAxis, yAxis);
//   }
// }

// REGISTER_FLIGHT_HOOK_HASH(bool, 1645613950, CanSoundBePlayed, Vector3 & position, CName name) {
//   return CanSoundBePlayed_Original(position, name);
// }

// struct Limiter {
//   uint8_t flags;
//   Vector3 position;
//   uint16_t unk10;
//   uint16_t unk12;
//   uint16_t unk14;
//   uint16_t unk16;
//   uint16_t unk18;
//   uint16_t unk1A;
//   uint16_t playingGruntsCount1;
//   uint16_t playingGruntsCount2;
// };

// REGISTER_FLIGHT_HOOK_HASH(bool, 3803060724, Limiter_CanSoundBePlayed, Limiter * self, CName name, Vector3 const & position) {
//   return Limiter_CanSoundBePlayed_Original(self, name, position);
// }

// REGISTER_FLIGHT_HOOK_HASH(float, 3973389091, CalculateCullingDistance, CName name, uint8_t type) {
//   auto ret = CalculateCullingDistance_Original(name, type);
//   return ret;
// }
