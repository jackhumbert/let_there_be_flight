#pragma once

// The effects of a flying car's scrapes (docs/design/scrape-effects.md): each scrape the component's tracker holds
// (Flight::ScrapeTracker, fed by the contacts in Physics/ScrapeAudio.cpp) gets a spark effect and a trail decal effect
// for its surface, spawned through the game's FxSystem where it begins and moved with the car every frame, so the
// trail is laid along the slide; its loops are broken when the scrape ends.

#include "Flight/Scrape.hpp"

#include <cstdint>

namespace RED4ext::vehicle {
struct BaseObject;
}
class FlightComponent;

namespace ScrapeEffects {

// Steps the component's scrapes and moves, starts and ends their effects; ends them all when flight is off.
void Update(FlightComponent *aComponent, RED4ext::vehicle::BaseObject *aVehicle, float aDt);

// The surface of a physics material by its CName's hash (Flight::SurfaceOf its name), cached; None for none (0).
Flight::Surface SurfaceOfMaterial(uint64_t aMaterial);

// A physics material's grip, a factor on Ground Friction (Physics/ScrapeAudio.cpp's table); 1 for none (0), as hard
// ground.
float FrictionOfMaterial(uint64_t aMaterial);

} // namespace ScrapeEffects
