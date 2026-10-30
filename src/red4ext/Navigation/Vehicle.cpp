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
    else if (key == "sway") field = &sway;
    else if (key == "lift") field = &lift;
    else if (key == "gravityFactor") field = &gravityFactor;
    else if (key == "brakeLinear") field = &brakeLinear;
    else if (key == "maxTilt") field = &maxTilt;
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
  const float push = aSpec.drone ? DroneAccel(aSpec, aSpec.surge) : aSpec.surge;
  const float top = k > 0.0f ? std::sqrt(push / k) : aSpec.speedCap;
  return std::min(0.97f * top, aSpec.speedCap);
}

float Lateral(const VehicleSpec &aSpec) {
  if (aSpec.drone) {
    return DroneAccel(aSpec, aSpec.sway);
  }
  const float g = kG * aSpec.gravityFactor;
  const float lift = g + aSpec.lift;
  return std::sqrt(std::max(lift * lift - g * g, 1.0f)) + aSpec.surge * std::sin(aSpec.driftMax * kDeg);
}

float Accel(const VehicleSpec &aSpec, float aSpeed) { return std::max(aSpec.surge - Drag(aSpec, aSpeed), 0.5f); }

float Decel(const VehicleSpec &aSpec, float aSpeed) {
  return aSpec.surge + aSpec.brakeLinear * aSpec.brakingFriction * aSpeed + Drag(aSpec, aSpeed);
}

float PlanDecel(const VehicleSpec &aSpec, float aSpeed) {
  return std::min(aSpec.margin * Decel(aSpec, aSpeed), aSpec.planDecel);
}

float PullUp(const VehicleSpec &aSpec) { return aSpec.lift; }

SpeedProfile Profile(const VehicleSpec &aSpec, const std::vector<float> &aX, const std::vector<float> &aY,
                     const std::vector<float> &aZ, float aStartSpeed, float aStep, float aSpan) {
  SpeedProfile out;
  out.top = TopSpeed(aSpec);
  const size_t m = aX.size();
  if (m < 2) {
    return out;
  }
  std::vector<float> s(m, 0.0f);
  for (size_t i = 1; i < m; ++i) {
    s[i] = s[i - 1] + std::sqrt((aX[i] - aX[i - 1]) * (aX[i] - aX[i - 1]) + (aY[i] - aY[i - 1]) * (aY[i] - aY[i - 1]) +
                                (aZ[i] - aZ[i - 1]) * (aZ[i] - aZ[i - 1]));
  }
  // resampled every aStep metres
  std::vector<float> px, py, pz;
  size_t j = 0;
  for (float d = 0.0f; d < s[m - 1]; d += aStep) {
    while (j + 2 < m && s[j + 1] < d) {
      ++j;
    }
    const float span = std::max(s[j + 1] - s[j], 1e-6f), t = std::clamp((d - s[j]) / span, 0.0f, 1.0f);
    out.distance.push_back(d);
    px.push_back(aX[j] + t * (aX[j + 1] - aX[j]));
    py.push_back(aY[j] + t * (aY[j + 1] - aY[j]));
    pz.push_back(aZ[j] + t * (aZ[j + 1] - aZ[j]));
  }
  const size_t n = out.distance.size();
  const int32_t k = std::max(1, int32_t(aSpan / aStep));
  const float lateral = aSpec.margin * Lateral(aSpec), pushOver = aSpec.pushOver,
              pullUp = aSpec.margin * PullUp(aSpec);
  out.limit.assign(n, out.top);
  out.why.assign(n, SpeedProfile::Limit::None);
  for (size_t i = k; i + k < n; ++i) {
    const float ax = px[i] - px[i - k], ay = py[i] - py[i - k], az = pz[i] - pz[i - k];
    const float bx = px[i + k] - px[i], by = py[i + k] - py[i], bz = pz[i + k] - pz[i];
    const float h1 = std::hypot(ax, ay), h2 = std::hypot(bx, by);
    if (h1 < 1.0f || h2 < 1.0f) {
      continue;
    }
    const float turn = std::acos(std::clamp((ax * bx + ay * by) / (h1 * h2), -1.0f, 1.0f));
    const float kh = turn / (0.5f * (h1 + h2));
    const float kv = (bz / h2 - az / h1) / (0.5f * (h1 + h2));
    auto take = [&](float aLimit, SpeedProfile::Limit aWhy) {
      if (aLimit < out.limit[i]) {
        out.limit[i] = aLimit;
        out.why[i] = aWhy;
      }
    };
    if (kh > 1e-6f) {
      take(std::sqrt(lateral / kh), SpeedProfile::Limit::Turn);
    }
    if (kv < -1e-6f) {
      take(std::sqrt(pushOver / -kv), SpeedProfile::Limit::Crest);
    } else if (kv > 1e-6f) {
      take(std::sqrt(pullUp / kv), SpeedProfile::Limit::PullUp);
    }
  }
  out.speed = out.limit;
  out.speed[0] = std::min(out.speed[0], aStartSpeed);
  for (size_t i = 1; i < n; ++i) {
    const float v = out.speed[i - 1];
    out.speed[i] = std::min(out.speed[i], std::sqrt(v * v + 2.0f * Accel(aSpec, v) * aStep));
  }
  out.speed[n - 1] = 0.0f;
  for (size_t i = n - 1; i-- > 0;) {
    const float v = out.speed[i + 1];
    out.speed[i] = std::min(out.speed[i], std::sqrt(v * v + 2.0f * PlanDecel(aSpec, v) * aStep));
  }
  return out;
}

} // namespace Navigation
