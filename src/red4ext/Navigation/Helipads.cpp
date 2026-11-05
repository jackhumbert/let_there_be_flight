#include "Helipads.hpp"
#include "World.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <spdlog/spdlog.h>
#include <sstream>

namespace Navigation {

namespace {
// Fields of one line: id, x, y, z, yaw, radius, clearance ("inf" = open sky), name (may be empty).
bool ParseLine(const std::string &aLine, Helipad &aOut) {
  std::vector<std::string> fields;
  std::stringstream stream(aLine);
  std::string field;
  while (std::getline(stream, field, '\t')) {
    fields.push_back(field);
  }
  if (fields.size() < 7) {
    return false;
  }
  try {
    aOut.id = fields[0];
    aOut.x = std::stof(fields[1]);
    aOut.y = std::stof(fields[2]);
    aOut.z = std::stof(fields[3]);
    aOut.yaw = std::stof(fields[4]);
    aOut.radius = std::stof(fields[5]);
    aOut.clearance = fields[6] == "inf" ? std::numeric_limits<float>::infinity() : std::stof(fields[6]);
    aOut.name = fields.size() > 7 ? fields[7] : "";
  } catch (const std::exception &) {
    return false;
  }
  return !aOut.id.empty();
}
} // namespace

Helipads &Helipads::Get() {
  static Helipads instance;
  return instance;
}

bool Helipads::Load(const std::filesystem::path &aPath) {
  loaded = false;
  pads.clear();
  std::ifstream in(aPath);
  if (!in) {
    spdlog::warn("[Helipads] {} not found, no landing pads", aPath.string());
    return false;
  }
  std::string line;
  int lineNumber = 0;
  while (std::getline(in, line)) {
    ++lineNumber;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty() || line[0] == '#') {
      continue;
    }
    Helipad pad;
    if (ParseLine(line, pad)) {
      pads.push_back(std::move(pad));
    } else {
      spdlog::warn("[Helipads] {}:{}: cannot read the line, skipped", aPath.filename().string(), lineNumber);
    }
  }
  loaded = true;
  spdlog::info("[Helipads] loaded {} landing pads from {}", pads.size(), aPath.filename().string());
  return true;
}

int32_t Helipads::Nearest(float aX, float aY, float aMaxDistance) const { return NearestPad(pads, aX, aY, aMaxDistance); }

std::vector<int32_t> Helipads::InRange(float aX, float aY, float aRadius) const {
  return PadsInRange(pads, aX, aY, aRadius);
}

} // namespace Navigation
