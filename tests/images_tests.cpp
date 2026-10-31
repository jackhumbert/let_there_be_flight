// Image sources and the reflections streams (src/red4ext/Audio/Acoustics/Images, Reflections::ProcessStreams) and the
// ring between the mixer's DSPs (Voices/Ring.hpp).
#include "check.hpp"

#include "Audio/Acoustics/Images.hpp"
#include "Audio/Acoustics/Reflections.hpp"
#include "Audio/Voices/Ring.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace Acoustics;

namespace {

void CheckNear(const Vec3 &aActual, const Vec3 &aExpected, float aTolerance, int aLine) {
  if (std::fabs(aActual.x - aExpected.x) > aTolerance || std::fabs(aActual.y - aExpected.y) > aTolerance ||
      std::fabs(aActual.z - aExpected.z) > aTolerance) {
    char text[200];
    std::snprintf(text, sizeof(text), "image (%g, %g, %g) against (%g, %g, %g)", aActual.x, aActual.y, aActual.z,
                  aExpected.x, aExpected.y, aExpected.z);
    Check::Fail(__FILE__, aLine, text);
  }
}

void CheckEcho(const Echo &aActual, const Echo &aExpected, int aLine) {
  CHECK_NEAR(aActual.delay, aExpected.delay, 1e-6);
  CHECK_NEAR(aActual.rate, aExpected.rate, 1e-6);
  CHECK_NEAR(aActual.gain, aExpected.gain, 1e-6);
  (void)aLine;
}

} // namespace

TEST(images_floor) {
  const Vec3 source{100, 200, 50};
  const Plane floor{{100, 200, 40}, {0, 0, 1}};
  CheckNear(Mirror(source, floor), {100, 200, 30}, 1e-3f, __LINE__);
}

TEST(images_ceiling) {
  const Vec3 source{100, 200, 50};
  const Plane ceiling{{100, 200, 65}, {0, 0, -1}};
  CheckNear(Mirror(source, ceiling), {100, 200, 80}, 1e-3f, __LINE__);
}

TEST(images_walls) {
  const Vec3 source{100, 200, 50};
  // rightDir (1, 0, 0): the left wall is outward (-1, 0, 0), 7 m away; the right wall 12 m
  CheckNear(Mirror(source, SquareOn(source, {-1, 0, 0}, 7)), {86, 200, 50}, 1e-3f, __LINE__);
  CheckNear(Mirror(source, SquareOn(source, {1, 0, 0}, 12)), {124, 200, 50}, 1e-3f, __LINE__);
}

TEST(images_angled_wall) {
  const float c = std::cos(20.0f * 3.14159265f / 180.0f), s = std::sin(20.0f * 3.14159265f / 180.0f);
  const Vec3 source{0, 0, 0}, listener{0, -3, 1};
  const Plane wall{{8, 0, 0}, {-c, -s, 0}};
  // the wall seen at an angle: its image is twice the source's distance to the plane away
  const float d = (source.x - wall.point.x) * wall.normal.x + (source.y - wall.point.y) * wall.normal.y +
                  (source.z - wall.point.z) * wall.normal.z;
  const Vec3 image = Mirror(source, wall);
  CHECK_NEAR(std::sqrt(image.x * image.x + image.y * image.y + image.z * image.z), 2 * d, 1e-4);
  const Tuning tuning;
  const Echo e = EchoFor(wall, source, listener, {}, 1.0f, tuning, true);
  const double via = std::sqrt(double(listener.x - image.x) * (listener.x - image.x) + double(listener.y - image.y) * (listener.y - image.y) +
                               double(listener.z - image.z) * (listener.z - image.z));
  const double direct = std::sqrt(9.0 + 1.0);
  CHECK_NEAR(e.delay, (via - direct) / 343.0, 1e-7);
}

TEST(images_targets_use_images) {
  const Vec3 source{100, 200, 50};
  const Vec3 listener{source.x, source.y - 4, source.z + 1.5f};
  const Vec3 velocity{0, 30, 0};
  Features f{};
  f.floor = 10;
  f.ceiling = 15;
  f.left = 7;
  f.right = 12;
  f.rightDir = {1, 0, 0};
  f.enclosure = 0.5f;
  f.meanFreePath = 20;
  f.nearDistance.fill(std::numeric_limits<float>::infinity());
  f.nearReflectivity.fill(-1.0f);
  const Tuning tuning;
  const Params p = Targets(f, source, listener, tuning, velocity);

  CheckEcho(p.floor, EchoFor(SquareOn(source, {0, 0, -1}, 10), source, listener, velocity, tuning.groundReflectivity, tuning, true), __LINE__);
  CheckEcho(p.ceiling, EchoFor(SquareOn(source, {0, 0, 1}, 15), source, listener, velocity, tuning.wallReflectivity, tuning, true), __LINE__);
  CheckEcho(p.left, EchoFor(SquareOn(source, {-1, 0, 0}, 7), source, listener, velocity, tuning.wallReflectivity, tuning, true), __LINE__);
  CheckEcho(p.right, EchoFor(SquareOn(source, {1, 0, 0}, 12), source, listener, velocity, tuning.wallReflectivity, tuning, true), __LINE__);

  CheckNear(p.floorImage, Mirror(source, SquareOn(source, {0, 0, -1}, 10)), 1e-3f, __LINE__);
  CheckNear(p.ceilingImage, Mirror(source, SquareOn(source, {0, 0, 1}, 15)), 1e-3f, __LINE__);
  CheckNear(p.leftImage, Mirror(source, SquareOn(source, {-1, 0, 0}, 7)), 1e-3f, __LINE__);
  CheckNear(p.rightImage, Mirror(source, SquareOn(source, {1, 0, 0}, 12)), 1e-3f, __LINE__);
  // no scatter hits: both sides' images are the source
  CheckNear(p.scatterRightImage, source, 1e-3f, __LINE__);
  CheckNear(p.scatterLeftImage, source, 1e-3f, __LINE__);
}

TEST(images_streams_match_reflections) {
  constexpr uint32_t kFrames = 4096, kImpulse = 100;
  Params params;
  params.floor = {0.02f, 0.0f, 0.5f};
  params.floorFeedback = 0;
  params.wallFeedback = 0;
  params.wet = 0;
  params.air = false;

  std::vector<float> in(kFrames, 0.0f);
  in[kImpulse] = 1.0f;

  Reflections streamsOf(48000, 2400);
  streamsOf.SetTargets(params);
  std::vector<float> s[8];
  for (auto &v : s) {
    v.assign(kFrames, 0.0f);
  }
  streamsOf.ProcessStreams(in.data(), {s[0].data(), s[1].data(), s[2].data(), s[3].data(), s[4].data(), s[5].data(),
                                       s[6].data(), s[7].data()}, kFrames);

  // the floor stream: a smeared impulse 0.02 s (960 samples) after the input
  const auto peak = std::max_element(s[0].begin(), s[0].end(), [](float a, float b) { return std::fabs(a) < std::fabs(b); });
  CHECK(std::abs(int(peak - s[0].begin()) - int(kImpulse + 960)) <= 2);
  float sum = 0;
  for (uint32_t i = 1050; i <= 1070; ++i) {
    sum += s[0][i];
  }
  CHECK_NEAR(sum, 0.5, 0.01);
  for (int k = 1; k < 8; ++k) {
    for (uint32_t i = 0; i < kFrames; ++i) {
      if (std::fabs(s[k][i]) >= 1e-6f) {
        CHECK(std::fabs(s[k][i]) < 1e-6f);
        break;
      }
    }
  }

  // Process() of the same input: the input plus the floor stream
  Reflections processed(48000, 2400);
  processed.SetTargets(params);
  std::vector<float> out(kFrames, 0.0f);
  processed.Process(in.data(), out.data(), kFrames, 1);
  float worst = 0;
  for (uint32_t i = 0; i < kFrames; ++i) {
    worst = std::max(worst, std::fabs(out[i] - in[i] - s[0][i]));
  }
  CHECK(worst < 1e-6f);
}

TEST(images_ring_one_block_behind) {
  Ring ring(4096, 1024);
  std::vector<float> a(1024), b(1024), got(1024, -1.0f);
  for (uint32_t i = 0; i < 1024; ++i) {
    a[i] = float(i) + 1.0f;
    b[i] = -float(i) - 1.0f;
  }
  ring.Write(a.data(), 1024);
  ring.Read(got.data(), 1024);
  CHECK(std::all_of(got.begin(), got.end(), [](float x) { return x == 0.0f; }));
  ring.Write(b.data(), 1024);
  ring.Read(got.data(), 1024);
  CHECK(got == a);
  CHECK(ring.Underruns() == 0);
  // B is still in the ring, then nothing is written: zeros, an underrun
  ring.Read(got.data(), 1024);
  CHECK(got == b);
  CHECK(ring.Underruns() == 0);
  ring.Read(got.data(), 1024);
  CHECK(std::all_of(got.begin(), got.end(), [](float x) { return x == 0.0f; }));
  CHECK(ring.Underruns() == 1);
}
