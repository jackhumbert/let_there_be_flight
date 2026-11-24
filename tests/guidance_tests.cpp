// The flight route shown to the player (src/red4ext/Navigation/Guidance): planned when flying with a
// destination, again when it moves, rejoined off the line, the Autopilot's route shown as it is, and
// cleared when flight ends.
#include "check.hpp"

#include "Navigation/Guidance.hpp"
#include "nav_fixture.hpp"

#include <cmath>

using namespace Navigation;

namespace {
float Distance(const Vec3 &aA, const Vec3 &aB) { return std::hypot(aA.x - aB.x, aA.y - aB.y, aA.z - aB.z); }

// 256 x 256 m, floor at 0, a 60 m tower from (100, 60) to (140, 200)
const NavMap &Fixture() {
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    spec.blocks.push_back({100, 60, 140, 200, 60});
    return map.Load(NavFixture::Make(spec), "fixture");
  }();
  CHECK(loaded);
  return map;
}

// flying from the west side of the tower to the east side
GuidanceInput Flying(Vec3 aCar, Vec3 aDestination) {
  GuidanceInput in;
  in.flying = true;
  in.hasDestination = true;
  in.car = aCar;
  in.destination = aDestination;
  in.plan.doorways = false; // none baked into the fixture
  in.timeDelta = 0.1f;
  return in;
}

// steps until the guidance has a route of a revision past aRevision, at most aFrames
bool StepUntilNew(Guidance &aGuidance, const GuidanceInput &aIn, uint32_t aRevision, int aFrames = 20) {
  for (int i = 0; i < aFrames; ++i) {
    aGuidance.Step(aIn);
    if (aGuidance.HasRoute() && aGuidance.Revision() != aRevision) {
      return true;
    }
  }
  return false;
}
} // namespace

TEST(guidance_plans_from_the_car_to_the_destination) {
  PlanQueue plans(Fixture(), false);
  Guidance g(plans, {});
  const Vec3 car{60, 130, 20}, goal{200, 130, 20};

  // nothing tracked, or not flying: no route and no plan
  auto idle = Flying(car, goal);
  idle.hasDestination = false;
  g.Step(idle);
  idle.hasDestination = true;
  idle.flying = false;
  g.Step(idle);
  CHECK(!g.HasRoute());
  CHECK(plans.Pending() == 0);

  CHECK(StepUntilNew(g, Flying(car, goal), 0));
  CHECK(!g.FromAutopilot());
  CHECK(g.Revision() == 1);
  auto ahead = g.Ahead();
  CHECK(ahead.size() >= 2);
  CHECK(Distance(ahead.front(), car) < 5.0f);
  CHECK(Distance(ahead.back(), goal) < 10.0f);
  // round the tower, not through it: every point is free
  for (const auto &p : ahead) {
    CHECK(Fixture().IsFree(p.x, p.y, p.z));
  }

  // moving along it: the same line, its start following the car
  const Vec3 later = ahead[ahead.size() / 3];
  auto in = Flying(later, goal);
  g.Step(in);
  CHECK(g.Revision() == 1);
  CHECK(g.OffLine() < 1.0f);
  CHECK(Distance(g.Ahead().front(), later) < 1.0f);
  CHECK(g.Ahead().size() < ahead.size());
}

TEST(guidance_plans_again_when_the_destination_moves) {
  PlanQueue plans(Fixture(), false);
  Guidance g(plans, {});
  const Vec3 car{60, 130, 20};
  CHECK(StepUntilNew(g, Flying(car, {200, 130, 20}), 0));
  const uint32_t first = g.Revision();

  // a few metres: kept
  g.Step(Flying(car, {205, 130, 20}));
  CHECK(g.Revision() == first);
  CHECK(g.HasRoute());

  // somewhere else: the old line goes at once, a new one follows
  g.Step(Flying(car, {200, 30, 20}));
  CHECK(!g.HasRoute());
  CHECK(StepUntilNew(g, Flying(car, {200, 30, 20}), g.Revision()));
  CHECK(Distance(g.Ahead().back(), {200, 30, 20}) < 10.0f);
}

TEST(guidance_rejoins_the_line_off_it) {
  PlanQueue plans(Fixture(), false);
  Guidance g(plans, {});
  const Vec3 goal{200, 130, 20};
  CHECK(StepUntilNew(g, Flying({60, 130, 20}, goal), 0));
  const uint32_t first = g.Revision();

  // 60 m to the side of the line: a rejoin from there
  const Vec3 strayed{60, 70, 20};
  CHECK(StepUntilNew(g, Flying(strayed, goal), first));
  CHECK(Distance(g.Ahead().front(), strayed) < 5.0f);
  CHECK(Distance(g.Ahead().back(), goal) < 10.0f);
}

TEST(guidance_shows_the_autopilots_route) {
  PlanQueue plans(Fixture(), false);
  Guidance g(plans, {});
  const Vec3 goal{200, 130, 20};
  Route ap = Route::Make({{60, 30, 20}, {130, 30, 20}, {200, 30, 20}, {200, 130, 20}});
  auto in = Flying({60, 30, 20}, goal);
  in.autopilot = &ap;
  g.Step(in);
  CHECK(g.HasRoute());
  CHECK(g.FromAutopilot());
  CHECK(plans.Pending() == 0);
  const uint32_t first = g.Revision();
  CHECK(g.Ahead().size() == 4);

  // the same route next frame: no new revision
  g.Step(in);
  CHECK(g.Revision() == first);

  // the Autopilot replans: copied again
  Route replanned = Route::Make({{60, 30, 20}, {200, 30, 20}, {200, 130, 20}});
  in.autopilot = &replanned;
  g.Step(in);
  CHECK(g.Revision() == first + 1);
  CHECK(g.Ahead().size() == 3);

  // disengaged: its line stays, flown by hand from here
  in.autopilot = nullptr;
  g.Step(in);
  CHECK(g.HasRoute());
  CHECK(!g.FromAutopilot());
  CHECK(g.Revision() == first + 1);
}

TEST(guidance_clears_when_flight_ends) {
  PlanQueue plans(Fixture(), false);
  Guidance g(plans, {});
  auto in = Flying({60, 130, 20}, {200, 130, 20});
  CHECK(StepUntilNew(g, in, 0));
  const uint32_t first = g.Revision();
  // a plan asked for, then flight off: nothing left
  g.Step(Flying({60, 130, 20}, {200, 30, 20}));
  in.flying = false;
  g.Step(in);
  CHECK(!g.HasRoute());
  CHECK(g.Ahead().empty());
  CHECK(g.Revision() > first);
  CHECK(plans.Pending() == 0);
}

TEST(guidance_waits_after_a_failed_plan) {
  PlanQueue plans(Fixture(), false);
  GuidanceParams params;
  params.retry = 1.0f;
  Guidance g(plans, params);
  // off the map: no plan reaches it
  auto in = Flying({60, 130, 20}, {2000, 2000, 20});
  g.Step(in); // asks
  g.Step(in); // takes the failure
  CHECK(!g.HasRoute());
  CHECK(plans.Pending() == 0);
  // nothing asked again for a second
  for (int i = 0; i < 8; ++i) {
    g.Step(in);
    CHECK(plans.Pending() == 0);
  }
  // then asked again
  bool asked = false;
  for (int i = 0; i < 5 && !asked; ++i) {
    g.Step(in);
    asked = plans.Pending() == 1;
  }
  CHECK(asked);
}
