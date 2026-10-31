// The route geometry the follower asks of the Autopilot's route (src/red4ext/Navigation/Route).
#include "check.hpp"

#include "box_space.hpp"
#include "Navigation/Route.hpp"

#include <cmath>

using namespace Navigation;

namespace {
// 100 m east, then 100 m north
Route Corner() { return Route::Make({{0, 0, 0}, {100, 0, 0}, {100, 100, 0}}); }

void CheckVec(const Vec3 &aA, float aX, float aY, float aZ) {
  CHECK_NEAR(aA.x, aX, 1e-4);
  CHECK_NEAR(aA.y, aY, 1e-4);
  CHECK_NEAR(aA.z, aZ, 1e-4);
}
} // namespace

TEST(route_at) {
  Route r = Corner();
  CheckVec(r.At(150), 100, 50, 0);
  CheckVec(r.At(-5), 0, 0, 0);
  CheckVec(r.At(500), 100, 100, 0);
  CHECK_NEAR(r.Length(), 200, 1e-4);
}

TEST(route_ahead) {
  Route r = Corner();
  CheckVec(r.Ahead(0, {10, 0, 0}, 100), 100, 10, 0);
  CheckVec(r.Ahead(0, {10, 0, 0}, 1000), 100, 100, 0);
}

TEST(route_nearest_project) {
  Route r = Corner();
  CHECK(r.Nearest({50, 5, 0}, 0, 12) == 0);
  CheckVec(r.Project({50, 5, 0}, 0), 50, 0, 0);
  CHECK(r.Nearest({105, 60, 0}, 0, 12) == 1);
}

TEST(route_tangent_bend) {
  Route r = Corner();
  CheckVec(r.Tangent(50, 10), 1, 0, 0);
  // legs (10, 0) and (0, 10): (bc / 10 - ab / 10) * 2 / 20
  CheckVec(r.Bend(100, 10), -0.1f, 0.1f, 0);
}

TEST(route_sharp_bend) {
  Route r = Corner();
  // the directions 15 m either side of the corner are at right angles, past the 25 degree limit
  CHECK_NEAR(r.SharpBend(0, {0, 0, 0}, 15), 100, 1e-4);
}

TEST(route_turn_sign) {
  Route r = Corner();
  // a left turn (anticlockwise from above) is negative: positive is right
  CHECK_NEAR(r.Turn(0, {0, 0, 0}, 100), -(3.14159265358979 / 2) / 50, 1e-4);
}

TEST(route_clear_ahead) {
  Route r = Corner();
  BoxSpace space;
  space.boxes.push_back({{50, -1, -1}, {60, 1, 1}});
  CHECK(r.ClearAhead(0, {0, 0, 0}, 40, space));
  CHECK(!r.ClearAhead(0, {0, 0, 0}, 55, space));
}

TEST(route_empty) {
  Route r = Route::Make({{1, 2, 3}});
  CHECK(r.Size() == 0);
  CheckVec(r.At(10), 0, 0, 0);
  CheckVec(r.Tangent(10, 5), 0, 0, 0);
  CHECK_NEAR(r.DistanceTo({0, 0, 0}, 0, 10), 1e9, 1e3);
}

TEST(route_curvature) {
  Route r = Corner();
  // points 20, 80 and 140 m on: (20, 0), (80, 0), (100, 40); legs 60 and sqrt(2000), towards the corner's inside
  const float l2 = std::sqrt(2000.0f), k = 2.0f / (60.0f + l2);
  CheckVec(r.Curvature(0, {0, 0, 0}, 200), (20.0f / l2 - 1.0f) * k, (40.0f / l2) * k, 0);
}

TEST(route_sharp_bend_none) {
  // straight, a gentle bend (11 degrees, under the 25 degree limit), and a corner past the 300 m window
  CHECK_NEAR(Route::Make({{0, 0, 0}, {100, 0, 0}, {200, 0, 0}}).SharpBend(0, {0, 0, 0}, 15), 1e6, 1);
  CHECK_NEAR(Route::Make({{0, 0, 0}, {100, 0, 0}, {200, 20, 0}}).SharpBend(0, {0, 0, 0}, 15), 1e6, 1);
  Route far = Route::Make({{0, 0, 0}, {400, 0, 0}, {400, 100, 0}});
  CHECK_NEAR(far.SharpBend(0, {0, 0, 0}, 15), 1e6, 1);
  // 250 m from the car: inside the window, measured from the car
  CHECK_NEAR(far.SharpBend(0, {150, 0, 0}, 15), 250, 1e-3);
}

TEST(route_nearest_window) {
  // a U: east, north, west. Only segments aFrom to aFrom + aCount are searched
  Route u = Route::Make({{0, 0, 0}, {100, 0, 0}, {100, 100, 0}, {0, 100, 0}});
  CHECK(u.Nearest({50, 95, 0}, 0, 5) == 2);
  CHECK(u.Nearest({50, 95, 0}, 0, 0) == 0);
  CHECK(u.Nearest({50, 5, 0}, 1, 5) == 1);
}

TEST(route_distance_to) {
  Route r = Corner();
  // 50 m ahead covers the first leg only; 200 m reaches the second, 5 m away
  CHECK_NEAR(r.DistanceTo({105, 60, 0}, 0, 50), std::sqrt(25.0f + 3600.0f), 1e-3);
  CHECK_NEAR(r.DistanceTo({105, 60, 0}, 0, 200), 5, 1e-4);
}

TEST(route_project_clamped) {
  // a segment past the end is the last one
  CheckVec(Corner().Project({150, 150, 0}, 5), 100, 100, 0);
}

TEST(route_clear_ahead_corner) {
  Route r = Corner();
  BoxSpace space;
  space.boxes.push_back({{99, 30, -1}, {101, 40, 1}});
  // 120 m reaches 20 m up the second leg, short of the box; 150 m reaches 50 m, through it
  CHECK(r.ClearAhead(0, {0, 0, 0}, 120, space));
  CHECK(!r.ClearAhead(0, {0, 0, 0}, 150, space));
}
