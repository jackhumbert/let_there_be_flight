#include "Vehicle.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace Navigation {

namespace {
constexpr float kG = 9.81f;
constexpr float kDeg = 3.14159265f / 180.0f;
} // namespace

bool VehicleSpec::Parse(const std::string &aText, std::string &aError) {
  std::stringstream items(aText);
  std::string item;
  while (std::getline(items, item, ',')) {
    if (item.empty()) {
      continue;
    }
    auto eq = item.find('=');
    if (eq == std::string::npos) {
      aError = "no '=' in '" + item + "'";
      return false;
    }
    std::string key = item.substr(0, eq);
    float value = std::strtof(item.c_str() + eq + 1, nullptr);
    float *field = nullptr;
    if (key == "mass") field = &mass;
    else if (key == "airResistance") field = &airResistance;
    else if (key == "brakingFriction") field = &brakingFriction;
    else if (key == "surge") field = &surge;
    else if (key == "surgeShare") field = &surgeShare;
    else if (key == "sway") field = &sway;
    else if (key == "lift") field = &lift;
    else if (key == "gravityFactor") field = &gravityFactor;
    else if (key == "brakeLinear") field = &brakeLinear;
    else if (key == "maxTilt") field = &maxTilt;
    else if (key == "brakeShare") field = &brakeShare;
    else if (key == "tiltFloor") field = &tiltFloor;
    else if (key == "driftMax") field = &driftMax;
    else if (key == "speedCap") field = &speedCap;
    else if (key == "margin") field = &margin;
    else if (key == "planDecel") field = &planDecel;
    else if (key == "pushOver") field = &pushOver;
    else if (key == "drag") {
      drag = value != 0.0f;
      continue;
    } else if (key == "drone") {
      drone = value != 0.0f;
      continue;
    } else {
      aError = "unknown vehicle field '" + key + "'";
      return false;
    }
    *field = value;
  }
  return true;
}

std::string VehicleSpec::Describe() const {
  char text[256];
  std::snprintf(text, sizeof(text), "%s: top %.1f m/s, lateral %.1f m/s2, accel %.1f at 0, decel %.1f at 0 (+%.1f at 30), push-over %.1f, pull-up %.1f",
                drone ? "drone" : "plane", TopSpeed(*this), Lateral(*this), Accel(*this, 0.0f), Decel(*this, 0.0f),
                Decel(*this, 30.0f) - Decel(*this, 0.0f), pushOver, PullUp(*this));
  return text;
}

float Drag(const VehicleSpec &aSpec, float aSpeed) {
  if (!aSpec.drag || aSpec.mass <= 0.0f) {
    return 0.0f;
  }
  return 1.2f * aSpec.airResistance * aSpeed * aSpeed / aSpec.mass;
}

float DroneAccel(const VehicleSpec &aSpec, float aThrusters) {
  const float g = kG * aSpec.gravityFactor;
  const float liftMax = g + aSpec.lift;
  float best = aThrusters;
  for (float deg = 1.0f; deg <= aSpec.maxTilt; deg += 1.0f) {
    const float b = deg * kDeg;
    const float lift = (g + aThrusters * std::sin(b)) / std::cos(b);
    if (lift > liftMax) {
      break;
    }
    best = std::max(best, lift * std::sin(b) + aThrusters * std::cos(b));
  }
  return best;
}

float TopSpeed(const VehicleSpec &aSpec) {
  const float k = aSpec.drag ? 1.2f * aSpec.airResistance / std::max(aSpec.mass, 1.0f) : 0.0f;
  // (drone style was DroneAccel(surge): 120 m/s, while the car levels off at 89 to 91)
  const float push = aSpec.surge * aSpec.surgeShare;
  const float top = k > 0.0f ? std::sqrt(push / k) : aSpec.speedCap;
  return std::min(0.97f * top, aSpec.speedCap);
}

float Lateral(const VehicleSpec &aSpec) {
  if (aSpec.drone) {
    // plus the surge held through the bend with the nose drifted into it, as a player flies it
    return DroneAccel(aSpec, aSpec.sway) + aSpec.surge * aSpec.surgeShare * std::sin(aSpec.driftMax * kDeg);
  }
  const float g = kG * aSpec.gravityFactor;
  const float lift = g + aSpec.lift;
  return std::sqrt(std::max(lift * lift - g * g, 1.0f)) + aSpec.surge * std::sin(aSpec.driftMax * kDeg);
}

float LateralAt(const VehicleSpec &aSpec, float aVertical) {
  if (!aSpec.drone) {
    return Lateral(aSpec);
  }
  const float g = kG * aSpec.gravityFactor;
  const float tilt = DroneAccel(aSpec, aSpec.sway) - aSpec.sway;
  const float left = std::clamp(aVertical / std::max(g, 0.1f), 0.0f, 1.0f);
  return aSpec.sway + tilt * left + aSpec.surge * aSpec.surgeShare * std::sin(aSpec.driftMax * kDeg);
}

float Accel(const VehicleSpec &aSpec, float aSpeed) {
  return std::max(aSpec.surge * aSpec.surgeShare - Drag(aSpec, aSpeed), 0.5f);
}

float Decel(const VehicleSpec &aSpec, float aSpeed) {
  return aSpec.surge + aSpec.brakeLinear * aSpec.brakingFriction * aSpeed + Drag(aSpec, aSpeed);
}

float PlanDecel(const VehicleSpec &aSpec, float aSpeed) {
  return std::min(aSpec.margin * Decel(aSpec, aSpeed), aSpec.planDecel);
}

float PullUp(const VehicleSpec &aSpec) { return aSpec.lift; }

float DroneBodyAngle(const VehicleSpec &aSpec, float aAngle, float aThrust, float aThrusters, float aShare) {
  const float reach = std::asin(std::clamp(aSpec.margin * aThrusters / std::max(aThrust, 0.1f), 0.0f, 1.0f));
  const float body = aAngle * aShare, rest = aAngle - body;
  return rest > reach ? aAngle - reach : (rest < -reach ? aAngle + reach : body);
}

} // namespace Navigation
