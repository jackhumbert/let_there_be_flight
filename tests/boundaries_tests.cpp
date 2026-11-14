// World boundaries (src/red4ext/Navigation/Boundaries): the gates, the areas in force, the mask they lay over
// the nav map, and the planner, free space and doorway layer keeping out of it.
#include "check.hpp"

#include "nav_fixture.hpp"
#include "Navigation/Boundaries.hpp"
#include "Navigation/Doorways.hpp"
#include "Navigation/Planner.hpp"
#include "Navigation/World.hpp"

#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>

using namespace Navigation;

namespace {
// 256 x 256 m from the origin, floor at 0, nothing standing (the car's radius 2 m)
NavMap &Fixture() {
  static NavMap map;
  static bool loaded = [] {
    NavFixture::Spec spec;
    return map.Load(NavFixture::Make(spec), "fixture");
  }();
  CHECK(loaded);
  return map;
}

// a wall from x 120 to 130 across y 0 .. 200, always there; a box gated on a fact; one on another fact
const char *kAreas = "# a comment\n"
                     "wall\t-\t0\t300\t120\t0\t130\t0\t130\t200\t120\t200\n"
                     "lock\twatson_prolog_lock>0\t0\t300\t40\t40\t60\t40\t60\t60\t40\t60\n"
                     "port\tep1_installed==0\t0\t300\t200\t200\t220\t200\t220\t220\t200\t220\n";

FactReader Facts(std::unordered_map<std::string, int32_t> aValues) {
  return [aValues](const std::string &aFact) {
    auto it = aValues.find(aFact);
    return it == aValues.end() ? 0 : it->second;
  };
}

bool Blocked(const NavMap &aMap, float aX, float aY) { return aMap.GetGaps(aX, aY).empty(); }
} // namespace

TEST(boundaries_gate_parse) {
  BoundaryGate g;
  CHECK(BoundaryGate::Parse("-", g) && g.op == BoundaryGate::Op::Always && g.Holds(123));
  CHECK(BoundaryGate::Parse("watson_prolog_lock>0", g) && g.fact == "watson_prolog_lock" && g.value == 0);
  CHECK(g.Holds(1) && !g.Holds(0));
  CHECK(BoundaryGate::Parse("ep1_installed==0", g) && g.op == BoundaryGate::Op::Equal && g.Holds(0) && !g.Holds(1));
  CHECK(BoundaryGate::Parse("a>=2", g) && g.op == BoundaryGate::Op::GreaterEqual && g.Holds(2) && !g.Holds(1));
  CHECK(BoundaryGate::Parse("a<=-1", g) && g.value == -1 && g.Holds(-1) && !g.Holds(0));
  CHECK(BoundaryGate::Parse("a!=3", g) && g.Holds(2) && !g.Holds(3));
  CHECK(BoundaryGate::Parse("a<3", g) && g.Holds(2) && !g.Holds(3));
  // no fact, no number, junk after it, no operator
  CHECK(!BoundaryGate::Parse(">1", g));
  CHECK(!BoundaryGate::Parse("a>x", g));
  CHECK(!BoundaryGate::Parse("a>1x", g));
  CHECK(!BoundaryGate::Parse("a", g));
}

TEST(boundaries_parse_and_in_force) {
  Boundaries b;
  CHECK(b.Parse(std::string(kAreas) + "short\t-\t0\t1\t0\t0\n", "test"));
  CHECK(b.All().size() == 3); // the short line is skipped
  CHECK(b.All()[0].name == "wall" && b.All()[0].outline.size() == 4);
  CHECK_NEAR(b.All()[0].outline[2].second, 200.0, 1e-4);
  const auto facts = b.Facts();
  CHECK(facts.size() == 2 && facts[0] == "watson_prolog_lock" && facts[1] == "ep1_installed");
  // a new game before the prologue's lockdown, without Phantom Liberty
  auto in = b.InForce(Facts({}));
  CHECK(in[0] && !in[1] && in[2]);
  // Watson locked down, Phantom Liberty installed
  in = b.InForce(Facts({{"watson_prolog_lock", 1}, {"ep1_installed", 1}}));
  CHECK(in[0] && in[1] && !in[2]);
}

TEST(boundaries_mask_blocks_columns) {
  NavMap &map = Fixture();
  Boundaries b;
  b.Parse(kAreas, "test");
  CHECK(!Blocked(map, 125, 100));
  CHECK(b.Apply(map, Facts({{"ep1_installed", 1}})));
  CHECK(b.InForceCount() == 1);
  // inside the wall, and within the car's radius (2 m) of it, at every height; free past that
  CHECK(Blocked(map, 125, 100));
  CHECK(Blocked(map, 119, 100));  // the column centre 1 m short of the wall
  CHECK(!Blocked(map, 117, 100)); // 3 m short
  CHECK(!Blocked(map, 125, 205)); // past its end (the column at 205 is 5 m off)
  CHECK(!map.IsFree(125, 100, 50));
  CHECK(!map.IsFree(125, 100, 1000));
  CHECK(map.IsFree(100, 100, 50));
  CHECK(map.SegmentClear(100, 100, 10, 150, 100, 10) < 0.5f);
  CHECK(map.CrossesMask(100, 100, 150, 100));
  CHECK(!map.CrossesMask(100, 100, 110, 150));
  // a view reads the mask it was made under, with its key
  NavMap::View view(map);
  const uint16_t *gaps;
  bool hasData = false;
  int32_t cx, cy;
  map.CellOf(125, 100, cx, cy);
  CHECK(view.Column(cx, cy, gaps, &hasData) == 0 && hasData);
  CHECK(view.MaskKey() == map.Mask()->key && view.MaskKey() != 0);
  map.SetMask(nullptr);
}

TEST(boundaries_mask_follows_facts) {
  NavMap &map = Fixture();
  Boundaries b;
  b.Parse(kAreas, "test");
  CHECK(b.Apply(map, Facts({{"ep1_installed", 1}})));
  const ColumnMask *after = map.Mask();
  CHECK(!Blocked(map, 50, 50));
  // the same facts: the same mask, nothing changes
  CHECK(!b.Apply(map, Facts({{"ep1_installed", 1}})));
  CHECK(map.Mask() == after);
  // the lockdown starts: its box blocks too, under another key
  CHECK(b.Apply(map, Facts({{"ep1_installed", 1}, {"watson_prolog_lock", 1}})));
  CHECK(Blocked(map, 50, 50) && map.Mask()->key != after->key);
  // and ends: the first mask again, the very same one (kept for the session)
  CHECK(b.Apply(map, Facts({{"ep1_installed", 1}, {"watson_prolog_lock", 0}})));
  CHECK(map.Mask() == after && !Blocked(map, 50, 50));
  // a view taken before a change keeps the mask it took
  NavMap::View old(map);
  b.Apply(map, Facts({{"ep1_installed", 1}, {"watson_prolog_lock", 1}}));
  CHECK(old.MaskKey() == after->key && old.MaskKey() != map.Mask()->key);
  // nothing in force: no mask
  Boundaries none;
  none.Parse("lock\twatson_prolog_lock>0\t0\t300\t40\t40\t60\t40\t60\t60\t40\t60\n", "test");
  CHECK(none.Apply(map, Facts({})));
  CHECK(map.Mask() == nullptr);
}

TEST(boundaries_planner_goes_round) {
  NavMap &map = Fixture();
  PlanParams params;
  params.line = false;
  const Vec3 start{100, 100, 5}, goal{150, 100, 5};
  // without the wall, the route goes straight across
  map.SetMask(nullptr);
  auto direct = Plan(map, start, goal, params);
  CHECK(direct.ok);
  Boundaries b;
  b.Parse(kAreas, "test");
  b.Apply(map, Facts({{"ep1_installed", 1}}));
  auto round = Plan(map, start, goal, params);
  CHECK(round.ok);
  if (round.ok) {
    // round the wall's end (y 200 and the car's radius)
    float highest = 0;
    for (const auto &p : round.points) {
      highest = std::max(highest, p.y);
      int32_t cx, cy;
      CHECK(!(map.CellOf(p.x, p.y, cx, cy) && map.Mask()->Blocked(cx, cy)));
    }
    CHECK(highest > 200.0f);
    for (size_t i = 1; i < round.points.size(); ++i) {
      const auto &a = round.points[i - 1], &c = round.points[i];
      CHECK(!map.CrossesMask(a.x, a.y, c.x, c.y));
    }
  }
  map.SetMask(nullptr);
}

TEST(boundaries_world_crossing) {
  NavMap &map = Fixture();
  Boundaries b;
  b.Parse(kAreas, "test");
  b.Apply(map, Facts({{"ep1_installed", 1}}));
  std::vector<Helipad> pads;
  std::vector<Gate> gates;
  MapWorld world(map, pads, gates);
  CHECK(world.CrossesBoundary({100, 100, 10}, {150, 100, 10}));
  CHECK(!world.CrossesBoundary({100, 100, 10}, {100, 150, 10}));
  map.SetMask(nullptr);
  CHECK(!world.CrossesBoundary({100, 100, 10}, {150, 100, 10}));
}

TEST(boundaries_doorways_keyed_by_mask) {
  // a set saved under one mask is not taken for another, and one saved before masks only for none
  detail::StepCosts costs;
  detail::Doorways saved(costs, 128, 42);
  const auto bytes = saved.Save();
  detail::Doorways same(costs, 128, 42), other(costs, 128, 7), none(costs, 128, 0);
  CHECK(same.Load(bytes));
  CHECK(!other.Load(bytes));
  CHECK(!none.Load(bytes));
  // version 1 (no mask field): the same bytes without it, read as made with no mask
  auto v1 = detail::Doorways(costs, 128, 0).Save();
  const uint32_t one = 1;
  std::memcpy(v1.data(), &one, 4);
  v1.erase(v1.begin() + 4 + sizeof(costs) + 4, v1.begin() + 4 + sizeof(costs) + 4 + 8);
  detail::Doorways fresh(costs, 128, 0), masked(costs, 128, 42);
  CHECK(fresh.Load(v1));
  CHECK(!masked.Load(v1));
}
