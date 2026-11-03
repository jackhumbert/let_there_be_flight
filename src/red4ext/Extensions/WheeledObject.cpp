#include "Engine/RTTIExpansion.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/WheeledBaseObject.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysics.hpp>
#ifdef __APPLE__
#include "MacLayout.hpp"
#endif


class WheeledObject : public Engine::RTTIExpansion<WheeledObject, RED4ext::vehicle::WheeledBaseObject> {
public:
  inline void ResetWheels() { 
    auto physics = (RED4ext::vehicle::WheeledPhysics*)this->physics;
    // RED4ext::RelocFunc<decltype(&RED4ext::vehicle::UnkD10::Reset)> call(vehicleUnkD10_Reset_Addr);
    // call(physics->unkD10, physics->unkD10->numWheels);
  }
  inline float GetDampedSpringForce(int32_t wheelIndex) {
#ifdef __APPLE__
    // LTBF R41: the wheel array is at WheeledPhysics+0x5D0 on macOS (Windows 0x5E0), stride 0x190 on both, count at +0xC10 (R40).
    // The field offset inside a wheel entry is the fork's (not verified on macOS; macos/NOT_YET.md).
    auto physics = reinterpret_cast<uint8_t *>(this->physics);
    if (!physics || wheelIndex < 0 ||
        wheelIndex >= LtbfMac::Layout::At<uint8_t>(physics, LtbfMac::Layout::WheeledPhysics_wheelCount))
      return 0.0f;
    auto wheel = physics + LtbfMac::Layout::WheeledPhysics_wheels + wheelIndex * LtbfMac::Layout::WheeledPhysics_wheelStride;
    return *reinterpret_cast<float *>(wheel + offsetof(RED4ext::physics::VehiclePhysicsInsert2, dampedSpringForce));
#else
    return ((RED4ext::vehicle::WheeledPhysics*)this->physics)->insert2[wheelIndex].dampedSpringForce;
#endif
  }
	
private:
  friend Descriptor;

  inline static void OnExpand(Descriptor *aType, RED4ext::CRTTISystem *) {
    aType->AddFunction<&WheeledObject::ResetWheels>("ResetWheels");
    aType->AddFunction<&WheeledObject::GetDampedSpringForce>("GetDampedSpringForce");
  }
};
LTBF_FORCE_RTTI_REGISTRAR(WheeledObject) // LTBF R41: macOS, see Engine/RTTIRegistrar.hpp
