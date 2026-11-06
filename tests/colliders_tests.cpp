// The city's collision offline (tools/flightsim/Colliders, docs/design/hover-fly-terrain.md): its rays, its region
// file, and the Sim's ground and underside contact on it.
#include "check.hpp"

#include "../tools/flightsim/Colliders.hpp"
#include "../tools/flightsim/Sim.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace FlightSim;

namespace {
// a square a side, its top at z
void Square(Colliders &aC, float aX0, float aY0, float aX1, float aY1, float aZ,
            Colliders::Kind aKind = Colliders::Collision) {
  aC.Add({aX0, aY0, aZ}, {aX1, aY0, aZ}, {aX1, aY1, aZ}, aKind);
  aC.Add({aX0, aY0, aZ}, {aX1, aY1, aZ}, {aX0, aY1, aZ}, aKind);
}

// the generic car flown hands off for aSeconds from aStart in the redesign; the last frame
Frame HandsOff(Sim &aSim, Vec3 aStart, float aSeconds) {
  aSim.mode = Mode::Assist;
  aSim.Place(aStart, 0.0f);
  Frame f;
  for (int i = 0; i < int(aSeconds * 60.0f); ++i)
    f = aSim.Step({}, 1.0f / 60.0f);
  return f;
}
} // namespace

TEST(colliders_ray_hits_floor) {
  Colliders c;
  Square(c, -50, -50, 50, 50, 2.0f);
  c.Build();
  Colliders::Hit h;
  CHECK(c.Ray({3, 4, 10}, {3, 4, -10}, h));
  CHECK_NEAR(h.position.z, 2.0f, 1e-4f);
  CHECK_NEAR(h.t, 0.4f, 1e-5f);
  CHECK_NEAR(h.normal.z, 1.0f, 1e-5f);
  // from below the normal faces down: both faces are hit
  CHECK(c.Ray({3, 4, -10}, {3, 4, 10}, h));
  CHECK_NEAR(h.normal.z, -1.0f, 1e-5f);
  // short of it, and beside it
  CHECK(!c.Ray({3, 4, 10}, {3, 4, 3}, h));
  CHECK(!c.Ray({60, 4, 10}, {60, 4, -10}, h));
  // slanted, across many squares
  CHECK(c.Ray({-40, -40, 10}, {40, 40, -6}, h));
  CHECK_NEAR(h.position.z, 2.0f, 1e-3f);
  CHECK_NEAR(h.position.x, 0.0f, 1e-2f);
}

TEST(colliders_ray_first_hit_under_overpass) {
  // a road at 0 and a deck at 8 over the middle: a ray from under the deck finds the road, one from above the deck
  Colliders c;
  Square(c, -50, -50, 50, 50, 0.0f, Colliders::Terrain);
  Square(c, -10, -50, 10, 50, 8.0f);
  c.Build();
  Colliders::Hit h;
  CHECK(c.Ray({0, 0, 4}, {0, 0, -10}, h));
  CHECK_NEAR(h.position.z, 0.0f, 1e-4f);
  CHECK(h.kind == Colliders::Terrain);
  CHECK(c.Ray({0, 0, 20}, {0, 0, -10}, h));
  CHECK_NEAR(h.position.z, 8.0f, 1e-4f);
  CHECK(h.kind == Colliders::Collision);
}

TEST(colliders_region_file_round_trip) {
  // the layout region_pack.py writes
  auto path = std::filesystem::temp_directory_path() / "ltbf_colliders_test.ltbfregion";
  {
    std::ofstream f(path, std::ios::binary);
    uint32_t version = 1, n = 1;
    float bounds[6] = {0, 0, 1, 10, 10, 1};
    float tri[9] = {0, 0, 1, 10, 0, 1, 0, 10, 1};
    uint8_t kind = Colliders::Proxy;
    f.write("LTBFREG1", 8);
    f.write(reinterpret_cast<const char *>(&version), 4);
    f.write(reinterpret_cast<const char *>(&n), 4);
    f.write(reinterpret_cast<const char *>(bounds), sizeof(bounds));
    f.write(reinterpret_cast<const char *>(tri), sizeof(tri));
    f.write(reinterpret_cast<const char *>(&kind), 1);
  }
  Colliders c;
  std::string error;
  CHECK(c.Load(path.string(), &error));
  CHECK(c.Size() == 1);
  Colliders::Hit h;
  CHECK(c.Ray({2, 2, 5}, {2, 2, -5}, h));
  CHECK_NEAR(h.position.z, 1.0f, 1e-5f);
  CHECK(h.kind == Colliders::Proxy);
  std::filesystem::remove(path);
  Colliders missing;
  CHECK(!missing.Load(path.string(), &error));
  CHECK(!error.empty());
}

TEST(colliders_sim_flat_matches_height_field) {
  // a floor of triangles flies as the height field does: the same height to the centimetre
  Colliders c;
  Square(c, -200, -200, 200, 200, 0.0f);
  c.Build();
  Sim onField(Car{}, Flat(0.0f));
  Sim onColliders(Car{}, Flat(-1e9f));
  onColliders.groundRay = RayOn(c);
  Frame a = HandsOff(onField, {0, 0, 3}, 6.0f);
  Frame b = HandsOff(onColliders, {0, 0, 3}, 6.0f);
  CHECK(b.foundGround);
  CHECK(!b.contact);
  CHECK_NEAR(b.position.z, a.position.z, 0.01f);
  CHECK_NEAR(b.groundDistance, a.groundDistance, 0.01f);
}

TEST(colliders_sim_ignores_deck_overhead) {
  // under an overpass the rays find the road below, as on open road
  Colliders open, under;
  Square(open, -200, -200, 200, 200, 0.0f);
  Square(under, -200, -200, 200, 200, 0.0f);
  Square(under, -20, -200, 20, 200, 6.0f);
  open.Build();
  under.Build();
  Sim a(Car{}, Flat(-1e9f)), b(Car{}, Flat(-1e9f));
  a.groundRay = RayOn(open);
  b.groundRay = RayOn(under);
  Frame fa = HandsOff(a, {0, 0, 3}, 6.0f);
  Frame fb = HandsOff(b, {0, 0, 3}, 6.0f);
  CHECK_NEAR(fb.position.z, fa.position.z, 0.01f);
  CHECK(!fb.contact);
}

TEST(colliders_sim_underside_corner_on_ledge) {
  // a ledge 0.5 m up under the front left corner only: the car resting 0.2 m over the floor is pushed up by it, where
  // the height field under its centre would have missed it
  Car car;
  Colliders c;
  Square(c, -200, -200, 200, 200, 0.0f);
  Square(c, -5, car.wheelY - 0.2f, -car.wheelX + 0.2f, 5, 0.5f);
  c.Build();
  Sim sim(car, Flat(-1e9f));
  sim.groundRay = RayOn(c);
  sim.mode = Mode::Assist;
  float z0 = 0.2f - car.bottom;
  sim.Place({0, 0, z0}, 0.0f);
  Frame f = sim.Step({}, 1.0f / 60.0f);
  CHECK(f.contact);
  CHECK(f.position.z > z0 + 0.25f);
}

TEST(colliders_underside_parse) {
  // squares row by row along Y, each at its centre, the empty one left out
  std::vector<Vec3> p = ParseUnderside("0.5,-1,-2,2,2:-0.4,,-0.5,-0.3");
  CHECK(p.size() == 3);
  CHECK_NEAR(p[0].x, -0.75f, 1e-5f);
  CHECK_NEAR(p[0].y, -1.75f, 1e-5f);
  CHECK_NEAR(p[0].z, -0.4f, 1e-5f);
  CHECK_NEAR(p[1].x, -0.75f, 1e-5f);
  CHECK_NEAR(p[1].y, -1.25f, 1e-5f);
  CHECK_NEAR(p[2].x, -0.25f, 1e-5f);
  CHECK(ParseUnderside("").empty());
  CHECK(ParseUnderside("0.5,-1:-0.4").empty());
}

TEST(colliders_sim_underside_front_overhang) {
  // a block 0.4 m high just past the front wheels: the tyres and the centre miss it, the chassis's front overhang
  // (a flat plate 0.2 m over the tyres' bottom, 0.9 m past the front wheels) does not
  Car plain;
  plain.wheelX = 0.8f;
  plain.wheelY = 1.5f;
  plain.bottom = -0.6f;
  Car shaped = plain;
  for (float x = -0.75f; x < 0.8f; x += 0.5f)
    for (float y = -2.25f; y < 2.4f; y += 0.5f)
      shaped.underside.push_back({x, y, -0.4f});
  Colliders c;
  Square(c, -200, -200, 200, 200, 0.0f);
  Square(c, -5, 1.9f, 5, 2.4f, 0.4f);
  c.Build();
  float z0 = 0.1f - plain.bottom; // the tyres 0.1 m over the floor, the plate 0.3 m
  auto first = [&](const Car &aCar) {
    Sim sim(aCar, Flat(-1e9f));
    sim.groundRay = RayOn(c);
    sim.mode = Mode::Assist;
    sim.Place({0, 0, z0}, 0.0f);
    return sim.Step({}, 1.0f / 60.0f);
  };
  CHECK(!first(plain).contact);
  Frame f = first(shaped);
  CHECK(f.contact);
  CHECK(f.position.z > z0 + 0.05f);
}

TEST(colliders_fleet_undersides) {
  // every car with a chassis shape has its underside in vehicles.tsv (aero.py, tweak_physics.py --aero), on the body
  // (the Bratsk with its trailer reaches 10 m back, its cab nose's underside 1.7 m up)
  std::vector<Car> cars = LoadCars(LTBF_SOURCE_DIR "/tools/flightsim/vehicles.tsv");
  CHECK(cars.size() > 250);
  size_t with = 0;
  for (const Car &car : cars) {
    with += !car.underside.empty();
    for (Vec3 p : car.underside)
      CHECK(std::fabs(p.x) < 3.0f && std::fabs(p.y) < 12.0f && p.z > -2.0f && p.z < 2.5f);
  }
  CHECK(with + 3 >= cars.size());
}
