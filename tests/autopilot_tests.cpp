// The Autopilot as pure C++ (src/red4ext/Navigation/Autopilot), worked from FlightModeAutopilot.reds: flown
// over a nav map made in memory with plans that take frames to come, the car a point mass whose attitude
// takes the target the attitude control asks for at once.
#include "check.hpp"

#include "nav_fixture.hpp"
#include "Navigation/Autopilot.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <memory>

using namespace Navigation;

namespace {
constexpr float kDt = 1.0f / 30.0f;
constexpr float kG = 9.81f;

// 256 x 256 m, floor at 0 (a resting car's centre about 1 m up); a 30 m block (150..190, 60..100) off the paths
const NavMap &Small() {
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    spec.blocks.push_back({150, 60, 190, 100, 30});
    return map.Load(NavFixture::Make(spec), "autopilot small");
  }();
  CHECK(loaded);
  return map;
}

// 1280 x 128 m, floor at 0: room for a trip long enough to be planned ahead
const NavMap &Long() {
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    spec.width = 640;
    spec.height = 64;
    return map.Load(NavFixture::Make(spec), "autopilot long");
  }();
  CHECK(loaded);
  return map;
}

float Distance2D(const Vec3 &a, const Vec3 &b) { return std::hypot(a.x - b.x, a.y - b.y); }

// the rotation taking X, Y, Z to right, forward, up (orthonormal)
Flight::Quat FromAxes(const Flight::Vec3 &r, const Flight::Vec3 &f, const Flight::Vec3 &u) {
  const float m00 = r.x, m01 = f.x, m02 = u.x, m10 = r.y, m11 = f.y, m12 = u.y, m20 = r.z, m21 = f.z, m22 = u.z;
  const float trace = m00 + m11 + m22;
  Flight::Quat q;
  if (trace > 0.0f) {
    const float s = 0.5f / std::sqrt(trace + 1.0f);
    q = {(m21 - m12) * s, (m02 - m20) * s, (m10 - m01) * s, 0.25f / s};
  } else if (m00 > m11 && m00 > m22) {
    const float s = 2.0f * std::sqrt(1.0f + m00 - m11 - m22);
    q = {0.25f * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
  } else if (m11 > m22) {
    const float s = 2.0f * std::sqrt(1.0f + m11 - m00 - m22);
    q = {(m01 + m10) / s, 0.25f * s, (m12 + m21) / s, (m02 - m20) / s};
  } else {
    const float s = 2.0f * std::sqrt(1.0f + m22 - m00 - m11);
    q = {(m02 + m20) / s, (m12 + m21) / s, 0.25f * s, (m10 - m01) / s};
  }
  return q;
}

// One car flown by the Autopilot: the plans come aDelay seconds after they are asked for, the force is
// applied to a point mass with the game's drag, the attitude is what the attitude control asked for.
struct Rig {
  std::vector<Helipad> pads;
  std::vector<Gate> gates;
  MapWorld world;
  PlanQueue queue;
  AutopilotParams params;
  Rng rng;
  Autopilot ap;
  AutopilotInput in;
  Flight::Vec3 position{}, velocity{};
  Flight::Quat orientation{0.0f, 0.0f, 0.0f, 1.0f};
  AutopilotOutput out;
  AutopilotEvents seen;
  int splices = 0, frames = 0;
  float time = 0.0f;
  float floorZ = 0.8f; // a resting car's centre, a little under the bake's floor

  Rig(const NavMap &aMap, Vec3 aStart, float aDelay = 0.3f, uint64_t aSeed = 1)
      : world(aMap, pads, gates), queue(aMap, false, aDelay), rng(aSeed), ap(world, queue, params, rng) {
    ap.planBase.doorways = false;
    in.statics.mass = 1500.0f;
    in.statics.airResistanceFactor = 2.0f;
    in.statics.brakingFrictionFactor = 1.4f;
    in.statics.momentOfInertia = {2500.0f, 900.0f, 3000.0f};
    in.isPlayerMounted = true;
    position = {aStart.x, aStart.y, aStart.z};
    Stats();
  }

  void Stats() {
    Flight::StatsInput s;
    s.orientation = orientation;
    s.origin = position;
    s.velocity = velocity;
    Flight::UpdateDynamic(in.stats, s);
  }
  void Track(Vec3 aAt) { in.tracked = {true, aAt}; }
  void Engage() { ap.Activate(in); }
  Vec3 P() const { return {position.x, position.y, position.z}; }

  void Frame() {
    in.dt = kDt;
    const size_t before = ap.route.points.size();
    const bool wasFollowing = ap.status == "following";
    out = ap.Step(in);
    // a splice: the route grew while it was followed
    if (wasFollowing && ap.status == "following" && before >= 2 && ap.route.points.size() > before + 2) {
      ++splices;
    }
    seen.exitAutopilot |= out.events.exitAutopilot;
    seen.flightOff |= out.events.flightOff;
    seen.notifications.insert(seen.notifications.end(), out.events.notifications.begin(), out.events.notifications.end());
    Flight::Vec3 accel{};
    if (!out.manual) {
      accel = Flight::Rotate(orientation, out.force);
      accel.z -= kG;
      const float speed = Flight::Length(velocity);
      if (speed > 0.01f) {
        const float drag = Drag(ap.vehicle, speed) / speed;
        accel = Flight::Sub(accel, Flight::Mul(velocity, drag));
      }
      // the attitude the control asked for, at once
      const auto &c = ap.control;
      if (Flight::Length(c.targetForward) > 0.5f && Flight::Length(c.targetUp) > 0.5f) {
        const Flight::Vec3 r = Flight::Normalize(Flight::Cross(c.targetForward, c.targetUp));
        const Flight::Vec3 u = Flight::Cross(r, c.targetForward);
        orientation = FromAxes(r, c.targetForward, u);
      }
    } else {
      // the player's own flying, not modelled: the speed dies away
      accel = Flight::Mul(velocity, -1.0f);
    }
    velocity = Flight::Add(velocity, Flight::Mul(accel, kDt));
    position = Flight::Add(position, Flight::Mul(velocity, kDt));
    if (position.z < floorZ) {
      position.z = floorZ;
      velocity.z = std::max(velocity.z, 0.0f);
      // on the wheels: friction
      velocity.x *= 0.9f;
      velocity.y *= 0.9f;
    }
    queue.Advance(kDt);
    time += kDt;
    ++frames;
    Stats();
  }

  // frames until aDone (true) or aSeconds pass (false)
  bool Until(const std::function<bool()> &aDone, float aSeconds) {
    const float end = time + aSeconds;
    while (time < end) {
      Frame();
      if (aDone()) {
        return true;
      }
    }
    return false;
  }
  bool UntilStatus(const char *aStatus, float aSeconds) {
    return Until([&] { return ap.status == aStatus; }, aSeconds);
  }
};
} // namespace

TEST(autopilot_rng) {
  Rng a(7), b(7), c(8);
  bool differ = false;
  for (int i = 0; i < 200; ++i) {
    const float x = a.RangeF(-1.0f, 1.0f);
    CHECK(x >= -1.0f && x < 1.0f);
    CHECK(x == b.RangeF(-1.0f, 1.0f));
    differ |= x != c.RangeF(-1.0f, 1.0f);
    const int32_t k = a.Range(0, 5);
    b.Range(0, 5);
    c.Range(0, 5);
    CHECK(k >= 0 && k < 5);
  }
  CHECK(differ);
  // an empty range: its start, as an index into nothing is never asked for
  CHECK(a.Range(3, 3) == 3);
}

TEST(autopilot_steepest_pitch_and_blend) {
  // 45 degrees over 20 m, flat before; a 15 m wall in under 10 m across
  CHECK_NEAR(SteepestPitch({{0, 0, 0}, {20, 0, 0}, {40, 0, 20}}, 10.0f), 45.0, 1e-3);
  CHECK(SteepestPitch({{0, 0, 0}, {5, 0, 15}}, 10.0f) > 70.0f);
  CHECK_NEAR(SteepestPitch({{0, 0, 0}, {5, 0, 1}}, 10.0f), 0.0, 1e-6);
  // halfway from +X to +Y; and across the -X axis the short way round, not through +X
  const Vec3 h = BlendHeading({1, 0, 0}, {0, 1, 0}, 0.5f);
  CHECK_NEAR(h.x, std::cos(0.7853982), 1e-5);
  CHECK_NEAR(h.y, std::sin(0.7853982), 1e-5);
  const Vec3 w = BlendHeading({-1, 0.1f, 0}, {-1, -0.1f, 0}, 0.5f);
  CHECK_NEAR(w.x, -1.0, 1e-4);
}

TEST(autopilot_round_corners_and_smooth_join) {
  struct Open : Space {
    bool IsFree(const Vec3 &) const override { return true; }
    float SegmentClear(const Vec3 &, const Vec3 &) const override { return 1.0f; }
  } open;
  // one corner: nine arc points instead of it, the ends kept
  auto r = RoundCorners({{0, 0, 0}, {100, 0, 0}, {100, 100, 0}}, 150.0f, 0.5f, 8.0f, 0.7f, open);
  CHECK(r.size() == 11);
  CHECK_NEAR(r[1].x, 50.0, 1e-4);
  CHECK_NEAR(r[9].y, 50.0, 1e-4);
  // two points come back as they are
  CHECK(RoundCorners({{0, 0, 0}, {1, 0, 0}}, 150.0f, 0.5f, 8.0f, 0.7f, open).size() == 2);
  // the join's kink eased, the ends left
  std::vector<Vec3> kink;
  for (int i = 0; i <= 10; ++i) {
    kink.push_back({5.0f * float(i), i <= 5 ? 0.0f : 5.0f * float(i - 5), 0.0f});
  }
  auto s = SmoothJoin(kink, 5, 1, 9, 30.0f, 20, open);
  CHECK(s.front().x == 0.0f && s.back().x == 50.0f);
  CHECK(s[5].y > 0.5f);
}

TEST(autopilot_engage_plans_then_follows) {
  Rig rig(Small(), {40, 40, 1});
  rig.Track({200, 200, 1});
  rig.Engage();
  CHECK(rig.ap.status == "idle");
  CHECK(!rig.ap.freeRoamChosen);
  rig.Frame();
  // asked for, a landing spot by the destination, held where it is meanwhile
  CHECK(rig.ap.status == "planning");
  CHECK(rig.ap.request > 0);
  CHECK(rig.ap.landing);
  CHECK(Distance2D(rig.ap.landingSpot, {200, 200, 1}) < 3.0f);
  CHECK_NEAR(rig.ap.routeGoal.z, rig.ap.landingSpot.z + rig.params.approachHeight, 1e-3);
  // the plan takes 0.3 s: still holding until it comes
  for (int i = 0; i < 7; ++i) {
    rig.Frame();
    CHECK(rig.ap.status == "planning");
    CHECK(rig.out.writeControls);
  }
  CHECK(Distance2D(rig.P(), {40, 40, 1}) < 0.5f);
  CHECK(rig.UntilStatus("following", 1.0f));
  CHECK(rig.ap.RouteSize() >= 2);
  CHECK(rig.ap.FollowsTrajectory());
  CHECK(rig.ap.request == 0);
  CHECK(!rig.ap.lastPlanInfo.empty());
  CHECK(rig.queue.Pending() == 0);
  // and it flies off along it
  CHECK(rig.Until([&] { return rig.ap.tracker.closestS > 20.0f; }, 10.0f));
}

TEST(autopilot_lands_and_asks_flight_off) {
  Rig rig(Small(), {40, 40, 1});
  rig.Track({200, 200, 1});
  rig.Engage();
  CHECK(rig.UntilStatus("descending", 60.0f));
  CHECK(rig.ap.RouteSize() == 0);
  CHECK(rig.ap.holdPosition.x == rig.ap.landingSpot.x);
  CHECK(!rig.seen.flightOff);
  CHECK(rig.UntilStatus("landed", 40.0f));
  CHECK(rig.seen.flightOff);
  CHECK(std::find(rig.seen.notifications.begin(), rig.seen.notifications.end(), "LocKey#96672") !=
        rig.seen.notifications.end());
  CHECK(Distance2D(rig.P(), rig.ap.landingSpot) < rig.params.landingOverSpot);
  CHECK(rig.position.z - rig.ap.landingSpot.z < 3.0f);
  // not mounted (a summon): landed, but nobody to switch flight off for
  Rig summon(Small(), {40, 40, 1});
  summon.in.isPlayerMounted = false;
  summon.Track({200, 200, 1});
  summon.Engage();
  CHECK(summon.UntilStatus("landed", 100.0f));
  CHECK(!summon.seen.flightOff);
  CHECK(summon.seen.notifications.empty());
}

TEST(autopilot_arrives_without_landing) {
  Rig rig(Small(), {40, 40, 1});
  rig.params.landingMode = kLandHover;
  rig.Track({200, 200, 30});
  rig.Engage();
  CHECK(rig.UntilStatus("arrived", 60.0f));
  CHECK(!rig.ap.landing);
  CHECK(rig.ap.RouteSize() == 0);
  CHECK(Distance2D(rig.P(), {200, 200, 30}) < rig.params.arriveDistance + 1.0f);
  // held until still, then handed back
  CHECK(!rig.seen.exitAutopilot);
  CHECK(rig.UntilStatus("idle", 10.0f));
  CHECK(rig.seen.exitAutopilot);
}

TEST(autopilot_destination_moves) {
  Rig rig(Small(), {40, 40, 1});
  rig.Track({200, 200, 1});
  rig.Engage();
  CHECK(rig.UntilStatus("following", 2.0f));
  rig.Until([] { return false; }, 1.0f);
  const size_t points = rig.ap.route.points.size();
  // 10 m: the same trip
  rig.Track({207, 207, 1});
  rig.Frame();
  CHECK(rig.ap.status == "following");
  CHECK(rig.ap.request == 0);
  CHECK(rig.ap.route.points.size() == points);
  CHECK_NEAR(rig.ap.destination.x, 200.0, 1e-4);
  // 25 m from the one flown to: planned again from here
  rig.Track({200, 225, 1});
  rig.Frame();
  CHECK(rig.ap.status == "planning");
  CHECK(rig.ap.RouteSize() == 0);
  CHECK(rig.ap.request > 0);
  CHECK_NEAR(rig.ap.destination.y, 225.0, 1e-4);
  CHECK(rig.UntilStatus("following", 2.0f));
}

TEST(autopilot_untracked_hands_back_but_not_mid_descent) {
  Rig rig(Small(), {40, 40, 1});
  rig.Track({200, 200, 1});
  rig.Engage();
  CHECK(rig.UntilStatus("following", 2.0f));
  rig.in.tracked.has = false;
  rig.Frame();
  CHECK(rig.out.events.exitAutopilot);

  Rig landing(Small(), {40, 40, 1});
  landing.Track({200, 200, 1});
  landing.Engage();
  CHECK(landing.UntilStatus("descending", 60.0f));
  // the game untracks a pin once the player reaches it: the landing goes on
  landing.in.tracked.has = false;
  CHECK(landing.UntilStatus("landed", 40.0f));
  CHECK(!landing.seen.exitAutopilot);
}

// A map pin reached: the game untracks it and tracks the quest's objective again. The trip is over, so the car
// lands (or hovers and hands back) where it is, and does not take off for the quest.
TEST(autopilot_retracked_after_arrival_does_not_leave) {
  Rig landing(Small(), {40, 40, 1});
  landing.Track({200, 200, 1});
  landing.Engage();
  CHECK(landing.UntilStatus("descending", 60.0f));
  landing.Track({60, 220, 1});
  CHECK(landing.UntilStatus("landed", 40.0f));
  CHECK(landing.seen.flightOff);
  CHECK_NEAR(landing.ap.destination.x, 200.0, 1e-4);
  CHECK(Distance2D(landing.P(), {200, 200, 1}) < 30.0f);

  Rig hover(Small(), {40, 40, 1});
  hover.params.landingMode = kLandHover;
  hover.Track({200, 200, 30});
  hover.Engage();
  CHECK(hover.UntilStatus("arrived", 60.0f));
  hover.Track({60, 220, 1});
  CHECK(hover.UntilStatus("idle", 10.0f));
  CHECK(hover.seen.exitAutopilot);
  CHECK_NEAR(hover.ap.destination.x, 200.0, 1e-4);
}

TEST(autopilot_no_route) {
  Rig rig(Small(), {40, 40, 1});
  // off the map: no landing spot, and the planner finds no way there
  rig.Track({2000, 2000, 30});
  rig.Engage();
  CHECK(rig.UntilStatus("no route", 2.0f));
  CHECK(rig.ap.RouteSize() == 0);
  CHECK(!rig.ap.landing);
  // never retried on a flight to a destination
  rig.Until([] { return false; }, 5.0f);
  CHECK(rig.ap.status == "no route");
  CHECK(rig.ap.request == 0);
}

TEST(autopilot_no_nav_map) {
  NavMap empty;
  Rig rig(empty, {40, 40, 1});
  rig.Track({200, 200, 1});
  rig.Engage();
  rig.Frame();
  CHECK(rig.ap.status == "no nav map");
  CHECK(rig.ap.request == 0);
}

TEST(autopilot_hit_recovers_then_rejoins) {
  Rig rig(Small(), {40, 80, 1});
  rig.Track({240, 80, 1});
  rig.Engage();
  CHECK(rig.UntilStatus("following", 2.0f));
  CHECK(rig.Until([&] { return rig.ap.tracker.closestS > 40.0f; }, 10.0f));
  CHECK(rig.ap.routeLine);
  // the component's collision timer drops: a hit
  rig.in.collisionTimer = 0.0f;
  rig.Frame();
  CHECK(rig.ap.hits.size() == 1);
  CHECK_NEAR(rig.ap.hits[0].radius, rig.params.hitAvoidRadius, 1e-6);
  CHECK(rig.ap.recovering > 0.0f);
  CHECK(rig.ap.request == 0);
  // the timer rising again is no hit; the replan waits out collisionRecoveryDelay
  bool asked = false;
  float askedAt = 0.0f;
  const float hitAt = rig.time;
  for (int i = 0; i < 60 && !asked; ++i) {
    rig.in.collisionTimer += kDt;
    rig.Frame();
    if (rig.ap.request > 0) {
      asked = true;
      askedAt = rig.time - hitAt;
    }
  }
  CHECK(asked);
  CHECK(rig.ap.requestRejoin);
  CHECK(askedAt >= rig.in.settings.collisionRecoveryDelay - 2.0f * kDt);
  CHECK(rig.ap.hits.size() == 1);
  // the rejoin taken: following again, its info logged
  CHECK(rig.Until([&] { return rig.ap.request == 0; }, 2.0f));
  CHECK(!rig.ap.requestRejoin);
  CHECK(rig.ap.status == "following");
  // remembered for hitAvoidTime, then forgotten
  rig.params.hitAvoidTime = 1.0f;
  rig.Frame();
  CHECK(rig.ap.hits.empty());
}

TEST(autopilot_landing_spot_taken_moves_route_end) {
  Rig rig(Small(), {40, 80, 1});
  rig.Track({240, 80, 1});
  rig.Engage();
  CHECK(rig.UntilStatus("following", 2.0f));
  const Vec3 spot = rig.ap.landingSpot;
  // a car parks on the spot once the flight is under way
  rig.world.vehicles.push_back({{spot.x - 2.2f, spot.y - 1.0f, 0.0f}, {spot.x + 2.2f, spot.y + 1.0f, 1.5f}});
  CHECK(rig.Until([&] { return Distance2D(rig.ap.landingSpot, spot) > 2.0f; }, 30.0f));
  CHECK(rig.ap.status == "following");
  const Vec3 end = rig.ap.route.Point(rig.ap.RouteSize() - 1);
  CHECK(Distance2D(end, rig.ap.landingSpot) < 0.01f);
  CHECK_NEAR(rig.ap.routeGoal.x, end.x, 1e-4);
  CHECK(rig.ap.FollowsTrajectory());
  // and it lands on the new one
  CHECK(rig.UntilStatus("landed", 80.0f));
  CHECK(Distance2D(rig.P(), rig.ap.landingSpot) < rig.params.landingOverSpot);
}

TEST(autopilot_free_roam_chains_legs) {
  auto fly = [](uint64_t aSeed, Vec3 &aFirstGoal) {
    auto rig = std::make_unique<Rig>(Small(), Vec3{128, 40, 1}, 0.3f, aSeed);
    auto &p = rig->params;
    p.freeRoamLegMin = 100.0f;
    p.freeRoamLegMax = 150.0f;
    p.freeRoamRadius = 400.0f;
    p.freeRoamChainAt = 100.0f;
    p.freeRoamJoinMin = 30.0f;
    rig->Engage();
    CHECK(rig->ap.freeRoamChosen);
    rig->Frame();
    CHECK(rig->ap.freeRoam);
    CHECK(rig->ap.status == "free roam" || rig->ap.status == "planning");
    CHECK(rig->ap.request > 0);
    aFirstGoal = rig->ap.routeGoal;
    const float d = Distance2D(aFirstGoal, {128, 40, 1});
    CHECK(d >= 100.0f - 1e-3f && d <= 150.0f + 1e-3f);
    rig->Until([&] { return rig->splices >= 2; }, 60.0f);
    return rig;
  };
  Vec3 a{}, b{}, c{};
  auto first = fly(7, a);
  CHECK(first->splices >= 2);
  // never handed back, never landed
  CHECK(!first->seen.exitAutopilot);
  CHECK(!first->ap.landing);
  // the same seed, the same legs
  auto again = fly(7, b);
  CHECK(a.x == b.x && a.y == b.y && a.z == b.z);
  auto other = fly(8, c);
  CHECK(a.x != c.x || a.y != c.y);
}

TEST(autopilot_free_roam_through_gate) {
  Rig rig(Small(), {128, 30, 1});
  auto &p = rig.params;
  p.freeRoamLegMin = 100.0f;
  p.freeRoamLegMax = 150.0f;
  p.freeRoamGateMin = 50.0f;
  p.gateLead = 20.0f;
  // a gate 110 m ahead, its way across the heading's line (either direction)
  rig.gates.push_back({128, 140, 20, 90.0f, 20.0f, 10.0f, 1e9f});
  rig.Engage();
  rig.Frame();
  CHECK(rig.ap.gateTail.size() == 8);
  CHECK_NEAR(rig.ap.routeGoal.y, 120.0, 1e-3);
  CHECK_NEAR(rig.ap.destination.y, 160.0, 1e-3);
  CHECK(rig.ap.recentGates.size() == 1);
  CHECK(rig.UntilStatus("following", 2.0f));
  // the leg runs on straight through the gate
  CHECK(rig.ap.spliceGate);
  CHECK(rig.ap.routeGateTail.size() == 8);
  const Vec3 end = rig.ap.route.Point(rig.ap.RouteSize() - 1);
  CHECK_NEAR(end.y, 160.0, 1e-3);
  CHECK_NEAR(rig.ap.gateEndS, rig.ap.route.Length(), 1e-3);
}

// A gate whose run passes under a deck lower than raceMinPassage is never reached by the search
// (it squeezes through nothing lower): it is not picked, the open one beside it is.
TEST(autopilot_free_roam_skips_low_gate) {
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    // a deck from 10 to 30 m: under it the car's centre has 1 to 9 m, 8 m of room, too little for
    // raceMinPassage 12
    spec.blocks.push_back({100, 120, 140, 160, 30, 10});
    return map.Load(NavFixture::Make(spec), "autopilot low gate");
  }();
  CHECK(loaded);
  Rig rig(map, {128, 30, 1});
  auto &p = rig.params;
  p.freeRoamLegMin = 100.0f;
  p.freeRoamLegMax = 150.0f;
  p.freeRoamGateMin = 50.0f;
  p.gateLead = 20.0f;
  // under the deck at 5 m (gap 1 to 9: 8 m), and an open one 60 m to the east
  rig.gates.push_back({120, 140, 5, 90.0f, 20.0f, 10.0f, 4.0f});
  rig.gates.push_back({180, 140, 20, 90.0f, 20.0f, 10.0f, 1e9f});
  for (int i = 0; i < 8; ++i) {
    Rig r(map, {128, 30, 1}, 0.3f, uint64_t(i + 1));
    r.params.freeRoamLegMin = 100.0f;
    r.params.freeRoamLegMax = 150.0f;
    r.params.freeRoamGateMin = 50.0f;
    r.params.freeRoamSpread = 60.0f;
    r.params.gateLead = 20.0f;
    r.gates = rig.gates;
    r.Engage();
    r.Frame();
    // never the low gate: the open one, or a plain point when that is off the heading
    CHECK(r.ap.recentGates.empty() || r.ap.recentGates.back() == 1);
  }
  // with the room asked for lowered to what the deck leaves, the low gate counts again
  rig.params.raceMinPassage = 6.0f;
  rig.params.freeRoamSpread = 10.0f; // the open gate off to the side, outside the spread
  rig.Engage();
  rig.Frame();
  CHECK(rig.ap.recentGates.size() == 1 && rig.ap.recentGates.back() == 0);
}

TEST(autopilot_player_input_overrides) {
  Rig rig(Small(), {40, 40, 1});
  rig.Track({200, 200, 1});
  rig.Engage();
  CHECK(rig.UntilStatus("following", 2.0f));
  rig.Until([] { return false; }, 1.0f);
  rig.in.controls.surge = 1.0f;
  rig.Frame();
  CHECK(rig.out.manual);
  CHECK(!rig.out.writeControls);
  CHECK_NEAR(rig.ap.overrideTimer, rig.params.overrideHold - kDt, 1e-5);
  CHECK(Distance2D(rig.ap.holdPosition, rig.P()) < 1.0f);
  CHECK_NEAR(rig.out.manualLift, kG - rig.velocity.z, 0.5);
  // let go: held off overrideHold, then the route again
  rig.in.controls.surge = 0.0f;
  int manual = 0;
  while (true) {
    rig.Frame();
    if (!rig.out.manual) {
      break;
    }
    ++manual;
  }
  CHECK(std::abs(manual - int(rig.params.overrideHold / kDt)) <= 2);
  CHECK(rig.ap.status == "following");
  // the PIDs given: the Standard mode flies the frame
  Flight::Pid pitch, roll;
  rig.in.pitchPid = &pitch;
  rig.in.rollPid = &roll;
  rig.in.controls.surge = 1.0f;
  rig.Frame();
  CHECK(rig.out.manual);
  CHECK(Flight::Length(rig.out.force) > 0.0f);
  // a summon's own flight ignores the drive sim's inputs
  Rig summon(Small(), {40, 40, 1});
  summon.Track({200, 200, 1});
  summon.Engage();
  summon.ap.hasExternalDestination = true;
  summon.ap.externalDestination = {200, 200, 1};
  summon.in.controls.surge = 1.0f;
  summon.Frame();
  CHECK(!summon.out.manual);
}

TEST(autopilot_teleport_resets) {
  Rig rig(Small(), {40, 40, 1});
  rig.Track({200, 200, 1});
  rig.Engage();
  CHECK(rig.UntilStatus("following", 2.0f));
  rig.Until([] { return false; }, 1.0f);
  // a jump the car could not have flown is held off for positionJumpHold
  rig.position = {60, 200, 1};
  rig.velocity = {};
  rig.Stats();
  rig.Frame();
  CHECK(rig.ap.positionJumpTimer > 0.0f);
  // the game's teleport: held there, planned again from there
  rig.ap.Teleported(rig.in);
  CHECK(rig.ap.status == "planning");
  CHECK(rig.ap.RouteSize() == 0);
  CHECK(rig.ap.request > 0);
  CHECK(rig.ap.positionJumpTimer == 0.0f);
  CHECK(Distance2D(rig.ap.holdPosition, rig.P()) < 0.01f);
  CHECK(rig.ap.tracker.speedCmd == 0.0f);
  CHECK(rig.UntilStatus("following", 2.0f));
  CHECK(Distance2D(rig.ap.route.Point(0), {60, 200, 1}) < 5.0f);
  // the old plan dropped, not left in the queue
  CHECK(rig.queue.Pending() == 0);
}

TEST(autopilot_plan_ahead_first_leg_then_rest) {
  Rig rig(Long(), {40, 64, 1});
  rig.ap.planBase.doorways = true;
  rig.Track({1200, 64, 1});
  rig.Engage();
  rig.Frame();
  CHECK(rig.ap.request > 0);
  CHECK(rig.ap.ahead.trip == rig.ap.request);
  CHECK(rig.UntilStatus("following", 2.0f));
  CHECK(rig.ap.lastPlanPartial);
  CHECK(rig.ap.ahead.firstLeg);
  CHECK(rig.ap.ahead.waiting);
  CHECK(rig.ap.joinS > 0.0f);
  const float firstLength = rig.ap.route.Length();
  CHECK(Distance2D(rig.ap.route.Point(rig.ap.RouteSize() - 1), {1200, 64, 1}) > 100.0f);
  // the rest comes a plan's delay later and is spliced on at the join
  CHECK(rig.Until([&] { return !rig.ap.ahead.waiting; }, 2.0f));
  CHECK(!rig.ap.ahead.firstLeg);
  CHECK(rig.ap.route.Length() > firstLength);
  CHECK(Distance2D(rig.ap.route.Point(rig.ap.RouteSize() - 1), rig.ap.landingSpot) < 1.0f);
  CHECK(rig.ap.status == "following");
  CHECK(rig.queue.Pending() == 0);
}

TEST(autopilot_toggle_free_roam) {
  Rig rig(Small(), {40, 40, 1});
  rig.Track({200, 200, 1});
  rig.Engage();
  CHECK(rig.UntilStatus("following", 2.0f));
  rig.ap.ToggleFreeRoam(rig.in);
  CHECK(rig.ap.freeRoamChosen);
  CHECK(!rig.ap.landing);
  rig.Frame();
  CHECK(rig.ap.freeRoam);
  CHECK(rig.out.events.freeRoamUI && rig.out.events.freeRoamEnabled);
  // and back to the tracked destination
  rig.ap.ToggleFreeRoam(rig.in);
  CHECK(!rig.ap.freeRoamChosen);
  const auto events = rig.ap.TakeEvents();
  CHECK(events.freeRoamUI && !events.freeRoamEnabled);
  rig.Frame();
  CHECK(rig.ap.hasDestination);
  CHECK(rig.ap.request > 0);
}
