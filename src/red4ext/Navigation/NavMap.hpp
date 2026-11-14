#pragma once

// NavMap: the baked city-wide free-space map for the autopilot (tools/navmap_bake/navmap.py).
//
// Per 2 m column it holds the vertical gaps a car's centre can occupy, reachable from open sky.
// The bake knows no water: over the sea its open-sky gap reaches down to the seabed. Load raises
// such a gap's bottom to the sea's surface (kSeaLevel) plus the car's half height, so nothing
// plans through water (a car flying into the sea kills V).
// Solids were inflated by a car cylinder when the map was baked, so a point inside a gap is a
// valid car position. Several gaps per column = something overhead (an overpass, a tunnel).
// The top gap of most columns is open sky (top = +infinity).
//
// The whole file (~15 MB) is read at Load; tiles (128 x 128 columns, zlib) are inflated on first
// use and kept in a small LRU cache. Queries are thread safe. Heavy users (the planner) take a
// View, which keeps the tiles it touched alive and reads them without locking.

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Navigation {

struct Gap {
  float bottom; // metres
  float top;    // metres, +infinity for open sky
};

// Columns a map treats as blocked on top of its bake, at every height: the world boundaries that are
// in force (Boundaries.hpp builds it). Immutable once built; whoever sets it on a map keeps it alive
// for as long as the map may read it.
struct ColumnMask {
  static constexpr int32_t kTile = 128; // columns per side of a mask tile
  uint64_t key = 0;                     // what is masked (never 0): the doorway layer keeps a set per key
  int32_t width = 0, height = 0;        // the map's size in columns
  int32_t tilesX = 0, tilesY = 0;
  std::vector<int32_t> tiles; // per tile, its first word in bits; -1: nothing masked in the tile
  std::vector<uint64_t> bits; // kTile * kTile bits per masked tile, row by row

  ColumnMask(uint64_t aKey, int32_t aWidth, int32_t aHeight);
  void Set(int32_t aCx, int32_t aCy);
  bool Blocked(int32_t aCx, int32_t aCy) const {
    if (aCx < 0 || aCy < 0 || aCx >= width || aCy >= height) {
      return false;
    }
    const int32_t tile = tiles[size_t(aCy / kTile) * tilesX + aCx / kTile];
    if (tile < 0) {
      return false;
    }
    const uint32_t bit = uint32_t(aCy % kTile) * kTile + uint32_t(aCx % kTile);
    return (bits[size_t(tile) + bit / 64] >> (bit % 64)) & 1u;
  }
  size_t Count() const; // masked columns
};

class NavMap {
public:
  // the sea's surface, metres: the ocean's water patches (worldWaterPatchNode) are placed at z 0. Wherever the
  // open sky reaches below it the column is water (the sea, the bay, the city's canals)
  static constexpr float kSeaLevel = 0.0f;

  struct Tile {
    std::vector<uint8_t> raw;     // counts[tile * tile] then gaps
    std::vector<uint32_t> starts; // first gap of each column, tile * tile + 1 entries
    const uint16_t *gaps = nullptr;
  };

  // Planner access: integer cells, raw voxel gaps, no locking after a tile's first use.
  class View {
  public:
    // takes the map's mask as it is now: one plan sees one set of boundaries throughout
    explicit View(const NavMap &aMap) : map(aMap), mask(aMap.Mask()) {}
    // Gaps of cell (cx, cy) as voxel index pairs (bottom, top); count 0 = blocked / no data.
    // aHasData, if given, tells the two apart: false outside the map or in a tile with no data
    // (a masked column has data: it is blocked).
    uint32_t Column(int32_t aCx, int32_t aCy, const uint16_t *&aGaps, bool *aHasData = nullptr);
    const NavMap &Map() const { return map; }
    // the key of the mask this view reads, 0 for none
    uint64_t MaskKey() const { return mask ? mask->key : 0; }

  private:
    const NavMap &map;
    const ColumnMask *mask;
    std::unordered_map<uint32_t, std::shared_ptr<const Tile>> tiles;
  };

  static NavMap &Get();

  bool Load(const std::filesystem::path &aPath);
  // the map already in memory (embedded in the plugin); aPath only names it in the log
  bool Load(std::vector<uint8_t> aBytes, const std::filesystem::path &aPath);
  bool IsLoaded() const { return loaded; }

  // Gaps of the column containing (x, y), bottom up. Empty: outside the map, no data, or no
  // reachable space (treat as blocked).
  std::vector<Gap> GetGaps(float aX, float aY) const;

  // The gap containing z, if any.
  bool FindGap(float aX, float aY, float aZ, Gap &aOut) const;
  bool IsFree(float aX, float aY, float aZ) const;

  // Walks a -> b in steps of half a cell and returns the fraction of the way that is free
  // before the first blocked sample (1 = clear all the way, 0 = a itself is blocked).
  float SegmentClear(float aX0, float aY0, float aZ0, float aX1, float aY1, float aZ1) const;

  // Columns blocked on top of the bake (the world boundaries in force), read by every query from
  // the next one on; nullptr: none. The mask must outlive its use by the map; Load clears it.
  // Thread safe: plans running meanwhile keep the mask their View took.
  void SetMask(const ColumnMask *aMask) { mask.store(aMask, std::memory_order_release); }
  const ColumnMask *Mask() const { return mask.load(std::memory_order_acquire); }
  // the straight line (x0, y0) -> (x1, y1) passes a masked column (sampled every half cell)
  bool CrossesMask(float aX0, float aY0, float aX1, float aY1) const;

  // Geometry helpers.
  bool CellOf(float aX, float aY, int32_t &aCx, int32_t &aCy) const;
  void CellCentre(int32_t aCx, int32_t aCy, float &aX, float &aY) const;
  float VoxelZ(uint16_t aIndex) const; // +infinity for nz (open sky)
  float VoxelHeight() const { return vz; }
  uint16_t SkyIndex() const { return uint16_t(nz); }
  int32_t Width() const { return int32_t(width); }
  int32_t Height() const { return int32_t(height); }
  float CellSize() const { return cell; }
  float CarRadius() const { return radius; }
  float CarHalfHeight() const { return halfHeight; }

  // A section appended to the file after the map (a bake's extras, such as the planner's
  // doorways): the file ends in packed size (u64), raw size (u64) and the section's 8 byte tag,
  // after the zlib data. False if the file has none with that tag.
  // The aSkip-th section with this tag among those appended to the end of the file (each: zlib
  // payload, packed and raw size, tag; the last one first), inflated.
  bool Section(const char aTag[8], std::vector<uint8_t> &aOut, size_t aSkip = 0) const;

private:
  struct IndexEntry {
    uint64_t offset;
    uint32_t packedSize;
    uint32_t rawSize;
  };
  struct CacheEntry {
    std::shared_ptr<const Tile> tile; // nullptr: tile has no data
    std::list<uint32_t>::iterator lruPos;
  };

  std::shared_ptr<const Tile> GetTile(uint32_t aKey) const; // takes the lock
  // GetTile through a small per-thread cache of the tiles used last (queries along a line stay in
  // one or two tiles): no lock, no reference counting. Valid until the map is loaded again.
  const Tile *RecentTile(uint32_t aKey) const;
  // the gaps of cell (cx, cy) as voxel pairs, through RecentTile; 0 outside the map or without data
  uint32_t ColumnAt(int32_t aCx, int32_t aCy, const uint16_t *&aGaps) const;
  std::shared_ptr<const Tile> Inflate(uint32_t aKey) const;

  bool loaded = false;
  std::vector<uint8_t> file;
  std::vector<IndexEntry> index;
  float originX = 0, originY = 0, cell = 2, z0 = 0, vz = 0.5f, radius = 0, halfHeight = 0;
  uint32_t nz = 0, width = 0, height = 0, tileCells = 128, tilesX = 0, tilesY = 0;

  static constexpr size_t kCacheTiles = 256;
  mutable std::mutex lock;
  mutable std::unordered_map<uint32_t, CacheEntry> cache;
  mutable std::list<uint32_t> lru; // most recent first
  std::atomic<uint32_t> generation{0}; // bumped by every Load: RecentTile's caches drop what they hold
  std::atomic<const ColumnMask *> mask{nullptr};
};

} // namespace Navigation
