#pragma once

// The flight route shown to the player (docs/design/guidance.md): while the player flies, the line
// from the car to the tracked destination, for other mods to draw (FlightGuidance, the script API).
// With the Autopilot engaged it is the Autopilot's own route; flown by hand it is planned here on
// the PlanQueue the Autopilot uses, as a Race to the destination, again when the destination moves
// and as a rejoin when the car leaves the line. Pure code: the caller gathers the car and the
// destination each frame (FlightGuidance.cpp in the game, the tests offline).

#include "Plans.hpp"
#include "Route.hpp"

#include <cstdint>
#include <vector>

namespace Navigation {

struct GuidanceParams {
  float destinationMoved = 25.0f; // m the destination moves before the route is planned again
  float offLine = 40.0f;          // m from the line before a rejoin is planned
  float lost = 300.0f;            // m from the line (a teleport): planned again from the car, not rejoined
  float rejoinAhead = 150.0f;     // m along the line a rejoin joins it
  float retry = 3.0f;             // s after a failed plan before the next
  int32_t searchAhead = 40;       // segments past the car's last one searched for its place on the line
};

struct GuidanceInput {
  bool flying = false;         // the player's vehicle is in flight
  bool hasDestination = false; // something is tracked (FlightModeAutopilot.GetDestination)
  Vec3 destination{};
  Vec3 car{};
  Vec3 velocity{};
  // the Autopilot's route while it is engaged (null when it is not, or has none yet): shown as it is
  const Route *autopilot = nullptr;
  // the planner's params for a plan from the car (RaceParams with the Autopilot's race shape)
  PlanParams plan{};
  float timeDelta = 0.0f;
};

class Guidance {
public:
  Guidance(PlanQueue &aPlans, const GuidanceParams &aParams) : plans(aPlans), params(aParams) {}
  ~Guidance() { Clear(); }
  Guidance(const Guidance &) = delete;
  Guidance &operator=(const Guidance &) = delete;

  // one frame: takes a finished plan, asks for a new one when needed, and moves the car's place on the line
  void Step(const GuidanceInput &aIn);
  // no route, no request (flight off, nothing tracked, the world detached)
  void Clear();

  bool HasRoute() const { return line.Size() >= 2; }
  // the line is the Autopilot's route (false: planned here, or none)
  bool FromAutopilot() const { return fromAutopilot; }
  // changes with every new line (not as the car moves along it)
  uint32_t Revision() const { return revision; }
  // The line ahead of the car, car end first: the point on the line nearest the car, then the line's
  // points after it to the destination end. Empty without a route.
  std::vector<Vec3> Ahead() const;
  // m from the car to the line at its place on it (0 without a route)
  float OffLine() const { return offLine; }

private:
  void SetLine(std::vector<Vec3> aPoints, bool aFromAutopilot);
  void Drop();
  void Request(const GuidanceInput &aIn);

  PlanQueue &plans;
  GuidanceParams params;
  Route line;
  bool fromAutopilot = false;
  uint32_t revision = 0;
  int32_t segment = 0; // the car's segment on the line
  Vec3 onLine{};       // the car's place on it
  float offLine = 0.0f;
  int32_t request = 0;     // the plan asked for, 0 for none
  Vec3 plannedFor{};       // the destination the line (or the request) goes to
  bool planned = false;    // plannedFor is set
  float retryTimer = 0.0f; // s before another plan after a failure
  // the Autopilot route last copied: its size, length and first point, to tell a new one
  int32_t apSize = 0;
  float apLength = 0.0f;
  Vec3 apFirst{};
};

} // namespace Navigation
