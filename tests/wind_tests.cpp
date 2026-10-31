// The wind (src/red4ext/Audio/Wind/Model): the U^6 law and its mix, Strouhal scaling, the cabin, the
// whistle's lock-in, the sides, and the city's wind field (height, shelter, gusts, edges).
#include "check.hpp"

#include "Audio/Wind/Model.hpp"

#include <cmath>
#include <vector>

using namespace Wind;

namespace {
constexpr float kDt = 1.0f / 60.0f;

// RMS of the left channel, of its first difference (which weighs the highs up 6 dB an octave), and in
// a band round aBandHz (three RBJ band-passes of Q 2 in a row: one alone lets the rumble's 40 Hz,
// 50 dB louder, through its skirts)
struct Measure {
  double rms = 0, slope = 0, band = 0;
};

Measure Rms(const Params &aParams, float aBandHz = 800, int aFrames = 96000) {
  Render render(48000.0f, 5);
  render.Set(aParams);
  std::vector<float> block(2 * 512);
  for (int done = 0; done < 9600; done += 512) {
    render.Process(block.data(), 512);
  }
  const double w = 2.0 * 3.14159265358979 * aBandHz / 48000.0, alpha = std::sin(w) / (2.0 * 2.0);
  const double a0 = 1 + alpha, b0 = alpha / a0, b2 = -alpha / a0, a1 = -2 * std::cos(w) / a0, a2 = (1 - alpha) / a0;
  double x1[3] = {}, x2[3] = {}, y1[3] = {}, y2[3] = {};
  double sum = 0, diff = 0, band = 0, last = 0;
  long n = 0;
  for (int done = 0; done < aFrames; done += 512) {
    render.Process(block.data(), 512);
    for (int i = 0; i < 512; ++i) {
      const double x = block[2 * i];
      double y = x;
      for (int k = 0; k < 3; ++k) {
        const double in = y;
        y = b0 * in + b2 * x2[k] - a1 * y1[k] - a2 * y2[k];
        x2[k] = x1[k];
        x1[k] = in;
        y2[k] = y1[k];
        y1[k] = y;
      }
      sum += x * x;
      diff += (x - last) * (x - last);
      band += y * y;
      last = x;
      ++n;
    }
  }
  return {std::sqrt(sum / double(n)), std::sqrt(diff / double(n)), std::sqrt(band / double(n))};
}

Params Air(float aSpeed, View aView) {
  Input in;
  in.view = aView;
  in.velocity = {0, aSpeed, 0};
  in.dt = kDt;
  return Model().Update(in);
}

double Deviation(const std::vector<double> &aValues) {
  double mean = 0;
  for (double v : aValues) {
    mean += v;
  }
  mean /= double(aValues.size());
  double var = 0;
  for (double v : aValues) {
    var += (v - mean) * (v - mean);
  }
  return std::sqrt(var / double(aValues.size() - 1));
}
} // namespace

TEST(wind_spl_goes_as_u6) {
  CHECK_NEAR(Spl(56) - Spl(28), 18.062, 0.01);
  CHECK_NEAR(Spl(280) - Spl(28), 60.0, 0.01);
  CHECK_NEAR(Spl(28), 100.0, 1e-4);
}

TEST(wind_mix_compresses_the_law) {
  const Tuning t;
  CHECK_NEAR(LevelDb(56) - LevelDb(28), 18.062 / t.compression, 0.01);
  CHECK(LevelDb(400) == t.maxDb);
  CHECK(Level(0) == 0.0f);
}

TEST(wind_silent_in_still_air) {
  Input in;
  in.velocity = {3, 4, 0};
  in.wind = {3, 4, 0};
  in.dt = kDt;
  const Params p = Model().Update(in);
  CHECK(p.airspeed == 0.0f);
  Render render(48000.0f, 5);
  render.Set(p);
  std::vector<float> out(2 * 48000);
  render.Process(out.data(), 48000);
  bool silent = true;
  for (float s : out) {
    silent = silent && s == 0.0f;
  }
  CHECK(silent);
}

TEST(wind_frequencies_scale_with_speed) {
  const Params slow = Air(40, View::Tpp), fast = Air(80, View::Tpp);
  CHECK_NEAR(fast.flowHz / slow.flowHz, 2.0, 1e-4);
  CHECK_NEAR(fast.edgeHz / slow.edgeHz, 2.0, 1e-4);
  CHECK_NEAR(fast.aeolianHz / slow.aeolianHz, 2.0, 1e-4);
  // Strouhal 0.2 over a 0.2 m head: as many Hz as m/s
  CHECK_NEAR(slow.flowHz, 40.0, 1e-3);
  // and brighter with speed when rendered
  const Measure a = Rms(slow), b = Rms(fast);
  CHECK(b.slope / b.rms > 1.5 * (a.slope / a.rms));
}

TEST(wind_cabin_quieter_and_darker) {
  const Params tpp = Air(40, View::Tpp), fpp = Air(40, View::Fpp);
  CHECK(tpp.cabinHz == 0.0f);
  // the mass law for 10 kg/m2: no loss at about 22 Hz
  CHECK_NEAR(fpp.cabinHz, 22.4, 0.1);
  // the panels: the rumble gets through (the mass law loses little at 40 Hz); at 800 Hz it loses 31 dB,
  // the seals' leak (-32 dB) bringing that to about 28. The boom and the whistle are the cabin's own.
  Params panels = fpp;
  panels.boomLevel = 0;
  panels.whistleLevel = 0;
  const Measure out = Rms(tpp), in = Rms(panels);
  CHECK(in.rms < out.rms);
  CHECK(in.slope / in.rms < 0.5 * (out.slope / out.rms));
  CHECK_NEAR(20.0 * std::log10(out.band / in.band), 28.5, 3.0);
}

TEST(wind_whistle_locks_on_in_bands) {
  // a 1100 Hz resonance and a 3 cm gap: Rossiter's modes 3, 2, 1 cross it near 22, 35 and 89 m/s
  CHECK(WhistleLock(22) > 0.9f);
  CHECK(WhistleLock(35) > 0.9f);
  CHECK(WhistleLock(89) > 0.9f);
  CHECK(WhistleLock(28) < 0.1f);
  CHECK(WhistleLock(55) < 0.05f);
  CHECK(WhistleLock(150) < 0.05f);
  CHECK(Air(35, View::Fpp).whistleLevel > 0.0f);
  CHECK(Air(35, View::Tpp).whistleLevel == 0.0f);
}

TEST(wind_air_from_the_side_it_comes) {
  Input in;
  in.view = View::Fpp;
  in.right = {1, 0, 0};
  in.dt = kDt;
  Model model;
  // moving forward and to the left: the air comes from the front left
  in.velocity = {-15, 40, 0};
  const Params left = model.Update(in);
  CHECK(left.left > left.right);
  in.velocity = {15, 40, 0};
  const Params right = model.Update(in);
  CHECK(right.right > right.left);
  in.velocity = {0, 40, 0};
  const Params centre = model.Update(in);
  CHECK_NEAR(centre.left, centre.right, 1e-6);
  // a crosswind on a hovering car does the same
  in.velocity = {};
  in.wind = {8, 0, 0};
  const Params gust = model.Update(in);
  CHECK(gust.left > gust.right);
}

TEST(wind_crossflow_strengthens_the_tone) {
  Input in;
  in.right = {1, 0, 0};
  in.dt = kDt;
  in.velocity = {0, 40, 0};
  const Params straight = Model().Update(in);
  in.velocity = {40, 0, 0};
  const Params sideways = Model().Update(in);
  CHECK_NEAR(20.0 * std::log10(sideways.aeolianLevel / straight.aeolianLevel), Tuning().slipDb, 0.01);
}

TEST(wind_field_grows_with_height) {
  FieldInput in;
  in.dt = kDt;
  in.floorDistance = 10;
  Field low(3);
  low.Update(in);
  in.floorDistance = 100;
  Field high(3);
  high.Update(in);
  CHECK_NEAR(high.Mean() / low.Mean(), std::pow(10.0, 0.3), 1e-3);
  CHECK_NEAR(low.Mean(), FieldTuning().refWind, 1e-3);
}

TEST(wind_field_sheltered_in_a_canyon) {
  FieldInput in;
  in.dt = kDt;
  in.floorDistance = 20;
  Field open(3), canyon(3);
  open.Update(in);
  in.openSky = 0;
  in.wallDistance = 5;
  canyon.Update(in);
  CHECK(canyon.Mean() < 0.25f * open.Mean());
}

TEST(wind_field_gust_intensity) {
  FieldInput in;
  in.dt = kDt;
  in.floorDistance = 50;
  Field field(7);
  std::vector<double> x;
  for (int i = 0; i < 60 * 600; ++i) {
    const Vec w = field.Update(in);
    x.push_back(w.x);
  }
  // the drift of the prevailing direction adds a little to the gusts' own sigma
  CHECK_NEAR(Deviation(x), field.Sigma(), 0.35 * field.Sigma());
}

TEST(wind_gusts_count_less_flying_fast) {
  auto swing = [](float aCarSpeed) {
    FieldInput fin;
    fin.dt = kDt;
    fin.floorDistance = 60;
    fin.crossing = aCarSpeed;
    Field field(9);
    Model model;
    Input in;
    in.dt = kDt;
    in.velocity = {0, aCarSpeed, 0};
    std::vector<double> db;
    for (int i = 0; i < 60 * 120; ++i) {
      in.wind = field.Update(fin);
      db.push_back(LevelDb(model.Update(in).airspeed));
    }
    return Deviation(db);
  };
  const double hover = swing(0), fast = swing(150);
  CHECK(hover > 1.0);
  CHECK(fast < hover / 5.0);
}

TEST(wind_gusts_quicker_flying_fast) {
  // the gusts' correlation a second apart: the eddies' size over the speed they are crossed at
  auto correlation = [](float aCrossing) {
    FieldInput in;
    in.dt = kDt;
    in.floorDistance = 60;
    in.crossing = aCrossing;
    // the gusts alone: no drift of the prevailing direction
    FieldTuning tuning;
    tuning.driftDegrees = 0;
    Field field(11, tuning);
    std::vector<double> x;
    for (int i = 0; i < 60 * 600; ++i) {
      x.push_back(field.Update(in).x);
    }
    double mean = 0;
    for (double v : x) {
      mean += v;
    }
    mean /= double(x.size());
    double num = 0, den = 0;
    for (size_t i = 0; i + 60 < x.size(); ++i) {
      num += (x[i] - mean) * (x[i + 60] - mean);
      den += (x[i] - mean) * (x[i] - mean);
    }
    return num / den;
  };
  // 42 m eddies in a 10 m/s wind: about exp(-1 / 4.1) hovering, exp(-1 / 0.38) at 100 m/s
  CHECK(correlation(0) > 0.6);
  CHECK(correlation(100) < 0.2);
}

TEST(wind_edge_stirs_turbulence) {
  FieldInput in;
  in.dt = kDt;
  in.floorDistance = 30;
  in.crossing = 40;
  in.wallDistance = 5;
  Field field(1);
  field.Update(in);
  CHECK(field.Stir() == 0.0f);
  in.wallDistance = 30;
  field.Update(in);
  CHECK(field.Stir() > 0.5f);
  for (int i = 0; i < 60; ++i) {
    field.Update(in);
  }
  // 40 m of travel past the edge: settled
  CHECK(field.Stir() < 0.01f);
}
