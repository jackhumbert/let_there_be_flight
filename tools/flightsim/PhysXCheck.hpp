#pragma once

// flightsim --physx-joint (PhysXCheck.cpp): a body sliding over flush mesh joints in the game's own PhysX.

#include <string>

namespace FlightSim {

struct PhysXJointSetup {
  std::string dllDir;      // the game's bin/x64; empty: the one flightsim was built with
  bool pcm = false;        // PxSceneFlag::eENABLE_PCM
  float openBottom = 0.0f; // over 0: the thrusters are boxes open this far up, not spheres
  float lift = 0.0f;       // over 0: held this far over resting on its shapes (m), as the skid holds it
};

// Prints, for one mesh, two and the road's 10.4 m meshes, the body's highest vertical speed and rise and how often it
// was kicked up; 2 when PhysX cannot be loaded.
int PhysXJointCheck(const std::string &aDllDir, const PhysXJointSetup &aSetup);

} // namespace FlightSim
