#include "FlightGuidance.hpp"

#include "Flight/FlightModeAutopilot.hpp"
#include "Flight/FlightStats.hpp"
#include "FlightNavMap.hpp"
#include "FlightSystem.hpp"
#include "Guidance.hpp"
#include "NavMap.hpp"

#include <mutex>
#include <vector>

namespace {

// how often the tracked destination is read (a script call through vanilla's autodrive system), s
constexpr float kDestinationPeriod = 0.25f;

// Written by Update (the flight system's update), read by the script API from any thread.
struct Published {
  bool flying = false;
  bool fromAutopilot = false;
  int32_t revision = 0;
  float offLine = 0.0f;
  std::vector<Navigation::Vec3> route;
};

std::mutex lock;
Published published;

// the update's own state, touched by Update and Reset only
struct State {
  Navigation::Guidance guidance{FlightNavMap::Queue(), Navigation::GuidanceParams{}};
  float destinationClock = 0.0f;
  bool hasDestination = false;
  Navigation::Vec3 destination{};
};

State &Updating() {
  static State state;
  return state;
}

// The player's component's modes and the one it flies, from its script fields (FlightComponent.reds: modes, mode).
struct Modes {
  FlightModeAutopilot *autopilot = nullptr; // the Autopilot mode in the list, engaged or not
  bool autopilotActive = false;             // it is the mode being flown
};

Modes FindModes(RED4ext::IScriptable *aComponent) {
  Modes out;
  auto rtti = RED4ext::CRTTISystem::Get();
  static auto *cls = rtti->GetClass("FlightComponent");
  static auto *modesProp = cls ? cls->GetProperty("modes") : nullptr;
  static auto *modeProp = cls ? cls->GetProperty("mode") : nullptr;
  static auto *autopilotClass = rtti->GetClass("FlightModeAutopilot");
  if (!aComponent || !modesProp || !modeProp || !autopilotClass) {
    return out;
  }
  auto &modes = *modesProp->GetValuePtr<RED4ext::DynArray<RED4ext::Handle<RED4ext::IScriptable>>>(aComponent);
  int32_t mode = *modeProp->GetValuePtr<int32_t>(aComponent);
  // FlightComponent.ModeInList: an index past the car's list is its first mode
  if (mode < 0 || mode >= int32_t(modes.size)) {
    mode = 0;
  }
  for (uint32_t i = 0; i < modes.size; ++i) {
    auto *m = modes[i].GetPtr();
    if (m && m->GetType()->IsA(autopilotClass)) {
      out.autopilot = static_cast<FlightModeAutopilot *>(m);
      out.autopilotActive = int32_t(i) == mode;
      break;
    }
  }
  return out;
}

} // namespace

void FlightGuidance::Update(float aTimeDelta) {
  State &s = Updating();
  Navigation::GuidanceInput in;
  in.timeDelta = aTimeDelta;

  auto fs = FlightSystem::GetInstance();
  auto component = fs ? fs->playerComponent.Lock() : RED4ext::Handle<FlightComponent>{};
  FlightStats *stats = component ? FlightStats::Of(component.GetPtr()) : nullptr;
  in.flying = component && component->active && stats;

  if (in.flying && Navigation::NavMap::Get().IsLoaded()) {
    const Modes modes = FindModes(component.GetPtr());
    in.car = {stats->d_position.X, stats->d_position.Y, stats->d_position.Z};
    in.velocity = {stats->d_velocity.X, stats->d_velocity.Y, stats->d_velocity.Z};

    s.destinationClock -= aTimeDelta;
    if (s.destinationClock <= 0.0f) {
      s.destinationClock = kDestinationPeriod;
      s.hasDestination = modes.autopilot && modes.autopilot->TrackedTarget(s.destination);
    }
    in.hasDestination = s.hasDestination;
    in.destination = s.destination;

    if (modes.autopilot) {
      if (modes.autopilotActive && modes.autopilot->FlownRoute().Size() >= 2) {
        in.autopilot = &modes.autopilot->FlownRoute();
      }
      // a hand-flown line in the Autopilot's race character, without the car's limits (shown, not flown)
      const auto &p = modes.autopilot->params;
      const Navigation::RouteShape shape{p.raceRouteHeight, p.raceClimbCost, p.raceMinPassage, p.raceSearchWeight,
                                         p.raceTurnCost};
      in.plan = Navigation::RaceParams(FlightNavMap::PlanBase(), shape, nullptr, false, {}, in.velocity);
    } else {
      in.hasDestination = false;
    }
  } else {
    in.flying = false;
    s.destinationClock = 0.0f;
  }

  s.guidance.Step(in);

  std::lock_guard guard(lock);
  published.flying = in.flying;
  published.fromAutopilot = s.guidance.FromAutopilot();
  published.revision = int32_t(s.guidance.Revision());
  published.offLine = s.guidance.OffLine();
  published.route = s.guidance.Ahead();
}

void FlightGuidance::Reset() {
  State &s = Updating();
  s.guidance.Clear();
  s.hasDestination = false;
  s.destinationClock = 0.0f;
  std::lock_guard guard(lock);
  const int32_t revision = published.revision;
  published = Published{};
  published.revision = revision;
}

bool FlightGuidance::IsFlying() {
  std::lock_guard guard(lock);
  return published.flying;
}

bool FlightGuidance::HasRoute() {
  std::lock_guard guard(lock);
  return published.route.size() >= 2;
}

bool FlightGuidance::IsAutopilotRoute() {
  std::lock_guard guard(lock);
  return published.route.size() >= 2 && published.fromAutopilot;
}

RED4ext::DynArray<RED4ext::Vector4> FlightGuidance::GetRoute() {
  RED4ext::DynArray<RED4ext::Vector4> out;
  std::lock_guard guard(lock);
  if (published.route.size() >= 2) {
    out.Reserve(uint32_t(published.route.size()));
    for (const auto &p : published.route) {
      out.EmplaceBack(p.x, p.y, p.z, 1.0f);
    }
  }
  return out;
}

int32_t FlightGuidance::GetRouteRevision() {
  std::lock_guard guard(lock);
  return published.revision;
}

float FlightGuidance::GetDistanceToRoute() {
  std::lock_guard guard(lock);
  return published.route.size() >= 2 ? published.offLine : 0.0f;
}
