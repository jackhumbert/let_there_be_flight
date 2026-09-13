#pragma once
#include <filesystem>
#include <cstdint>
#include <RED4ext/DynArray.hpp>
#include <spdlog/spdlog.h>

// Log a warning the first time a given site is hit, then stay quiet.
#define LTBF_WARN_ONCE(...)                                                                                          \
  do {                                                                                                               \
    static bool s_ltbfWarned = false;                                                                                \
    if (!s_ltbfWarned) {                                                                                             \
      s_ltbfWarned = true;                                                                                           \
      spdlog::warn(__VA_ARGS__);                                                                                     \
    }                                                                                                                \
  } while (0)

namespace Utils
{
void CreateLogger();
std::filesystem::path GetRootDir();
std::wstring ToWString(const char* aText);

// Cheap sanity check on a DynArray header that we reach through a hand-written
// struct layout. If the layout drifted after a game patch the header is garbage
// (the engine then asserts with "Detected integer overflow for offset" in
// dynamicBuffer.cpp), so refuse to read or write through it.
template <typename T>
inline bool LooksLikeDynArray(const RED4ext::DynArray<T>& aArray, uint32_t aMaxCapacity = 0x10000)
{
    if (aArray.size > aArray.capacity)
        return false;
    if (aArray.capacity > aMaxCapacity)
        return false;
    if (aArray.capacity != 0 && aArray.entries == nullptr)
        return false;
    return true;
}
}
