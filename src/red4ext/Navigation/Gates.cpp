#include "Gates.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

namespace Navigation {

namespace {
// Fields of one line: x, y, z, yaw, width, wall, ceiling ("inf" = open sky), score (unused here).
bool ParseLine(const std::string &aLine, Gate &aOut) {
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
    aOut.x = std::stof(fields[0]);
    aOut.y = std::stof(fields[1]);
    aOut.z = std::stof(fields[2]);
    aOut.yaw = std::stof(fields[3]);
    aOut.width = std::stof(fields[4]);
    aOut.wall = std::stof(fields[5]);
    aOut.ceiling = fields[6] == "inf" ? std::numeric_limits<float>::infinity() : std::stof(fields[6]);
  } catch (const std::exception &) {
    return false;
  }
  return true;
}
} // namespace

Gates &Gates::Get() {
  static Gates instance;
  return instance;
}

bool Gates::Load(const std::filesystem::path &aPath) {
  loaded = false;
  gates.clear();
  std::ifstream in(aPath);
  if (!in) {
    spdlog::warn("[Gates] {} not found, free roam picks its legs at random", aPath.string());
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
    Gate gate;
    if (ParseLine(line, gate)) {
      gates.push_back(gate);
    } else {
      spdlog::warn("[Gates] {}:{}: cannot read the line, skipped", aPath.filename().string(), lineNumber);
    }
  }
  loaded = true;
  spdlog::info("[Gates] loaded {} gates from {}", gates.size(), aPath.filename().string());
  return true;
}

std::vector<int32_t> Gates::InRange(float aX, float aY, float aRadius) const {
  std::vector<std::pair<float, int32_t>> found;
  float range = aRadius * aRadius;
  for (size_t i = 0; i < gates.size(); ++i) {
    float dx = gates[i].x - aX, dy = gates[i].y - aY;
    float d = dx * dx + dy * dy;
    if (d <= range) {
      found.emplace_back(d, int32_t(i));
    }
  }
  std::sort(found.begin(), found.end());
  std::vector<int32_t> out;
  out.reserve(found.size());
  for (const auto &[d, i] : found) {
    out.push_back(i);
  }
  return out;
}

} // namespace Navigation
