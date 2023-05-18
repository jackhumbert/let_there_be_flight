#include "Engine/RTTIExpansion.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/WheeledBaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#include "Addresses.hpp"

class WheeledObject : public Engine::RTTIExpansion<WheeledObject, RED4ext::vehicle::WheeledBaseObject> {
public:
  inline void ResetWheels() { 
    auto physics = (RED4ext::vehicle::WheeledPhysics*)this->physics;
    RED4ext::RelocFunc<decltype(&RED4ext::vehicle::UnkD10::Reset)> call(vehicleUnkD10_Reset_Addr);
    call(physics->unkD10, physics->unkD10->numWheels);
  }
  inline float GetDampedSpringForce(int32_t wheelIndex) {
    return ((RED4ext::vehicle::WheeledPhysics*)this->physics)->insert2[wheelIndex].dampedSpringForce;
  }
	
private:
  friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&WheeledObject::ResetWheels>("ResetWheels");
    aType->AddFunction<&WheeledObject::GetDampedSpringForce>("GetDampedSpringForce");
  }
};