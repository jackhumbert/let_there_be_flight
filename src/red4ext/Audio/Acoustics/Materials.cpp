#include "Materials.hpp"

namespace Acoustics::Materials {

namespace {

struct Entry {
  std::string_view prefix;
  Bands bands;
};

// Mid is the single value this table had before; hard and smooth surfaces high, soft and broken ones
// low. Soft surfaces keep their lows and lose their highs (grass, fabric, foliage), glass the other way.
constexpr Entry kTable[] = {
    {"glass", {0.80f, 0.95f, 0.95f}},      {"metal", {0.90f, 0.90f, 0.90f}},   {"concrete", {0.90f, 0.85f, 0.80f}},
    {"stone", {0.90f, 0.85f, 0.80f}},      {"brick", {0.90f, 0.85f, 0.80f}},   {"asphalt", {0.85f, 0.75f, 0.60f}},
    {"plastic", {0.75f, 0.70f, 0.65f}},    {"wood", {0.70f, 0.60f, 0.50f}},    {"water", {0.85f, 0.80f, 0.75f}},
    {"dirt", {0.60f, 0.45f, 0.30f}},       {"mud", {0.55f, 0.40f, 0.25f}},     {"sand", {0.50f, 0.35f, 0.20f}},
    {"trash", {0.45f, 0.30f, 0.15f}},      {"cardboard", {0.45f, 0.30f, 0.15f}}, {"grass", {0.45f, 0.30f, 0.15f}},
    {"leaves", {0.35f, 0.20f, 0.10f}},     {"foliage", {0.35f, 0.20f, 0.10f}}, {"fabric", {0.35f, 0.20f, 0.05f}},
    {"upholstery", {0.35f, 0.20f, 0.05f}},
};

} // namespace

std::optional<Bands> Find(std::string_view aPhysmat) {
  for (const Entry &entry : kTable) {
    if (aPhysmat.substr(0, entry.prefix.size()) == entry.prefix) {
      return entry.bands;
    }
  }
  return std::nullopt;
}

} // namespace Acoustics::Materials
