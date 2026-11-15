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
// REGISTER_FLIGHT_HOOK_HASH(void, 562370452, SoundListenerComponent_OnTransformUpdated, ent::SoundListenerComponent *self, Box & box) {
//   if (!skip) {
//     SoundListenerComponent_OnTransformUpdated_Original(self, box);
//   }
// }

// void audio::SoundSystem::SetListenerVectors(struct Vector4 const &,struct Vector4 const &,struct Vector4 const &)
// (On game 2.5 it is called with the player's soundListener component exactly, yAxis
// forward and zAxis up, in TPP and through the cinematic camera's shots alike; the game does not hear from
// the cinematic camera either. The FMOD listener takes the active camera there instead, FlightComponent.)
// REGISTER_FLIGHT_HOOK_HASH(void, 3465549286, SoundSystem_SetListenerVectors, void *self, Vector4 & position, Vector4 & zAxis, Vector4 & yAxis) {
//   if (!skip) {
//     SoundSystem_SetListenerVectors_Original(self, position, zAxis, yAxis);
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

// bool audio::AudioSoundsLimiter::CanSoundBePlayed(CName, Vector3 const &) const
// REGISTER_FLIGHT_HOOK_HASH(bool, 3803060724, AudioSoundsLimiter_CanSoundBePlayed, Limiter * self, CName name, Vector3 const & position) {
//   return AudioSoundsLimiter_CanSoundBePlayed_Original(self, name, position);
// }

// float utils::CalculateCullingDistanceSquared(uint8_t, audio::LimitedSoundType)
// REGISTER_FLIGHT_HOOK_HASH(float, 3973389091, CalculateCullingDistanceSquared, CName name, uint8_t type) {
//   auto ret = CalculateCullingDistanceSquared_Original(name, type);
//   return ret;
// }
