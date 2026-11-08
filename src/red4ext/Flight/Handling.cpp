// Handling (Flight/Handling.hpp): the Loose and Tight sets.
#include "Flight/Handling.hpp"

namespace Flight {

namespace {
HandlingFactors Loose() {
  HandlingFactors h;
  h.generalDampFactorAngular = 1.25f;
  h.generalDampFactorAngularMax = 2.5f;
  h.generalPitchDirectionalityFactor = 8.0f;
  h.generalYawDirectionalityFactor = 2.5f;
  h.generalYawDampFactor = 0.75f;
  return h;
}

HandlingFactors Tight() {
  HandlingFactors h;
  h.generalDampFactorAngular = 3.0f;
  h.generalDampFactorAngularMax = 5.0f;
  h.generalPitchDirectionalityFactor = 20.0f;
  h.generalYawDirectionalityFactor = 15.0f;
  h.generalYawDampFactor = 1.5f;
  return h;
}
} // namespace

HandlingFactors HandlingSet(Handling aHandling, const HandlingFactors &aDefault) {
  switch (aHandling) {
  case Handling::Loose:
    return Loose();
  case Handling::Tight:
    return Tight();
  default:
    return aDefault;
  }
}

HandlingFactors HandlingSet(int32_t aHandling, const HandlingFactors &aDefault) {
  return HandlingSet(static_cast<Handling>(aHandling), aDefault);
}

ForceParams WithHandling(ForceParams aParams, int32_t aHandling) {
  HandlingFactors d;
  d.generalDampFactorLinear = aParams.generalDampFactorLinear;
  d.generalDampFactorAngular = aParams.generalDampFactorAngular;
  d.generalDampFactorAngularMax = aParams.generalDampFactorAngularMax;
  d.generalPitchAeroFactor = aParams.generalPitchAeroFactor;
  d.generalYawAeroFactor = aParams.generalYawAeroFactor;
  d.generalPitchDirectionalityFactor = aParams.generalPitchDirectionalityFactor;
  d.generalYawDirectionalityFactor = aParams.generalYawDirectionalityFactor;
  d.generalYawDampFactor = aParams.generalYawDampFactor;
  d.generalSwayWithYaw = aParams.generalSwayWithYaw;
  const HandlingFactors h = HandlingSet(aHandling, d);
  aParams.generalDampFactorLinear = h.generalDampFactorLinear;
  aParams.generalDampFactorAngular = h.generalDampFactorAngular;
  aParams.generalDampFactorAngularMax = h.generalDampFactorAngularMax;
  aParams.generalPitchAeroFactor = h.generalPitchAeroFactor;
  aParams.generalYawAeroFactor = h.generalYawAeroFactor;
  aParams.generalPitchDirectionalityFactor = h.generalPitchDirectionalityFactor;
  aParams.generalYawDirectionalityFactor = h.generalYawDirectionalityFactor;
  aParams.generalYawDampFactor = h.generalYawDampFactor;
  aParams.generalSwayWithYaw = h.generalSwayWithYaw;
  return aParams;
}

StandardGains WithHandling(StandardGains aGains, int32_t aHandling) {
  switch (static_cast<Handling>(aHandling)) {
  case Handling::Loose:
    aGains.brake = 0.8f;
    aGains.sideBrake = 2.2f;
    break;
  case Handling::Tight:
    aGains.brake = 2.5f;
    aGains.sideBrake = 5.0f;
    aGains.maxBrake = 18.0f;
    break;
  default:
    break;
  }
  return aGains;
}

} // namespace Flight
