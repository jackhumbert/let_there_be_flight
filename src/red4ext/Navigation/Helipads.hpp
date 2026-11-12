#pragma once

// Helipads: the AV landing pads found in the world sectors (tools/pads/pads.py,
// docs/design/landing-pads.md), loaded from helipads.tsv in the plugin folder.
//
// Positions are the deck the car rests on (not the car's centre). Read once at plugin load and
// read-only afterwards, so queries need no locking.

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Navigation {

struct Helipad {
  std::string id;   // the pad prefab's global node id, or <sector>/<slot> for a painted marking
  std::string name; // empty unless named in tools/pads/helipads_overrides.json
  float x, y, z;    // deck centre, metres
  float yaw;        // degrees, from the pad prefab or its marking
  float radius;     // pad size, metres
  float clearance;  // free height above the deck, metres; +infinity for open sky
};

class Helipads {
public:
  static Helipads &Get();

  bool Load(const std::filesystem::path &aPath);
  bool IsLoaded() const { return loaded; }

  const std::vector<Helipad> &All() const { return pads; }

  // Index of the pad nearest (x, y) within aMaxDistance metres (horizontal), or -1.
  int32_t Nearest(float aX, float aY, float aMaxDistance) const;
  // Indices of the pads within aRadius metres of (x, y) (horizontal), nearest first.
  std::vector<int32_t> InRange(float aX, float aY, float aRadius) const;

private:
  bool loaded = false;
  std::vector<Helipad> pads;
};

} // namespace Navigation
