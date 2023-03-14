#include "Engine/RTTIExpansion.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/WheeledBaseObject.hpp>
#include "Addresses.hpp"

class WheeledObject : public Engine::RTTIExpansion<WheeledObject, RED4ext::vehicle::WheeledBaseObject> {
public:
  inline void ResetWheels() { 
    auto physics = (RED4ext::vehicle::WheeledPhysics*)this->physics;
    RED4ext::RelocFunc<decltype(&RED4ext::vehicle::UnkD10::Reset)> call(vehicleUnkD10_ResetAddr);
    call(physics->unkD10, physics->unkD10->numWheels);
  }
	
private:
  friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&WheeledObject::ResetWheels>("ResetWheels");
  }
};