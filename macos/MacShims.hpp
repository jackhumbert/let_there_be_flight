#pragma once

// LTBF R41: the macOS layer of Let There Be Flight (everything Windows-only in the plugin has an __APPLE__ branch that calls here).

#include <cstdint>
#include <dlfcn.h>
#include <filesystem>
#include <string>

#include <RED4ext/Api/PluginHandle.hpp>
#include <RED4ext/Api/Sdk.hpp>

extern "C"
{
// Fail-safe for an address hash with no Mac row: logs the hash once and returns a stub that logs its first call and returns 0.
uintptr_t ltbf_mac_trap_for_hash(uint32_t aHash);
// The Mac function behind a fork "VFT hash + Windows slot" base call (macos/MacShims.cpp table), or the trap stub.
uintptr_t ltbf_mac_vft_slot(uint32_t aVftHash, uintptr_t aWindowsOffset);
}

namespace LtbfMac
{
// The game image base (the MH_EXECUTE header), as the SDK's RelocBase::GetImageBase.
uintptr_t ImageBase();

// True when the address is the trap stub (an unmapped hash).
bool IsTrap(uintptr_t aAddress);

// The plugin folder (red4ext/plugins/let_there_be_flight) and the game root, both through dladdr.
std::filesystem::path PluginDir();
std::filesystem::path RootDir();

// Writes a line to stderr (the launcher log) and to the plugin log once spdlog is up. Used before spdlog exists, too.
void Log(const char* aFormat, ...) __attribute__((format(printf, 1, 2)));

// The plugin's RED4ext handle and SDK, saved by Main(Load) for ltbf_registerNatives.
void SetPlugin(const RED4ext::Sdk* aSdk, RED4ext::PluginHandle aHandle);
const RED4ext::Sdk* PluginSdk();
RED4ext::PluginHandle PluginHandleValue();

// Hook targets Codeware hooks too (macOS RED4ext accepts one hook per target): Entity_Initialize 0xC985EC and
// ScriptValidator_Validate 0x21FBF90. LTBF attaches these from ltbf_registerNatives through Codeware's chain only.
bool IsCodewareShared(uintptr_t aRva);

enum class HookResult
{
    Installed,
    Chained,
    NotActive, // RED4ext-macos accepted the attach but the offset is not on RED4EXT_GUM_HOOK_OFFSETS (no-op)
    Refused,   // no Mac address, or Codeware owns the target and refused the chain
    Failed
};
const char* HookResultName(HookResult aResult);

// One hook attach attempt with a log line (the policy of Mod Settings' macos/MacShims.cpp):
//  - a target shared with Codeware goes through codeware_mac_hook_chain_attach only; if Codeware is loaded and refuses, the hook is
//    REFUSED and the feature is skipped; a direct attach happens only when Codeware is not loaded at all;
//  - any other target asks Codeware and ArchiveXL first (a plugin that does not own the target answers false), then RED4ext once.
// *aOriginal is null unless the result is Installed or Chained.
HookResult AttachHook(const RED4ext::Sdk* aSdk, RED4ext::PluginHandle aHandle, uintptr_t aTarget, void* aDetour, void** aOriginal,
                      const char* aName, bool aSharedWithCodeware);

// Dependency check on macOS: a loaded image whose path ends with red4ext/plugins/<name>/<name>.dylib (or dlopen(RTLD_NOLOAD) of
// that file), then dlsym("Query"). Same result codes as the Windows HasDependency, plus PresentNotLoaded: RED4ext-macos loads the
// plugins one by one in folder order and calls each Main right away, so a dependency in a later folder is not loaded yet when
// LTBF's Main runs. With aAllowNotLoaded, a dependency whose dylib exists but is not loaded is PresentNotLoaded (its version is
// checked again in ltbf_registerNatives, when every plugin is loaded). Nothing is ever dlopened that is not already loaded.
enum class DepStatus
{
    FoundWithCorrectVersion = 0,
    FoundWithIncorrectVersion = 1,
    FoundCouldNotBeQueried = 2,
    FoundNoVersion = 3,
    NotFound = 4,
    PresentNotLoaded = 5
};
DepStatus HasDependency(const char* aName, RED4ext::SemVer aMinVersion, bool aAllowNotLoaded);

// A one-time log line for a feature that waits on an R41b address or offset (LTBF_MAC_TODO_R41b_* placeholders).
void SkipOnce(const char* aPlaceholder, const char* aFeature);
}
