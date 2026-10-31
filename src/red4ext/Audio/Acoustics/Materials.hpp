#pragma once

// Reflectivity by band per physics material (docs/design/audio-native.md D.12): how much of the low
// (below 250 Hz), mid and high (above 2.5 kHz) part of a sound a surface sends back. Pure; no game
// or FMOD types.

#include <optional>
#include <string_view>

namespace Acoustics {

// -1: unknown. Constructors rather than an aggregate so one number sets all three (a flat surface).
struct Bands {
  float low = -1, mid = -1, high = -1;
  constexpr Bands() = default;
  constexpr Bands(float aFlat) : low(aFlat), mid(aFlat), high(aFlat) {}
  constexpr Bands(float aLow, float aMid, float aHigh) : low(aLow), mid(aMid), high(aHigh) {}
};

namespace Materials {

// The bands of a .physmat name the game reports, matched by prefix (the first entry that fits);
// nullopt for a material not in the table.
std::optional<Bands> Find(std::string_view aPhysmat);

} // namespace Materials

} // namespace Acoustics
