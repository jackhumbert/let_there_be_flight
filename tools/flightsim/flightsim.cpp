// Hover & Fly and Drone flown offline (Sim.hpp): runs the scenarios (Scenarios.hpp) and prints each one's numbers.
//
//   flightsim [--mode standard|hover|fly|drone|agdrone|droneassist] [--dt 0.0333] [--csv <dir>] [--set name=value]...
//             [scenario]...
//   flightsim --vehicles <vehicles.tsv> [--match <name part>] [--mode ...] [--set ...] [scenario]...
//
//   flightsim --replay <ltbf_hoverfly_*.csv> [--mode <Hover & Fly's mode index>] [--vehicles <tsv> --match <car>]
//
//   flightsim --terrain <terrain_runs.tsv> [--only <line name or tag part>] [--speeds 40,80,130] [--hover 0]
//             [--regions <dir>] [--vehicles <tsv> [--match <car>]] [--mode ...] [--set ...] [--csv <dir>]
//   flightsim --sample <region> <count> [--length 200] [--min-length 100] [--seed 1] [--regions <dir>]
//   flightsim --seams <region> [--csv <dir>]
//   flightsim --ground-check <ltbf_terrain_*.csv> --region <name> [--vehicles <tsv> --match <car or record>]
//   flightsim --replay <ltbf_terrain_*.csv> --region <name> [--window 3] [--vehicles <tsv> --match <car or record>]
//   flightsim --physx-joint [--pcm] [--open-bottom <m>] [--lift <m>]
//   flightsim --terrain ... --physx [--pcm]
//
// --mode picks the mode flown (Sim.hpp's Mode), Standard by default; without scenario names it flies that kind of
// mode's scenarios (the drone_* ones for the drone modes). Standard's gains (Flight::StandardGains) take --set as a.*,
// as DroneAssist's d.* do; --assist is --mode standard, kept for older command lines. A number after
// --mode is the recording's mode index for --replay.
// --shape-drag flies Flight::ShapeDragAccel from each car's drag areas (vehicles.tsv's cdaX, cdaY, cdaZ) in place of
// today's airResistanceFactor drag; --set dragK=<k> scales it (4.1 by default). --set thrust=<0..1> scales each car's
// surge and sway by its thrust (Flight::ThrustScale; thrustN=<newtons per thruster>, 6000 by default).
// --vehicles flies every car of a survey table (LoadCars in Sim.hpp) and prints a tab-separated table, a car a row.
// --replay flies a game recording's pieces offline and prints the game's numbers beside the simulator's (Replay.hpp);
// with --csv, both traces go to <dir>/replay.csv.
// --terrain flies each line of a runs file over its region's collision (TerrainRuns.hpp; the redesign, --mode assist,
// unless --mode says otherwise) at each speed (km/h) and hover height (m; 0, the default: each car's from flight on
// at rest), one car or every car of --vehicles, and prints a tab-separated table, a run a row: car, line, km/h, the
// line's usable length and tags, the metrics.
// --sample prints <count> random lines in a region, tagged by their ground, as rows for a runs file. Regions are
// <regions>/<name>.ltbfregion (region_pack.py --regions), by default %LTBF_WORK%/flightsim_regions
// (build/tools_work/flightsim_regions without it, tools/common/paths.py's default work folder).
// --ground-check casts the rays of a game recording of the terrain runs (a car driven along each line, its FindGround rays logged a frame) again
// through the region and prints where they agree and where not (TerrainCheck.hpp); --replay with --region flies
// the recording in windows of --window s over the region's collision (the redesign unless --mode says otherwise).
// --match takes a car's name or its vehicles.tsv record.
// --seams lists where a region's drivable ground meets without a shared edge (Seams.hpp): how many, how big a step
// or gap, and with --csv every one in <dir>/seams_<region>.tsv.
// --physx-joint slides a Shion-sized body over flush mesh joints in the game's own PhysX (PhysXCheck.hpp; built when
// the PhysX 3.4 headers are there, LTBF_PHYSX34): --pcm with PCM contacts, --open-bottom with the thrusters as boxes
// open that far up, --lift held that far over resting. --physx with --terrain flies each line on the game's PhysX:
// the line's ground within 30 m as one PhysX mesh per mesh id of the region (region_pack.py, version 2), the car its
// thruster spheres (or boxes, --set openBottom; none with --set sphereContacts=0) and its chassis box.
//
// --set changes the tuning: mass, airResistance, brakingFriction, gainX/Y/Z, torqueDelay, gravity (the body's
// gravity against what the modes hold up); Standard's surge, yaw,
// sway, pitch, lift, hover, pitchAngle, roll, rollAngle, rollWithYaw, hoverClamp, minHover, maxHover; Hover & Fly's
// altitudeHold, heightDampening, heightCorrection; the physics' dampAngular, dampAngularMax, yawDamp,
// yawDirectionality, pitchDirectionality. --csv writes each run's frames to <dir>/<scenario>.csv.

#ifdef LTBF_PHYSX
#include "PhysXCheck.hpp"
#include "PhysXWorld.hpp"

#include <unordered_map>
#endif
#include "Replay.hpp"
#include "Scenarios.hpp"
#include "Seams.hpp"
#include "TerrainCheck.hpp"
#include "TerrainRuns.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <sstream>
#include <string>

using namespace FlightSim;

namespace {
std::map<std::string, float *> Knobs(Setup &s) {
  return {
      {"mass", &s.car.mass},
      {"airResistance", &s.car.airResistance},
      {"brakingFriction", &s.car.brakingFriction},
      {"gainX", &s.car.angularGain.x},
      {"gainY", &s.car.angularGain.y},
      {"gainZ", &s.car.angularGain.z},
      {"torqueDelay", &s.car.torqueDelay},
      {"maxSpin", &s.car.maxAngularSpeed},
      {"dragK", &s.car.dragK},
      {"thrust", &s.thrustStrength},
      {"thrustN", &s.thrustPerThruster},
      {"errX", &s.car.responseError.x},
      {"errY", &s.car.responseError.y},
      {"errZ", &s.car.responseError.z},
      {"gravity", &s.car.gravityScale},
      {"velocityLag", &s.car.velocityLag},
      {"a.attitudeP", &s.standardGains.attitudeP},
      {"a.attitudeD", &s.standardGains.attitudeD},
      {"a.maxYawRate", &s.standardGains.maxYawRate},
      {"a.yawP", &s.standardGains.yawP},
      {"a.brake", &s.standardGains.brake},
      {"a.sideBrake", &s.standardGains.sideBrake},
      {"a.maxBrake", &s.standardGains.maxBrake},
      {"a.maxClimbRate", &s.standardGains.maxClimbRate},
      {"a.climbP", &s.standardGains.climbP},
      {"a.holdP", &s.standardGains.holdP},
      {"a.holdD", &s.standardGains.holdD},
      {"a.hoverP", &s.standardGains.hoverP},
      {"a.hoverD", &s.standardGains.hoverD},
      {"d.maxPitchRate", &s.droneAssist.maxRate.x},
      {"d.maxRollRate", &s.droneAssist.maxRate.y},
      {"d.maxYawRate", &s.droneAssist.maxRate.z},
      {"d.rateP", &s.droneAssist.rateP},
      {"drone.lift", &s.drone.liftFactor},
      {"drone.pitch", &s.drone.pitchFactor},
      {"drone.roll", &s.drone.rollFactor},
      {"drone.surge", &s.drone.surgeFactor},
      {"drone.yaw", &s.drone.yawFactor},
      {"drone.sway", &s.drone.swayFactor},
      {"handling", &s.handling},
      {"skid", &s.skid},
      {"skidPerThruster", &s.skidPerThruster},
      {"skidTurn", &s.skidTurn},
      {"sphereRadius", &s.sphereRadius},
      {"openBottom", &s.openBottom},
      {"pointContact", &s.pointContact},
      {"sphereContacts", &s.sphereContacts},
      {"surge", &s.standard.surgeFactor},
      {"yaw", &s.standard.yawFactor},
      {"sway", &s.standard.swayFactor},
      {"pitch", &s.standard.pitchFactor},
      {"lift", &s.standard.liftFactor},
      {"hover", &s.standard.hoverFactor},
      {"pitchAngle", &s.standard.pitchInputAngle},
      {"roll", &s.standard.rollFactor},
      {"rollAngle", &s.standard.rollInputAngle},
      {"rollWithYaw", &s.standard.rollWithYaw},
      {"hoverClamp", &s.standard.hoverClamp},
      {"minHover", &s.standard.minHoverHeight},
      {"maxHover", &s.standard.maxHoverHeight},
      {"dampAngular", &s.force.generalDampFactorAngular},
      {"dampAngularMax", &s.force.generalDampFactorAngularMax},
      {"yawDamp", &s.force.generalYawDampFactor},
      {"yawDirectionality", &s.force.generalYawDirectionalityFactor},
      {"pitchDirectionality", &s.force.generalPitchDirectionalityFactor},
  };
}

void WriteCsv(const std::string &aPath, const Run &aRun) {
  FILE *f = std::fopen(aPath.c_str(), "w");
  if (!f)
    return;
  std::fprintf(f, "t,x,y,z,vx,vy,vz,heading,pitch,roll,wx,wy,wz,surge,lift,sway,yaw,ground,distance,found,hovering,"
                  "hoverHeight,holding,referenceZ,contact\n");
  for (const Frame &x : aRun.frames)
    std::fprintf(f, "%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.2f,%.3f,%d,"
                    "%.3f,%.3f,%d,%.3f,%d\n",
                 x.t, x.position.x, x.position.y, x.position.z, x.velocity.x, x.velocity.y, x.velocity.z, x.headingDeg,
                 x.pitchDeg, x.rollDeg, x.angularVelocity.x, x.angularVelocity.y, x.angularVelocity.z,
                 x.controls.surge, x.controls.lift, x.controls.sway, x.controls.yaw, x.groundHeight, x.groundDistance,
                 x.foundGround ? 1 : 0, x.hovering, x.hoverHeight, x.holdingAltitude ? 1 : 0, x.referenceZ,
                 x.contact ? 1 : 0);
  std::fclose(f);
}
} // namespace

// A car of the table with the --set values that are not the table's: the response error (errX..), the drag factor,
// the gravity scale, the velocity lag and the torque delay apply to every car flown.
Car FromTable(Car aTable, const Car &aSet) {
  aTable.responseError = aSet.responseError;
  aTable.dragK = aSet.dragK;
  aTable.gravityScale = aSet.gravityScale;
  aTable.velocityLag = aSet.velocityLag;
  aTable.torqueDelay = aSet.torqueDelay;
  return aTable;
}

std::string RegionsDir() {
  const char *work = std::getenv("LTBF_WORK");
  return std::string(work ? work : "build/tools_work") + "/flightsim_regions";
}

// the region files, each loaded once
const Colliders *Region(const std::string &aDir, const std::string &aName) {
  static std::map<std::string, std::unique_ptr<Colliders>> loaded;
  auto &slot = loaded[aName];
  if (!slot) {
    auto c = std::make_unique<Colliders>();
    std::string error;
    if (!c->Load(aDir + "/" + aName + ".ltbfregion", &error)) {
      std::fprintf(stderr, "%s\n", error.c_str());
      return nullptr;
    }
    slot = std::move(c);
  }
  return slot.get();
}

#ifdef LTBF_PHYSX
// The line's ground within 30 m in PhysX, a mesh per mesh id, and the Sim's car body handed to it.
std::function<void(Sim &)> PhysXBody(const Colliders &aWorld, const Line &aLine, const PhysXOptions &aOptions) {
  return [&aWorld, aLine, aOptions](Sim &aSim) {
    std::string error;
    PhysXOptions options = aOptions;
    const Car &car = aSim.Vehicle();
    options.gravity = 9.81f * car.gravityScale;
    std::shared_ptr<PhysXWorld> px(PhysXWorld::Create(LTBF_GAME_BIN, options, error).release());
    if (!px) {
      std::fprintf(stderr, "%s\n", error.c_str());
      return;
    }
    const Vec3 a = aLine.a, d = Flight::Sub(aLine.b, aLine.a);
    const float len2 = std::max(d.x * d.x + d.y * d.y, 1e-6f);
    std::unordered_map<uint32_t, std::vector<Vec3>> meshes;
    for (size_t i = 0; i < aWorld.Size(); ++i) {
      Vec3 p0, p1, p2;
      Colliders::Kind kind;
      aWorld.Triangle(i, p0, p1, p2, kind);
      const Vec3 c{(p0.x + p1.x + p2.x) / 3.0f, (p0.y + p1.y + p2.y) / 3.0f, 0.0f};
      const float t = std::clamp(((c.x - a.x) * d.x + (c.y - a.y) * d.y) / len2, 0.0f, 1.0f);
      if (std::hypot(c.x - (a.x + d.x * t), c.y - (a.y + d.y * t)) > 30.0f)
        continue;
      auto &m = meshes[aWorld.Mesh(i)];
      m.insert(m.end(), {p0, p1, p2});
    }
    for (auto &[id, corners] : meshes)
      px->AddMesh(corners);
    // the thruster pods (spheres of the table's bottom, or the boxes over the pods' sides and top), the chassis box
    std::vector<PhysXWorld::Shape> shapes;
    if (aSim.sphereContacts > 0.5f)
      for (float sx : {-1.0f, 1.0f})
        for (float sy : {-1.0f, 1.0f}) {
          PhysXWorld::Shape s;
          const float centre = car.bottom + 0.4f;
          if (aSim.openBottom > 0.0f) {
            s.box = true;
            s.halfExtents = {0.33f, 0.36f, (0.49f - aSim.openBottom) * 0.5f};
            s.center = {sx * car.wheelX, sy * car.wheelY, centre + aSim.openBottom * 0.5f};
          } else {
            s.radius = aSim.sphereRadius;
            s.center = {sx * car.wheelX, sy * car.wheelY, centre};
          }
          shapes.push_back(s);
        }
    if (car.boxMax.x > car.boxMin.x) {
      PhysXWorld::Shape box;
      box.box = true;
      box.center = Flight::Mul(Flight::Add(car.boxMin, car.boxMax), 0.5f);
      box.halfExtents = Flight::Mul(Flight::Sub(car.boxMax, car.boxMin), 0.5f);
      shapes.push_back(box);
    }
    px->SetBody(car.mass, aSim.Statics().momentOfInertia, shapes, aSim.Position(), aSim.Orientation(), aSim.Velocity(),
                aSim.AngularVelocity());
    std::fprintf(stderr, "%s: %zu PhysX meshes, %zu shapes on the car\n", aLine.name.c_str(), px->Meshes(),
                 shapes.size());
    aSim.bodyStep = [px](const BodyStepIn &in, BodyStepOut &out) {
      px->Step(in.dt, in.force, in.torque, in.linearDamping, in.maxAngularSpeed);
      out.position = px->Position();
      out.orientation = px->Orientation();
      out.velocity = px->Velocity();
      out.angularVelocity = px->AngularVelocity();
      out.contactPoints = px->ContactPoints();
    };
  };
}
#endif

int FlyTerrain(const std::string &aRuns, const std::string &aRegions, const std::string &aOnly,
            const std::vector<float> &aSpeeds, float aHover, const Setup &aSetup, const std::vector<Car> &aCars,
            const std::string &aCsv, bool aPhysX, bool aPcm) {
  std::vector<Line> lines = LoadLines(aRuns);
  if (lines.empty()) {
    std::fprintf(stderr, "no lines in %s\n", aRuns.c_str());
    return 2;
  }
  bool header = false;
  for (const Line &line : lines) {
    if (!aOnly.empty() && line.name.find(aOnly) == std::string::npos && line.tags.find(aOnly) == std::string::npos)
      continue;
    const Colliders *world = Region(aRegions, line.region);
    if (!world)
      return 2;
    Profile profile = GroundProfile(*world, line.a, line.b, line.z0);
    if (profile.usable < 10.0f) {
      std::fprintf(stderr, "%s: under 10 m to fly before a wall, left out\n", line.name.c_str());
      continue;
    }
    for (const Car &car : aCars) {
      Setup s = aSetup;
      if (!car.name.empty() || aCars.size() > 1)
        s.car = FromTable(car, aSetup.car);
      for (float kmh : aSpeeds) {
        std::function<void(Sim &)> attach;
#ifdef LTBF_PHYSX
        PhysXOptions options;
        options.pcm = aPcm;
        if (aPhysX)
          attach = PhysXBody(*world, line, options);
#else
        (void)aPcm;
        if (aPhysX) {
          std::fprintf(stderr, "built without the PhysX 3.4 headers (LTBF_PHYSX34)\n");
          return 2;
        }
#endif
        Run run = FlyLine(s, *world, line, profile, kmh / 3.6f, aHover, attach);
        if (!header) {
          // the line's ground (Profile), then the run's metrics
          std::printf("car\tline\tkmh\tusable\ttags\tslope\tstep_max\tkerbs\tsteps\tdrops\tbump\tcrest\tdip\tterrain"
                      "\tgaps");
          for (const Metric &m : run.metrics)
            std::printf("\t%s", m.name.c_str());
          std::printf("\n");
          header = true;
        }
        std::printf("%s\t%s\t%.0f\t%.0f\t%s\t%.3g\t%.3g\t%d\t%d\t%d\t%.3g\t%.3g\t%.3g\t%.2f\t%.2f",
                    s.car.name.empty() ? "car" : s.car.name.c_str(), line.name.c_str(), kmh, profile.usable,
                    line.tags.c_str(), profile.slopeP95, profile.stepMax, profile.kerbs, profile.steps, profile.drops,
                    profile.bumpRms, profile.crest, profile.dip, profile.terrain, profile.gaps);
        for (const Metric &m : run.metrics)
          std::printf("\t%.3g", m.value);
        std::printf("\n");
        std::fflush(stdout);
        if (!aCsv.empty())
          WriteCsv(aCsv + "/" + run.name + ".csv", run);
      }
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  Setup setup;
  auto knobs = Knobs(setup);
  std::string csv, vehicles, match, replay, terrain, only, sample, groundCheck, region, seams, regions = RegionsDir();
  float window = 3.0f;
  std::vector<float> speeds{40.0f, 80.0f, 130.0f};
  float hover = 0.0f, length = 200.0f, minLength = 100.0f;
  int sampleCount = 0;
  uint32_t seed = 1;
  int hoverFlyMode = 0;
  std::vector<std::string> names;
  bool physxJoint = false, physx = false, pcm = false;
#ifdef LTBF_PHYSX
  PhysXJointSetup joint;
#endif
  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "--physx-joint")) {
      physxJoint = true;
    } else if (!std::strcmp(argv[i], "--physx")) {
      physx = true;
#ifdef LTBF_PHYSX
    } else if (!std::strcmp(argv[i], "--pcm")) {
      joint.pcm = pcm = true;
    } else if (!std::strcmp(argv[i], "--open-bottom") && i + 1 < argc) {
      joint.openBottom = std::strtof(argv[++i], nullptr);
    } else if (!std::strcmp(argv[i], "--lift") && i + 1 < argc) {
      joint.lift = std::strtof(argv[++i], nullptr);
#endif
    } else if (!std::strcmp(argv[i], "--terrain") && i + 1 < argc) {
      terrain = argv[++i];
    } else if (!std::strcmp(argv[i], "--sample") && i + 2 < argc) {
      sample = argv[++i];
      sampleCount = std::atoi(argv[++i]);
    } else if (!std::strcmp(argv[i], "--seams") && i + 1 < argc) {
      seams = argv[++i];
    } else if (!std::strcmp(argv[i], "--ground-check") && i + 1 < argc) {
      groundCheck = argv[++i];
    } else if (!std::strcmp(argv[i], "--region") && i + 1 < argc) {
      region = argv[++i];
    } else if (!std::strcmp(argv[i], "--window") && i + 1 < argc) {
      window = std::strtof(argv[++i], nullptr);
    } else if (!std::strcmp(argv[i], "--regions") && i + 1 < argc) {
      regions = argv[++i];
    } else if (!std::strcmp(argv[i], "--only") && i + 1 < argc) {
      only = argv[++i];
    } else if (!std::strcmp(argv[i], "--speeds") && i + 1 < argc) {
      speeds.clear();
      std::stringstream ss(argv[++i]);
      std::string v;
      while (std::getline(ss, v, ','))
        speeds.push_back(std::strtof(v.c_str(), nullptr));
    } else if (!std::strcmp(argv[i], "--hover") && i + 1 < argc) {
      hover = std::strtof(argv[++i], nullptr);
    } else if (!std::strcmp(argv[i], "--length") && i + 1 < argc) {
      length = std::strtof(argv[++i], nullptr);
    } else if (!std::strcmp(argv[i], "--min-length") && i + 1 < argc) {
      minLength = std::strtof(argv[++i], nullptr);
    } else if (!std::strcmp(argv[i], "--seed") && i + 1 < argc) {
      seed = uint32_t(std::strtoul(argv[++i], nullptr, 10));
    } else if (!std::strcmp(argv[i], "--assist")) {
      setup.mode = Mode::Standard;
    } else if (!std::strcmp(argv[i], "--shape-drag")) {
      setup.shapeDrag = true;
    } else if (!std::strcmp(argv[i], "--replay") && i + 1 < argc) {
      replay = argv[++i];
    } else if (!std::strcmp(argv[i], "--mode") && i + 1 < argc) {
      const char *v = argv[++i];
      if (*v >= '0' && *v <= '9') {
        hoverFlyMode = std::atoi(v);
      } else if (!ParseMode(v, setup.mode)) {
        std::fprintf(stderr, "unknown mode: %s\n", v);
        return 2;
      }
    } else if (!std::strcmp(argv[i], "--vehicles") && i + 1 < argc) {
      vehicles = argv[++i];
    } else if (!std::strcmp(argv[i], "--match") && i + 1 < argc) {
      match = argv[++i];
    } else if (!std::strcmp(argv[i], "--dt") && i + 1 < argc) {
      setup.dt = std::strtof(argv[++i], nullptr);
    } else if (!std::strcmp(argv[i], "--csv") && i + 1 < argc) {
      csv = argv[++i];
    } else if (!std::strcmp(argv[i], "--set") && i + 1 < argc) {
      std::string kv = argv[++i];
      size_t eq = kv.find('=');
      auto it = eq == std::string::npos ? knobs.end() : knobs.find(kv.substr(0, eq));
      if (it == knobs.end()) {
        std::fprintf(stderr, "unknown setting: %s\n", kv.c_str());
        return 2;
      }
      *it->second = std::strtof(kv.c_str() + eq + 1, nullptr);
    } else {
      names.push_back(argv[i]);
    }
  }
  if (physxJoint) {
#ifdef LTBF_PHYSX
    return PhysXJointCheck(LTBF_GAME_BIN, joint);
#else
    std::fprintf(stderr, "built without the PhysX 3.4 headers (LTBF_PHYSX34)
");
    return 2;
#endif
  }
  if (!seams.empty()) {
    const Colliders *world = Region(regions, seams);
    if (!world)
      return 2;
    std::vector<Seam> found = FindSeams(*world);
    // by the size of the step a car meets crossing it, either way
    const float bins[] = {0.002f, 0.005f, 0.01f, 0.02f, 0.05f, 0.1f, 0.25f};
    int counts[8] = {}, gaps = 0;
    double metres[8] = {};
    for (const Seam &s : found) {
      int b = 0;
      while (b < 7 && std::fabs(s.step) >= bins[b])
        ++b;
      ++counts[b];
      metres[b] += s.length;
      gaps += s.gap > 0.0f;
    }
    std::printf("%s: %zu seams on drivable ground (%d with a gap)\n", seams.c_str(), found.size(), gaps);
    const char *labels[] = {"under 2 mm", "2 to 5 mm", "5 to 10 mm", "1 to 2 cm", "2 to 5 cm", "5 to 10 cm",
                            "10 to 25 cm", "25 cm on"};
    for (int b = 0; b < 8; ++b)
      std::printf("  step %-12s %7d seams, %8.0f m of edge\n", labels[b], counts[b], metres[b]);
    if (!csv.empty()) {
      std::string path = csv + "/seams_" + seams + ".tsv";
      if (FILE *out = std::fopen(path.c_str(), "w")) {
        std::fprintf(out, "x\ty\tz\toutx\touty\tlength\tstep\tgap\tinside\toutside\n");
        for (const Seam &s : found)
          std::fprintf(out, "%.2f\t%.2f\t%.3f\t%.3f\t%.3f\t%.3f\t%.4f\t%.3f\t%d\t%d\n", s.at.x, s.at.y, s.at.z, s.out.x,
                       s.out.y, s.length, s.step, s.gap, int(s.inside), int(s.outside));
        std::fclose(out);
        std::printf("written %s\n", path.c_str());
      }
    }
    return 0;
  }
  if (!sample.empty()) {
    const Colliders *world = Region(regions, sample);
    if (!world)
      return 2;
    for (const Line &l : SampleLines(*world, sample, sampleCount, length, minLength, seed))
      std::printf("%s\n", FormatLine(l).c_str());
    return 0;
  }
  if (!terrain.empty()) {
    std::vector<Car> cars{setup.car};
    if (!vehicles.empty()) {
      cars.clear();
      for (const Car &car : LoadCars(vehicles))
        if (match.empty() || car.name.find(match) != std::string::npos)
          cars.push_back(car);
      if (cars.empty()) {
        std::fprintf(stderr, "no cars in %s\n", vehicles.c_str());
        return 2;
      }
    }
    return FlyTerrain(terrain, regions, only, speeds, hover, setup, cars, csv, physx, pcm);
  }
  if (names.empty())
    names = ScenarioNames(setup.mode);

  if (!vehicles.empty() && (!replay.empty() || !groundCheck.empty())) {
    // the car the recording was flown in, by name or by record
    for (const Car &car : LoadCars(vehicles))
      if (!match.empty() && (car.name.find(match) != std::string::npos || car.record == match)) {
        setup.car = FromTable(car, setup.car);
      }
  }
  if (!groundCheck.empty()) {
    const Colliders *world = Region(regions, region);
    TerrainRecording rec = LoadTerrainRecording(groundCheck);
    if (!world || rec.rays.empty()) {
      std::fprintf(stderr, "no region %s or no rays in %s\n", region.c_str(), groundCheck.c_str());
      return 2;
    }
    GroundReport g = GroundCheck(rec, *world, setup.car);
    std::printf("%d rays: both hit %d, the game only %d, the region only %d, neither %d\n", g.rays, g.both, g.gameOnly,
                g.regionOnly, g.neither);
    std::printf("where both hit: |dz| p50 %.3f m, p95 %.3f m, max %.2f m; normals apart p95 %.1f deg\n", g.dzP50, g.dzP95,
                g.dzMax, g.normalP95);
    std::printf("the game's car against the region: lowest clearance %.2f m, %d touches, %d frames in touch\n",
                g.gameClearanceMin, g.gameTouches, g.gameTouchFrames);
    std::printf("wheel emitters on the recorded body: |x| %.3f |y| %.3f z %.3f (%s in the table: %.3f %.3f %.3f)\n",
                g.wheel.x, g.wheel.y, g.wheel.z, setup.car.name.c_str(), setup.car.wheelX, setup.car.wheelY,
                setup.car.wheelZ);
    std::printf("where they disagree (2 m cells; z NaN: no hit):\n%9s %9s %8s %8s %5s\n", "x", "y", "game z", "region z",
                "rays");
    for (const auto &s : g.spots)
      std::printf("%9.1f %9.1f %8.2f %8.2f %5d\n", s.x, s.y, s.gameZ, s.regionZ, s.rays);
    return 0;
  }
  if (!replay.empty() && !region.empty()) {
    const Colliders *world = Region(regions, region);
    TerrainRecording rec = LoadTerrainRecording(replay);
    if (!world || rec.rows.empty()) {
      std::fprintf(stderr, "no region %s or no rows in %s\n", region.c_str(), replay.c_str());
      return 2;
    }
    std::printf("car %s, %zu rows, %.0f s windows over %s\n%7s %5s | %7s %7s | %7s | %15s | %s\n", setup.car.name.c_str(),
                rec.rows.size(), window, region.c_str(), "start", "len", "dz rms", "dz max", "dist rms", "vz m/s g/s",
                "sim touches");
    for (const TerrainPiece &p :
         TerrainReplay(rec, *world, setup, hoverFlyMode, window, csv.empty() ? "" : csv + "/terrain_replay.csv"))
      std::printf("%7.2f %5.2f | %7.2f %7.2f | %8.2f | %7.2f %7.2f | %d\n", p.start, p.length, p.heightRms, p.heightMax,
                  p.distanceRms, p.gameClimb, p.simClimb, p.simContacts);
    return 0;
  }
  if (!replay.empty()) {
    std::vector<Recorded> rows = LoadRecording(replay);
    if (rows.empty()) {
      std::fprintf(stderr, "no rows in %s\n", replay.c_str());
      return 2;
    }
    std::printf("car %s, %zu rows\n", setup.car.name.c_str(), rows.size());
    std::printf("%7s %5s %-8s | %16s | %16s | %16s | %16s | %6s %6s\n", "start", "len", "control", "yaw deg/s g/s",
                "vz m/s g/s", "speed m/s g/s", "tilt deg g/s", "dz m", "dpos m");
    for (const Piece &p : Replay(rows, setup, hoverFlyMode, csv.empty() ? "" : csv + "/replay.csv"))
      std::printf("%7.2f %5.2f %-8s | %7.1f %8.1f | %7.2f %8.2f | %7.2f %8.2f | %7.1f %8.1f | %6.2f %6.2f\n", p.start,
                  p.length, p.what.c_str(), p.gameYawRate, p.simYawRate, p.gameClimb, p.simClimb, p.gameSpeed,
                  p.simSpeed, p.gameTilt, p.simTilt, p.heightError, p.positionError);
    return 0;
  }

  if (!vehicles.empty()) {
    // every car of the table through every scenario: one row a car, one column a metric (scenario.metric)
    std::vector<Car> cars = LoadCars(vehicles);
    if (cars.empty()) {
      std::fprintf(stderr, "no cars in %s\n", vehicles.c_str());
      return 2;
    }
    bool header = false;
    for (const Car &car : cars) {
      if (!match.empty() && car.name.find(match) == std::string::npos)
        continue;
      Setup s = setup;
      s.car = FromTable(car, setup.car);
      std::vector<Run> runs;
      for (const std::string &name : names)
        runs.push_back(RunScenario(name, s));
      if (!header) {
        std::printf("car");
        for (const Run &r : runs)
          for (const Metric &m : r.metrics)
            std::printf("\t%s.%s", r.name.c_str(), m.name.c_str());
        std::printf("\n");
        header = true;
      }
      std::printf("%s", car.name.c_str());
      for (const Run &r : runs)
        for (const Metric &m : r.metrics)
          std::printf("\t%.2f", m.value);
      std::printf("\n");
    }
    return 0;
  }

  for (const std::string &name : names) {
    Run run = RunScenario(name, setup);
    if (run.frames.empty()) {
      std::fprintf(stderr, "unknown scenario: %s\n", name.c_str());
      return 2;
    }
    std::printf("%s\n", name.c_str());
    for (const Metric &m : run.metrics)
      std::printf("  %-28s %9.2f %s\n", m.name.c_str(), m.value, m.unit.c_str());
    if (!csv.empty())
      WriteCsv(csv + "/" + name + ".csv", run);
  }
  return 0;
}
