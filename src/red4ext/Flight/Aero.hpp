#pragma once

// The car's drag area from its chassis collision shape (docs/research/2026-11-04-aero-drag.md), measured when flight
// starts: tools/flightsim/aero.py did the same offline from the .phys files.

#include "Flight/ScriptMath.hpp"

#include <vector>

namespace Flight {

// One convex piece of the collision shape as points in the body frame (X right, Y forward, Z up): a box's corners, a
// convex mesh's vertices, points round a sphere or capsule.
using ShapePiece = std::vector<Vec3>;

// The area of the shape's silhouette seen along X, Y and Z, m2: the union of each piece's projected convex hull,
// rasterised on aCell metre cells (aero.py's 2 cm). Pieces with fewer than 3 points add nothing.
Vec3 SilhouetteAreas(const std::vector<ShapePiece> &aPieces, float aCell = 0.02f);

// The shape's height (its extent along Z), m; 0 with no points.
float ShapeHeight(const std::vector<ShapePiece> &aPieces);

// The drag area per body axis (CdA, m2) for silhouette areas aAreas and a shape aHeight tall. The drag coefficient
// head-on rises with the height, as aero.py's body classes did (a coupe 1.25 m tall 0.30, a saloon 1.35 m 0.33, an
// SUV 1.8 m 0.42, a truck 2.35 m or more 0.70), and is 1.0 side-on and top-down (1.1 for the trucks). A bike's
// collider is the bike alone, so a seated rider leaning forward is added (0.6 side-on, 0.35 head-on, 0.3 top-down).
Vec3 DragAreaOf(Vec3 aAreas, float aHeight, bool aBike);

// A sphere's or a capsule's points for SilhouetteAreas: a ring of points round each of the three axes through each
// centre (aCentres one point for a sphere, both ends of the axis for a capsule), radius aRadius.
ShapePiece RoundPiece(const std::vector<Vec3> &aCentres, float aRadius);

} // namespace Flight
