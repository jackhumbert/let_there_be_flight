// The buses' volumes and limiters (src/red4ext/Audio/Buses): each bus's setting times the game's (the Engine bus 3 dB
// up), and the two limiters the Studio project had (Master.xml, the Engine group's Metadata).
#include "check.hpp"

#include "Audio/Buses.hpp"

using namespace Buses;

TEST(buses_volume) {
  const Volumes v{0.5f, 1.0f, 0.6f, 0.5f};
  // the Engine bus 3 dB over its setting: x 1.4125
  CHECK_NEAR(BusVolume(Bus::Engine, v), 0.5f * 1.41254f, 1e-5f);
  CHECK_NEAR(BusVolume(Bus::Wind, v), 0.3f, 1e-6f);
  CHECK_NEAR(BusVolume(Bus::Warning, v), 0.25f, 1e-6f);
  // the game silent: every bus is
  const Volumes muted{0.0f, 1.0f, 0.6f, 0.5f};
  CHECK_NEAR(BusVolume(Bus::Engine, muted), 0.0f, 1e-6f);
  CHECK_NEAR(BusVolume(Bus::Wind, muted), 0.0f, 1e-6f);
  CHECK_NEAR(BusVolume(Bus::Warning, muted), 0.0f, 1e-6f);
}

TEST(buses_limiters) {
  const Limiter group = LimiterFor(false), master = LimiterFor(true);
  CHECK_NEAR(group.ceilingDb, -12.0f, 1e-6f);
  CHECK_NEAR(group.releaseMs, 10.0f, 1e-6f);
  CHECK_NEAR(master.ceilingDb, -12.0f, 1e-6f);
  CHECK_NEAR(master.releaseMs, 7.8f, 1e-6f);
}
