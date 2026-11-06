#pragma once

// Hover & Fly flown along lines over the city's real ground (docs/design/hover-fly-terrain.md, step 4): each line's
// ground profile and what kind of ground it is, random lines drawn in a region, and a driver that holds the speed and
// the heading along the line and never touches lift, so the mode alone keeps the height.
//
// The lines (terrain_runs.tsv): tab-separated, '#' comments; name, region (a region file's name: <regions>/<region>
// .ltbfregion, region_pack.py --regions), x0, y0, x1, y1 (m, world), z0 (the start's surface: the one at or under
// it; empty for the topmost), tags (comma-separated, what the ground is: Profile::Tags).
//
// The Sim models height only: nothing stops the car sideways. A line is flown only up to its first wall (the
// profile's usable length), and a run stops (blocked) if its nose runs through anything half a metre over its
// underside in a frame, or the contact lifts the car more than kWallLift in one.

#include "Colliders.hpp"
#include "Scenarios.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace FlightSim {

struct Line {
  std::string name, region;
  Vec3 a, b; // z unused
  float z0 = std::numeric_limits<float>::quiet_NaN();
  std::string tags;
};

// The lines of a runs file; empty when it cannot be read, bad lines skipped.
std::vector<Line> LoadLines(const std::string &aPath);
// One line a row, as LoadLines reads it.
std::string FormatLine(const Line &aLine);

// Every surface over (x, y), top down, each with the room over it up to the next one (infinity for the topmost).
struct Surface {
  float z = 0, headroom = 0;
  Colliders::Kind kind = Colliders::Collision;
};
std::vector<Surface> Surfaces(const Colliders &aWorld, float aX, float aY);

// The ground along a line as the car would follow it: from the start's surface (the topmost, or the one at or under
// aZ0), each kStep the ground cast from kFollowAbove over the last one down, cut at the first wall (a ray from the
// last ground to the next point hits at any of kWallHeights over the ground, where the car's body is, along the line
// or kWallSide to either side of it, the widest car's half width). A start with something within kWallSide either
// side of it is no line (usable 0): a car does not fit there (in game a car placed in a 1.5 m gap between a fence and
// a billboard stuck on its side, 2026-11-06).
struct Profile {
  struct Point {
    float s = 0, z = 0;
    bool found = false;
    Colliders::Kind kind = Colliders::Collision;
  };
  std::vector<Point> points;
  float length = 0; // m of the line
  float usable = 0; // m before the first wall
  // over the usable part
  float slopeP95 = 0; // rise over 2 m, p95
  float stepMax = 0;  // m, the largest change between neighbouring points
  int kerbs = 0;      // changes of 0.08 to 0.5 m between neighbours, against the slope round them
  int steps = 0;      // changes over 0.5 m
  int drops = 0;      // falls over 1 m
  float bumpRms = 0;  // m off the 10 m running mean
  float crest = 0, dip = 0; // m the ground stands over (or sinks under) the mean of the ground 10 m either side, at most
  float terrain = 0, gaps = 0; // the share of the points on terrain, and with no ground found
  std::string Tags() const;    // what the numbers make of it: flat, slope, kerbs, steps, drop, crest, dip, rough, ...
};
constexpr float kStep = 0.5f, kFollowAbove = 2.5f;
constexpr float kWallHeights[] = {0.5f, 1.0f, 2.0f};
constexpr float kWallSide = 1.6f;
Profile GroundProfile(const Colliders &aWorld, Vec3 aA, Vec3 aB,
                      float aZ0 = std::numeric_limits<float>::quiet_NaN());

// aCount lines of aLength m in the region (its bounds less kSampleInset), random start, heading and start surface from
// aSeed and the region's name, each cut at its first wall and kept when at least aMinLength m are left; named
// <region>_<k>, tagged by their profile. A start surface is open to the sky (a street, a roof) or has kSampleHeadroom
// to kSampleHeadroomMax over it (a street under a deck), and is no lower than kSampleLowest: the bake has no water,
// so the sea bed is a surface, and so is the terrain left under the city's streets, roofed by them 15 to 25 m up.
constexpr float kSampleInset = 30.0f, kSampleHeadroom = 4.0f, kSampleHeadroomMax = 12.0f, kSampleLowest = -1.0f;
std::vector<Line> SampleLines(const Colliders &aWorld, const std::string &aRegion, int aCount, float aLength,
                              float aMinLength, uint32_t aSeed);

// m the contact may lift the car in one frame before the run counts it as driven into a wall and stops; and the least
// up a surface's normal is for the nose running into it to be rising ground (45 degrees), not a wall
constexpr float kWallLift = 1.0f, kGroundNormalZ = 0.7f;

// One line flown at aSpeed (m/s) holding aHover m over the ground (0: the height flight on from the ground gives the
// car, its resting height held at least at the lowest hover height) (Hover & Fly's hover height, from the wheel
// emitters): placed level at the start, already at speed along the line. The frames and the metrics (named
// <metric>, units m, m/s, m/s2, m/s3, deg, s): speed; the height's error (the mode's own: ground distance against hover
// height; RMS, p95, and p95 of the air over it and of the sink under it apart); lag (s the car's height trails the
// profile's ground under it, the delay that lines them up best, up to 1.5 s; NaN where the ground changes by under
// 0.3 m); the underside's clearance (min, p05); contacts (count, a touch already on at the start counted, time, worst
// impact, and where the first touch of each was: front, middle, back); the vertical acceleration (max, RMS) and jerk
// (p95), contact frames left out; pitch_wobble (RMS of the pitch against the profile's slope over the wheelbase) and
// roll_wobble (RMS of the roll off its own 1 s running mean); the times the nose ran into rising ground (nose_hits:
// the underside's contact does not see the front's face); rays a frame; the metres flown and whether a wall stopped
// it. The first second is left out of all but the last two.
Run FlyLine(const Setup &aSetup, const Colliders &aWorld, const Line &aLine, const Profile &aProfile, float aSpeed,
            float aHover);

} // namespace FlightSim
