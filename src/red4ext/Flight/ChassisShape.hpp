#pragma once

// The vehicle's chassis collision shape as convex pieces for Flight::SilhouetteAreas (Flight/Aero.hpp), read from
// the chassis component's collisionResource (a physicsSystemResource), as tools/flightsim/aero.py reads the .phys files.

#include "Flight/Aero.hpp"

#include <RED4ext/Scripting/Natives/vehicleChassisComponent.hpp>
#include <string>
#include <vector>

namespace Flight {

// The shape's box, convex, sphere and capsule colliders as point sets in the vehicle's frame (each collider's
// localToBody, its body's localToModel and the chassis component's localTransform). False with no pieces: while the
// resource is still loading (the load is started, so a later call finds it) or when it has none of those colliders;
// aWhy says which.
bool ChassisPieces(RED4ext::vehicle::ChassisComponent *aChassis, std::vector<ShapePiece> &aPieces, std::string &aWhy);

} // namespace Flight
