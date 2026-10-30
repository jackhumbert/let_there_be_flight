#pragma once

// Geometry for drawing the nav map in game (FlightNavMap.GetOutlines / GetNavEdges, drawn with
// debug_draw by tools/ingame/ltbf_draw.reds). No game types.

#include "NavMap.hpp"

#include <vector>

namespace Navigation {

struct DrawPoint {
  float x, y, z;
};

// The blocked space's outlines within aRadius of (aX, aY), sliced at each of aHeights: every edge
// between a blocked and a free cell, as point pairs (a, b, a, b, ...).
void Outlines(const NavMap &aMap, float aX, float aY, float aRadius, const std::vector<float> &aHeights,
              std::vector<DrawPoint> &aOut);

// The free space as a wireframe around (aX, aY), heights aZMin to aZMax, as point pairs: wherever one
// of two neighbouring columns is free and the other blocked, the face on their shared edge as its
// bottom and top lines, plus a vertical line at each corner of the free space. Columns are sampled
// every cell to 60 m out, every 2 cells to 150 m and every 4 beyond, so the map can be shown far out.
void NavEdges(const NavMap &aMap, float aX, float aY, float aRadius, float aZMin, float aZMax,
              std::vector<DrawPoint> &aOut);

// The blocked space around (aX, aY), heights aZMin to aZMax, as boxes (min, max, min, max, ...):
// each column's blocked height ranges, merged with the same range in the next columns along x,
// then those runs with the same run in the next rows along y. To fill the map in solid.
void NavBlocks(const NavMap &aMap, float aX, float aY, float aRadius, float aZMin, float aZMax,
               std::vector<DrawPoint> &aOut);

} // namespace Navigation
