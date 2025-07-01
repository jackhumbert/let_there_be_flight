
#include <Utils/FlightModule.hpp>
#include <FlightController.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>

using namespace RED4ext;

// void vehicle::AreaSpeedLimiter::Update_PreMove(float,class THandle<class vehicle::BaseObject> const &)
REGISTER_FLIGHT_HOOK_HASH(void, 1836326624, AreaSpeedLimiter_Update, void * self, 
  float delta, Handle<vehicle::BaseObject> const & vehicle
) {
  auto fc = FlightController::FlightController::GetInstance();
  if (fc && fc->active)
    return;
  return AreaSpeedLimiter_Update_Original(self, delta, vehicle);
}