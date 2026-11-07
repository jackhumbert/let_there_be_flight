// flightsim --physx-joint: a Shion-sized body sliding at 40 km/h on its four thruster spheres over flat ground that is
// one mesh, two meshes, or 10.4 m meshes laid end to end like the city's roads, all flush, in the game's own PhysX
// (PhysXWorld.hpp). Whether PhysX throws it at the joints is the question flightsim's own contact cannot answer.

#include "PhysXCheck.hpp"

#include "PhysXWorld.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace FlightSim {

namespace {
using Flight::Vec3;

// flat ground z = 0 over x in [-6, 6], y in [y0, y1], in rows 2.6 m long of two triangles a row
std::vector<Vec3> Strip(float aY0, float aY1) {
  std::vector<Vec3> c;
  for (float y = aY0; y < aY1 - 1e-3f; y += 2.6f) {
    const float y2 = std::min(y + 2.6f, aY1);
    c.insert(c.end(), {{-6, y, 0}, {6, y, 0}, {6, y2, 0}});
    c.insert(c.end(), {{-6, y, 0}, {6, y2, 0}, {-6, y2, 0}});
  }
  return c;
}
} // namespace

int PhysXJointCheck(const std::string &aDllDir, const PhysXJointSetup &aSetup) {
  struct World {
    const char *name;
    std::vector<std::pair<float, float>> pieces;
  };
  const std::vector<World> worlds{
      {"one mesh", {{-30.0f, 60.0f}}},
      {"two meshes, a joint at y 0", {{-30.0f, 0.0f}, {0.0f, 60.0f}}},
      {"10.4 m meshes end to end", {}},
  };
  std::printf("world\tshapes\tmax_vz\tmax_rise\tkicks\tkicks at y (joints: every 10.4 m from -30, or 0)\n");
  for (World w : worlds) {
    if (w.pieces.empty())
      for (float y = -30.0f; y < 60.0f; y += 10.4f)
        w.pieces.push_back({y, std::min(y + 10.4f, 60.0f)});
    std::string error;
    PhysXOptions options;
    options.pcm = aSetup.pcm;
    auto px = PhysXWorld::Create(aSetup.dllDir.empty() ? aDllDir : aSetup.dllDir, options, error);
    if (!px) {
      std::fprintf(stderr, "%s\n", error.c_str());
      return 2;
    }
    for (auto [a, b] : w.pieces)
      px->AddMesh(Strip(a, b));
    // the Shion: its mass and inertia (vehicles.tsv), a sphere at each wheel emitter with its bottom at the table's
    // bottom (or a box open at the bottom: the pod's sides and top), and its chassis box
    std::vector<PhysXWorld::Shape> shapes;
    const float wheelX = 1.11f, wheelY = 1.69f, bottom = -0.55f;
    for (float sx : {-1.0f, 1.0f})
      for (float sy : {-1.0f, 1.0f}) {
        PhysXWorld::Shape s;
        const float centre = bottom + 0.4f;
        if (aSetup.openBottom > 0.0f) {
          s.box = true;
          s.halfExtents = {0.33f, 0.36f, (0.49f - aSetup.openBottom) * 0.5f};
          s.center = {sx * wheelX, sy * wheelY, centre + aSetup.openBottom * 0.5f};
        } else {
          s.radius = 0.4f;
          s.center = {sx * wheelX, sy * wheelY, centre};
        }
        shapes.push_back(s);
      }
    PhysXWorld::Shape chassis;
    chassis.box = true;
    chassis.halfExtents = {1.23f, 2.53f, 0.64f};
    chassis.center = {0.0f, 0.07f, 0.29f};
    shapes.push_back(chassis);
    const float mass = 1600.0f, speed = 40.0f / 3.6f;
    const float rest = -bottom + aSetup.lift; // the centre of mass over the ground
    px->SetBody(mass, {2551.0f, 817.0f, 2896.0f}, shapes, {0.0f, -28.0f, rest + 0.02f}, {0, 0, 0, 1}, {}, {});
    const float dt = 1.0f / 60.0f;
    float maxVz = 0.0f, maxRise = 0.0f, lastVz = 0.0f;
    std::string at;
    int kicks = 0, since = 1000;
    const int settle = static_cast<int>(1.5f / dt);
    for (int i = 0; i < static_cast<int>(10.0f / dt); ++i) {
      const Vec3 v = px->Velocity(), p = px->Position();
      // 1.5 s settling on the ground, still; then the driver holds the speed; with lift, a force holds the body that
      // far up (the skid's job, stiff)
      Vec3 force{0.0f, i < settle ? 0.0f : mass * 3.0f * (speed - v.y), 0.0f};
      if (aSetup.lift > 0.0f)
        force.z = mass * (9.81f + 60.0f * (rest - p.z) - 12.0f * v.z);
      px->Step(dt, force, {}, 0.0f, 7.0f);
      const Vec3 v2 = px->Velocity(), p2 = px->Position();
      if (p2.y > 50.0f)
        break;
      if (i == settle - 1)
        std::fprintf(stderr, "%s, %s: settled with the centre of mass %.3f m up (placed %.3f), %d contact points\n",
                     w.name, aSetup.openBottom > 0.0f ? "boxes" : "spheres", p2.z, rest + 0.02f, px->ContactPoints());
      if (i < settle + 30) {
        lastVz = v2.z;
        continue;
      }
      maxVz = std::max(maxVz, v2.z);
      maxRise = std::max(maxRise, p2.z - rest);
      ++since;
      if (v2.z - lastVz > 0.5f && since > 20) {
        ++kicks;
        since = 0;
        char y[16];
        std::snprintf(y, sizeof(y), " %.1f", p2.y);
        at += y;
      }
      lastVz = v2.z;
    }
    std::printf("%s\t%s\t%.2f\t%.3f\t%d\t%s\n", w.name, aSetup.openBottom > 0.0f ? "boxes" : "spheres", maxVz,
                maxRise, kicks, at.c_str());
  }
  return 0;
}

} // namespace FlightSim
