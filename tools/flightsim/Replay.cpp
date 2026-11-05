#include "Replay.hpp"

#include "Scenarios.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace FlightSim {

namespace {
constexpr float kDeg = 57.2957795f;

float Axis(const Flight::Controls &c, int i) {
  const float v[] = {c.surge, c.lift, c.sway, c.yaw, c.roll, c.pitch, c.linearBrake, c.angularBrake};
  return v[i];
}
const char *kAxisNames[] = {"surge", "lift", "sway", "yaw", "roll", "pitch", "brake", "angularBrake"};

// The control that starts at row i (a smoothed control leaving 0); -1 for none.
int Starts(const std::vector<Recorded> &aRows, size_t i) {
  if (i == 0)
    return -1;
  for (int a = 0; a < 8; ++a)
    if (std::fabs(Axis(aRows[i - 1].controls, a)) < 0.01f && std::fabs(Axis(aRows[i].controls, a)) >= 0.01f)
      return a;
  return -1;
}

float Tilt(Vec3 aForward, Vec3 aRight) {
  return std::max(std::fabs(std::asin(std::clamp(aForward.z, -1.0f, 1.0f))),
                  std::fabs(std::asin(std::clamp(aRight.z, -1.0f, 1.0f)))) *
         kDeg;
}
} // namespace

std::vector<Recorded> LoadRecording(const std::string &aPath) {
  std::vector<Recorded> rows;
  std::ifstream in(aPath);
  std::string line;
  std::getline(in, line);
  while (std::getline(in, line)) {
    std::vector<float> v;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ','))
      v.push_back(std::strtof(cell.c_str(), nullptr));
    if (v.size() < 31)
      continue;
    Recorded r;
    r.t = v[0];
    r.dt = v[1];
    r.position = {v[2], v[3], v[4]};
    r.velocity = {v[5], v[6], v[7]};
    r.forward = {v[8], v[9], v[10]};
    r.right = {v[11], v[12], v[13]};
    r.angularVelocity = {v[14], v[15], v[16]};
    r.controls = {v[17], v[18], v[19], v[20], v[21], v[22], v[23], v[24]};
    r.distance = v[25];
    r.hovering = v[26];
    r.hoverHeight = v[27];
    r.holding = v[28] > 0.5f;
    r.referenceZ = v[29];
    r.mode = static_cast<int>(std::lround(v[30]));
    rows.push_back(r);
  }
  return rows;
}

std::vector<Piece> Replay(const std::vector<Recorded> &aRows, const Setup &aSetup, int aHoverFlyMode,
                          const std::string &aTrace) {
  std::vector<Piece> pieces;
  FILE *trace = aTrace.empty() ? nullptr : std::fopen(aTrace.c_str(), "w");
  if (trace)
    std::fprintf(trace, "t,piece,game_z,sim_z,game_vz,sim_vz,game_speed,sim_speed,game_wz,sim_wz,game_tilt,sim_tilt\n");

  // the pieces: runs of rows in Hover & Fly, cut where a control starts
  std::vector<std::pair<size_t, size_t>> spans;
  size_t from = aRows.size();
  for (size_t i = 0; i <= aRows.size(); ++i) {
    bool in = i < aRows.size() && aRows[i].mode == aHoverFlyMode;
    bool cut = !in || (from < i && Starts(aRows, i) >= 0);
    if (cut && from < i) {
      spans.emplace_back(from, i);
      from = aRows.size();
    }
    if (in && from == aRows.size())
      from = i;
  }

  for (auto [a, b] : spans) {
    if (b - a < 10)
      continue;
    Sim sim(aSetup.car, Flat());
    sim.standard = aSetup.standard;
    sim.hoverFly = aSetup.hoverFly;
    sim.force = aSetup.force;
    sim.shapeDrag = aSetup.shapeDrag;
    sim.thrustStrength = aSetup.thrustStrength;
    sim.thrustPerThruster = aSetup.thrustPerThruster;
    sim.mode = aSetup.mode;
    sim.assistParams = aSetup.assistParams;
    sim.drone = aSetup.drone;
    sim.droneAssist = aSetup.droneAssist;
    const Recorded &r0 = aRows[a];
    sim.Place(r0.position, 0.0f);
    sim.SetState({r0.position, r0.velocity, r0.forward, r0.right, r0.angularVelocity, r0.hovering, r0.hoverHeight,
                  r0.referenceZ, r0.distance, r0.holding});

    Piece p;
    p.start = r0.t;
    int axis = Starts(aRows, a);
    p.what = axis < 0 ? "none" : std::string(kAxisNames[axis]) + (Axis(r0.controls, axis) > 0 ? "+" : "-");
    float bestClimbGame = 0, bestClimbSim = 0;
    for (size_t i = a + 1; i < b; ++i) {
      const Recorded &r = aRows[i];
      // the ground the game found, under the simulator's car: the game's distance moved by how far the simulator's
      // height is off the game's, and found while the ray could reach (FindGround's length)
      Flight::Ground g;
      g.distance = r.distance + (sim.Last().position.z - aRows[i - 1].position.z);
      g.found = r.distance <= aSetup.standard.maxHoverHeight + 10.0f;
      g.normal = {0, 0, 1};
      Frame f = sim.Advance(r.controls, &g, r.dt);

      float gameYaw = std::fabs(r.angularVelocity.z) * kDeg, simYaw = std::fabs(f.angularVelocity.z) * kDeg;
      p.gameYawRate = std::max(p.gameYawRate, gameYaw);
      p.simYawRate = std::max(p.simYawRate, simYaw);
      if (std::fabs(r.velocity.z) > std::fabs(bestClimbGame))
        bestClimbGame = r.velocity.z;
      if (std::fabs(f.velocity.z) > std::fabs(bestClimbSim))
        bestClimbSim = f.velocity.z;
      float gameTilt = Tilt(r.forward, r.right);
      float simTilt = std::max(std::fabs(f.pitchDeg), std::fabs(f.rollDeg));
      p.gameTilt = std::max(p.gameTilt, gameTilt);
      p.simTilt = std::max(p.simTilt, simTilt);
      p.heightError = std::max(p.heightError, std::fabs(f.position.z - r.position.z));
      p.positionError = std::max(p.positionError, Flight::Length(Flight::Sub(f.position, r.position)));
      p.gameSpeed = std::hypot(r.velocity.x, r.velocity.y);
      p.simSpeed = std::hypot(f.velocity.x, f.velocity.y);
      if (trace)
        std::fprintf(trace, "%.4f,%zu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.2f,%.2f\n", r.t, pieces.size(),
                     r.position.z, f.position.z, r.velocity.z, f.velocity.z, std::hypot(r.velocity.x, r.velocity.y),
                     std::hypot(f.velocity.x, f.velocity.y), r.angularVelocity.z, f.angularVelocity.z, gameTilt,
                     simTilt);
    }
    p.gameClimb = bestClimbGame;
    p.simClimb = bestClimbSim;
    p.length = aRows[b - 1].t - r0.t;
    pieces.push_back(p);
  }
  if (trace)
    std::fclose(trace);
  return pieces;
}

} // namespace FlightSim
