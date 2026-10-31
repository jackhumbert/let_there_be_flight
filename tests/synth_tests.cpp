// The engine synth (src/red4ext/Audio/EngineSynth): it renders finite sound, louder with the surge held.
#include "check.hpp"

#include "Audio/EngineSynth/Synth.hpp"

#include <cmath>
#include <vector>

namespace {
float Rms(EngineSynth::Synth &aSynth, uint32_t aFrames) {
  std::vector<float> out(aFrames * 2);
  aSynth.Render(out.data(), aFrames);
  double sum = 0;
  for (float x : out) {
    if (!std::isfinite(x)) {
      return -1;
    }
    sum += double(x) * x;
  }
  return float(std::sqrt(sum / out.size()));
}
} // namespace

TEST(synth_finite_and_louder_with_surge) {
  EngineSynth::Synth idle(48000, 1, 1), surge(48000, 1, 1);
  EngineSynth::Inputs in;
  in.speed = 40;
  idle.Set(in);
  in.surge = 1;
  surge.Set(in);
  Rms(idle, 48000);
  Rms(surge, 48000);
  const float quiet = Rms(idle, 48000), loud = Rms(surge, 48000);
  CHECK(quiet >= 0 && loud >= 0);
  CHECK(loud > quiet);
}

TEST(synth_character_clean_by_default) {
  const EngineSynth::Character byDefault;
  CHECK(!byDefault.cone && !byDefault.distortion && !byDefault.darkness && !byDefault.grit);
  EngineSynth::Character c;
  CHECK(EngineSynth::ParseCharacter("fmod", c));
  CHECK(c.cone && c.distortion && c.darkness && c.grit);
  CHECK(EngineSynth::ParseCharacter("clean", c));
  CHECK(c == byDefault);
  CHECK(EngineSynth::ParseCharacter("darkness=1", c));
  CHECK(c.darkness && !c.cone);
  CHECK(!EngineSynth::ParseCharacter("loud=1", c));
}
