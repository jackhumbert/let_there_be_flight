#include "NavMap.hpp"
#include "ThirdParty/miniz/miniz_tinfl.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <spdlog/spdlog.h>

namespace Navigation {

namespace {
#pragma pack(push, 1)
struct Header {
  char magic[8];
  uint32_t version;
  uint32_t flags;
  float originX, originY, cell, z0, vz;
  uint32_t nz;
  uint32_t width, height;
  uint32_t tile;
  uint32_t tilesX, tilesY;
  float radius, halfHeight;
};
#pragma pack(pop)
static_assert(sizeof(Header) == 68, "navmap header layout");

constexpr uint32_t kFlagZlib = 1;
} // namespace

NavMap &NavMap::Get() {
  static NavMap instance;
  return instance;
}

bool NavMap::Load(const std::filesystem::path &aPath) {
  std::ifstream in(aPath, std::ios::binary | std::ios::ate);
  if (!in) {
    spdlog::warn("[NavMap] {} not found, autopilot navigation disabled", aPath.string());
    return false;
  }
  std::vector<uint8_t> bytes(static_cast<size_t>(in.tellg()));
  in.seekg(0);
  in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
  return Load(std::move(bytes), aPath);
}

bool NavMap::Load(std::vector<uint8_t> aBytes, const std::filesystem::path &aPath) {
  std::lock_guard guard(lock);
  loaded = false;
  ++generation;
  cache.clear();
  lru.clear();
  file = std::move(aBytes);

  if (file.size() < sizeof(Header)) {
    spdlog::error("[NavMap] {} is too small", aPath.string());
    return false;
  }
  Header h;
  std::memcpy(&h, file.data(), sizeof(h));
  if (std::memcmp(h.magic, "LTBFNAV1", 8) != 0 || h.version != 1 || !(h.flags & kFlagZlib)) {
    spdlog::error("[NavMap] {}: unknown format (version {})", aPath.string(), h.version);
    return false;
  }
  size_t tiles = size_t(h.tilesX) * h.tilesY;
  if (file.size() < sizeof(Header) + tiles * sizeof(IndexEntry)) {
    spdlog::error("[NavMap] {}: truncated tile index", aPath.string());
    return false;
  }
  index.resize(tiles);
  std::memcpy(index.data(), file.data() + sizeof(Header), tiles * sizeof(IndexEntry));
  for (const auto &e : index) {
    if (e.offset && e.offset + e.packedSize > file.size()) {
      spdlog::error("[NavMap] {}: tile past end of file", aPath.string());
      return false;
    }
  }

  originX = h.originX;
  originY = h.originY;
  cell = h.cell;
  z0 = h.z0;
  vz = h.vz;
  nz = h.nz;
  width = h.width;
  height = h.height;
  tileCells = h.tile;
  tilesX = h.tilesX;
  tilesY = h.tilesY;
  radius = h.radius;
  halfHeight = h.halfHeight;
  loaded = true;
  spdlog::info("[NavMap] loaded {} ({:.1f} MB): {} x {} cells of {} m from ({}, {}), car radius {} m, half height {} m",
               aPath.filename().string(), file.size() / 1e6, width, height, cell, originX, originY, radius,
               halfHeight);
  return true;
}

bool NavMap::Section(const char aTag[8], std::vector<uint8_t> &aOut, size_t aSkip) const {
  if (!loaded) {
    return false;
  }
  // back from the end over the appended sections, all tagged "LTBF..."
  size_t end = file.size();
  while (end >= 24 && std::memcmp(file.data() + end - 8, "LTBF", 4) == 0) {
    uint64_t packed, raw;
    std::memcpy(&packed, file.data() + end - 24, 8);
    std::memcpy(&raw, file.data() + end - 16, 8);
    if (packed + 24 > end) {
      return false;
    }
    if (std::memcmp(file.data() + end - 8, aTag, 8) == 0 && aSkip-- == 0) {
      aOut.resize(size_t(raw));
      size_t got = tinfl_decompress_mem_to_mem(aOut.data(), aOut.size(), file.data() + end - 24 - packed, size_t(packed),
                                               TINFL_FLAG_PARSE_ZLIB_HEADER);
      return got == raw;
    }
    end -= size_t(packed) + 24;
  }
  return false;
}

std::shared_ptr<const NavMap::Tile> NavMap::Inflate(uint32_t aKey) const {
  const auto &e = index[aKey];
  if (!e.offset) {
    return nullptr;
  }
  auto tile = std::make_shared<Tile>();
  tile->raw.resize(e.rawSize);
  size_t got = tinfl_decompress_mem_to_mem(tile->raw.data(), tile->raw.size(), file.data() + e.offset, e.packedSize,
                                          TINFL_FLAG_PARSE_ZLIB_HEADER);
  size_t columns = size_t(tileCells) * tileCells;
  if (got != e.rawSize || got < columns) {
    spdlog::error("[NavMap] tile {} failed to inflate ({} of {} bytes)", aKey, got, e.rawSize);
    return nullptr;
  }
  tile->starts.resize(columns + 1);
  uint32_t n = 0;
  for (size_t c = 0; c < columns; ++c) {
    tile->starts[c] = n;
    n += tile->raw[c];
  }
  tile->starts[columns] = n;
  if (columns + size_t(n) * 4 > got) {
    spdlog::error("[NavMap] tile {}: gap data truncated", aKey);
    return nullptr;
  }
  tile->gaps = reinterpret_cast<const uint16_t *>(tile->raw.data() + columns);
  return tile;
}

std::shared_ptr<const NavMap::Tile> NavMap::GetTile(uint32_t aKey) const {
  std::lock_guard guard(lock);
  if (auto it = cache.find(aKey); it != cache.end()) {
    lru.splice(lru.begin(), lru, it->second.lruPos);
    return it->second.tile;
  }
  auto tile = Inflate(aKey);
  if (cache.size() >= kCacheTiles) {
    cache.erase(lru.back());
    lru.pop_back();
  }
  lru.push_front(aKey);
  cache[aKey] = {tile, lru.begin()};
  return tile;
}

const NavMap::Tile *NavMap::RecentTile(uint32_t aKey) const {
  constexpr int kSlots = 8;
  thread_local struct {
    const NavMap *map = nullptr;
    uint32_t generation = 0;
    uint32_t keys[kSlots];
    std::shared_ptr<const Tile> tiles[kSlots];
    int used = 0, next = 0;
  } recent;
  const uint32_t now = generation.load(std::memory_order_relaxed);
  if (recent.map != this || recent.generation != now) {
    for (auto &t : recent.tiles) {
      t.reset();
    }
    recent.map = this;
    recent.generation = now;
    recent.used = recent.next = 0;
  }
  for (int i = 0; i < recent.used; ++i) {
    if (recent.keys[i] == aKey) {
      return recent.tiles[i].get();
    }
  }
  const int slot = recent.next;
  recent.next = (recent.next + 1) % kSlots;
  recent.used = std::max(recent.used, slot + 1);
  recent.keys[slot] = aKey;
  recent.tiles[slot] = GetTile(aKey);
  return recent.tiles[slot].get();
}

uint32_t NavMap::ColumnAt(int32_t aCx, int32_t aCy, const uint16_t *&aGaps) const {
  if (!loaded || aCx < 0 || aCy < 0 || aCx >= int32_t(width) || aCy >= int32_t(height)) {
    return 0;
  }
  const Tile *tile = RecentTile(uint32_t(aCy) / tileCells * tilesX + uint32_t(aCx) / tileCells);
  if (!tile) {
    return 0;
  }
  uint32_t col = (uint32_t(aCy) % tileCells) * tileCells + (uint32_t(aCx) % tileCells);
  aGaps = tile->gaps + 2 * size_t(tile->starts[col]);
  return tile->starts[col + 1] - tile->starts[col];
}

uint32_t NavMap::View::Column(int32_t aCx, int32_t aCy, const uint16_t *&aGaps, bool *aHasData) {
  if (aHasData) {
    *aHasData = false;
  }
  if (!map.loaded || aCx < 0 || aCy < 0 || aCx >= int32_t(map.width) || aCy >= int32_t(map.height)) {
    return 0;
  }
  uint32_t key = uint32_t(aCy) / map.tileCells * map.tilesX + uint32_t(aCx) / map.tileCells;
  auto it = tiles.find(key);
  if (it == tiles.end()) {
    it = tiles.emplace(key, map.GetTile(key)).first;
  }
  const Tile *tile = it->second.get();
  if (!tile) {
    return 0;
  }
  if (aHasData) {
    *aHasData = true;
  }
  uint32_t col = (uint32_t(aCy) % map.tileCells) * map.tileCells + (uint32_t(aCx) % map.tileCells);
  aGaps = tile->gaps + 2 * size_t(tile->starts[col]);
  return tile->starts[col + 1] - tile->starts[col];
}

bool NavMap::CellOf(float aX, float aY, int32_t &aCx, int32_t &aCy) const {
  float fx = std::floor((aX - originX) / cell);
  float fy = std::floor((aY - originY) / cell);
  if (!loaded || fx < 0 || fy < 0 || fx >= float(width) || fy >= float(height)) {
    return false;
  }
  aCx = int32_t(fx);
  aCy = int32_t(fy);
  return true;
}

void NavMap::CellCentre(int32_t aCx, int32_t aCy, float &aX, float &aY) const {
  aX = originX + (float(aCx) + 0.5f) * cell;
  aY = originY + (float(aCy) + 0.5f) * cell;
}

float NavMap::VoxelZ(uint16_t aIndex) const {
  if (aIndex >= nz) {
    return std::numeric_limits<float>::infinity();
  }
  return z0 + float(aIndex) * vz;
}

std::vector<Gap> NavMap::GetGaps(float aX, float aY) const {
  std::vector<Gap> out;
  int32_t cx, cy;
  if (!CellOf(aX, aY, cx, cy)) {
    return out;
  }
  const uint16_t *gaps;
  uint32_t n = ColumnAt(cx, cy, gaps);
  for (uint32_t g = 0; g < n; ++g) {
    out.push_back({VoxelZ(gaps[2 * g]), VoxelZ(gaps[2 * g + 1])});
  }
  return out;
}

bool NavMap::FindGap(float aX, float aY, float aZ, Gap &aOut) const {
  int32_t cx, cy;
  if (!CellOf(aX, aY, cx, cy)) {
    return false;
  }
  const uint16_t *gaps;
  uint32_t n = ColumnAt(cx, cy, gaps);
  for (uint32_t g = 0; g < n; ++g) {
    const float bottom = VoxelZ(gaps[2 * g]), top = VoxelZ(gaps[2 * g + 1]);
    if (aZ >= bottom && aZ < top) {
      aOut = {bottom, top};
      return true;
    }
  }
  return false;
}

bool NavMap::IsFree(float aX, float aY, float aZ) const {
  Gap gap;
  return FindGap(aX, aY, aZ, gap);
}

float NavMap::SegmentClear(float aX0, float aY0, float aZ0, float aX1, float aY1, float aZ1) const {
  float dx = aX1 - aX0, dy = aY1 - aY0, dz = aZ1 - aZ0;
  float length = std::sqrt(dx * dx + dy * dy + dz * dz);
  int steps = std::max(1, int(std::ceil(length / (0.5f * cell))));
  for (int i = 0; i <= steps; ++i) {
    float t = float(i) / float(steps);
    float x = aX0 + dx * t, y = aY0 + dy * t, z = aZ0 + dz * t;
    int32_t cx, cy;
    bool free = false;
    if (CellOf(x, y, cx, cy)) {
      const uint16_t *gaps;
      uint32_t n = ColumnAt(cx, cy, gaps);
      for (uint32_t g = 0; g < n && !free; ++g) {
        free = z >= VoxelZ(gaps[2 * g]) && z < VoxelZ(gaps[2 * g + 1]);
      }
    }
    if (!free) {
      return i == 0 ? 0.0f : float(i - 1) / float(steps);
    }
  }
  return 1.0f;
}

} // namespace Navigation
