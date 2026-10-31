#include "Trajectory.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Navigation {

namespace {
constexpr float kG = 9.81f;

Vec3 Add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 Sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 Mul(Vec3 a, float k) { return {a.x * k, a.y * k, a.z * k}; }
float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float Len(Vec3 a) { return std::sqrt(Dot(a, a)); }
Vec3 Cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

// The line every aStep metres from its start (the last sample at its end).
void Resample(const std::vector<Vec3> &aLine, float aStep, std::vector<float> &aS, std::vector<Vec3> &aP) {
  std::vector<float> cum(aLine.size(), 0.0f);
  for (size_t i = 1; i < aLine.size(); ++i) {
    cum[i] = cum[i - 1] + Len(Sub(aLine[i], aLine[i - 1]));
  }
  const float total = cum.back();
  size_t j = 0;
  for (float d = 0.0f;; d += aStep) {
    d = std::min(d, total);
    while (j + 2 < aLine.size() && cum[j + 1] < d) {
      ++j;
    }
    const float leg = std::max(cum[j + 1] - cum[j], 1e-6f), t = std::clamp((d - cum[j]) / leg, 0.0f, 1.0f);
    aS.push_back(d);
    aP.push_back(Add(aLine[j], Mul(Sub(aLine[j + 1], aLine[j]), t)));
    if (d >= total) {
      break;
    }
  }
}

// The point at distance aD along the resampled line (clamped to its ends).
Vec3 At(const std::vector<float> &aS, const std::vector<Vec3> &aP, float aD) {
  if (aD <= 0.0f) {
    return aP.front();
  }
  if (aD >= aS.back()) {
    return aP.back();
  }
  size_t i = std::upper_bound(aS.begin(), aS.end(), aD) - aS.begin();
  const float leg = std::max(aS[i] - aS[i - 1], 1e-6f);
  return Add(aP[i - 1], Mul(Sub(aP[i], aP[i - 1]), (aD - aS[i - 1]) / leg));
}

// Radius of the circle through three points (1e6 when they are in line).
float Circumradius(Vec3 a, Vec3 b, Vec3 c) {
  const float ab = Len(Sub(b, a)), bc = Len(Sub(c, b)), ca = Len(Sub(a, c));
  const float area2 = Len(Cross(Sub(b, a), Sub(c, a)));
  return area2 < 1e-6f ? 1e6f : ab * bc * ca / (2.0f * area2);
}
} // namespace

const char *LimitName(TrajectorySample::Limit aLimit) {
  switch (aLimit) {
  case TrajectorySample::Limit::Top:
    return "top";
  case TrajectorySample::Limit::Turn:
    return "turn";
  case TrajectorySample::Limit::Crest:
    return "crest";
  case TrajectorySample::Limit::PullUp:
    return "pull-up";
  case TrajectorySample::Limit::Sink:
    return "sink";
  case TrajectorySample::Limit::Arrive:
    return "arrive";
  case TrajectorySample::Limit::Brake:
    return "brake";
  case TrajectorySample::Limit::Accel:
    return "accel";
  case TrajectorySample::Limit::Start:
    return "start";
  case TrajectorySample::Limit::End:
    return "end";
  case TrajectorySample::Limit::Room:
    return "room";
  }
  return "";
}

float DipLoss(const Trajectory &aTrajectory, float aApproach, float aDip) {
  const auto &q = aTrajectory.samples;
  if (q.size() < 3) {
    return 0.0f;
  }
  const float end = q.back().s - aApproach;
  float lost = 0.0f;
  for (size_t i = 1; i + 1 < q.size();) {
    if (q[i].speed >= q[i - 1].speed - 1e-4f) {
      ++i;
      continue;
    }
    const float top = q[i - 1].speed;
    size_t j = i;
    while (j + 1 < q.size() && q[j + 1].speed <= q[j].speed + 1e-4f) {
      ++j;
    }
    const size_t low = j;
    while (j + 1 < q.size() && q[j + 1].speed >= q[j].speed - 1e-4f) {
      ++j;
    }
    if (q[low].s < end && top - q[low].speed >= aDip && q[j].speed > q[low].speed + aDip) {
      lost += top - q[low].speed;
    }
    i = j + 1;
  }
  return lost;
}

Trajectory BuildTrajectory(const VehicleSpec &aSpec, const std::vector<Vec3> &aLine, const TrajectoryParams &aParams,
                           const NavMap *aMap) {
  using Limit = TrajectorySample::Limit;
  Trajectory out;
  out.top = TopSpeed(aSpec);
  if (aLine.size() < 2) {
    return out;
  }
  std::vector<float> s;
  std::vector<Vec3> p;
  Resample(aLine, aParams.step, s, p);
  const size_t n = s.size();
  const float total = s.back();
  const float lateral = aSpec.margin * Lateral(aSpec), pullUp = aSpec.margin * PullUp(aSpec);

  // geometry: direction and bend vector over bendSpan, bend radius over radiusSpan
  std::vector<Vec3> tangent(n), bend(n, Vec3{0, 0, 0});
  std::vector<float> radius(n, 1e6f);
  for (size_t i = 0; i < n; ++i) {
    const Vec3 a = At(s, p, s[i] - aParams.bendSpan), c = At(s, p, s[i] + aParams.bendSpan);
    const Vec3 ab = Sub(p[i], a), bc = Sub(c, p[i]);
    const float l1 = Len(ab), l2 = Len(bc), lac = Len(Sub(c, a));
    tangent[i] = lac > 0.01f ? Mul(Sub(c, a), 1.0f / lac) : (i + 1 < n ? Mul(Sub(p[i + 1], p[i]), 1.0f / std::max(Len(Sub(p[i + 1], p[i])), 1e-6f)) : tangent[i - 1]);
    if (l1 > 0.5f && l2 > 0.5f) {
      bend[i] = Mul(Sub(Mul(bc, 1.0f / l2), Mul(ab, 1.0f / l1)), 2.0f / (l1 + l2));
    }
    if (i > 0 && i + 1 < n) {
      radius[i] = Circumradius(At(s, p, s[i] - aParams.radiusSpan), p[i], At(s, p, s[i] + aParams.radiusSpan));
    }
  }

  // room across the line: the nearest blocked space left, right, up and down (the map is car-centre
  // space, the car's size already in it), and the speed that leaves the car enough of it
  std::vector<float> room(n, aParams.roomLook), roomCap(n, 1e6f);
  if (aMap) {
    for (size_t i = 0; i < n; ++i) {
      const float flat = std::hypot(tangent[i].x, tangent[i].y);
      const Vec3 side = flat > 0.1f ? Vec3{-tangent[i].y / flat, tangent[i].x / flat, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
      Vec3 up = Cross(side, tangent[i]);
      up = Mul(up, 1.0f / std::max(Len(up), 1e-6f));
      const Vec3 dirs[4] = {side, Mul(side, -1.0f), up, Mul(up, -1.0f)};
      for (const auto &d : dirs) {
        for (float r = 0.5f; r < room[i]; r += 0.5f) {
          const Vec3 q = Add(p[i], Mul(d, r));
          if (!aMap->IsFree(q.x, q.y, q.z)) {
            room[i] = r;
            break;
          }
        }
      }
      if (room[i] < aParams.roomLook) {
        const float spare = std::max(room[i] - aParams.roomMin, 0.0f);
        roomCap[i] = std::max(aParams.roomSpeed * std::sqrt(spare / aParams.roomGrow), aParams.roomSpeedMin);
      }
    }
  }

  // the speed along the line, with or without the room's caps
  std::vector<float> v(n);
  std::vector<Limit> why(n, Limit::Top);
  auto speeds = [&](bool aRoom) {
    std::fill(v.begin(), v.end(), 0.0f);
    std::fill(why.begin(), why.end(), Limit::Top);
    auto take = [&](size_t i, float aLimit, Limit aWhy) {
      if (aLimit < v[i]) {
        v[i] = aLimit;
        why[i] = aWhy;
      }
    };
    for (size_t i = 0; i < n; ++i) {
      v[i] = out.top;
      // the tighter of the bend radius and the bend vector's sideways part (level), the one the
      // acceleration is worked out from: over a short tight bend the radius alone ran past the limit
      const Vec3 across = Sub(bend[i], Mul(tangent[i], Dot(bend[i], tangent[i])));
      const float level = std::hypot(across.x, across.y);
      const float curve = std::max(1.0f / std::max(radius[i], 1.0f), level);
      float turn = std::sqrt(lateral / curve);
      if (bend[i].z < -1e-4f) {
        // over a crest the lift goes into pushing over and turns the car less (LateralAt): the
        // fastest speed whose bend fits what is left, by bisection (faster pushes over harder)
        const float g = kG * aSpec.gravityFactor;
        float lo = 0.0f, hi = turn;
        for (int32_t k = 0; k < 24; ++k) {
          const float mid = 0.5f * (lo + hi);
          const bool fits = mid * mid * curve <= aSpec.margin * LateralAt(aSpec, g + bend[i].z * mid * mid);
          (fits ? lo : hi) = mid;
        }
        turn = lo;
      }
      take(i, turn, Limit::Turn);
      if (bend[i].z < -1e-4f) {
        take(i, std::sqrt(aSpec.pushOver / -bend[i].z), Limit::Crest);
      } else if (bend[i].z > 1e-4f) {
        take(i, std::sqrt(pullUp / bend[i].z), Limit::PullUp);
      }
      const float left = total - s[i];
      if (left < aParams.sinkApproach && tangent[i].z < -0.05f) {
        take(i, aParams.maxSink / -tangent[i].z, Limit::Sink);
      }
      if (aParams.stopAtEnd && left < aParams.arriveLength) {
        take(i, std::sqrt(2.0f * aParams.arriveDecel * left), Limit::Arrive);
      }
      if (aRoom) {
        take(i, roomCap[i], Limit::Room);
      }
    }
    if (aParams.stopAtEnd) {
      v[n - 1] = 0.0f;
      why[n - 1] = Limit::End;
    }

    // braking back: lateral and braking share one envelope (a friction circle), down to brakeGripMin
    auto brakeBack = [&]() {
      for (size_t i = n - 1; i-- > 0;) {
        const float ds = s[i + 1] - s[i], r = std::min(radius[i], radius[i + 1]), vv = v[i + 1];
        const float use = std::min(vv * vv / std::max(r, 1.0f) / std::max(lateral, 1.0f), 1.0f);
        const float grip = std::max(std::sqrt(1.0f - use * use), aParams.brakeGripMin);
        const float back = std::sqrt(vv * vv + 2.0f * PlanDecel(aSpec, vv) * grip * ds);
        if (back < v[i]) {
          v[i] = back;
          why[i] = Limit::Brake;
        }
      }
    };
    brakeBack();

    // small rises between two slowings held down, so two slowings close together are one
    {
      std::vector<float> flat = v;
      for (size_t i = 0; i < n; ++i) {
        float left = v[i], right = v[i];
        for (size_t j = i; j > 0 && s[i] - s[j - 1] <= aParams.humpSpan; --j) {
          left = std::min(left, v[j - 1]);
        }
        for (size_t j = i; j + 1 < n && s[j + 1] - s[i] <= aParams.humpSpan; ++j) {
          right = std::min(right, v[j + 1]);
        }
        const float low = std::max(left, right);
        if (v[i] > low && v[i] - low < aParams.humpMerge) {
          flat[i] = low;
        }
      }
      v = flat;
    }
    // lowering a stretch moves its braking earlier: braked back again (without this the merge left
    // drops of 40 m/s2 at a merged stretch's start)
    brakeBack();

    // speeding up from the start: surge less drag, sharing the envelope with the bend as braking does
    if (aParams.startSpeed < v[0]) {
      v[0] = aParams.startSpeed;
      why[0] = Limit::Start;
    }
    for (size_t i = 1; i < n; ++i) {
      const float ds = s[i] - s[i - 1], r = std::min(radius[i], radius[i - 1]), vv = v[i - 1];
      const float use = std::min(vv * vv / std::max(r, 1.0f) / std::max(lateral, 1.0f), 1.0f);
      const float grip = std::max(std::sqrt(1.0f - use * use), aParams.brakeGripMin);
      const float up = std::sqrt(vv * vv + 2.0f * Accel(aSpec, vv) * grip * ds);
      if (up < v[i]) {
        v[i] = up;
        why[i] = Limit::Accel;
      }
    }
  };
  speeds(false);
  const std::vector<float> free = v;
  speeds(true);

  // time, velocity and acceleration
  out.samples.resize(n);
  for (size_t i = 0; i < n; ++i) {
    auto &q = out.samples[i];
    q.s = s[i];
    q.position = p[i];
    q.speed = v[i];
    q.limit = why[i];
    q.room = room[i];
    q.roomLoss = std::max(free[i] - v[i], 0.0f);
    q.velocity = Mul(tangent[i], v[i]);
    if (i > 0) {
      const float ds = s[i] - s[i - 1];
      q.t = out.samples[i - 1].t + ds / std::max(0.5f * (v[i] + v[i - 1]), 0.25f);
    }
  }
  for (size_t i = 0; i < n; ++i) {
    // along: v dv/ds over the neighbouring legs
    const size_t a = i > 0 ? i - 1 : i, b = i + 1 < n ? i + 1 : i;
    const float ds = s[b] - s[a];
    auto &q = out.samples[i];
    q.along = ds > 0.0f ? (v[b] * v[b] - v[a] * v[a]) / (2.0f * ds) : 0.0f;
    const Vec3 centripetal = Mul(bend[i], v[i] * v[i]);
    q.acceleration = Add(Mul(tangent[i], q.along), centripetal);
    const Vec3 across = Sub(centripetal, Mul(tangent[i], Dot(centripetal, tangent[i])));
    q.lateral = std::hypot(across.x, across.y); // level: up and down are the lift's (crest, pull-up)
  }

  // attitude: as FlightModeAutopilot.DroneAttitude works it out from an acceleration, each angle
  // the body's share of the thrust's tilt (more where the thrusters cannot point the rest)
  {
    const float g = kG * aSpec.gravityFactor, cap = aSpec.maxTilt * 3.14159265f / 180.0f;
    auto bodyAngle = [&](float aAngle, float aThrust, float aThrusters, float aShare) {
      const float reach = std::asin(std::clamp(aSpec.margin * aThrusters / std::max(aThrust, 0.1f), 0.0f, 1.0f));
      const float body = aAngle * aShare, rest = aAngle - body;
      return rest > reach ? aAngle - reach : (rest < -reach ? aAngle + reach : body);
    };
    // each side's lean started early enough to reach its peak at the rate limit, and let go no
    // faster: an envelope over each sign, backward (rising) then forward (falling)
    auto envelope = [&](std::vector<float> &aAngle, float aRate) {
      std::vector<float> result(n, 0.0f);
      for (int32_t sign = 1; sign >= -1; sign -= 2) {
        std::vector<float> e(n);
        for (size_t i = 0; i < n; ++i) {
          e[i] = std::max(sign * aAngle[i], 0.0f);
        }
        for (size_t i = n - 1; i-- > 0;) {
          e[i] = std::max(e[i], e[i + 1] - aRate * (out.samples[i + 1].t - out.samples[i].t));
        }
        for (size_t i = 1; i < n; ++i) {
          e[i] = std::max(e[i], e[i - 1] - aRate * (out.samples[i].t - out.samples[i - 1].t));
        }
        for (size_t i = 0; i < n; ++i) {
          result[i] += sign * e[i]; // where a lean changes sides the two overlap and cross over
        }
      }
      aAngle = result;
    };
    std::vector<float> lat(n), up(n), size(n), pitch(n), roll(n), drift(n, 0.0f);
    std::vector<Vec3> heading(n), side(n);
    Vec3 h{1.0f, 0.0f, 0.0f};
    for (size_t i = 0; i < n; ++i) {
      const float flat = std::hypot(tangent[i].x, tangent[i].y);
      if (flat > 0.15f) { // near vertical the heading holds
        h = {tangent[i].x / flat, tangent[i].y / flat, 0.0f};
      }
      heading[i] = h;
      // drift: the nose into the bend, by how hard the bend pulls sideways (left > 0)
      if (aSpec.drone && aSpec.driftMax > 0.0f) {
        const auto &q = out.samples[i];
        const float pull = Dot(Mul(bend[i], q.speed * q.speed), Vec3{-h.y, h.x, 0.0f});
        const float f = std::clamp((std::abs(pull) - aParams.driftStart) / std::max(aParams.driftFull - aParams.driftStart, 0.1f), 0.0f, 1.0f);
        drift[i] = (pull >= 0.0f ? 1.0f : -1.0f) * aSpec.driftMax * 3.14159265f / 180.0f * f * f * (3.0f - 2.0f * f);
      }
    }
    envelope(drift, aParams.driftRateMax);
    for (size_t i = 0; i < n; ++i) {
      const float c = std::cos(drift[i]), sn = std::sin(drift[i]);
      const Vec3 h0 = heading[i];
      h = {h0.x * c - h0.y * sn, h0.x * sn + h0.y * c, 0.0f};
      heading[i] = h;
      side[i] = {h.y, -h.x, 0.0f}; // h x up
      auto &q = out.samples[i];
      q.yaw = std::atan2(h.y, h.x);
      q.drift = drift[i];
      // the thrust also holds off the air drag along the line (at 89 m/s about 13 m/s2 forward)
      Vec3 thrust = Add(Add(q.acceleration, Mul(tangent[i], Drag(aSpec, q.speed))), Vec3{0.0f, 0.0f, g});
      // bending down (a crest), the tilt as if the lift still held the car up: the lift pushes less or
      // down and the thrusters take the rest. Worked out against the small vertical left, a modest
      // turn read as a thrust near horizontal and rolled the body to its cap with the lift sideways
      thrust.z = std::max(thrust.z, aSpec.tiltFloor * g);
      size[i] = Len(thrust);
      up[i] = thrust.z;
      lat[i] = Dot(thrust, side[i]);
      const float ahead = Dot(thrust, h);
      // slowing, the body takes brakeShare: the nose pulls up less than it leans speeding up and
      // reverse surge slows the car, as a player pulls back on the surge rather than the nose
      const float along = ahead >= 0.0f ? aSpec.surge : Decel(aSpec, q.speed) - Drag(aSpec, q.speed);
      const float share = ahead >= 0.0f ? aParams.bodyShare : aSpec.brakeShare;
      pitch[i] = std::clamp(bodyAngle(std::atan2(ahead, thrust.z), size[i], along, share), -cap, cap);
    }
    // a coming turn's roll held from rollLook before it (same side, or nearly level now)
    for (size_t i = 0; i < n; ++i) {
      float lateral = lat[i];
      for (size_t j = i + 1; j < n && out.samples[j].t - out.samples[i].t <= aParams.rollLook; ++j) {
        const float coming = lat[j];
        if (std::abs(coming) > std::abs(lateral) && (coming * lateral >= 0.0f || std::abs(lateral) < 2.0f)) {
          lateral = coming;
        }
      }
      roll[i] = std::clamp(bodyAngle(std::atan2(lateral, up[i]), size[i], aSpec.sway, aParams.bodyShare), -cap, cap);
    }
    envelope(roll, aParams.rollRateMax);
    envelope(pitch, aParams.pitchRateMax);
    for (size_t i = 0; i < n; ++i) {
      out.samples[i].pitch = pitch[i];
      out.samples[i].roll = roll[i];
    }
  }

  // check against the car's full limits (the plan uses margin of them)
  const float latMax = Lateral(aSpec) * 1.02f;
  for (size_t i = 0; i < n; ++i) {
    const auto &q = out.samples[i];
    char text[160];
    auto flag = [&](const char *aWhat, float aHave, float aMax) {
      if (out.violations.size() < 20) {
        std::snprintf(text, sizeof(text), "s %.0f m: %s %.1f over %.1f at %.1f m/s", q.s, aWhat, aHave, aMax, q.speed);
        out.violations.push_back(text);
      }
    };
    if (q.speed > out.top * 1.01f) {
      flag("speed", q.speed, out.top);
    }
    const float latHere = LateralAt(aSpec, q.acceleration.z + kG * aSpec.gravityFactor) * 1.02f;
    if (q.lateral > std::min(latMax, latHere)) {
      flag("lateral", q.lateral, std::min(latMax, latHere));
    }
    if (q.along > Accel(aSpec, q.speed) * 1.05f + 0.5f) {
      flag("accel", q.along, Accel(aSpec, q.speed));
    }
    if (-q.along > Decel(aSpec, q.speed) * 1.05f + 0.5f) {
      flag("decel", -q.along, Decel(aSpec, q.speed));
    }
    const float upNeed = q.acceleration.z + kG * aSpec.gravityFactor;
    if (upNeed > kG * aSpec.gravityFactor + aSpec.lift * 1.05f) {
      flag("lift", upNeed, kG * aSpec.gravityFactor + aSpec.lift);
    }
  }
  return out;
}

} // namespace Navigation
