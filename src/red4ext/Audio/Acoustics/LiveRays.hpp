#pragma once

// The near rays through the game's own PhysX scene (docs/design/acoustics.md): cast from the
// player's flying car's fixed update (Physics/VehiclePhysicsUpdate.cpp), where the game runs its own
// wheel raycasts, about 20 times a second when Acoustics asks for them (RaySource::PhysX).

struct FlightComponent;

namespace Acoustics {

// Casts if due and aComponent's chassis is the car the scripts reported (within a few metres).
void CastLiveRays(FlightComponent *aComponent);

} // namespace Acoustics
