// Hover & Fly and Drone flown offline (Sim.hpp): runs the scenarios (Scenarios.hpp) and prints each one's numbers.
//
//   flightsim [--mode hoverfly|assist|drone|agdrone|droneassist] [--dt 0.0333] [--csv <dir>] [--set name=value]...
//             [scenario]...
//   flightsim --vehicles <vehicles.tsv> [--match <name part>] [--mode ...] [--set ...] [scenario]...
//
//   flightsim --replay <ltbf_hoverfly_*.csv> [--mode <Hover & Fly's mode index>] [--vehicles <tsv> --match <car>]
//
//   flightsim --terrain <terrain_runs.tsv> [--only <line name or tag part>] [--speeds 40,80,130] [--hover 1]
//             [--regions <dir>] [--vehicles <tsv> [--match <car>]] [--mode ...] [--set ...] [--csv <dir>]
//   flightsim --sample <region> <count> [--length 200] [--min-length 100] [--seed 1] [--regions <dir>]
//
// --mode picks the mode flown (Sim.hpp's Mode), today's Hover & Fly by default; without scenario names it flies that
// kind of mode's scenarios (the drone_* ones for the drone modes). --assist is --mode assist: the redesigned
// controller (Flight::HoverFlyAssist), whose a.* settings take --set too, as DroneAssist's d.* do. A number after
// --mode is the recording's mode index for --replay.
// --shape-drag flies Flight::ShapeDragAccel from each car's drag areas (vehicles.tsv's cdaX, cdaY, cdaZ) in place of
// today's airResistanceFactor drag; --set dragK=<k> scales it (4.1 by default). --set thrust=<0..1> scales each car's
// surge and sway by its thrust (Flight::ThrustScale; thrustN=<newtons per thruster>, 6000 by default).
// --vehicles flies every car of a survey table (LoadCars in Sim.hpp) and prints a tab-separated table, a car a row.
// --replay flies a game recording's pieces offline and prints the game's numbers beside the simulator's (Replay.hpp);
// with --csv, both traces go to <dir>/replay.csv.
// --terrain flies each line of a runs file over its region's collision (TerrainRuns.hpp; the redesign, --mode assist,
// unless --mode says otherwise) at each speed (km/h) and hover height (m), one car or every car of --vehicles, and
// prints a tab-separated table, a run a row: car, line, km/h, the line's usable length and tags, the metrics.
// --sample prints <count> random lines in a region, tagged by their ground, as rows for a runs file. Regions are
// <regions>/<name>.ltbfregion (region_pack.py --regions), by default %LTBF_WORK%/flightsim_regions
// (D:/cp2077_work/build/flightsim_regions without it).
//
// --set changes the tuning: mass, airResistance, brakingFriction, gainX/Y/Z, torqueDelay, gravity (the body's
// gravity against what the modes hold up); Standard's surge, yaw,
// sway, pitch, lift, hover, pitchAngle, roll, rollAngle, rollWithYaw, hoverClamp, minHover, maxHover; Hover & Fly's
// altitudeHold, heightDampening, heightCorrection; the physics' dampAngular, dampAngularMax, yawDamp,
// yawDirectionality, pitchDirectionality. --csv writes each run's frames to <dir>/<scenario>.csv.

#include "Replay.hpp"
#include "Scenarios.hpp"
#include "TerrainRuns.hpp"

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
      {"a.attitudeP", &s.assistParams.attitudeP},
      {"a.attitudeD", &s.assistParams.attitudeD},
      {"a.maxYawRate", &s.assistParams.maxYawRate},
      {"a.yawP", &s.assistParams.yawP},
      {"a.brake", &s.assistParams.brake},
      {"a.sideBrake", &s.assistParams.sideBrake},
      {"a.maxBrake", &s.assistParams.maxBrake},
      {"a.maxClimbRate", &s.assistParams.maxClimbRate},
      {"a.climbP", &s.assistParams.climbP},
      {"a.holdP", &s.assistParams.holdP},
      {"a.holdD", &s.assistParams.holdD},
      {"a.hoverP", &s.assistParams.hoverP},
      {"a.hoverD", &s.assistParams.hoverD},
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
      {"altitudeHold", &s.hoverFly.altitudeHoldFactor},
      {"heightDampening", &s.hoverFly.heightDampening},
      {"heightCorrection", &s.hoverFly.heightCorrectionFactor},
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

std::string RegionsDir() {
  const char *work = std::getenv("LTBF_WORK");
  return std::string(work ? work : "D:/cp2077_work/build") + "/flightsim_regions";
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

int FlyTerrain(const std::string &aRuns, const std::string &aRegions, const std::string &aOnly,
            const std::vector<float> &aSpeeds, float aHover, const Setup &aSetup, const std::vector<Car> &aCars,
            const std::string &aCsv) {
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
      if (!car.name.empty() || aCars.size() > 1) {
        s.car = car;
        s.car.responseError = aSetup.car.responseError;
        s.car.dragK = aSetup.car.dragK;
      }
      for (float kmh : aSpeeds) {
        Run run = FlyLine(s, *world, line, profile, kmh / 3.6f, aHover);
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
  std::string csv, vehicles, match, replay, terrain, only, sample, regions = RegionsDir();
  std::vector<float> speeds{40.0f, 80.0f, 130.0f};
  float hover = 1.0f, length = 200.0f, minLength = 100.0f;
  int sampleCount = 0;
  uint32_t seed = 1;
  bool modeGiven = false;
  int hoverFlyMode = 0;
  std::vector<std::string> names;
  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "--terrain") && i + 1 < argc) {
      terrain = argv[++i];
    } else if (!std::strcmp(argv[i], "--sample") && i + 2 < argc) {
      sample = argv[++i];
      sampleCount = std::atoi(argv[++i]);
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
      setup.mode = Mode::Assist;
    } else if (!std::strcmp(argv[i], "--shape-drag")) {
      setup.shapeDrag = true;
    } else if (!std::strcmp(argv[i], "--replay") && i + 1 < argc) {
      replay = argv[++i];
    } else if (!std::strcmp(argv[i], "--mode") && i + 1 < argc) {
      const char *v = argv[++i];
      modeGiven = true;
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
  if (!sample.empty()) {
    const Colliders *world = Region(regions, sample);
    if (!world)
      return 2;
    for (const Line &l : SampleLines(*world, sample, sampleCount, length, minLength, seed))
      std::printf("%s\n", FormatLine(l).c_str());
    return 0;
  }
  if (!terrain.empty()) {
    if (!modeGiven && setup.mode == Mode::HoverFly)
      setup.mode = Mode::Assist;
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
    return FlyTerrain(terrain, regions, only, speeds, hover, setup, cars, csv);
  }
  if (names.empty())
    names = ScenarioNames(setup.mode);

  if (!vehicles.empty() && !replay.empty()) {
    // the car the recording was flown in
    for (const Car &car : LoadCars(vehicles))
      if (!match.empty() && car.name.find(match) != std::string::npos) {
        const Vec3 error = setup.car.responseError; // --set errX.. apply to the replayed car too
        setup.car = car;
        setup.car.responseError = error;
      }
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
      s.car = car;
      s.car.responseError = setup.car.responseError; // --set errX.. apply to every car
      s.car.dragK = setup.car.dragK;
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
