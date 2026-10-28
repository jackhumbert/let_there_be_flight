#pragma once

// Gates: openings between buildings (and under things) for the Autopilot's free roam to fly through,
// mined from the nav map's doorways (tools/heightmap/gates.py), loaded from gates.tsv in the plugin
// folder. Read once at plugin load and read-only afterwards, so queries need no locking.

#include <cstdint>
#include <filesystem>
#include <vector>

namespace Navigation {

struct Gate {
  float x, y, z; // the opening's centre at the height to fly it, metres
  float yaw;     // degrees, the way through (either direction)
  float width;   // free space across on the nav map, metres (the car's radius already off)
  float wall;    // how far the lower of the two walls rises above the gate, metres
  float ceiling; // free height above the gate, metres; +infinity for open sky
};

class Gates {
public:
  static Gates &Get();

  bool Load(const std::filesystem::path &aPath);
  bool IsLoaded() const { return loaded; }

  const std::vector<Gate> &All() const { return gates; }

  // Indices of the gates within aRadius metres of (x, y) (horizontal), nearest first.
  std::vector<int32_t> InRange(float aX, float aY, float aRadius) const;

private:
  bool loaded = false;
  std::vector<Gate> gates;
};

} // namespace Navigation
