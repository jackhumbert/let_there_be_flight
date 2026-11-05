// The drag area from the collision shape (src/red4ext/Flight/Aero) and the developer settings that turn it on
// (Flight/DeveloperSettings), and the shape's drag in the force tail (Flight/Force).
#include "check.hpp"

#include "Flight/Aero.hpp"
#include "Flight/DeveloperSettings.hpp"
#include "Flight/Force.hpp"

#include <cmath>

using namespace Flight;

namespace {

ShapePiece Box(Vec3 aCentre, Vec3 aHalf) {
  ShapePiece out;
  for (float x : {-1.0f, 1.0f})
    for (float y : {-1.0f, 1.0f})
      for (float z : {-1.0f, 1.0f})
        out.push_back({aCentre.x + x * aHalf.x, aCentre.y + y * aHalf.y, aCentre.z + z * aHalf.z});
  return out;
}

} // namespace

TEST(aero_box_areas) {
  // a 2 x 4.6 x 1.4 m box: its faces, to the 2 cm raster's cells
  const Vec3 a = SilhouetteAreas({Box({0.0f, 0.0f, 0.7f}, {1.0f, 2.3f, 0.7f})});
  CHECK_NEAR(a.x, 4.6f * 1.4f, 0.1f);
  CHECK_NEAR(a.y, 2.0f * 1.4f, 0.06f);
  CHECK_NEAR(a.z, 2.0f * 4.6f, 0.12f);
  CHECK_NEAR(ShapeHeight({Box({0.0f, 0.0f, 0.7f}, {1.0f, 2.3f, 0.7f})}), 1.4f, 1e-5f);
}

TEST(aero_union_not_sum) {
  // two boxes overlapping by half seen head-on count once where they overlap, and a gap between two counts as nothing
  const ShapePiece a = Box({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}), b = Box({1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
  CHECK_NEAR(SilhouetteAreas({a, b}).y, 3.0f * 2.0f, 0.1f);
  const ShapePiece c = Box({3.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
  CHECK_NEAR(SilhouetteAreas({a, c}).y, 2.0f * 4.0f, 0.1f);
}

TEST(aero_rotated_and_round) {
  // a unit square turned 45 deg about Y still shows 1 m2 head-on; a sphere of radius 0.5 shows about pi r2
  ShapePiece square;
  const float s = std::sqrt(0.5f);
  for (Vec3 p : {Vec3{s, 0.0f, 0.0f}, Vec3{0.0f, 0.0f, s}, Vec3{-s, 0.0f, 0.0f}, Vec3{0.0f, 0.0f, -s}})
    square.push_back(p);
  CHECK_NEAR(SilhouetteAreas({square}).y, 1.0f, 0.04f);
  CHECK_NEAR(SilhouetteAreas({RoundPiece({{}}, 0.5f)}).y, 3.14159f * 0.25f, 0.03f);
  // fewer than 3 points add nothing
  CHECK(SilhouetteAreas({{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}}}).y == 0.0f);
}

TEST(aero_drag_area_by_height) {
  // head-on the coefficient rises with the height (a coupe 0.30 to a truck 0.70), side-on and top-down 1.0 (1.1 for a
  // truck); a bike adds its rider
  const Vec3 areas{8.0f, 2.0f, 9.0f};
  const Vec3 coupe = DragAreaOf(areas, 1.2f, false), suv = DragAreaOf(areas, 1.8f, false);
  const Vec3 truck = DragAreaOf(areas, 3.0f, false), bike = DragAreaOf(areas, 1.0f, true);
  CHECK_NEAR(coupe.y, 0.60f, 1e-5f);
  CHECK_NEAR(coupe.x, 8.0f, 1e-5f);
  CHECK_NEAR(suv.y, 0.84f, 1e-5f);
  CHECK_NEAR(truck.y, 1.40f, 1e-5f);
  CHECK_NEAR(truck.z, 9.9f, 1e-4f);
  CHECK_NEAR(DragAreaOf(areas, 1.3f, false).y, 2.0f * 0.315f, 1e-5f);
  CHECK_NEAR(bike.y, 0.6f * 2.0f + 0.35f, 1e-5f);
  CHECK_NEAR(bike.x, 8.6f, 1e-5f);
}

TEST(aero_force_tail_shape_drag) {
  // with shapeDrag and a drag area ApplyPhysics' drag is ShapeDragAccel's on each body axis; without either, today's
  // airResistanceFactor drag along the velocity
  ForceParams p;
  StatsStatic st;
  st.mass = 1500.0f;
  st.airResistanceFactor = 2.0f;
  st.dragArea = {6.0f, 2.0f, 9.0f};
  StatsState s;
  s.d_localVelocity = {5.0f, 40.0f, 0.0f};
  s.d_speed = Length(s.d_localVelocity);
  s.d_forward = {0.0f, 1.0f, 0.0f};
  s.d_up = {0.0f, 0.0f, 1.0f};
  s.d_right = {1.0f, 0.0f, 0.0f};
  s.d_direction = Normalize(s.d_localVelocity);
  s.d_localDirection = s.d_direction;
  ForceInput in;
  ForceOutput today, shaped;
  ApplyPhysics(p, st, s, in, today);
  p.shapeDrag = true;
  ApplyPhysics(p, st, s, in, shaped);
  const Vec3 aero = ShapeDragAccel(s.d_localVelocity, Mul(st.dragArea, 0.5f * 1.2f * kShapeDragK), st.mass);
  const Vec3 old = Mul(s.d_localVelocity, -s.d_speed * 1.2f * 2.0f / st.mass);
  // the rest of the physics force (the directionality terms) is the same in both
  const Vec3 rest = Sub(today.physicsForce, old);
  CHECK_NEAR(shaped.physicsForce.x, aero.x + rest.x, 1e-4f);
  CHECK_NEAR(shaped.physicsForce.y, aero.y + rest.y, 1e-4f);
  st.dragArea = {};
  ForceOutput none;
  ApplyPhysics(p, st, s, in, none);
  CHECK_NEAR(none.physicsForce.y, today.physicsForce.y, 1e-6f);
}

TEST(developer_settings_parse) {
  DeveloperSettings d;
  const auto messages = LoadDeveloperSettings(d, "; comment\n[section]\n\nshapeDrag = on\n# another\n"
                                                 "thrustStrength=2\nnoSuchSetting = 1\ndragK = abc\nthrustPerThruster\n");
  CHECK(d.shapeDrag);
  CHECK_NEAR(d.thrustStrength, 1.0f, 0.0f); // clamped
  CHECK_NEAR(d.dragK, kShapeDragK, 0.0f);   // refused
  CHECK(messages.size() == 4);              // clamped, unknown, not a number, not name = value
  CHECK(messages[0].rfind("line 6:", 0) == 0);
  const auto changed = ChangedDeveloperSettings(d);
  CHECK(changed.size() == 2);
  CHECK(changed[0] == "shapeDrag = true");
  std::string value, message;
  CHECK(!SetDeveloperSetting(d, "shapeDrag", "maybe", message));
  CHECK(d.shapeDrag);
  CHECK(SetDeveloperSetting(d, " dragK ", "5", message) && message.empty());
  CHECK(GetDeveloperSetting(d, "dragK", value) && value == "5");
  CHECK(!GetDeveloperSetting(d, "nope", value));
  // the defaults change nothing in flight
  const DeveloperSettings defaults;
  CHECK(!defaults.shapeDrag && defaults.thrustStrength == 0.0f);
  CHECK(ThrustScale(4.0f, 1500.0f, defaults.thrustPerThruster, 15.0f, defaults.thrustStrength) == 1.0f);
}
