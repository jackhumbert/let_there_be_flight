#pragma once

// The simulator checked against the game: a Hover & Fly recording (HoverFlyRecorder in cp2077_tools'
// mods/let_there_be_flight/ltbf_bridge.reds, ltbf_hoverfly_<time>.csv) is cut where a control starts, and each piece
// is flown offline from the recorded state on the recorded controls and ground distance. What differs is the body
// model (Sim.hpp): its gains, its delay, and what it leaves out.

#include "Scenarios.hpp"
#include "Sim.hpp"

#include <string>
#include <vector>

namespace FlightSim {

// One row of the recording.
struct Recorded {
  float t = 0, dt = 0;
  Vec3 position, velocity, forward, right, angularVelocity;
  Flight::Controls controls;
  float distance = 0, hovering = 0, hoverHeight = 0, referenceZ = 0;
  bool holding = false;
  int mode = -1;
};

std::vector<Recorded> LoadRecording(const std::string &aPath);

// One piece: from a control's start to the next one's, the game's and the simulator's peaks over it.
struct Piece {
  float start = 0, length = 0;
  std::string what;                         // the control that starts it ("yaw+", "lift-", "none")
  float gameYawRate = 0, simYawRate = 0;    // deg/s, the largest
  float gameClimb = 0, simClimb = 0;        // m/s, the largest vertical speed either way (signed)
  float gameSpeed = 0, simSpeed = 0;        // m/s, horizontal at the end
  float gameTilt = 0, simTilt = 0;          // deg, the largest
  float heightError = 0, positionError = 0; // m, the largest
};

// Flies every piece of a recording made in Hover & Fly (mode index hoverFlyMode); rows in another mode are skipped.
// aTrace, when not empty, gets the simulator's frames beside the game's (CSV).
std::vector<Piece> Replay(const std::vector<Recorded> &aRows, const Setup &aSetup, int aHoverFlyMode,
                          const std::string &aTrace);

} // namespace FlightSim
