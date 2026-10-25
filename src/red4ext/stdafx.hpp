#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#ifdef __APPLE__
#include "MacShims.hpp" // LTBF R41: no Windows.h, TlHelp32 or Detours on macOS
// The Windows build gets the core RED4ext types into every file through the jackhumbert SDK headers' own include chains; the
// macOS SDK's headers include less, so the precompiled header carries them.
#include <RED4ext/RED4ext.hpp>
#include "LtbfMacPrelude.hpp"
#else
#include <Windows.h>
#include <TlHelp32.h>
#include <detours.h>
#endif

#include <RED4ext/Common.hpp>
#include <spdlog/spdlog.h>

#ifdef __APPLE__
// LTBF R42f (macOS): the macOS SDK defines RED4EXT_ASSERT_OFFSET as nothing under clang, so LTBF's own layout checks (the offsets its
// scripts use through @runtimeProperty("offset", ...)) never ran. clang does accept offsetof in static_assert; from here on (LTBF's own
// headers, after the SDK in this precompiled header) the checks are real again.
#undef RED4EXT_ASSERT_OFFSET
#define RED4EXT_ASSERT_OFFSET(cls, mbr, offset)                                                                                    \
  _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Winvalid-offsetof\"")                                       \
  static_assert(offsetof(cls, mbr) == offset, #cls "::" #mbr " is not on the expected offset (" #offset ")");                   \
  _Pragma("clang diagnostic pop")
#endif
