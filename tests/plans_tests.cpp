// The Autopilot's plans (src/red4ext/Navigation/Plans): the planner's params for each kind, and the
// request queue, threaded as in the game and delayed as offline.
#include "check.hpp"

#include "nav_fixture.hpp"
#include "Navigation/Plans.hpp"

#include <chrono>
#include <thread>

using namespace Navigation;

namespace {
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

// a short plan round the fixture: no doorway layer (none is baked into it), no detours
PlanParams Short() {
  PlanParams params;
  params.doorways = false;
  return params;
}

bool WaitReady(const PlanQueue &aQueue, int32_t aId) {
  for (int i = 0; i < 2000 && !aQueue.Ready(aId); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return aQueue.Ready(aId);
}
} // namespace

TEST(plans_car_params) {
  PlanParams base;
  base.slotAvoidCost = 7.0f;
  base.preferredHeight = 123.0f;
  VehicleSpec spec;
  spec.mass = 4321.0f;
  auto p = CarPlanParams(base, &spec, {{{1, 2, 3}, 0.2f}, {{4, 5, 6}, 9.0f}}, {10, 0, 0});
  // the Race's preset, then the car
  PlanParams preset = base;
  RacePreset(preset);
  CHECK_NEAR(p.preferredHeight, preset.preferredHeight, 1e-6);
  CHECK_NEAR(p.vehicle.mass, 4321.0, 1e-3);
  CHECK_NEAR(p.startVelocity.x, 10.0, 1e-6);
  // avoid spots at least 1 m, at the slot's cost
  CHECK(p.avoid.size() == base.avoid.size() + 2);
  CHECK_NEAR(p.avoid[p.avoid.size() - 2].radius, 1.0, 1e-6);
  CHECK_NEAR(p.avoid.back().radius, 9.0, 1e-6);
  CHECK_NEAR(p.avoid.back().cost, preset.slotAvoidCost, 1e-6);
  // no car: the params' own
  auto none = CarPlanParams(base, nullptr, {}, {0, 0, 0});
  CHECK_NEAR(none.vehicle.mass, preset.vehicle.mass, 1e-6);
}

TEST(plans_race_params) {
  PlanParams base;
  RouteShape shape{150.0f, 2.0f, 9.0f, 0.5f, -1.0f};
  auto p = RaceParams(base, shape, nullptr, true, {}, {0, 0, 0});
  CHECK_NEAR(p.preferredHeight, 150.0, 1e-6);
  CHECK_NEAR(p.climbCost, 2.0, 1e-6);
  CHECK_NEAR(p.minPassage, 9.0, 1e-6);
  // the weight at least 1, the turn cost at least 0
  CHECK_NEAR(p.heuristicWeight, 1.0, 1e-6);
  CHECK_NEAR(p.turnCost, 0.0, 1e-6);
  PlanParams preset = base;
  RacePreset(preset);
  CHECK_NEAR(p.slotDetourLoss, preset.slotDetourLoss, 1e-6);
  // no detours: never worth it
  auto q = RaceParams(base, shape, nullptr, false, {}, {0, 0, 0});
  CHECK(q.slotDetourLoss >= 1e9f && q.slotDetourLossMin >= 1e9f);
}

TEST(plans_free_roam_params) {
  PlanParams base;
  RouteShape shape{60.0f, 1.0f, 8.0f, 1.2f, 3.0f};
  auto p = FreeRoamParams(base, shape, {4.0f, 300.0f, -2.0f}, nullptr, {}, {0, 0, 0});
  CHECK_NEAR(p.preferredHeight, 60.0, 1e-6);
  CHECK_NEAR(p.lineHeightFree, 4.0, 1e-6);
  CHECK_NEAR(p.lineHeightWavelength, 300.0, 1e-6);
  CHECK_NEAR(p.overCost, 0.0, 1e-6);
  CHECK(!p.doorways);
}

TEST(plans_queue_delayed) {
  PlanQueue queue(Fixture(), false, 0.5f);
  const int32_t id = queue.Race({40, 120, 20}, {200, 120, 20}, Short());
  CHECK(id > 0);
  TakenPlan taken;
  // planned, but handed over only after the delay
  CHECK(!queue.Ready(id));
  CHECK(!queue.Take(id, taken));
  queue.Advance(0.3f);
  CHECK(!queue.Ready(id));
  queue.Advance(0.3f);
  CHECK(queue.Ready(id));
  CHECK(queue.Take(id, taken));
  CHECK(taken.plan.ok);
  CHECK(!taken.plan.points.empty());
  // once
  CHECK(!queue.Take(id, taken));
  CHECK(queue.Pending() == 0);
  // unknown
  CHECK(!queue.Ready(999));
}

TEST(plans_queue_route_round_tower) {
  PlanQueue queue(Fixture(), false);
  const int32_t id = queue.Race({40, 120, 20}, {200, 120, 20}, Short());
  TakenPlan taken;
  CHECK(queue.Take(id, taken));
  CHECK(taken.plan.ok);
  const auto &pts = taken.plan.points;
  CHECK(pts.size() >= 2);
  // from the start to the goal, every point free, over or round the tower
  CHECK_NEAR(pts.front().x, 40.0, 3.0);
  CHECK_NEAR(pts.back().x, 200.0, 3.0);
  for (size_t i = 0; i < pts.size(); ++i) {
    CHECK(Fixture().IsFree(pts[i].x, pts[i].y, pts[i].z));
  }
}

TEST(plans_queue_drop) {
  PlanQueue queue(Fixture(), false);
  const int32_t a = queue.Race({40, 120, 20}, {60, 120, 20}, Short());
  const int32_t b = queue.Race({40, 120, 20}, {60, 140, 20}, Short());
  CHECK(a != b);
  CHECK(queue.Pending() == 2);
  queue.Drop(a);
  TakenPlan taken;
  CHECK(!queue.Take(a, taken));
  CHECK(queue.Pending() == 1);
  CHECK(queue.Take(b, taken));
  CHECK(queue.Pending() == 0);
}

TEST(plans_queue_threaded) {
  // as in the game: the queue outlives its threads
  static PlanQueue queue(Fixture(), true);
  const int32_t id = queue.Race({40, 120, 20}, {200, 120, 20}, Short());
  CHECK(WaitReady(queue, id));
  TakenPlan taken;
  CHECK(queue.Take(id, taken));
  CHECK(taken.plan.ok);
  // dropped while it plans: freed when it finishes, never handed over
  const int32_t dropped = queue.Race({40, 120, 20}, {200, 130, 20}, Short());
  queue.Drop(dropped);
  CHECK(!queue.Take(dropped, taken));
  CHECK(queue.Pending() == 0);
}

TEST(plans_queue_rejoin) {
  PlanQueue queue(Fixture(), false);
  std::vector<Vec3> line;
  for (int i = 0; i <= 40; ++i) {
    line.push_back({20.0f + 5.0f * float(i), 30, 20});
  }
  const int32_t id = queue.Rejoin({30, 40, 20}, line, 40.0f, CarPlanParams(Short(), nullptr, {}, {10, 0, 0}));
  TakenPlan taken;
  CHECK(queue.Take(id, taken));
  CHECK(taken.plan.ok);
  CHECK(!taken.plan.points.empty());
}
