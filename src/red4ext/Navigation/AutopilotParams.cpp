#include "AutopilotParams.hpp"

#include <cmath>

namespace Navigation {

const std::vector<AutopilotParamRef> &AutopilotParamTable() {
  using P = AutopilotParams;
  static const std::vector<AutopilotParamRef> table{
      {"brakeAccel", &P::brakeAccel, nullptr, nullptr},
      {"landAtDestination", nullptr, nullptr, &P::landAtDestination},
      {"padRange", &P::padRange, nullptr, nullptr},
      {"droneBrakeTurn", nullptr, nullptr, &P::droneBrakeTurn},
      {"droneBodyShare", &P::droneBodyShare, nullptr, nullptr},
      {"droneBrakeShare", &P::droneBrakeShare, nullptr, nullptr},
      {"droneTiltFloor", &P::droneTiltFloor, nullptr, nullptr},
      {"droneTakeoffTilt", &P::droneTakeoffTilt, nullptr, nullptr},
      {"droneTakeoffIn", &P::droneTakeoffIn, nullptr, nullptr},
      {"droneTakeoffFull", &P::droneTakeoffFull, nullptr, nullptr},
      {"droneTakeoffOut", &P::droneTakeoffOut, nullptr, nullptr},
      {"droneTakeoffLength", &P::droneTakeoffLength, nullptr, nullptr},
      {"droneMaxTilt", &P::droneMaxTilt, nullptr, nullptr},
      {"droneTiltTime", &P::droneTiltTime, nullptr, nullptr},
      {"dronePushOver", &P::dronePushOver, nullptr, nullptr},
      {"trajectoryAttitudeLead", &P::trajectoryAttitudeLead, nullptr, nullptr},
      {"replanOffLine", &P::replanOffLine, nullptr, nullptr},
      {"trajectoryCorrectionMax", &P::trajectoryCorrectionMax, nullptr, nullptr},
      {"rejoinAhead", &P::rejoinAhead, nullptr, nullptr},
      {"rejoinMin", &P::rejoinMin, nullptr, nullptr},
      {"hitAvoidRadius", &P::hitAvoidRadius, nullptr, nullptr},
      {"hitAvoidTime", &P::hitAvoidTime, nullptr, nullptr},
      {"droneYawLead", &P::droneYawLead, nullptr, nullptr},
      {"droneYawFrequency", &P::droneYawFrequency, nullptr, nullptr},
      {"droneYawRateMax", &P::droneYawRateMax, nullptr, nullptr},
      {"droneYawAccelMax", &P::droneYawAccelMax, nullptr, nullptr},
      {"droneLandSlewYaw", &P::droneLandSlewYaw, nullptr, nullptr},
      {"droneLandSlewBefore", &P::droneLandSlewBefore, nullptr, nullptr},
      {"droneLandSlewDuring", &P::droneLandSlewDuring, nullptr, nullptr},
      {"approachHeight", &P::approachHeight, nullptr, nullptr},
      {"descentRate", &P::descentRate, nullptr, nullptr},
      {"touchdownRate", &P::touchdownRate, nullptr, nullptr},
      {"landingSearch", nullptr, &P::landingSearch, nullptr},
      {"landingShaft", &P::landingShaft, nullptr, nullptr},
      {"landingFlat", &P::landingFlat, nullptr, nullptr},
      {"landingFootprint", &P::landingFootprint, nullptr, nullptr},
      {"landingOverSpot", &P::landingOverSpot, nullptr, nullptr},
      {"landingEntry", &P::landingEntry, nullptr, nullptr},
      {"landingEntrySpeed", &P::landingEntrySpeed, nullptr, nullptr},
      {"landingRecheck", &P::landingRecheck, nullptr, nullptr},
      {"landingShiftSpan", &P::landingShiftSpan, nullptr, nullptr},
      {"padSnapDistance", &P::padSnapDistance, nullptr, nullptr},
      {"padSnapLevel", &P::padSnapLevel, nullptr, nullptr},
      {"brakeMax", &P::brakeMax, nullptr, nullptr},
      {"speedGain", &P::speedGain, nullptr, nullptr},
      {"accelJerk", &P::accelJerk, nullptr, nullptr},
      {"yawSmoothing", &P::yawSmoothing, nullptr, nullptr},
      {"velocityGain", &P::velocityGain, nullptr, nullptr},
      {"maxAccel", &P::maxAccel, nullptr, nullptr},
      {"overrideHold", &P::overrideHold, nullptr, nullptr},
      {"replanDistance", &P::replanDistance, nullptr, nullptr},
      {"arriveDistance", &P::arriveDistance, nullptr, nullptr},
      {"cornerOvershoot", &P::cornerOvershoot, nullptr, nullptr},
      {"cornerSpan", &P::cornerSpan, nullptr, nullptr},
      {"collisionHorizon", &P::collisionHorizon, nullptr, nullptr},
      {"attitudeFrequency", &P::attitudeFrequency, nullptr, nullptr},
      {"attitudeDamping", &P::attitudeDamping, nullptr, nullptr},
      {"maxAngularAccel", &P::maxAngularAccel, nullptr, nullptr},
      {"torqueGainPitch", &P::torqueGainPitch, nullptr, nullptr},
      {"torqueGainRoll", &P::torqueGainRoll, nullptr, nullptr},
      {"torqueGainYaw", &P::torqueGainYaw, nullptr, nullptr},
      {"attitudeLead", &P::attitudeLead, nullptr, nullptr},
      {"maxHeadingRate", &P::maxHeadingRate, nullptr, nullptr},
      {"raceMargin", &P::raceMargin, nullptr, nullptr},
      {"raceJerk", &P::raceJerk, nullptr, nullptr},
      {"raceSpeedCap", &P::raceSpeedCap, nullptr, nullptr},
      {"raceCornerRound", &P::raceCornerRound, nullptr, nullptr},
      {"raceRoundShare", &P::raceRoundShare, nullptr, nullptr},
      {"raceRoundStep", &P::raceRoundStep, nullptr, nullptr},
      {"raceRoundMin", &P::raceRoundMin, nullptr, nullptr},
      {"raceOffLine", &P::raceOffLine, nullptr, nullptr},
      {"raceRouteHeight", &P::raceRouteHeight, nullptr, nullptr},
      {"raceClimbCost", &P::raceClimbCost, nullptr, nullptr},
      {"raceMinPassage", &P::raceMinPassage, nullptr, nullptr},
      {"raceSearchWeight", &P::raceSearchWeight, nullptr, nullptr},
      {"raceTurnCost", &P::raceTurnCost, nullptr, nullptr},
      {"raceDriftMax", &P::raceDriftMax, nullptr, nullptr},
      {"racePlanDecel", &P::racePlanDecel, nullptr, nullptr},
      {"racePushOver", &P::racePushOver, nullptr, nullptr},
      {"lineLead", &P::lineLead, nullptr, nullptr},
      {"lineSpring", &P::lineSpring, nullptr, nullptr},
      {"lineVelocityGain", &P::lineVelocityGain, nullptr, nullptr},
      {"lineSpan", &P::lineSpan, nullptr, nullptr},
      {"raceRollFrequency", &P::raceRollFrequency, nullptr, nullptr},
      {"raceRollRateMax", &P::raceRollRateMax, nullptr, nullptr},
      {"raceRollAccelMax", &P::raceRollAccelMax, nullptr, nullptr},
      {"racePitchFrequency", &P::racePitchFrequency, nullptr, nullptr},
      {"racePitchRateMax", &P::racePitchRateMax, nullptr, nullptr},
      {"racePitchAccelMax", &P::racePitchAccelMax, nullptr, nullptr},
      {"freeRoamRadius", &P::freeRoamRadius, nullptr, nullptr},
      {"freeRoamLegMin", &P::freeRoamLegMin, nullptr, nullptr},
      {"freeRoamLegMax", &P::freeRoamLegMax, nullptr, nullptr},
      {"freeRoamSpread", &P::freeRoamSpread, nullptr, nullptr},
      {"freeRoamMaxDepth", &P::freeRoamMaxDepth, nullptr, nullptr},
      {"freeRoamMaxTurn", &P::freeRoamMaxTurn, nullptr, nullptr},
      {"freeRoamMaxPitch", &P::freeRoamMaxPitch, nullptr, nullptr},
      {"freeRoamGateMin", &P::freeRoamGateMin, nullptr, nullptr},
      {"gateLead", &P::gateLead, nullptr, nullptr},
      {"gateStep", &P::gateStep, nullptr, nullptr},
      {"gateMemory", nullptr, &P::gateMemory, nullptr},
      {"freeRoamChainAt", &P::freeRoamChainAt, nullptr, nullptr},
      {"freeRoamJoinAhead", &P::freeRoamJoinAhead, nullptr, nullptr},
      {"freeRoamJoinMin", &P::freeRoamJoinMin, nullptr, nullptr},
      {"freeRoamSmoothSpan", &P::freeRoamSmoothSpan, nullptr, nullptr},
      {"freeRoamTopSpeed", &P::freeRoamTopSpeed, nullptr, nullptr},
      {"freeRoamMaxRise", &P::freeRoamMaxRise, nullptr, nullptr},
      {"freeRoamRouteHeight", &P::freeRoamRouteHeight, nullptr, nullptr},
      {"freeRoamHeightFree", &P::freeRoamHeightFree, nullptr, nullptr},
      {"freeRoamHeightWavelength", &P::freeRoamHeightWavelength, nullptr, nullptr},
      {"freeRoamOverCost", &P::freeRoamOverCost, nullptr, nullptr},
      {"positionJumpHold", &P::positionJumpHold, nullptr, nullptr},
  };
  return table;
}

bool SetAutopilotParam(AutopilotParams &aParams, std::string_view aName, float aValue) {
  for (const auto &r : AutopilotParamTable()) {
    if (aName != r.name) {
      continue;
    }
    if (r.f) {
      aParams.*r.f = aValue;
    } else if (r.i) {
      aParams.*r.i = int32_t(std::lround(aValue));
    } else {
      aParams.*r.b = aValue != 0.0f;
    }
    return true;
  }
  return false;
}

} // namespace Navigation
