#include "Sends.hpp"

#include "Ids.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Wwise::Sends {

namespace {
// the game's reverb buses (base/sound/event/eventsmetadata.json's bus list), named in the log
constexpr const char *kReverbs[] = {
    "revb_exterior_default_2_large", "revb_exterior_default_2_medium", "revb_exterior_default_2_small",
    "revb_exterior_default__large", "revb_exterior_default__large_01_quiet", "revb_exterior_default__medium",
    "revb_exterior_default__medium_01_quiet", "revb_exterior_default__small", "revb_exterior_default__small_01_quiet",
    "revb_exterior_default_bl__large", "revb_exterior_default_bl__medium", "revb_exterior_default_bl__small",
    "revb_exterior_slight_echo_large", "revb_exterior_slight_echo_medium", "revb_exterior_slight_echo_small",
    "revb_exterior_weapon_from_distant_vehicle", "revb_indoor_bright__large", "revb_indoor_bright__medium",
    "revb_indoor_bright__small", "revb_indoor_car_common", "revb_indoor_car_common_small", "revb_indoor_car_hipercar",
    "revb_indoor_car_luxury_sedan", "revb_indoor_car_nomad", "revb_indoor_car_sport", "revb_indoor_car_suv",
    "revb_indoor_car_truck", "revb_indoor_metro_train", "revb_interior_boomy_large", "revb_interior_boomy_large_longer",
    "revb_interior_boomy_medium", "revb_interior_boomy_small", "revb_interior_default__large",
    "revb_interior_default__medium", "revb_interior_default__small", "revb_interior_default_small__large",
    "revb_interior_default_small__medium", "revb_interior_default_small__small", "revb_interior_elevator_small__large",
    "revb_interior_elevator_small__medium", "revb_interior_elevator_small__small", "revb_interior_factory_large__large",
    "revb_interior_factory_large__medium", "revb_interior_factory_large__small", "revb_interior_factory_medium__large",
    "revb_interior_factory_medium__medium", "revb_interior_factory_medium__small",
    "revb_interior_megabuilding_corridor_large__large", "revb_interior_megabuilding_corridor_large__medium",
    "revb_interior_megabuilding_corridor_large__small", "revb_interior_metro_station_large_loud__large",
    "revb_interior_metro_station_large_loud__medium", "revb_interior_metro_station_large_loud__small",
    "revb_interior_room_large__large", "revb_interior_room_large__medium", "revb_interior_room_large__small",
    "revb_interior_room_medium__large", "revb_interior_room_medium__medium", "revb_interior_room_medium__small",
    "revb_source_based_exterior", "revb_source_based_interior_large", "revb_source_based_interior_medium",
    "revb_source_based_interior_small", "exterior_default", "exterior_default_2", "exterior_default_bl",
    "exterior_default_quiet", "exterior_slight_echo", "interior_boomy", "interior_bright", "interior_car",
    "interior_car_new", "interior_default", "interior_default_small", "interior_elevator_small",
    "interior_factory_large", "interior_factory_medium", "interior_metro_station_large_loud", "interior_metro_train",
    "interior_room_large", "interior_room_medium", "interior_warehouse", "interior_warehouse_large",
    "interior_warehouse_medium", "interior_warehouse_small"};
} // namespace

bool Set::operator==(const Set &aOther) const {
  if (count != aOther.count)
    return false;
  for (uint32_t i = 0; i < count; ++i) {
    const AkAuxSendValue &a = values[i], &b = aOther.values[i];
    if (a.listenerID != b.listenerID || a.auxBusID != b.auxBusID || std::fabs(a.controlValue - b.controlValue) > 1e-3f)
      return false;
  }
  return true;
}

void Book::Note(AkGameObjectID aObject, const AkAuxSendValue *, uint32_t) {
  if (IsOurs(aObject))
    return;
  std::lock_guard _(lock);
  objects[aObject] = ++calls;
}

std::vector<AkGameObjectID> Book::Objects() const {
  std::vector<std::pair<uint64_t, AkGameObjectID>> byCall;
  {
    std::lock_guard _(lock);
    byCall.reserve(objects.size());
    for (const auto &[object, call] : objects)
      byCall.emplace_back(call, object);
  }
  std::sort(byCall.begin(), byCall.end());
  std::vector<AkGameObjectID> out;
  out.reserve(byCall.size());
  for (const auto &[_, object] : byCall)
    out.push_back(object);
  return out;
}

void Book::Forget(AkGameObjectID aObject) {
  std::lock_guard _(lock);
  objects.erase(aObject);
}

uint64_t Book::TakeCalls() {
  std::lock_guard _(lock);
  const uint64_t n = calls - counted;
  counted = calls;
  return n;
}

Book &TheBook() {
  static Book book;
  return book;
}

int Nearest(const std::vector<AkVector64> &aPositions, const AkVector64 &aCar, double aRange) {
  int best = -1;
  double bestDistance = aRange * aRange;
  for (size_t i = 0; i < aPositions.size(); ++i) {
    const double dx = aPositions[i].x - aCar.x, dy = aPositions[i].y - aCar.y, dz = aPositions[i].z - aCar.z;
    const double d = dx * dx + dy * dy + dz * dz;
    if (d <= bestDistance) {
      best = int(i);
      bestDistance = d;
    }
  }
  return best;
}

Set ForListener(const Set &aSends, AkGameObjectID aListener) {
  Set out = aSends;
  out.count = std::min(out.count, kMax);
  for (uint32_t i = 0; i < out.count; ++i)
    out.values[i].listenerID = aListener;
  return out;
}

std::string BusName(uint32_t aBus) {
  static const std::unordered_map<uint32_t, const char *> names = [] {
    std::unordered_map<uint32_t, const char *> m;
    for (const char *name : kReverbs)
      m.emplace(Fnv1(name), name);
    return m;
  }();
  auto it = names.find(aBus);
  return it != names.end() ? std::string(it->second) : "bus " + std::to_string(aBus);
}

std::string Describe(const Set &aSends) {
  if (aSends.count == 0)
    return "none";
  std::string out;
  for (uint32_t i = 0; i < aSends.count && i < kMax; ++i)
  {
    char level[16];
    std::snprintf(level, sizeof level, " %.2f", aSends.values[i].controlValue);
    out += (i ? ", " : "") + BusName(aSends.values[i].auxBusID) + level;
  }
  return out;
}

} // namespace Wwise::Sends
