#pragma once

// Seams in the city's drivable collision: where two pieces of ground meet without sharing an edge, and what a car
// sliding over the meeting finds there (a step up or down, a gap). A chassis dragged along the ground in Drone mode
// meets them with PhysX's mesh contacts; a step of a few millimetres can throw it (the drone launches over roads, Jack,
// 2026-11-06). flightsim --seams scans a region for them.

#include "Colliders.hpp"

#include <string>
#include <vector>

namespace FlightSim {

struct Seam {
  Vec3 at;               // the middle of the open edge
  Vec3 out;              // level, unit: across the edge, away from the triangle it bounds
  float length = 0;      // m of the edge
  float step = 0;        // m the ground across the edge (past the gap, if any) is over this side's
  float gap = 0;         // m to the ground across, where there is a gap (0 where the ground goes on)
  Colliders::Kind inside = Colliders::Collision, outside = Colliders::Collision;
};

// Ground counts as drivable within kSeamSlope of level (normal z at least); an open edge is one no other drivable
// triangle shares (corners matched to kSeamWeld); across it the ground is looked for kSeamProbe out, kSeamReach up and
// down, and up to kSeamGapMax further out when not there (an edge with no ground that close across is where the ground
// ends, not a seam, and is left out). Edges shorter than kSeamMinLength are left out.
constexpr float kSeamSlope = 0.9f, kSeamWeld = 0.002f, kSeamProbe = 0.03f, kSeamReach = 0.25f, kSeamGapMax = 0.5f,
                kSeamMinLength = 0.05f;
std::vector<Seam> FindSeams(const Colliders &aWorld);

} // namespace FlightSim
