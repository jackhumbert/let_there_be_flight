// The game-free parts of the audio backends (src/red4ext/Audio/Out, docs/design/audio.md, Wwise and Samples): a Wwise
// voice's gain ramp and low-pass (Mix), the sample player both backends play LTBF's samples with (Samples/Player:
// the WAV, the pitch, the loop and the one-shot's end, the tremolo and the distortion) and the bank's spread.
#include "check.hpp"

#include "Audio/Out/Mix.hpp"
#include "Audio/Samples/Player.hpp"
#include "Audio/Wwise/Bank.hpp"

#include <cmath>
#include <cstring>
#include <vector>

namespace {

double Rms(const float *x, size_t n) {
  double s = 0;
  for (size_t i = 0; i < n; ++i) {
    s += double(x[i]) * x[i];
  }
  return std::sqrt(s / double(n));
}

std::vector<float> Sine(float aHz, float aRate, size_t aFrames) {
  std::vector<float> v(aFrames);
  for (size_t i = 0; i < aFrames; ++i) {
    v[i] = std::sin(2.0f * 3.14159265f * aHz * float(i) / aRate);
  }
  return v;
}

} // namespace

TEST(out_mix_ramps_the_gain) {
  Out::Mix mix;
  std::vector<float> ones(512, 1.0f);
  // the first block starts at its gain
  mix.Process(ones.data(), 512, 1, 48000, 0.5f, 0);
  CHECK_NEAR(ones[0], 0.5, 1e-6);
  CHECK_NEAR(ones[511], 0.5, 1e-6);
  // then ramps from the last gain to the new over the block, no step
  ones.assign(512, 1.0f);
  mix.Process(ones.data(), 512, 1, 48000, 1.0f, 0);
  CHECK(ones[0] > 0.5f && ones[0] < 0.51f);
  CHECK_NEAR(ones[511], 1.0, 1e-6);
  CHECK(ones[255] > 0.74f && ones[255] < 0.76f);
  // stereo: both channels the same ramp
  std::vector<float> two(1024, 1.0f);
  mix.Reset();
  mix.Process(two.data(), 512, 2, 48000, 0.25f, 0);
  CHECK_NEAR(two[0], 0.25, 1e-6);
  CHECK_NEAR(two[512], 0.25, 1e-6);
}

TEST(out_mix_lowpass) {
  // an occluded car's cutoff (about 1 kHz) takes a 6 kHz tone down, leaves a 100 Hz one, and off passes both
  const float rate = 48000;
  auto level = [&](float aHz, float aCutoff) {
    Out::Mix mix;
    std::vector<float> x = Sine(aHz, rate, 9600);
    mix.Process(x.data(), 9600, 1, rate, 1.0f, aCutoff);
    return 20.0 * std::log10(Rms(x.data() + 4800, 4800) / std::sqrt(0.5));
  };
  CHECK(level(6000, 1000) < -12.0);
  CHECK(level(100, 1000) > -0.5);
  CHECK(std::abs(level(6000, 0)) < 0.1);
  // a cutoff past Nyquist is no filter
  CHECK(std::abs(level(6000, 30000)) < 0.1);
}

namespace {

// a 16-bit stereo WAV of aFrames frames: left a ramp, right its negative plus aOffset
std::vector<uint8_t> Wav(uint32_t aRate, uint32_t aFrames, float aOffset) {
  std::vector<uint8_t> b;
  auto u32 = [&](uint32_t v) {
    for (int k = 0; k < 4; ++k) {
      b.push_back(uint8_t(v >> (8 * k)));
    }
  };
  auto u16 = [&](uint32_t v) {
    b.push_back(uint8_t(v));
    b.push_back(uint8_t(v >> 8));
  };
  auto tag = [&](const char *t) { b.insert(b.end(), t, t + 4); };
  tag("RIFF");
  u32(36 + aFrames * 4);
  tag("WAVE");
  tag("fmt ");
  u32(16);
  u16(1);
  u16(2);
  u32(aRate);
  u32(aRate * 4);
  u16(4);
  u16(16);
  tag("data");
  u32(aFrames * 4);
  for (uint32_t i = 0; i < aFrames; ++i) {
    const float left = float(i) / float(aFrames), right = -left + aOffset;
    u16(uint16_t(int16_t(left * 32767.0f)));
    u16(uint16_t(int16_t(right * 32767.0f)));
  }
  return b;
}

} // namespace

TEST(out_sample_wave_reads_back) {
  Samples::Wave wave;
  CHECK(Samples::ParseWave(Wav(44100, 1000, 0.5f), wave));
  CHECK_NEAR(wave.rate, 44100, 0);
  CHECK(wave.mono.size() == 1000);
  // the channels averaged: (ramp - ramp + 0.5) / 2
  CHECK_NEAR(wave.mono[0], 0.25, 1e-3);
  CHECK_NEAR(wave.mono[999], 0.25, 1e-3);
  CHECK_NEAR(wave.Seconds(), 1000.0 / 44100.0, 1e-6);
  std::vector<uint8_t> broken = Wav(44100, 10, 0);
  std::memcpy(broken.data(), "RIFX", 4);
  CHECK(!Samples::ParseWave(broken, wave));
}

TEST(out_sample_player_pitch_loop_and_end) {
  // a wave at 24 kHz played out at 48 kHz: half a wave sample a frame; at a ratio of 2, one
  Samples::Wave wave;
  wave.rate = 24000;
  wave.mono.resize(100);
  for (size_t i = 0; i < wave.mono.size(); ++i) {
    wave.mono[i] = float(i);
  }
  Samples::Player once(wave, 48000, false);
  std::vector<float> out(64);
  once.Render(out.data(), 64);
  CHECK_NEAR(out[1], 0.5, 1e-5);
  CHECK_NEAR(out[63], 31.5, 1e-4);
  once.Set(2.0f, false, 0, 1, 0, 0, false, 0);
  once.Render(out.data(), 64);
  CHECK_NEAR(out[0], 32.0, 1e-4);
  CHECK_NEAR(out[1], 33.0, 1e-4);
  // past its end: silence, and Ended
  once.Render(out.data(), 64);
  CHECK(once.Ended());
  CHECK_NEAR(out[63], 0.0, 0);
  // a loop goes round
  Samples::Player loop(wave, 24000, true);
  std::vector<float> long_(250);
  loop.Render(long_.data(), 250);
  CHECK(!loop.Ended());
  CHECK_NEAR(long_[100], 1.0, 1e-4);
}

TEST(out_sample_tremolo_and_distortion) {
  // on for its duty, off after; flat at the top when square
  CHECK_NEAR(Samples::Tremolo(0.0, 0.5f, 0.0f, 0.0f), 0.0, 1e-6);
  CHECK_NEAR(Samples::Tremolo(0.25, 0.5f, 0.0f, 0.0f), 1.0, 1e-6);
  CHECK_NEAR(Samples::Tremolo(0.75, 0.5f, 0.0f, 0.0f), 0.0, 1e-6);
  CHECK_NEAR(Samples::Tremolo(0.1, 0.5f, 0.9f, 0.0f), 1.0, 1e-6);
  // the phase moves it on
  CHECK_NEAR(Samples::Tremolo(0.0, 0.5f, 0.0f, 0.25f), 1.0, 1e-6);
  // the distortion keeps the sign and full scale, and squashes more as its level rises
  CHECK_NEAR(Samples::Distort(1.0f, 0.5f), 1.0, 1e-5);
  CHECK(Samples::Distort(-0.2f, 0.5f) < 0.0f);
  CHECK(Samples::Distort(0.2f, 0.8f) > Samples::Distort(0.2f, 0.2f));
  CHECK(Samples::Distort(0.2f, 0.2f) > 0.2f);
}

TEST(out_bank_spread_layout) {
  // the spread attenuation as mod.bnk's attenuations are laid out: two flags, 19 curve slots, one curve, no RTPCs
  using namespace Wwise;
  const Bank::Spread spreads[] = {{"spread", Bank::kStreetSpread}};
  const Bank::Sound sounds[] = {{"voice", true, Bank::kSfxBus, 0, {}, false, 0, Fnv1("spread")}};
  const Bank::Parsed bank = Bank::Parse(Bank::Build("ltbf", sounds, spreads));
  CHECK(bank.ok);
  const Bank::Object *a = bank.Find(Fnv1("spread"));
  CHECK(a && a->type == Bank::kAttenuation);
  if (!a) {
    return;
  }
  const size_t points = std::size(Bank::kStreetSpread);
  CHECK(a->body.size() == 2 + 19 + 1 + 3 + 12 * points + 2);
  CHECK(a->body[0] == 1 && a->body[1] == 0);
  // only the spread (slot 5) has a curve, curve 0
  CHECK(a->body[2 + 5] == 0 && a->body[2 + 0] == 0xff && a->body[2 + 6] == 0xff && a->body[2 + 7] == 0xfe);
  CHECK(a->body[21] == 1);
  float last;
  std::memcpy(&last, a->body.data() + 25 + 12 * (points - 1), 4);
  CHECK_NEAR(last, 30.0, 0);
  // the voice points at it, with the attenuation on
  const Bank::Object *voice = bank.Find(Bank::IdsFor("voice").sound);
  CHECK(voice && voice->body[31] == 1 && voice->body[32] == 0x55);
  uint32_t id;
  std::memcpy(&id, voice->body.data() + 33, 4);
  CHECK(id == Fnv1("spread"));
  CHECK(voice->body[38] == 0x03 && voice->body[39] == 0x0a);
}

TEST(out_limiter_holds_the_ceiling) {
  // a -12 dB ceiling: a quiet signal passes, a loud one is held under it at once and let back up over the release
  Out::Limiter limiter;
  std::vector<float> quiet(480, 0.1f), gain(480);
  limiter.Process(quiet.data(), gain.data(), 480, 48000, -12.0f, 10.0f);
  CHECK_NEAR(gain[479], 1.0, 1e-6);
  std::vector<float> loud(480, 1.0f);
  limiter.Process(loud.data(), gain.data(), 480, 48000, -12.0f, 10.0f);
  CHECK_NEAR(gain[0] * 1.0f, std::pow(10.0, -12.0 / 20.0), 1e-4);
  limiter.Process(quiet.data(), gain.data(), 480, 48000, -12.0f, 10.0f);
  // 10 ms of release: most of the way back
  CHECK(gain[0] < 0.3f && gain[479] > 0.6f);
}
