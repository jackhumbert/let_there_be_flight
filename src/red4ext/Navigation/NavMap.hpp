#pragma once

// NavMap: the baked city-wide free-space map for the autopilot (tools/heightmap/navmap.py).
//
// Per 2 m column it holds the vertical gaps a car's centre can occupy, reachable from open sky.
// Solids were inflated by a car cylinder when the map was baked, so a point inside a gap is a
// valid car position. Several gaps per column = something overhead (an overpass, a tunnel).
// The top gap of most columns is open sky (top = +infinity).
//
// The whole file (~15 MB) is read at Load; tiles (128 x 128 columns, zlib) are inflated on first
// use and kept in a small LRU cache. Queries are thread safe. Heavy users (the planner) take a
// View, which keeps the tiles it touched alive and reads them without locking.

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

class NavMap {
public:
  struct Tile {
    std::vector<uint8_t> raw;     // counts[tile * tile] then gaps
    std::vector<uint32_t> starts; // first gap of each column, tile * tile + 1 entries
    const uint16_t *gaps = nullptr;
  };

  // Planner access: integer cells, raw voxel gaps, no locking after a tile's first use.
  class View {
  public:
    explicit View(const NavMap &aMap) : map(aMap) {}
    // Gaps of cell (cx, cy) as voxel index pairs (bottom, top); count 0 = blocked / no data.
    uint32_t Column(int32_t aCx, int32_t aCy, const uint16_t *&aGaps);
    const NavMap &Map() const { return map; }

  private:
    const NavMap &map;
    std::unordered_map<uint32_t, std::shared_ptr<const Tile>> tiles;
  };

  static NavMap &Get();

  bool Load(const std::filesystem::path &aPath);
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
};

} // namespace Navigation
