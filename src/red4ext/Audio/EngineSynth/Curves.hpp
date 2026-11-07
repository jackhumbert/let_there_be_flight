#pragma once

// How the thruster sound answers its parameters: the FMOD event vehicle3_TPP's automation curves
// (src/fmod_studio, docs/design/engine-synth.md), copied point for point where the synth has the
// same control. Pitch in semitones, levels in dB, x in the parameter's own units.

#include <algorithm>
#include <cstddef>
#include <initializer_list>

namespace EngineSynth {

struct Point {
  float x, y;
};

// piecewise linear through its points, flat beyond the ends
struct Curve {
  Point points[4];
  size_t count;
  constexpr Curve(std::initializer_list<Point> aPoints) : points{}, count(0) {
    for (auto p : aPoints) {
      points[count++] = p;
    }
  }
  constexpr float operator()(float x) const {
    if (x <= points[0].x) {
      return points[0].y;
    }
    for (size_t i = 1; i < count; ++i) {
      if (x <= points[i].x) {
        const Point a = points[i - 1], b = points[i];
        return a.y + (b.y - a.y) * (x - a.x) / (b.x - a.x);
      }
    }
    return points[count - 1].y;
  }
};

// a peaking EQ band: Hz, dB, Q (0 Hz: none)
struct Peak {
  float frequency, gain, q;
};

// A flight axis's track (Surge, Lift, Pitch, Roll, Sway), in its chain's order: the loop (its
// fundamental, pitch by axis and speed), the track's EQ, a distortion whose level follows speed and
// the axis, a gain by the axis, a second fixed distortion, a low-pass whose top follows speed, and
// the send to the event's Engine return. FMOD's distortion has a high gain for quiet sound and clips
// loud sound, so the gains before one are squeezed: the order matters.
struct AxisCurves {
  float fundamental;   // Hz, the loop's (2077-soft-5: about 46, 2077-soft-6: about 66)
  Curve pitch;         // st by axis
  Curve speedPitch;    // st by speed
  Peak eq[3];          // the track's EQ, before the first distortion
  Curve preLevel;      // dB by axis, before the first distortion (surge only)
  Curve drive;         // the first distortion's level (0..1) by speed
  Curve axisDrive;     // and by axis, added
  Curve level;         // dB by axis, after it
  float drive2;        // the second distortion's level (0: none)
  Curve cutoff;        // Hz by speed: the low-pass (0: none)
  float send;          // dB, the track's send to Engine
};

inline constexpr AxisCurves kSurge{46.0f,
                                   {{-1, 4}, {0, -8}, {1, 4}},
                                   {{1.27f, -4}, {300, 16}},
                                   {{165.5f, 10.8f, 4.46f}, {341.8f, -13.7f, 0.707f}, {799.3f, 11.3f, 0.707f}},
                                   {{-1, 5}, {0, -12}, {1, 5}},
                                   {{0, 0.56f}, {300, 1.0f}},
                                   {{0, 0}},
                                   {{-1, 5}, {0, -4}, {1, 5}},
                                   0.0f,
                                   {{0, 0}},
                                   -13.5f};
inline constexpr AxisCurves kLift{46.0f,
                                  {{-1, 11}, {0, -1}, {1, 11}},
                                  {{0, -4}, {300, 16}},
                                  {{1793, 14.4f, 1.67f}, {328, 10.2f, 1.75f}, {0, 0, 1}},
                                  {{0, 0}},
                                  {{0, 0}, {300, 0.88f}},
                                  {{-1, 0.4f}, {0, 0}, {1, 0.4f}},
                                  {{-1, 0}, {0, -27}, {1, 0}},
                                  0.835f,
                                  {{0.42f, 4340}, {300, 9220}},
                                  -14.0f};
inline constexpr AxisCurves kPitch{66.0f,
                                   {{-1, 9}, {0, -2}, {1, 10}},
                                   {{0, -4}, {300, 16}},
                                   {{3010, 5.6f, 1.67f}, {550, 6.1f, 1.75f}, {0, 0, 1}},
                                   {{0, 0}},
                                   {{0, 0}, {300, 0.88f}},
                                   {{-1, 0.4f}, {0, 0}, {1, 0.4f}},
                                   {{-1, 0}, {0, -27}, {1, 0}},
                                   0.835f,
                                   {{0.42f, 5290}, {300, 11300}},
                                   -18.5f};
inline constexpr AxisCurves kRoll{66.0f,
                                  {{-1, 2}, {0, -10}, {1, 2}},
                                  {{0, -4}, {300, 16}},
                                  {{4193, 5.0f, 1.67f}, {833, 5.6f, 1.75f}, {0, 0, 1}},
                                  {{0, 0}},
                                  {{0, 0}, {300, 0.88f}},
                                  {{-1, 0.46f}, {0, 0.28f}, {1, 0.46f}},
                                  {{-1, 0}, {0, -28}, {1, 0}},
                                  0.14f,
                                  {{0.42f, 5410}, {300, 11800}},
                                  -7.5f};
inline constexpr AxisCurves kSway{66.0f,
                                  {{-1, 5}, {0, -7}, {1, 5}},
                                  {{0, -4}, {300, 8}},
                                  {{3010, 5.6f, 1.67f}, {550, 6.1f, 1.75f}, {0, 0, 1}},
                                  {{0, 0}},
                                  {{0, 0}, {300, 0.88f}},
                                  {{-1, 0.4f}, {0, 0.24f}, {1, 0.4f}},
                                  {{-1, 0}, {0, -27}, {1, 0}},
                                  0.835f,
                                  {{0.42f, 5290}, {300, 11300}},
                                  -24.0f};

// The event's Engine return, which every axis track sends to: EQ (peaks), a distortion following
// speed, then a 12 dB low-pass following the cabin view, a high shelf (cuts all above 240 Hz) and a
// low shelf. Outside the cabin it also sends (+1 dB) to the Cone return, whose gain follows the
// Event Cone Angle (the thruster's facing against the way to the listener): facing the listener the
// two add, facing away only the Engine return is heard.
inline constexpr Peak kEngineEq[3] = {{4281, -11.1f, 0.36f}, {116.4f, 9.7f, 0.52f}, {476, -2.2f, 0.707f}};
inline constexpr Curve kEngineDrive{{0, 0.11f}, {300, 0.42f}};
inline constexpr Curve kEngineLowPass{{0, 2700}, {1, 3500}};
inline constexpr float kEngineLowPassQ = 0.68f;
inline constexpr Peak kEngineHighShelf{240.3f, -15.8f, 0.61f};
inline constexpr Peak kEngineLowShelf{116.4f, -5.9f, 0.707f};
inline constexpr Curve kConeSend{{0, 1}, {1, -80}};
inline constexpr Curve kConeLevel{{0, 0}, {180, -80}};

// every voice's pitch with damage (0..1)
inline constexpr Curve kDamagePitch{{0, 0}, {1, 6}};
// the surge voice's EQ bands: yaw lifts 540 Hz, lift 220 Hz (dB by axis)
inline constexpr Curve kYawBand{{-1, 10}, {0, 0}, {1, 10}};
inline constexpr Curve kLiftBand{{-1, 10}, {0, 0}, {1, 10}};

// Brake: hiss whose band follows brake, tremolo faster with speed, only when moving
inline constexpr Curve kBrakeLevel{{0, -80}, {0.98f, 0}};
inline constexpr Curve kBrakeSpeedLevel{{0, -80}, {45, -24}, {297, 1}};
inline constexpr float kBrakeFader = -20.5f;
inline constexpr Curve kBrakeBand{{0.01f, 6660}, {0.99f, 8660}};
inline constexpr Curve kBrakeTremolo{{0, 2}, {297, 20}};

// Damage: a whirl (whirling.wav, about 392 Hz) whose pitch rises with speed
inline constexpr Curve kDamageLevel{{0, -80}, {1, 0}};
inline constexpr Curve kWhirlPitch{{2.97f, -15}, {300, -0.2f}};
inline constexpr float kWhirlFundamental = 392.0f;
inline constexpr float kWhirlVolume = 7.0f;

// Scrape: only above 10 m/s, pitch up an octave by 300 m/s
inline constexpr Curve kScrapeLevel{{0, -80}, {1, 0}};
inline constexpr Curve kScrapeSpeedLevel{{0, -80}, {10, 0}};
inline constexpr Curve kScrapePitch{{0, 0}, {300, 12}};
inline constexpr float kScrapeSend = 1.0f;
// the pitch of a known surface's scrape with its slide speed (m/s)
inline constexpr Curve kScrapeSlidePitch{{0, -3}, {25, 9}};

// A surface's scrape (Flight::Surface's order): clicks a second at the scrape's start and more at full, the band
// (Hz) and its Q, how fast a click dies per sample, a steady noise under the clicks (the grind), and a level (dB).
// Hard a dense mid grind, metal a bright ringing screech, glass high and brittle, wood dull and low, soft a hiss.
struct ScrapeProfile {
  float rate, rateFull, band, q, decay, bed, level;
};
inline constexpr ScrapeProfile kScrapeProfiles[] = {
    {20, 200, 1650, 1.5f, 0.995f, 0.0f, 0},     // none known: the voice as before
    {300, 1500, 1300, 1.0f, 0.990f, 0.35f, 0},  // hard
    {120, 700, 3200, 4.0f, 0.9975f, 0.12f, -2}, // metal
    {50, 300, 5200, 6.0f, 0.993f, 0.04f, -4},   // glass
    {80, 350, 650, 1.2f, 0.985f, 0.2f, -3},     // wood
    {30, 150, 380, 0.8f, 0.980f, 0.6f, -6},     // soft
};
inline const ScrapeProfile &ScrapeProfileOf(int aSurface) {
  return kScrapeProfiles[aSurface >= 0 && aSurface < 6 ? aSurface : 0];
}

// the event's master: the cabin view's level, the water low-pass
inline constexpr Curve kInsideLevel{{0, -5.5f}, {1, -3.5f}};
inline constexpr Curve kWaterCutoff{{0, 22000}, {1, 500}};

} // namespace EngineSynth
