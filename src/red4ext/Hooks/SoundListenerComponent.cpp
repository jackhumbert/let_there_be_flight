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
REGISTER_FLIGHT_HOOK_HASH(void, 3465549286, SetListenerVectors, void *self, Vector4 & position, Vector4 & zAxis, Vector4 & yAxis) {
  if (!skip) {
    SetListenerVectors_Original(self, position, zAxis, yAxis);
  }
}