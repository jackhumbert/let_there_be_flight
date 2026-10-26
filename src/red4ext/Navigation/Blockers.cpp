#include "Blockers.hpp"

#include <RED4ext/ResourcePath.hpp>
#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

namespace Navigation {

Blockers &Blockers::Get() {
  static Blockers instance;
  return instance;
}

// Lines: "S <sector path>" starts a sector, "A <x> <y> <z>" lists an actor of it; # comments.
bool Blockers::Load(const std::filesystem::path &aPath) {
  loaded = false;
  sectors.clear();
  std::ifstream in(aPath);
  if (!in) {
    spdlog::warn("[Blockers] {} not found, invisible blockers stay", aPath.string());
    return false;
  }
  std::vector<Actor> *current = nullptr;
  size_t actors = 0;
  std::string line;
  int lineNumber = 0;
  while (std::getline(in, line)) {
    ++lineNumber;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.size() < 3 || line[0] == '#') {
      continue;
    }
    if (line[0] == 'S' && line[1] == ' ') {
      auto path = line.substr(2);
      current = &sectors[RED4ext::ResourcePath(path.c_str()).hash];
      continue;
    }
    Actor actor;
    std::istringstream fields(line.substr(1));
    if (line[0] != 'A' || !current || !(fields >> actor.x >> actor.y >> actor.z)) {
      spdlog::warn("[Blockers] {}:{}: cannot read the line, skipped", aPath.filename().string(), lineNumber);
      continue;
    }
    current->push_back(actor);
    ++actors;
  }
  loaded = true;
  spdlog::info("[Blockers] loaded {} actors in {} sectors", actors, sectors.size());
  return true;
}

const std::vector<Blockers::Actor> *Blockers::Find(uint64_t aSectorPath) const {
  auto it = sectors.find(aSectorPath);
  return it == sectors.end() ? nullptr : &it->second;
}

} // namespace Navigation
