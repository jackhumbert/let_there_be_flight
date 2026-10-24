// LTBF R41: macOS shims for Let There Be Flight. See MacShims.hpp.

#include "MacShims.hpp"
#include "MacLayout.hpp"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mutex>
#include <set>
#include <string>

#include <RED4ext/Api/v0/PluginInfo.hpp>
#include <spdlog/spdlog.h>

namespace
{
int g_anchor = 0;
std::atomic<bool> g_trapCalled{false};

// Function-local statics: the hook registrations (FlightModuleHookHash) resolve their hashes during static initialization of other
// files, which can run before this file's globals are constructed.
std::mutex& LogLock()
{
    static std::mutex s_lock;
    return s_lock;
}

std::set<uint32_t>& TrappedHashes()
{
    static std::set<uint32_t> s_set;
    return s_set;
}

std::set<std::string>& Skipped()
{
    static std::set<std::string> s_set;
    return s_set;
}

__attribute__((noinline)) uintptr_t ltbf_mac_trap_stub()
{
    if (!g_trapCalled.exchange(true))
    {
        LtbfMac::Log("[ltbf] trap stub called (a function with no Mac address yet); it returns 0. Further calls are not logged.");
    }
    return 0;
}

// The game classes behind the fork's "VFT hash + Windows slot" base calls, on macOS: each class's own vtable (R41b section 4, read
// from the class constructors). ISerializable-derived vtables keep slots 0x00..0x10; the destructor takes two slots on macOS, so
// every slot from Windows 0x20 on is Windows + 8 (R40, "Vtable slots").
constexpr uint32_t kVftIScriptable = 1180173983;
constexpr uint32_t kVftIComponent = 1586431952;
constexpr uint32_t kVftGameComponent = 331746781;
constexpr uintptr_t kVtableIComponent = 0x78C6FB0;     // entIComponent (constructor 0xCA5E34)
constexpr uintptr_t kVtableGameComponent = 0x7862FB8;  // gameComponent (constructor 0x1F2A148)
// IScriptable has three candidate macOS vtables (R41b); its one base call LTBF makes (slot 0xD8) is Codeware's row
// IScriptable_sub_D8 (4175699423).
constexpr uintptr_t kIScriptableSubD8 = 0x11e4da0;

const char* PluginPathC()
{
    Dl_info info{};
    if (dladdr(&g_anchor, &info) && info.dli_fname)
    {
        return info.dli_fname;
    }
    return nullptr;
}
} // namespace

extern "C" __attribute__((visibility("hidden"))) uintptr_t ltbf_mac_trap_for_hash(uint32_t aHash)
{
    {
        std::scoped_lock lock(LogLock());
        if (TrappedHashes().insert(aHash).second)
        {
            std::fprintf(stderr, "[ltbf] address hash %u has no Mac row: it resolves to the fail-safe trap stub (see macos/NOT_YET.md)\n",
                         aHash);
        }
    }
    return reinterpret_cast<uintptr_t>(&ltbf_mac_trap_stub);
}

extern "C" __attribute__((visibility("hidden"))) uintptr_t ltbf_mac_vft_slot(uint32_t aVftHash, uintptr_t aWindowsOffset)
{
    uintptr_t vtable = 0;
    if (aVftHash == kVftIComponent)
    {
        vtable = kVtableIComponent;
    }
    else if (aVftHash == kVftGameComponent)
    {
        vtable = kVtableGameComponent;
    }
    else if (aVftHash == kVftIScriptable && aWindowsOffset == 0xD8)
    {
        return LtbfMac::ImageBase() + kIScriptableSubD8;
    }
    if (!vtable || aWindowsOffset == 0x18 || (aWindowsOffset & 7) != 0)
    {
        LtbfMac::Log("[ltbf] base call VFT %u slot 0x%lx has no macOS function (macos/MacShims.cpp): trap stub", aVftHash,
                     static_cast<unsigned long>(aWindowsOffset));
        return reinterpret_cast<uintptr_t>(&ltbf_mac_trap_stub);
    }
    const uintptr_t macSlot = aWindowsOffset < 0x18 ? aWindowsOffset : aWindowsOffset + 8;
    // The vtable is in __DATA_CONST; dyld has applied its fixups before any plugin code runs, so the entries are absolute.
    const uintptr_t fn = *reinterpret_cast<const uintptr_t*>(LtbfMac::ImageBase() + vtable + macSlot);
    LtbfMac::Log("[ltbf] base call VFT %u Windows slot 0x%lx = macOS slot 0x%lx -> rva 0x%lx", aVftHash,
                 static_cast<unsigned long>(aWindowsOffset), static_cast<unsigned long>(macSlot),
                 static_cast<unsigned long>(fn - LtbfMac::ImageBase()));
    return fn;
}

// LTBF R42g: the game's gameComponent::GetAllocator (macOS vtable slot 0x10 -> 0x9666A0 `b 0x103DAAD70`, which returns the
// function-static allocator at 0x1089454A0 and never reads `this`). FlightComponent's GetAllocator (game::Component overlay) calls
// the same function, so the class side (Engine::RTTIClassDescriptor) can allocate from exactly the pool the game frees to.
extern "C" __attribute__((visibility("hidden"))) RED4ext::Memory::IAllocator* ltbf_mac_game_component_allocator()
{
    using Fn = RED4ext::Memory::IAllocator* (*)(void*);
    static const auto s_fn = reinterpret_cast<Fn>(ltbf_mac_vft_slot(kVftGameComponent, 0x10));
    static const auto s_allocator = s_fn ? s_fn(nullptr) : nullptr;
    static const bool s_logged = [] {
        LtbfMac::Log("[ltbf] game component allocator: %p (gameComponent vtable slot 0x10)", static_cast<void*>(s_allocator));
        return true;
    }();
    (void)s_logged;
    return s_allocator ? s_allocator : RED4ext::Memory::ScriptAllocator::Get();
}

// CRUID::Next on macOS (the SDK overlay's NativeTypes.hpp calls this). Windows inlines (InterlockedIncrement64(counter) * 4) | 1 on
// the global behind hash 2027097961; macOS has that global at 0x8DC7EB0 and an accessor, 0x91248C, that returns the same value
// (`ldaddal` on the global, then `(old + 1) * 4 | 1`). Calling the accessor keeps LTBF on the game's own counter (R41b section 2).
extern "C" __attribute__((visibility("hidden"))) int64_t ltbf_mac_cruid_next()
{
    using Next_t = uint64_t (*)();
    static const auto s_next = reinterpret_cast<Next_t>(LtbfMac::ImageBase() + LTBF_MAC_CRUID_NEXT_RVA);
    return static_cast<int64_t>(s_next());
}

namespace LtbfMac
{
uintptr_t ImageBase()
{
    static const uintptr_t base = []() -> uintptr_t {
        const uint32_t count = _dyld_image_count();
        for (uint32_t i = 0; i < count; ++i)
        {
            const auto* header = reinterpret_cast<const mach_header_64*>(_dyld_get_image_header(i));
            if (header && header->filetype == MH_EXECUTE)
            {
                return reinterpret_cast<uintptr_t>(header);
            }
        }
        return reinterpret_cast<uintptr_t>(_dyld_get_image_header(0));
    }();
    return base;
}

bool IsTrap(uintptr_t aAddress)
{
    return aAddress == reinterpret_cast<uintptr_t>(&ltbf_mac_trap_stub) || aAddress == 0;
}

std::filesystem::path PluginDir()
{
    const char* path = PluginPathC();
    return path ? std::filesystem::path(path).parent_path() : std::filesystem::path();
}

std::filesystem::path RootDir()
{
    // <root>/red4ext/plugins/let_there_be_flight/let_there_be_flight.dylib: three levels up from the plugin folder.
    return PluginDir().parent_path().parent_path().parent_path();
}

void Log(const char* aFormat, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, aFormat);
    std::vsnprintf(buffer, sizeof(buffer), aFormat, args);
    va_end(args);
    std::fprintf(stderr, "%s\n", buffer);
    if (spdlog::default_logger_raw())
    {
        spdlog::info("{}", buffer);
    }
}

void SkipOnce(const char* aPlaceholder, const char* aFeature)
{
    {
        std::scoped_lock lock(LogLock());
        if (!Skipped().insert(aPlaceholder).second)
        {
            return;
        }
    }
    Log("[ltbf] %s skipped: waits on %s (R41b). Logged once.", aFeature, aPlaceholder);
}

namespace
{
const RED4ext::Sdk* g_sdk = nullptr;
RED4ext::PluginHandle g_handle = nullptr;
}

void SetPlugin(const RED4ext::Sdk* aSdk, RED4ext::PluginHandle aHandle)
{
    g_sdk = aSdk;
    g_handle = aHandle;
}

const RED4ext::Sdk* PluginSdk()
{
    return g_sdk;
}

RED4ext::PluginHandle PluginHandleValue()
{
    return g_handle;
}

bool IsCodewareShared(uintptr_t aRva)
{
    return aRva == 0x19AA7D8 || aRva == 0x11A7E28;
}

const char* HookResultName(HookResult aResult)
{
    switch (aResult)
    {
    case HookResult::Installed:
        return "INSTALLED";
    case HookResult::Chained:
        return "CHAINED";
    case HookResult::NotActive:
        return "NOT ACTIVE (offset is not on the RED4ext allowlist, macos/hook-offsets.txt)";
    case HookResult::Refused:
        return "REFUSED";
    case HookResult::Failed:
        return "FAILED";
    }
    return "?";
}

HookResult AttachHook(const RED4ext::Sdk* aSdk, RED4ext::PluginHandle aHandle, uintptr_t aTarget, void* aDetour, void** aOriginal,
                      const char* aName, bool aSharedWithCodeware)
{
    if (aOriginal)
    {
        *aOriginal = nullptr;
    }
    // R42f (diagnostics): LTBF_MAC_NOHOOKS=all, or a comma list of hook names (substring match), leaves those hooks out, to find
    // which one corrupts memory in world (Boot C/C2/C3). Unset = every hook as before.
    if (const char* skip = std::getenv("LTBF_MAC_NOHOOKS"))
    {
        if (std::strcmp(skip, "all") == 0 || std::strstr(skip, aName) != nullptr)
        {
            Log("[ltbf] hook %s SKIPPED by LTBF_MAC_NOHOOKS=%s", aName, skip);
            return HookResult::Refused;
        }
    }
    if (IsTrap(aTarget))
    {
        Log("[ltbf] hook %s REFUSED: its address hash has no Mac row (fail-safe, feature skipped)", aName);
        return HookResult::Refused;
    }
    const auto rva = static_cast<unsigned long>(aTarget - ImageBase());
    using PeerAttach = bool (*)(uintptr_t, void*, void**);
    if (aSharedWithCodeware)
    {
        if (auto fn = reinterpret_cast<PeerAttach>(dlsym(RTLD_DEFAULT, "codeware_mac_hook_chain_attach")))
        {
            if (fn(aTarget, aDetour, aOriginal))
            {
                Log("[ltbf] hook %s rva=0x%lx CHAINED on codeware_mac_hook_chain_attach", aName, rva);
                return HookResult::Chained;
            }
            if (aOriginal)
            {
                *aOriginal = nullptr;
            }
            Log("[ltbf] hook %s rva=0x%lx REFUSED: Codeware is loaded but does not own this target, so it cannot be chained and a "
                "direct attach would race it. The feature is skipped.",
                aName, rva);
            return HookResult::Refused;
        }
        Log("[ltbf] hook %s rva=0x%lx: Codeware is not loaded, attaching directly", aName, rva);
    }
    else
    {
        for (const char* peer : {"codeware_mac_hook_chain_attach", "archivexl_mac_hook_chain_attach"})
        {
            if (auto fn = reinterpret_cast<PeerAttach>(dlsym(RTLD_DEFAULT, peer)))
            {
                if (fn(aTarget, aDetour, aOriginal))
                {
                    Log("[ltbf] hook %s rva=0x%lx CHAINED on %s", aName, rva, peer);
                    return HookResult::Chained;
                }
                if (aOriginal)
                {
                    *aOriginal = nullptr;
                }
            }
        }
    }
    if (!aSdk || !aSdk->hooking || !aSdk->hooking->Attach)
    {
        Log("[ltbf] hook %s rva=0x%lx FAILED: no RED4ext hooking interface", aName, rva);
        return HookResult::Failed;
    }
    // One attempt (the Windows build retries forever).
    if (!aSdk->hooking->Attach(aHandle, reinterpret_cast<void*>(aTarget), aDetour, aOriginal))
    {
        if (aOriginal)
        {
            *aOriginal = nullptr;
        }
        Log("[ltbf] hook %s rva=0x%lx FAILED (RED4ext refused the attach; skipped)", aName, rva);
        return HookResult::Failed;
    }
    if (aOriginal && *aOriginal == reinterpret_cast<void*>(aTarget))
    {
        // RED4ext-macos reports success for an offset that is not on RED4EXT_GUM_HOOK_OFFSETS and hands back the target itself.
        *aOriginal = nullptr;
        Log("[ltbf] hook %s rva=0x%lx %s", aName, rva, HookResultName(HookResult::NotActive));
        return HookResult::NotActive;
    }
    Log("[ltbf] hook %s rva=0x%lx INSTALLED", aName, rva);
    return HookResult::Installed;
}

DepStatus HasDependency(const char* aName, RED4ext::SemVer aMinVersion, bool aAllowNotLoaded)
{
    using Query_t = void (*)(RED4ext::PluginInfo*);
    // RED4ext loads every plugin before Main runs, so a dependency is an already loaded image.
    const auto expected = std::string("/red4ext/plugins/") + aName + "/" + aName + ".dylib";
    void* handle = nullptr;
    const uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count && !handle; ++i)
    {
        const char* path = _dyld_get_image_name(i);
        if (path && std::strlen(path) >= expected.size() &&
            std::strcmp(path + std::strlen(path) - expected.size(), expected.c_str()) == 0)
        {
            handle = dlopen(path, RTLD_LAZY | RTLD_NOLOAD);
        }
    }
    if (!handle)
    {
        const auto path = RootDir() / "red4ext" / "plugins" / aName / (std::string(aName) + ".dylib");
        handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_NOLOAD);
    }
    if (!handle)
    {
        const auto path = RootDir() / "red4ext" / "plugins" / aName / (std::string(aName) + ".dylib");
        std::error_code error;
        if (aAllowNotLoaded && std::filesystem::is_regular_file(path, error))
        {
            Log("[ltbf] %s present, loads later (RED4ext loads the plugins in folder order); its version is checked in "
                "ltbf_registerNatives",
                aName);
            return DepStatus::PresentNotLoaded;
        }
        Log("[ltbf] %s %s", aName, aAllowNotLoaded ? "not found" : "is not loaded");
        return DepStatus::NotFound;
    }
    if (aMinVersion > RED4EXT_SEMVER(0, 0, 0))
    {
        auto query = reinterpret_cast<Query_t>(dlsym(handle, "Query"));
        if (!query)
        {
            Log("[ltbf] %s found, but could not be queried for version", aName);
            return DepStatus::FoundCouldNotBeQueried;
        }
        RED4ext::PluginInfo info{};
        query(&info);
        const bool ok = info.version >= aMinVersion;
        Log("[ltbf] %s found with version %u.%u.%u%s", aName, info.version.major, info.version.minor, info.version.patch,
            ok ? "" : " (too old)");
        return ok ? DepStatus::FoundWithCorrectVersion : DepStatus::FoundWithIncorrectVersion;
    }
    Log("[ltbf] %s found", aName);
    return DepStatus::FoundNoVersion;
}
} // namespace LtbfMac
