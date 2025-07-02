
#include <Utils/FlightModule.hpp>
#include <FlightController.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>

using namespace RED4ext;

// prevents the camera from resetting when the left thumbstick is pressed
// TODO actually check to see if the left thumbstick is still bound to flight activation
// void vehicle::BaseObject::UpdateVehicleCameraInput(void)
REGISTER_FLIGHT_HOOK_HASH(void, 501486464, UpdateVehicleCameraInput, vehicle::BaseObject * self) {
  UpdateVehicleCameraInput_Original(self);
  auto fc = FlightController::FlightController::GetInstance();
  if (fc && fc->enabled) {
    self->input.vehicleCameraReset = 0;
  }
}