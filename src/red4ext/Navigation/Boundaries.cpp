#include "Boundaries.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <spdlog/spdlog.h>
#include <sstream>

namespace Navigation {

namespace {

std::vector<std::string> Fields(const std::string &aLine) {
  std::vector<std::string> fields;
  std::stringstream stream(aLine);
  std::string field;
  while (std::getline(stream, field, '\t')) {
    fields.push_back(field);
  }
  return fields;
}

// name, gate, z_min, z_max, then three x y pairs at least
bool ParseLine(const std::string &aLine, Boundary &aOut) {
  const auto fields = Fields(aLine);
  if (fields.size() < 10 || (fields.size() - 4) % 2 != 0) {
    return false;
  }
  aOut.name = fields[0];
  if (!BoundaryGate::Parse(fields[1], aOut.gate)) {
    return false;
  }
  try {
    aOut.zMin = std::stof(fields[2]);
    aOut.zMax = std::stof(fields[3]);
    for (size_t i = 4; i + 1 < fields.size(); i += 2) {
      aOut.outline.emplace_back(std::stof(fields[i]), std::stof(fields[i + 1]));
    }
  } catch (const std::exception &) {
    return false;
  }
  aOut.line = aLine;
  return true;
}

// FNV-1a 64 over the in-force lines and the map's size: the same areas give the same key on every
// machine, so doorways baked under a mask (tools/navmap_bake/doorways.py) are found again in the game
uint64_t MaskKey(const NavMap &aMap, const std::vector<const Boundary *> &aAreas) {
  uint64_t hash = 0xcbf29ce484222325ull;
  auto mix = [&](const void *aData, size_t aSize) {
    for (size_t i = 0; i < aSize; ++i) {
      hash = (hash ^ static_cast<const uint8_t *>(aData)[i]) * 0x100000001b3ull;
    }
  };
  const int32_t size[2] = {aMap.Width(), aMap.Height()};
  mix(size, sizeof(size));
  for (const Boundary *area : aAreas) {
    mix(area->line.data(), area->line.size());
    mix("\n", 1);
  }
  return hash == 0 ? 1 : hash; // 0 means no mask
}

// the outline contains (x, y) (even-odd)
bool Inside(const std::vector<std::pair<float, float>> &aOutline, float aX, float aY) {
  bool in = false;
  for (size_t i = 0, j = aOutline.size() - 1; i < aOutline.size(); j = i++) {
    const auto [xi, yi] = aOutline[i];
    const auto [xj, yj] = aOutline[j];
    if ((yi > aY) != (yj > aY) && aX < (xj - xi) * (aY - yi) / (yj - yi) + xi) {
      in = !in;
    }
  }
  return in;
}

// squared distance from (x, y) to the outline's edges
float EdgeDistance2(const std::vector<std::pair<float, float>> &aOutline, float aX, float aY) {
  float best = std::numeric_limits<float>::infinity();
  for (size_t i = 0, j = aOutline.size() - 1; i < aOutline.size(); j = i++) {
    const auto [ax, ay] = aOutline[j];
    const auto [bx, by] = aOutline[i];
    const float ex = bx - ax, ey = by - ay, length2 = ex * ex + ey * ey;
    const float t = length2 > 0.0f ? std::clamp(((aX - ax) * ex + (aY - ay) * ey) / length2, 0.0f, 1.0f) : 0.0f;
    const float dx = ax + ex * t - aX, dy = ay + ey * t - aY;
    best = std::min(best, dx * dx + dy * dy);
  }
  return best;
}

} // namespace

bool BoundaryGate::Parse(const std::string &aText, BoundaryGate &aOut) {
  aOut = {};
  if (aText == "-") {
    return true;
  }
  // the two-character operators first, so ">=" is not read as ">" and "=0"
  static const std::pair<const char *, Op> kOps[] = {{">=", Op::GreaterEqual}, {"<=", Op::LessEqual}, {"==", Op::Equal},
                                                     {"!=", Op::NotEqual},     {">", Op::Greater},    {"<", Op::Less}};
  for (const auto &[text, op] : kOps) {
    const size_t at = aText.find(text);
    if (at == std::string::npos || at == 0) {
      continue;
    }
    try {
      size_t used = 0;
      const std::string number = aText.substr(at + std::strlen(text));
      aOut.value = std::stoi(number, &used);
      if (used != number.size()) {
        return false;
      }
    } catch (const std::exception &) {
      return false;
    }
    aOut.op = op;
    aOut.fact = aText.substr(0, at);
    return true;
  }
  return false;
}

bool BoundaryGate::Holds(int32_t aFactValue) const {
  switch (op) {
  case Op::Always:
    return true;
  case Op::Greater:
    return aFactValue > value;
  case Op::Less:
    return aFactValue < value;
  case Op::GreaterEqual:
    return aFactValue >= value;
  case Op::LessEqual:
    return aFactValue <= value;
  case Op::Equal:
    return aFactValue == value;
  case Op::NotEqual:
    return aFactValue != value;
  }
  return true;
}

Boundaries &Boundaries::Get() {
  static Boundaries instance;
  return instance;
}

bool Boundaries::Load(const std::filesystem::path &aPath) {
  std::ifstream in(aPath, std::ios::binary);
  if (!in) {
    loaded = false;
    areas.clear();
    spdlog::warn("[Boundaries] {} not found: the Autopilot does not keep out of the world boundaries", aPath.string());
    return false;
  }
  std::stringstream text;
  text << in.rdbuf();
  return Parse(text.str(), aPath.filename().string());
}

bool Boundaries::Parse(const std::string &aText, const std::string &aName) {
  std::lock_guard guard(lock);
  areas.clear(); // the masks stay: a map may still read one, and each is keyed by its own areas
  std::stringstream stream(aText);
  std::string line;
  int lineNumber = 0;
  while (std::getline(stream, line)) {
    ++lineNumber;
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty() || line[0] == '#') {
      continue;
    }
    Boundary area;
    if (ParseLine(line, area)) {
      areas.push_back(std::move(area));
    } else {
      spdlog::warn("[Boundaries] {}:{}: cannot read the line, skipped", aName, lineNumber);
    }
  }
  loaded = true;
  spdlog::info("[Boundaries] loaded {} areas from {}", areas.size(), aName);
  return true;
}

std::vector<std::string> Boundaries::Facts() const {
  std::vector<std::string> facts;
  for (const auto &area : areas) {
    if (area.gate.op != BoundaryGate::Op::Always &&
        std::find(facts.begin(), facts.end(), area.gate.fact) == facts.end()) {
      facts.push_back(area.gate.fact);
    }
  }
  return facts;
}

std::vector<bool> Boundaries::InForce(const FactReader &aFacts) const {
  std::vector<bool> out(areas.size(), true);
  for (size_t i = 0; i < areas.size(); ++i) {
    const BoundaryGate &gate = areas[i].gate;
    if (gate.op != BoundaryGate::Op::Always) {
      out[i] = gate.Holds(aFacts ? aFacts(gate.fact) : 0);
    }
  }
  return out;
}

const ColumnMask *Boundaries::MaskFor(const NavMap &aMap, const std::vector<bool> &aInForce) {
  if (!aMap.IsLoaded()) {
    return nullptr;
  }
  std::lock_guard guard(lock);
  std::vector<const Boundary *> chosen;
  for (size_t i = 0; i < areas.size() && i < aInForce.size(); ++i) {
    if (aInForce[i]) {
      chosen.push_back(&areas[i]);
    }
  }
  if (chosen.empty()) {
    return nullptr;
  }
  const uint64_t key = MaskKey(aMap, chosen);
  for (const auto &mask : masks) {
    if (mask->key == key) {
      return mask.get();
    }
  }
  masks.push_back(BuildMask(aMap, chosen, key));
  spdlog::info("[Boundaries] {} of {} areas in force: {} columns blocked (mask {:016x})", chosen.size(), areas.size(),
               masks.back()->Count(), key);
  return masks.back().get();
}

bool Boundaries::Apply(NavMap &aMap, const FactReader &aFacts) {
  const auto inForce = InForce(aFacts);
  inForceCount = int32_t(std::count(inForce.begin(), inForce.end(), true));
  const ColumnMask *mask = MaskFor(aMap, inForce);
  if (mask == aMap.Mask()) {
    return false;
  }
  aMap.SetMask(mask);
  spdlog::info("[Boundaries] nav map mask now {:016x}", mask ? mask->key : 0);
  return true;
}

std::unique_ptr<ColumnMask> BuildMask(const NavMap &aMap, const std::vector<const Boundary *> &aAreas, uint64_t aKey) {
  auto mask = std::make_unique<ColumnMask>(aKey, aMap.Width(), aMap.Height());
  const float grow = aMap.CarRadius(), grow2 = grow * grow;
  for (const Boundary *area : aAreas) {
    if (area->outline.size() < 3) {
      continue;
    }
    float x0 = std::numeric_limits<float>::infinity(), y0 = x0, x1 = -x0, y1 = -x0;
    for (const auto [x, y] : area->outline) {
      x0 = std::min(x0, x);
      y0 = std::min(y0, y);
      x1 = std::max(x1, x);
      y1 = std::max(y1, y);
    }
    // the cells whose centres can be within reach, clamped to the map
    int32_t cx0, cy0, cx1, cy1;
    const float cell = aMap.CellSize();
    float ox, oy;
    aMap.CellCentre(0, 0, ox, oy);
    ox -= 0.5f * cell;
    oy -= 0.5f * cell;
    cx0 = std::max(0, int32_t(std::floor((x0 - grow - ox) / cell)));
    cy0 = std::max(0, int32_t(std::floor((y0 - grow - oy) / cell)));
    cx1 = std::min(aMap.Width() - 1, int32_t(std::floor((x1 + grow - ox) / cell)));
    cy1 = std::min(aMap.Height() - 1, int32_t(std::floor((y1 + grow - oy) / cell)));
    for (int32_t cy = cy0; cy <= cy1; ++cy) {
      for (int32_t cx = cx0; cx <= cx1; ++cx) {
        float x, y;
        aMap.CellCentre(cx, cy, x, y);
        if (Inside(area->outline, x, y) || EdgeDistance2(area->outline, x, y) <= grow2) {
          mask->Set(cx, cy);
        }
      }
    }
  }
  return mask;
}

} // namespace Navigation
