#include "Buses.hpp"

#include <cmath>

namespace Buses {

float BusVolume(Bus aBus, const Volumes &aVolumes) {
  switch (aBus) {
  case Bus::Engine:
  case Bus::Scrape:
    return aVolumes.engine * aVolumes.game * std::pow(10.0f, kEngineGainDb / 20.0f);
  case Bus::Wind:
    return aVolumes.wind * aVolumes.game;
  case Bus::Warning:
    return aVolumes.warning * aVolumes.game;
  }
  return 0.0f;
}

Limiter LimiterFor(bool aMaster) {
  return aMaster ? Limiter{-12.0f, 7.8f} : Limiter{-12.0f, 10.0f};
}

} // namespace Buses
