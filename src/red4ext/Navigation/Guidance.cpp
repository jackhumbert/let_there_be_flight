// The flight route shown to the player (Guidance.hpp, docs/design/guidance.md).

#include "Guidance.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace Navigation {

namespace {
float Distance(const Vec3 &aA, const Vec3 &aB) { return std::hypot(aA.x - aB.x, aA.y - aB.y, aA.z - aB.z); }
} // namespace

void Guidance::Clear() {
  Drop();
  if (HasRoute()) {
    ++revision;
  }
  line = Route{};
  fromAutopilot = false;
  segment = 0;
  offLine = 0.0f;
  planned = false;
  retryTimer = 0.0f;
  apSize = 0;
  apLength = 0.0f;
}

void Guidance::Drop() {
  if (request > 0) {
    plans.Drop(request);
  }
  request = 0;
}

void Guidance::SetLine(std::vector<Vec3> aPoints, bool aFromAutopilot) {
  line = Route::Make(std::move(aPoints));
  fromAutopilot = aFromAutopilot;
  segment = 0;
  ++revision;
}

void Guidance::Request(const GuidanceInput &aIn) {
  request = plans.Race(aIn.car, aIn.destination, aIn.plan);
  plannedFor = aIn.destination;
  planned = true;
}

void Guidance::Step(const GuidanceInput &aIn) {
  if (!aIn.flying || !aIn.hasDestination) {
    if (planned || HasRoute() || request > 0) {
      Clear();
    }
    return;
  }

  // the Autopilot flies its own route: show that one, copied again when it changes
  if (aIn.autopilot && aIn.autopilot->Size() >= 2) {
    Drop();
    const Route &ap = *aIn.autopilot;
    if (!fromAutopilot || ap.Size() != apSize || ap.Length() != apLength || Distance(ap.Point(0), apFirst) > 0.01f) {
      apSize = ap.Size();
      apLength = ap.Length();
      apFirst = ap.Point(0);
      SetLine(ap.points, true);
    }
    // flown by hand after it disengages, the line goes on from here
    plannedFor = aIn.destination;
    planned = true;
  } else {
    apSize = 0;
    fromAutopilot = false;
    retryTimer = std::max(0.0f, retryTimer - aIn.timeDelta);

    if (request > 0 && plans.Ready(request)) {
      TakenPlan taken;
      const bool took = plans.Take(request, taken);
      request = 0;
      if (took && taken.plan.ok && taken.plan.points.size() >= 2) {
        SetLine(taken.plan.points, false);
      } else {
        spdlog::info("[Guidance] no route: {}", taken.plan.info);
        retryTimer = params.retry;
      }
    }

    // a destination somewhere else: the old line leads to the wrong place
    if (planned && Distance(aIn.destination, plannedFor) > params.destinationMoved) {
      Drop();
      if (HasRoute()) {
        line = Route{};
        ++revision;
      }
      planned = false;
      retryTimer = 0.0f;
    }
  }

  if (HasRoute()) {
    segment = line.Nearest(aIn.car, std::max(0, segment - 2), params.searchAhead);
    onLine = line.Project(aIn.car, segment);
    offLine = Distance(aIn.car, onLine);
  } else {
    offLine = 0.0f;
  }

  if (fromAutopilot || request > 0 || retryTimer > 0.0f) {
    return;
  }
  if (!HasRoute()) {
    Request(aIn);
  } else if (offLine > params.lost) {
    // far from the line (a teleport): planned again from the car
    line = Route{};
    ++revision;
    Request(aIn);
  } else if (offLine > params.offLine) {
    std::vector<Vec3> rest(line.points.begin() + std::clamp(segment, 0, line.Size() - 1), line.points.end());
    request = plans.Rejoin(aIn.car, std::move(rest), params.rejoinAhead, aIn.plan);
  }
}

std::vector<Vec3> Guidance::Ahead() const {
  std::vector<Vec3> out;
  if (!HasRoute()) {
    return out;
  }
  out.reserve(size_t(line.Size() - segment));
  out.push_back(onLine);
  for (int32_t i = segment + 1; i < line.Size(); ++i) {
    out.push_back(line.points[size_t(i)]);
  }
  return out;
}

} // namespace Navigation
