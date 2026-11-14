#include "FlightBoundaries.hpp"

#include "Boundaries.hpp"
#include "NavMap.hpp"
#include "Utils/Utils.hpp"

#include <RED4ext/CNamePool.hpp>
#include <RED4ext/GameEngine.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/GameInstance.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <mutex>
#include <string>
#include <unordered_map>

namespace {

// the fact's value from the quests system (GetFactStr: GetFact with a CName reads 0 for a name the name pool does not
// hold yet, as ep1_installed at the first attach); 0 when there is no quests system or GetFactStr (warned once)
int32_t ReadFact(const std::string &aFact) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto engine = RED4ext::CGameEngine::Get();
  RED4ext::IScriptable *system = nullptr;
  if (rtti && engine && engine->framework && engine->framework->gameInstance) {
    // the game may keep a system under its interface
    for (auto cls = rtti->GetClass("questQuestsSystem"); cls && cls->GetName() != RED4ext::CName("IScriptable");
         cls = cls->parent) {
      if ((system = engine->framework->gameInstance->GetSystem(cls))) {
        break;
      }
    }
  }
  auto cls = rtti ? rtti->GetClass("questQuestsSystem") : nullptr;
  auto getFact = cls ? cls->GetFunction("GetFactStr") : nullptr;
  if (!system || !getFact) {
    LTBF_WARN_ONCE("[Boundaries] the quests system or its GetFactStr was not found: every boundary gate reads 0");
    return 0;
  }
  int32_t value = 0;
  RED4ext::CString name(aFact.c_str());
  RED4ext::ExecuteFunction(system, getFact, &value, name);
  return value;
}

std::mutex lock;

} // namespace

RED4ext::DynArray<RED4ext::CName> FlightBoundaries::GetFacts() {
  RED4ext::DynArray<RED4ext::CName> out;
  for (const auto &fact : Navigation::Boundaries::Get().Facts()) {
    // added to the name pool, so the quests system can name the fact it is listening for
    out.PushBack(RED4ext::CNamePool::Add(fact.c_str()));
  }
  return out;
}

bool FlightBoundaries::Refresh() {
  auto &boundaries = Navigation::Boundaries::Get();
  if (!boundaries.IsLoaded() || !Navigation::NavMap::Get().IsLoaded()) {
    return false;
  }
  std::lock_guard guard(lock);
  // each fact once, logged
  std::unordered_map<std::string, int32_t> facts;
  std::string read;
  for (const auto &fact : boundaries.Facts()) {
    facts[fact] = ReadFact(fact);
    read += fmt::format("{}{} = {}", read.empty() ? "" : ", ", fact, facts[fact]);
  }
  spdlog::info("[Boundaries] facts: {}", read.empty() ? "none" : read);
  boundaries.Apply(Navigation::NavMap::Get(), [&](const std::string &aFact) {
    auto it = facts.find(aFact);
    return it == facts.end() ? 0 : it->second;
  });
  return true;
}

int32_t FlightBoundaries::GetAreasInForce() { return Navigation::Boundaries::Get().InForceCount(); }

RED4ext::CString FlightBoundaries::GetMaskKey() {
  const auto *mask = Navigation::NavMap::Get().Mask();
  return RED4ext::CString(mask ? fmt::format("{:016x}", mask->key).c_str() : "0");
}

bool FlightBoundaries::IsBlocked(RED4ext::Vector4 aPosition) {
  const auto &map = Navigation::NavMap::Get();
  const auto *mask = map.Mask();
  int32_t cx, cy;
  return mask && map.CellOf(aPosition.X, aPosition.Y, cx, cy) && mask->Blocked(cx, cy);
}
