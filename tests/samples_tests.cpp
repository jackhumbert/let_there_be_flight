// The events LTBF's samples play with (src/red4ext/Audio/Samples/Events): the table and its curves.
#include "check.hpp"

#include "Audio/Samples/Events.hpp"

#include <cmath>
#include <cstring>

using namespace Samples;

TEST(samples_table) {
  const Event *on = Find("vehicle3_on");
  CHECK(on != nullptr);
  if (on) {
    CHECK(std::strcmp(on->file, "vehicle3_startup.wav") == 0);
    CHECK(on->bus == Bus::Engine);
    CHECK(!on->loop);
  }
  const Event *destroyed = Find("vehicle3_destroyed");
  CHECK(destroyed != nullptr);
  if (destroyed) {
    CHECK(std::strcmp(destroyed->file, "brake-1.wav") == 0);
    CHECK(destroyed->bus == Bus::Warning);
    CHECK(destroyed->loop);
    CHECK_NEAR(destroyed->levelDb, -4.0f, 1e-4f);
  }
  CHECK(Find("nope") == nullptr);
}

TEST(samples_curve) {
  const Curve c{{{0.0f, -4.5f}, {0.1f, 0.0f}}};
  CHECK_NEAR(Value(c, -1.0f), -4.5f, 1e-4f);
  CHECK_NEAR(Value(c, 0.0f), -4.5f, 1e-4f);
  CHECK_NEAR(Value(c, 0.05f), -2.25f, 1e-4f);
  CHECK_NEAR(Value(c, 0.2f), 0.0f, 1e-4f);
}

TEST(samples_pitch) {
  const Event *destroyed = Find("vehicle3_destroyed");
  const Event *off = Find("vehicle3_off");
  CHECK(destroyed && off);
  if (!destroyed || !off) {
    return;
  }
  const float pi = 3.14159265358979f;
  CHECK_NEAR(PitchSemitones(*destroyed, 0.9f, 0.0f), -6.0 + 0.5 * std::sin(2.0 * pi * 1.9 * 0.9), 1e-4);
  CHECK_NEAR(PitchRatio(-6.0f), 0.70711f, 1e-4f);
  CHECK_NEAR(PitchSemitones(*off, 1.0f, 0.0f), 2.0f, 1e-4f);
}

TEST(samples_tremolo) {
  const Event *destroyed = Find("vehicle3_destroyed");
  CHECK(destroyed != nullptr);
  if (!destroyed) {
    return;
  }
  CHECK_NEAR(Value(destroyed->tremoloHz, 10.0f), 10.9f, 1e-4f);
  CHECK_NEAR(Value(destroyed->distortionLevel, 30.0f), 0.80f, 1e-4f);
}
