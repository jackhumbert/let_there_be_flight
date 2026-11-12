#pragma once

// The terrain runs checked against the game (docs/design/hover-fly-terrain.md, step 6): a recording of
// a car driven along the terrain lines (the Hover & Fly recording's columns, then
// FindGround's four rays a frame: start, hit, hit point, normal) against a region of the city's collision.
// - GroundCheck casts each recorded ray again through the region: where both hit, how far apart and how differently
//   tilted; where only one did (missing or extra collision: streaming, a blocker the region kept, water, which the
//   bake has none of). Also where the wheel emitters sit on the recorded body, against the car's vehicles.tsv row.
// - TerrainReplay flies windows of the recording offline from the recorded state on the recorded controls, with
//   the ground from the region (Sim::groundRay), and sets the heights side by side.

#include "Colliders.hpp"
#include "Replay.hpp"

#include <array>
#include <string>
#include <vector>

namespace FlightSim {

// One recorded ray of FindGround.
struct RecordedRay {
  Vec3 origin, hit, normal;
  bool valid = false;
};

// A TerrainDriver recording: the rows LoadRecording reads, and each row's four rays (fl, fr, bl, br).
struct TerrainRecording {
  std::vector<Recorded> rows;
  std::vector<std::array<RecordedRay, 4>> rays;
};
TerrainRecording LoadTerrainRecording(const std::string &aPath);

struct GroundReport {
  int rays = 0, both = 0, gameOnly = 0, regionOnly = 0, neither = 0;
  // over the rays both hit: |game z - region z| at the 50th and 95th percentile and the largest, m; the angle
  // between the normals at the 95th percentile, deg
  float dzP50 = 0, dzP95 = 0, dzMax = 0, normalP95 = 0;
  // the 2 m cells where they disagree most: centre, the game's and the region's height (NaN where one missed), rays
  struct Spot {
    float x = 0, y = 0, gameZ = 0, regionZ = 0;
    int rays = 0;
  };
  std::vector<Spot> spots;
  // the wheel emitters on the recorded body (the mean of |x|, |y| and z of each ray's start in the body frame,
  // from the recorded position)
  Vec3 wheel;
  // the game's car against the region, as the simulator's contact measures it (Sim::Underside: the tyres and the
  // chassis's underside of aCar, at the recorded pose): the lowest clearance, the frames within kTouch of the ground
  // and the touches (runs of such frames)
  float gameClearanceMin = 0;
  int gameTouchFrames = 0, gameTouches = 0;
};
constexpr float kTouch = 0.03f;
// aSpots: how many disagreeing cells to list (those over aDz apart, or found by one only)
GroundReport GroundCheck(const TerrainRecording &aRecording, const Colliders &aRegion, const Car &aCar,
                         float aDz = 0.25f, size_t aSpots = 12);

// One window of the replay.
struct TerrainPiece {
  float start = 0, length = 0;
  float heightRms = 0, heightMax = 0;     // m, the simulator's height off the game's
  float distanceRms = 0;                  // m, the simulator's ground distance off the game's (both found)
  float gameClimb = 0, simClimb = 0;      // m/s, the largest vertical speed either way (signed)
  int simContacts = 0;
};
// Windows of aWindow s, each from the recorded state, rows in Hover & Fly (mode index aHoverFlyMode) only.
std::vector<TerrainPiece> TerrainReplay(const TerrainRecording &aRecording, const Colliders &aRegion,
                                        const Setup &aSetup, int aHoverFlyMode, float aWindow,
                                        const std::string &aTrace);

} // namespace FlightSim
