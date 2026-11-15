// Whole Autopilot trips flown offline (tools/flightsim/Trip.hpp; docs/design/native-ports.md, M6 step 5): the
// Autopilot's own C++ from the engage to "landed" on flightsim's rigid body with the real force tail, over a nav map
// made in memory, plans handed over 0.5 s after they are asked for, boxes for what the map leaves out. Each test names
// the in-game problems it checks (numbered as in the list of Autopilot problems found in game); every trip is held
// to CheckTrip (tests kind 2).
#include "check.hpp"

#include "nav_fixture.hpp"
#include "../tools/flightsim/Trip.hpp"

#include "Navigation/Landing.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace Navigation;
using FlightSim::Trip;
using FlightSim::TripBox;
using FlightSim::TripResult;
using FlightSim::TripSetup;

namespace {

#ifdef LTBF_SOURCE_DIR
const std::string kFleet = LTBF_SOURCE_DIR "/tools/flightsim/vehicles.tsv";
#else
const std::string kFleet = std::string(__FILE__).substr(0, std::string(__FILE__).find_last_of("/\\")) +
                           "/../tools/flightsim/vehicles.tsv";
#endif

// the benchmark car (tools/navplan/routes.json), as surveyed
const FlightSim::Car &Shion() {
  static const FlightSim::Car car = [] {
    for (const auto &c : FlightSim::LoadCars(kFleet)) {
      if (c.name == "Shion MZ1") {
        return c;
      }
    }
    return FlightSim::Car{};
  }();
  CHECK(car.name == "Shion MZ1");
  return car;
}

// 320 x 320 m, floor at 0 (a car's centre rests 0.6 m up): a 40 m tower with a pad on its roof
// (60..100, 140..180) and a 25 m block (140..160, 40..220) beside the diagonal the trips fly
const NavMap &Town() {
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    spec.width = 160;
    spec.height = 160;
    spec.blocks.push_back({60, 140, 100, 180, 40});
    spec.blocks.push_back({140, 40, 160, 220, 25});
    return map.Load(NavFixture::Make(spec), "trip town");
  }();
  CHECK(loaded);
  return map;
}

const std::vector<Helipad> kNoPads;
const std::vector<Gate> kNoGates;
const std::vector<Helipad> kPad{{"pad", "", 80, 160, 40, 30, 8, 1e9f}};

constexpr float kDeg = 57.29578f;

// a street to street trip across the town, 310 m
TripSetup Street() {
  TripSetup s;
  s.car = Shion();
  s.start = {40, 40, 0.6f};
  s.destination = {260, 260, 0};
  s.planBase.doorways = false; // the fixture has no doorway layer
  return s;
}

float Distance2D(const Vec3 &a, const Vec3 &b) { return std::hypot(a.x - b.x, a.y - b.y); }

// the rules a trip broke, printed; their count
size_t Broken(const char *aName, const TripResult &aResult, const AutopilotParams &aParams,
              const FlightSim::TripLimits &aLimits = {}) {
  const auto broken = FlightSim::CheckTrip(aResult, aParams, aLimits);
  for (const auto &b : broken) {
    std::printf("  %s: %s\n", aName, b.c_str());
  }
  if (const char *dir = std::getenv("LTBF_TRIP_CSV")) {
    FlightSim::WriteTripCsv(std::string(dir) + "/" + aName + ".csv", aResult);
  }
  return broken.size();
}

// the time of the first change to aStatus, -1 for none
float ChangeAt(const TripResult &aResult, const char *aStatus) {
  for (const auto &c : aResult.changes) {
    if (c.to == aStatus) {
      return c.t;
    }
  }
  return -1.0f;
}

// the first frame at or after aT
size_t FrameAt(const TripResult &aResult, float aT) {
  for (size_t i = 0; i < aResult.frames.size(); ++i) {
    if (aResult.frames[i].t >= aT - 1e-4f) {
      return i;
    }
  }
  return aResult.frames.size();
}

// runs of frames (s) with a flag, the longest
template <typename F> float LongestRun(const TripResult &aResult, size_t aFrom, size_t aTo, F &&aFlag) {
  float best = 0.0f, start = -1.0f;
  for (size_t i = aFrom; i < aTo && i < aResult.frames.size(); ++i) {
    const auto &f = aResult.frames[i];
    if (aFlag(f)) {
      start = start < 0.0f ? f.t : start;
      best = std::max(best, f.t - start);
    } else {
      start = -1.0f;
    }
  }
  return best;
}

// hits more than 0.5 s apart
int HitEpisodes(const TripResult &aResult) {
  int n = 0;
  float last = -10.0f;
  for (const auto &f : aResult.frames) {
    if (f.hit) {
      n += f.t - last > 0.5f ? 1 : 0;
      last = f.t;
    }
  }
  return n;
}

} // namespace

TEST(autopilot_trip_ground_spot) {
  // regressions 2.1 (no hang over the spot), 2.2 (into the descent slow enough), 2.5 (touchdown detection),
  // 9.2 (no tilt past the cap)
  TripSetup s = Street();
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  CHECK(Broken("ground", r, s.params) == 0);
  CHECK(r.landed);
  CHECK(r.time < 30.0f); // 22.1 s measured
  CHECK(r.hits == 0 && r.replans == 0);
  CHECK(r.flightOff); // mounted: landed asks for flight off
  // taken over by the descent within landingEntry of the route's end, below landingEntrySpeed
  const float descent = ChangeAt(r, "descending");
  const size_t d = FrameAt(r, descent);
  CHECK(d < r.frames.size());
  CHECK(r.frames[d].speed < s.params.landingEntrySpeed + 0.5f);
  CHECK(Distance2D(r.frames[d].p, r.landingSpot) < s.params.landingEntry + 1.0f);
  // the floor reached, then landed once it stops sinking: no new low for 0.7 s, within 2 s
  size_t touch = d;
  while (touch < r.frames.size() && !r.frames[touch].contact) {
    ++touch;
  }
  CHECK(touch < r.frames.size());
  const float landedAt = ChangeAt(r, "landed");
  std::printf("  ground: landed %.2f s after the floor was reached\n", landedAt - r.frames[touch].t);
  CHECK(landedAt - r.frames[touch].t >= 0.5f); // 0.7 s without a new low, the last low up to 0.1 m above the floor
  CHECK(landedAt - r.frames[touch].t < 2.0f);
  // never hanging still over the spot on the way down
  CHECK(LongestRun(r, d, touch, [](const auto &f) { return f.speed < 0.5f; }) < 0.5f);
  float tilt = 0.0f;
  for (const auto &f : r.frames) {
    tilt = std::max(tilt, f.tiltDeg);
  }
  std::printf("  ground: %.1f s, tilt at most %.1f deg, off p95 %.2f m, touchdown %.2f m/s, %lld rays\n", r.time, tilt,
              r.offP95, r.touchdownSpeed, static_cast<long long>(r.rays));
  CHECK(tilt < s.params.droneMaxTilt + 5.0f);
  CHECK(r.frames.back().tiltDeg < 5.0f); // on its wheels
}

TEST(autopilot_trip_pad_landing) {
  // regression 3.2: the landing turn done at approach speed (droneBrakeTurn, as the summon sets it) puts the nose
  // on the pad's heading; without it the descent's blend turns the target slower than the sink (the bug's mechanism)
  TripSetup s = Street();
  s.destination = {80, 160, 40};
  s.params.droneBrakeTurn = true;
  Trip turned(Town(), kPad, kNoGates, s);
  const TripResult r = turned.Run();
  FlightSim::TripLimits limits;
  limits.landingHeadingDeg = 10.0f;
  CHECK(Broken("pad", r, s.params, limits) == 0);
  CHECK(Distance2D(r.frames.back().p, {80, 160, 40}) < 2.0f);
  CHECK(r.frames.back().p.z > 40.3f && r.frames.back().p.z < 41.0f); // on the deck
  s.params.droneBrakeTurn = false;
  Trip blended(Town(), kPad, kNoGates, s);
  const TripResult b = blended.Run();
  std::printf("  pad: %.1f deg off the pad's heading with droneBrakeTurn, %.1f without\n", r.landingHeadingDeg,
              b.landingHeadingDeg);
  CHECK(b.landed);
  CHECK(b.landingHeadingDeg > 10.0f); // 39 deg measured
}

TEST(autopilot_trip_summon_to_pad) {
  // regressions 3.1 (unmounted, the force tail's tensor set: upright, little attitude error), 3.3 (the external
  // destination written after Activate: no free roam), 3.4 (the drive sim's inputs are not the player's)
  TripSetup s = Street();
  s.destination = {80, 160, 40};
  s.tracked = false;
  s.external = true;
  s.mounted = false;
  s.params.droneBrakeTurn = true;
  s.controls.surge = 0.5f;
  s.controls.yaw = 0.3f;
  Trip trip(Town(), kPad, kNoGates, s);
  const TripResult r = trip.Run();
  FlightSim::TripLimits limits;
  limits.landingHeadingDeg = 10.0f;
  CHECK(Broken("summon", r, s.params, limits) == 0);
  CHECK(ChangeAt(r, "free roam") < 0.0f);
  CHECK(!r.flightOff && !r.exitAutopilot && r.notifications.empty());
  float attitude = 0.0f;
  for (const auto &f : r.frames) {
    if (std::string(FlightSim::TripStatusName(f.status)) == "descending") {
      attitude = std::max(attitude, f.attitudeErrorDeg);
    }
  }
  std::printf("  summon: %.1f s, attitude error at most %.2f deg in the descent\n", r.time, attitude);
  CHECK(attitude < 5.0f);
  CHECK(r.frames.back().tiltDeg < 10.0f);
}

TEST(autopilot_trip_taken_spot) {
  // regression 1.4: a parked car on the nearest spot from the start
  TripSetup s = Street();
  s.boxes.push_back({{{258, 258, 0}, {262, 262, 1.5f}}});
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  CHECK(Broken("taken", r, s.params) == 0);
  CHECK(Distance2D(r.landingSpot, {260, 260, 0}) >= 4.0f);
  CHECK(r.hits == 0);
}

TEST(autopilot_trip_spot_taken_late) {
  // regression 1.5: the parked car streams in 85 m out; the route's end moves before the arrival, one motion into
  // the landing
  TripSetup s = Street();
  TripBox car{{{258, 258, 0}, {262, 262, 1.5f}}};
  car.appearWithin = 85.0f;
  s.boxes.push_back(car);
  Vec3 first{}, atDescent{};
  bool seen = false;
  s.onFrame = [&](Trip &aTrip) {
    if (!seen && aTrip.ap.landing) {
      first = aTrip.ap.landingSpot;
      seen = true;
    }
    if (aTrip.ap.status == "descending" && atDescent.x == 0.0f) {
      atDescent = aTrip.ap.landingSpot;
    }
    return true;
  };
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  CHECK(Broken("taken_late", r, s.params) == 0);
  CHECK(Distance2D(first, {260, 260, 0}) < 2.0f);
  CHECK(Distance2D(atDescent, first) > 4.0f); // moved on the approach, not on arrival
  CHECK(Distance2D(atDescent, r.landingSpot) < 0.01f);
  const size_t d = FrameAt(r, ChangeAt(r, "descending"));
  CHECK(Distance2D(r.frames[d].p, r.landingSpot) < s.params.landingEntry + 1.0f);
  CHECK(r.hits == 0);
}

TEST(autopilot_trip_spot_taken_on_arrival) {
  // regression 1.6: the parked car streams in 12 m out, past the approach's rechecks: the arrival moves the spot,
  // and the descent closes on it at its height before it sinks
  TripSetup s = Street();
  TripBox car{{{258, 258, 0}, {262, 262, 1.5f}}};
  car.appearWithin = 12.0f;
  s.boxes.push_back(car);
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  CHECK(Broken("taken_on_arrival", r, s.params) == 0);
  CHECK(Distance2D(r.landingSpot, {260, 260, 0}) >= 4.0f);
  const size_t d = FrameAt(r, ChangeAt(r, "descending"));
  CHECK(d < r.frames.size());
  const float startZ = r.frames[d].p.z;
  float sunk = 0.0f;
  for (size_t i = d; i < r.frames.size(); ++i) {
    if (Distance2D(r.frames[i].p, r.landingSpot) > s.params.landingOverSpot + 0.5f) {
      sunk = std::max(sunk, startZ - r.frames[i].p.z);
    }
  }
  std::printf("  taken on arrival: moved %.1f m, sank %.2f m before over the spot\n",
              Distance2D(r.landingSpot, {260, 260, 0}), sunk);
  CHECK(sunk < 2.0f);
  CHECK(r.hits == 0);
}

TEST(autopilot_trip_descent_held_up) {
  // regression 2.4: something solid the map lacks 4 m over the spot (a canopy the rays under it cannot see): it
  // stops sinking 3 m or more above the floor and hovers ("arrived"), not "landed"
  TripSetup s = Street();
  TripBox canopy{{{254, 254, 4.0f}, {266, 266, 4.5f}}};
  canopy.kind = TripBox::Kind::Static;
  s.boxes.push_back(canopy);
  s.maxTime = 60.0f;
  s.settle = 0.0f;
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  FlightSim::TripLimits limits;
  limits.mustLand = false;
  CHECK(Broken("held_up", r, s.params, limits) == 0);
  CHECK(!r.landed);
  CHECK(ChangeAt(r, "arrived") > ChangeAt(r, "descending"));
  CHECK(!r.flightOff);
  CHECK(r.frames.back().p.z > 4.5f);
}

TEST(autopilot_trip_prop_not_on_map) {
  // regression 6.9 with a box: a pillar the nav map does not have, across the route at its height. The Autopilot
  // cannot see it (no ray looks ahead), so the honest check is that the hit is survived: one hit, the rejoin asked
  // for after it, the trip landed, nothing hit again
  TripSetup s = Street();
  TripBox pillar{{{105, 105, 0}, {112, 112, 80}}};
  pillar.kind = TripBox::Kind::Static;
  s.boxes.push_back(pillar);
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  CHECK(Broken("prop", r, s.params) == 0);
  CHECK(HitEpisodes(r) == 1);
  CHECK(r.replans >= 1);
  float hitAt = -1.0f;
  for (const auto &f : r.frames) {
    if (f.hit) {
      hitAt = f.t;
      break;
    }
  }
  bool asked = false;
  for (const auto &f : r.frames) {
    asked |= f.planFrame && f.t > hitAt && f.t < hitAt + 2.0f;
  }
  std::printf("  prop: hit at %.1f s at %.1f m/s, landed at %.1f s, %d replans\n", hitAt, r.impactMax, r.time,
              r.replans);
  CHECK(asked);
}

TEST(autopilot_trip_knock_on_approach) {
  // regressions 6.1 (knocked on the approach: back on the line, landed), 6.2 (the rejoin does not climb), over
  // seeds as the in-game recoveries vary run to run
  for (uint64_t seed = 1; seed <= 5; ++seed) {
    Rng pick(seed * 7919u);
    TripSetup s = Street();
    s.seed = seed;
    FlightSim::TripKnock k;
    k.atRemaining = pick.RangeF(90.0f, 170.0f);
    k.bodySpin = {pick.RangeF(-1.5f, 1.5f), pick.RangeF(-3.0f, 3.0f), pick.RangeF(-1.5f, 1.5f)};
    const float side = pick.RangeF(6.0f, 12.0f) * (pick.Unit() < 0.5f ? -1.0f : 1.0f);
    k.velocity = {side * 0.7071f, -side * 0.7071f, 0.0f}; // across the diagonal
    s.knocks.push_back(k);
    float knockedAt = -1.0f, knockZ = 0.0f;
    s.onFrame = [&](Trip &aTrip) {
      if (knockedAt < 0.0f && aTrip.knocks > 0) {
        knockedAt = aTrip.time;
        knockZ = aTrip.sim.Position().z;
      }
      return true;
    };
    Trip trip(Town(), kNoPads, kNoGates, s);
    const TripResult r = trip.Run();
    const std::string name = "knock" + std::to_string(seed);
    CHECK(Broken(name.c_str(), r, s.params) == 0);
    CHECK(knockedAt > 0.0f);
    float climb = -100.0f, back = -1.0f, slowest = 1000.0f;
    for (const auto &f : r.frames) {
      if (f.t <= knockedAt || f.t > knockedAt + 8.0f) {
        continue;
      }
      climb = std::max(climb, f.p.z - knockZ);
      if (std::string(FlightSim::TripStatusName(f.status)) == "following") {
        slowest = std::min(slowest, f.speed);
      }
      if (back < 0.0f && f.following && f.off < 2.0f && f.t > knockedAt + 0.5f) {
        back = f.t - knockedAt;
      }
    }
    std::printf("  knock seed %llu: at %.1f s, %d replans, back on the line in %.1f s, climbed %.1f m, slowest %.1f "
                "m/s following, off p99 %.1f m, landed %d at %.1f s\n",
                static_cast<unsigned long long>(seed), knockedAt, r.replans, back, climb, slowest, r.offP99, r.landed,
                r.time);
    CHECK(r.replans >= 1);
    CHECK(back > 0.0f && back < 6.0f);
    CHECK(climb < 15.0f);
  }
}

TEST(autopilot_trip_same_seed_same_flight) {
  // tests kind 2: the same setup and seed fly the same trip, bit for bit (a knock and its replans included)
  TripSetup s = Street();
  s.seed = 3;
  FlightSim::TripKnock k;
  k.atRemaining = 120.0f;
  k.bodySpin = {1.0f, 2.0f, 0.5f};
  k.velocity = {6.0f, -6.0f, 0.0f};
  s.knocks.push_back(k);
  Trip a(Town(), kNoPads, kNoGates, s), b(Town(), kNoPads, kNoGates, s);
  const TripResult ra = a.Run(), rb = b.Run();
  CHECK(ra.frames.size() > 100);
  CHECK(FlightSim::SameTrip(ra, rb));
  // and free roam's legs follow the seed
  TripSetup roam = Street();
  roam.tracked = false;
  roam.maxTime = 12.0f;
  roam.params.freeRoamLegMin = 100.0f;
  roam.params.freeRoamLegMax = 150.0f;
  roam.params.freeRoamRadius = 150.0f;
  roam.start = {160, 250, 0.6f};
  roam.seed = 7;
  Trip c(Town(), kNoPads, kNoGates, roam), d(Town(), kNoPads, kNoGates, roam);
  roam.seed = 8;
  Trip e(Town(), kNoPads, kNoGates, roam);
  const TripResult rc = c.Run(), rd = d.Run(), re = e.Run();
  CHECK(FlightSim::SameTrip(rc, rd));
  CHECK(!FlightSim::SameTrip(rc, re));
}

TEST(autopilot_trip_free_roam) {
  // free roam legs chained over the town for a minute: no landing, every rule kept
  TripSetup s = Street();
  s.tracked = false;
  s.start = {160, 250, 0.6f};
  s.maxTime = 60.0f;
  s.params.freeRoamLegMin = 100.0f;
  s.params.freeRoamLegMax = 150.0f;
  s.params.freeRoamRadius = 150.0f;
  s.params.freeRoamChainAt = 100.0f;
  s.params.freeRoamJoinMin = 30.0f;
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  FlightSim::TripLimits limits;
  limits.mustLand = false;
  CHECK(Broken("free_roam", r, s.params, limits) == 0);
  CHECK(!r.landed && !r.exitAutopilot);
  float flown = 0.0f;
  for (size_t i = 1; i < r.frames.size(); ++i) {
    flown += Distance2D(r.frames[i].p, r.frames[i - 1].p);
  }
  std::printf("  free roam: %.0f m in %.0f s, %d plans, status %s\n", flown, r.time, r.requests, r.status.c_str());
  CHECK(flown > 600.0f);
  CHECK(r.requests >= 3);
}

TEST(autopilot_trip_no_route) {
  // nowhere to go (off the map): "no route", never retried, nothing left in the queue
  TripSetup s = Street();
  s.destination = {2000, 2000, 30};
  s.settle = 5.0f;
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  FlightSim::TripLimits limits;
  limits.mustLand = false;
  CHECK(Broken("no_route", r, s.params, limits) == 0);
  CHECK(r.status == "no route");
  CHECK(r.requests == 1);
  CHECK(r.time < 1.0f);
  CHECK(Distance2D(r.frames.back().p, s.start) < 1.0f);
}

TEST(autopilot_trip_teleport_mid_route) {
  // regression 6.7: teleported mid-route, the old route and plan dropped, planned again from where it is, landed
  TripSetup s = Street();
  s.teleports.push_back({6.0f, {60, 250, 0.6f}, 90.0f});
  Vec3 startAfter{};
  bool replanned = false, following = false;
  s.onFrame = [&](Trip &aTrip) {
    replanned |= aTrip.teleportDone[0] && aTrip.ap.status == "planning";
    if (replanned && !following && aTrip.ap.status == "following") {
      following = true;
      startAfter = aTrip.ap.route.Point(0);
    }
    return true;
  };
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  CHECK(Broken("teleport", r, s.params) == 0);
  // the first frame after the teleport: no frame follows the old route
  const size_t after = FrameAt(r, 6.0f + 1.5f * s.dt);
  CHECK(std::string(FlightSim::TripStatusName(r.frames[after].status)) == "planning");
  CHECK(r.requests == 2);
  CHECK(following && Distance2D(startAfter, {60, 250, 0}) < 5.0f);
  CHECK(r.landed);
}

TEST(autopilot_trip_arrives_and_hands_back) {
  // regression 8.1: no landing, a point in the air: held there until still (or 5 s), then handed back
  TripSetup s = Street();
  s.params.landAtDestination = false;
  s.destination = {260, 260, 30};
  s.settle = 0.0f;
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  FlightSim::TripLimits limits;
  limits.mustLand = false;
  CHECK(Broken("hand_back", r, s.params, limits) == 0);
  CHECK(r.exitAutopilot && r.status == "idle");
  const float arrived = ChangeAt(r, "arrived"), idle = ChangeAt(r, "idle");
  CHECK(arrived > 0.0f && idle > arrived);
  const auto &last = r.frames[FrameAt(r, idle)];
  CHECK(last.speed < 0.5f || idle - arrived > 4.9f);
  float drift = 0.0f;
  const float z = r.frames[FrameAt(r, arrived)].p.z;
  for (size_t i = FrameAt(r, arrived); i < r.frames.size(); ++i) {
    drift = std::max(drift, std::fabs(r.frames[i].p.z - z));
  }
  std::printf("  hand back: arrived at %.1f s, handed back %.1f s later at %.2f m/s, height held within %.2f m\n",
              arrived, idle - arrived, last.speed, drift);
  CHECK(drift < 1.0f);
}

TEST(autopilot_trip_budgets) {
  // tests kind 4: rays per frame and per trip, rays per landing search, time per Step and the plan's wait, so a
  // change that makes the Autopilot expensive again fails here
  TripSetup s = Street();
  s.boxes.push_back({{{258, 258, 0}, {262, 262, 1.5f}}});
  s.boxes.back().appearWithin = 85.0f;
  Trip trip(Town(), kNoPads, kNoGates, s);
  const TripResult r = trip.Run();
  CHECK(Broken("budgets", r, s.params) == 0);
  std::printf("  budgets: %lld rays in the trip, %d at most in a frame; Step %.4f ms mean, %.4f p99, %.3f max (frames "
              "that planned nothing)\n",
              static_cast<long long>(r.rays), r.raysMaxFrame, r.stepMsMean, r.stepMsP99, r.stepMsMax);
  CHECK(r.raysMaxFrame <= 200); // 150 measured: the recheck's search round the taken spot
  CHECK(r.rays <= 1500);        // 687 measured
  CHECK(r.stepMsP99 < 0.5f);    // 0.01 ms measured
  // the plan's wait: asked on the first frame, followed once the delay is up
  CHECK_NEAR(ChangeAt(r, "following") - ChangeAt(r, "planning"), s.planDelay, 2.0f * s.dt);
  // one landing search round a taken spot: the old search cast up to 18 rays for each of 469 candidates, twice
  FlightSim::TripWorld world(Town(), kNoPads, kNoGates);
  world.vehicles.push_back({{258, 258, 0}, {262, 262, 1.5f}});
  GroundSpot spot;
  CHECK(FindLandingSpot(world, s.params, {260, 260, 0}, {}, spot));
  std::printf("  one landing search round a taken spot: %lld rays\n", static_cast<long long>(world.rays));
  CHECK(world.rays <= 200);
}
