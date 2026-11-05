#pragma once

// A small nav map made in memory for tests, in the format NavMap::Load reads (tools/common/navmap.py):
// a header, the tile index, and each tile as zlib "stored" blocks (no compression, which tinfl reads).
// The world is a flat floor with blocks standing on it; columns are 2 m, voxels 0.5 m.

#include "Navigation/NavMap.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>

namespace NavFixture {

struct Block {
  float x0, y0, x1, y1; // metres, the column centres inside it are blocked up to top
  float top;            // metres; a gap's bottom (car centre) above it is top + halfHeight
};

struct Spec {
  float originX = 0.0f, originY = 0.0f;
  uint32_t width = 128, height = 128; // cells of 2 m
  float z0 = 0.0f;                    // the floor surface
  uint32_t nz = 400;                  // voxels of 0.5 m above z0
  float radius = 2.0f, halfHeight = 1.0f;
  std::vector<Block> blocks;
};

namespace detail {
inline void Put(std::vector<uint8_t> &aOut, const void *aData, size_t aSize) {
  const auto *p = static_cast<const uint8_t *>(aData);
  aOut.insert(aOut.end(), p, p + aSize);
}

// aRaw as a zlib stream of stored blocks
inline std::vector<uint8_t> Stored(const std::vector<uint8_t> &aRaw) {
  std::vector<uint8_t> out{0x78, 0x01};
  size_t at = 0;
  do {
    const uint16_t len = uint16_t(std::min<size_t>(65535, aRaw.size() - at));
    const bool last = at + len >= aRaw.size();
    out.push_back(last ? 1 : 0);
    const uint16_t nlen = uint16_t(~len);
    Put(out, &len, 2);
    Put(out, &nlen, 2);
    out.insert(out.end(), aRaw.begin() + at, aRaw.begin() + at + len);
    at += len;
  } while (at < aRaw.size());
  uint32_t a = 1, b = 0;
  for (uint8_t c : aRaw) {
    a = (a + c) % 65521;
    b = (b + a) % 65521;
  }
  const uint32_t adler = (b << 16) | a;
  const uint8_t be[4] = {uint8_t(adler >> 24), uint8_t(adler >> 16), uint8_t(adler >> 8), uint8_t(adler)};
  Put(out, be, 4);
  return out;
}
} // namespace detail

// The map's bytes: each column one gap from the floor (or a block's top) plus halfHeight to open sky.
inline std::vector<uint8_t> Make(const Spec &aSpec) {
  constexpr uint32_t kTile = 128;
  const float cell = 2.0f, vz = 0.5f;
  const uint32_t tilesX = (aSpec.width + kTile - 1) / kTile, tilesY = (aSpec.height + kTile - 1) / kTile;
#pragma pack(push, 1)
  struct Header {
    char magic[8];
    uint32_t version, flags;
    float originX, originY, cell, z0, vz;
    uint32_t nz, width, height, tile, tilesX, tilesY;
    float radius, halfHeight;
  } h{};
  struct Entry {
    uint64_t offset;
    uint32_t packedSize, rawSize;
  };
#pragma pack(pop)
  std::memcpy(h.magic, "LTBFNAV1", 8);
  h.version = 1;
  h.flags = 1;
  h.originX = aSpec.originX;
  h.originY = aSpec.originY;
  h.cell = cell;
  h.z0 = aSpec.z0;
  h.vz = vz;
  h.nz = aSpec.nz;
  h.width = aSpec.width;
  h.height = aSpec.height;
  h.tile = kTile;
  h.tilesX = tilesX;
  h.tilesY = tilesY;
  h.radius = aSpec.radius;
  h.halfHeight = aSpec.halfHeight;

  std::vector<std::vector<uint8_t>> packed;
  std::vector<uint32_t> rawSizes;
  for (uint32_t ty = 0; ty < tilesY; ++ty) {
    for (uint32_t tx = 0; tx < tilesX; ++tx) {
      std::vector<uint8_t> counts(kTile * kTile, 0);
      std::vector<uint16_t> gaps;
      for (uint32_t ly = 0; ly < kTile; ++ly) {
        for (uint32_t lx = 0; lx < kTile; ++lx) {
          const uint32_t cx = tx * kTile + lx, cy = ty * kTile + ly;
          if (cx >= aSpec.width || cy >= aSpec.height) {
            continue;
          }
          const float x = aSpec.originX + (float(cx) + 0.5f) * cell, y = aSpec.originY + (float(cy) + 0.5f) * cell;
          float ground = aSpec.z0;
          for (const auto &b : aSpec.blocks) {
            if (x >= b.x0 && x <= b.x1 && y >= b.y0 && y <= b.y1) {
              ground = std::max(ground, b.top);
            }
          }
          const float bottom = ground + aSpec.halfHeight;
          const uint16_t lo = uint16_t(std::ceil((bottom - aSpec.z0) / vz - 1e-4f));
          if (lo >= aSpec.nz) {
            continue;
          }
          counts[ly * kTile + lx] = 1;
          gaps.push_back(lo);
          gaps.push_back(uint16_t(aSpec.nz));
        }
      }
      std::vector<uint8_t> raw(counts);
      detail::Put(raw, gaps.data(), gaps.size() * 2);
      rawSizes.push_back(uint32_t(raw.size()));
      packed.push_back(detail::Stored(raw));
    }
  }
  std::vector<uint8_t> out;
  detail::Put(out, &h, sizeof(h));
  uint64_t offset = sizeof(h) + packed.size() * sizeof(Entry);
  for (size_t i = 0; i < packed.size(); ++i) {
    Entry e{offset, uint32_t(packed[i].size()), rawSizes[i]};
    detail::Put(out, &e, sizeof(e));
    offset += packed[i].size();
  }
  for (const auto &p : packed) {
    out.insert(out.end(), p.begin(), p.end());
  }
  return out;
}

} // namespace NavFixture
