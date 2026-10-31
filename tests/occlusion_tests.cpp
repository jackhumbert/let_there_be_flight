// NPC cars heard through buildings (src/red4ext/Audio/Acoustics/Occlusion): the smoothing, the cutoff and
// duck mapping and the round robin over the cars. exp(-dt/tau)^n equals exp(-n dt/tau), so the smoothing
// has closed forms and only float rounding to allow for.
#include "check.hpp"

#include "Audio/Acoustics/Occlusion.hpp"

#include <cmath>
#include <vector>

using namespace Acoustics::Occlusion;

TEST(occlusion_smoothing) {
  float o = 0.0f;
  for (int i = 0; i < 6; ++i) {
    o = Smooth(o, true, 1.0f / 60.0f);
  }
  // 0.1 s blocked: 1 - e^-1
  CHECK_NEAR(o, 0.63212f, 1e-4f);
  for (int i = 0; i < 6; ++i) {
    o = Smooth(o, false, 1.0f / 60.0f);
  }
  // 0.1 s open: that times e^-1
  CHECK_NEAR(o, 0.23254f, 1e-4f);
}

TEST(occlusion_mapping) {
  CHECK_NEAR(CutoffHz(0.0f), 22000.0f, 0.5f);
  CHECK_NEAR(CutoffHz(1.0f), 1200.0f, 0.5f);
  // halfway in the log is the geometric mean
  CHECK_NEAR(CutoffHz(0.5f), std::sqrt(22000.0f * 1200.0f), 1.0f);
  CHECK_NEAR(DuckDb(0.0f), 0.0f, 1e-6f);
  CHECK_NEAR(DuckDb(1.0f), -8.0f, 1e-6f);
}

TEST(occlusion_round_robin) {
  RoundRobin robin;
  CHECK((robin.Next(10) == std::vector<int>{0, 1, 2, 3}));
  CHECK((robin.Next(10) == std::vector<int>{4, 5, 6, 7}));
  CHECK((robin.Next(10) == std::vector<int>{8, 9}));
  CHECK((robin.Next(10) == std::vector<int>{0, 1, 2, 3}));
  CHECK(robin.Next(0).empty());
}
