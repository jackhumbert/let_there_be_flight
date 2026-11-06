#include "Axes.hpp"

#include <cmath>

namespace Wwise {

namespace {

struct V {
  double x, y, z;
};
double Dot(V a, V b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
V Minus(V a, V b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}
V Scale(V a, double s) {
  return {a.x * s, a.y * s, a.z * s};
}
// a's unit vector, or false when it is too short to have one
bool Unit(V &a) {
  const double n = std::sqrt(Dot(a, a));
  if (!(n > 1e-6)) {
    return false;
  }
  a = Scale(a, 1.0 / n);
  return true;
}
V Swap(const GameVector &g) {
  return {g.x, g.z, g.y};
}

} // namespace

AkWorldTransform ToWwise(const GameVector &aPosition, const GameVector &aForward, const GameVector &aUp) {
  V front = Swap(aForward);
  if (!Unit(front)) {
    front = {0, 0, 1};
  }
  // a candidate up less its part along the facing, if anything is left of it
  auto square = [&](V aCandidate, V &aTop) {
    aTop = Minus(aCandidate, Scale(front, Dot(aCandidate, front)));
    return Unit(aTop);
  };
  V top;
  if (!square(Swap(aUp), top) && !square({0, 1, 0}, top)) {
    // facing straight up or down, with no up given: the game's +y
    square({0, 0, 1}, top);
  }
  AkWorldTransform t{};
  t.front = {float(front.x), float(front.y), float(front.z)};
  t.top = {float(top.x), float(top.y), float(top.z)};
  t.position = {aPosition.x, aPosition.z, aPosition.y};
  return t;
}

} // namespace Wwise
