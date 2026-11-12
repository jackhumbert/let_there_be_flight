#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>

#include <atomic>

namespace VehicleAudio {
// entity id of the flying vehicle (0: none), set by UpdateTireParameters while its flight is on
std::atomic<uint64_t> flyingVehicle{0};
}

/// @hash 1874005891
// void audio::SetSoundParameter(uint64 entityId, CName param, float value, CName emitter, float interp)

// No road under a flying car: the game's driving wind (v_car_movement_wind, on the car's general
// emitter) carries a road surface layer that follows veh_speed and the last surface the wheels
// touched, so in the air it played a dirt or gravel rush that grew with speed. veh_speed is held at
// 0 on the general emitter while flying; the engine and wheel emitters keep it, and LTBF's own wind
// (FMOD) takes over. Found by muting in flight.
REGISTER_FLIGHT_HOOK_HASH(void __fastcall, 1874005891, SetSoundParameter, uint64_t aEntity, RED4ext::CName aParam, float aValue,
                          RED4ext::CName aEmitter, float aInterp) {
  SetSoundParameter_Original(aEntity, aParam, aValue, aEmitter, aInterp);
  static const RED4ext::CName speed("veh_speed"), general("vehicle_general_emitter");
  if (aParam == speed && aEntity != 0 && aEntity == VehicleAudio::flyingVehicle.load()) {
    SetSoundParameter_Original(aEntity, aParam, 0.0f, general, aInterp);
  }
}
