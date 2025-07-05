#include "Utils/FlightModule.hpp"
#include "FlightController.hpp"
#include "FlightSystem.hpp"
#include "FlightSettings.hpp"
#include <RED4ext/Common.hpp>

using namespace RED4ext;

struct VehicleCustomizationLayer {
  uint64_t setupRes[2];
  uint64_t maskRes[2];
  DynArray<CName> affectedComponents;
  DynArray<CName> excludedComponents;
  float coatLayerMin;
  float coatLayerMax;
  uint64_t globalClearCoatOverrides[3];
  DynArray<CName> partsClearCoatOverrrides;
};

REGISTER_FLIGHT_HOOK_HASH(void, 3955119979, VehicleCustomizationLayer_Create, VehicleCustomizationLayer * self,
  void *setupRes,
  void *maskRes,
  DynArray<CName> *affectedComponents,
  DynArray<CName> *excludedComponents,
  float coatLayerMin,
  float coatLayerMax,
  void * globalClearCoatOverrides,
  DynArray<CName> *partsClearCoatOverrides
) {  
  VehicleCustomizationLayer_Create_Original(self, setupRes, maskRes, affectedComponents, excludedComponents, coatLayerMin, coatLayerMax, globalClearCoatOverrides, partsClearCoatOverrides);
  self->affectedComponents.EmplaceBack("ThrusterFL");
}
