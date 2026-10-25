// LTBF R41 (macOS only). The Red::Mac* functions RedLib (cp2077-codeware/lib/Red) calls on macOS. Codeware's own definitions are
// hidden inside Codeware.dylib, so LTBF carries these:
//  - MacGameInstance and MacRuntimeSystemAt ask Codeware through its two exported C functions (Codeware owns the GameInstance
//    constructor and InitEngine hooks; the game has one hook slot per target). Without Codeware both answer null.
//  - MacPublishGlobal is a copy of Codeware's (macos/MacSupport.cpp): it puts a native global into the RTTI function maps when
//    RegisterFunction did not.
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Functions.hpp>

#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

#define MAC_DIAG(...)                                                        \
    do                                                                       \
    {                                                                        \
        static const bool s_diag = std::getenv("LTBF_MAC_DIAG") != nullptr; \
        if (s_diag)                                                          \
        {                                                                    \
            std::fprintf(stderr, __VA_ARGS__);                               \
        }                                                                    \
    } while (0)

namespace Red
{
::RED4ext::GameInstance* MacGameInstance();
void* MacRuntimeSystemAt(uint32_t aIndex);
bool MacSystemEnabled(const char* aName);
bool MacInstallGameInstanceHook();
void MacMarkPostLoadScripts();
void MacPublishGlobal(::RED4ext::CRTTISystem* aRtti, const char* aName, ::RED4ext::CGlobalFunction* aFunc);
}

::RED4ext::GameInstance* Red::MacGameInstance()
{
    using Fn = ::RED4ext::GameInstance* (*)();
    static const Fn fn = reinterpret_cast<Fn>(dlsym(RTLD_DEFAULT, "codeware_mac_game_instance"));
    return fn ? fn() : nullptr;
}

void* Red::MacRuntimeSystemAt(uint32_t aIndex)
{
    using Fn = void* (*)(uint32_t);
    static const Fn fn = reinterpret_cast<Fn>(dlsym(RTLD_DEFAULT, "codeware_mac_runtime_system_at"));
    return fn ? fn(aIndex) : nullptr;
}

// Codeware decides which world runtime systems are verified on Mac; LTBF does not use RedLib's runtime-system getters, so it
// answers "off" for every system.
bool Red::MacSystemEnabled(const char*)
{
    return false;
}

// Codeware owns the GameInstance constructor hook; LTBF never installs a second one.
bool Red::MacInstallGameInstanceHook()
{
    return false;
}

void Red::MacMarkPostLoadScripts()
{
}

void Red::MacPublishGlobal(RED4ext::CRTTISystem* aRtti, const char* aName, RED4ext::CGlobalFunction* aFunc)
{
    using Map = RED4ext::HashMap<RED4ext::CName, RED4ext::CGlobalFunction*>;
    auto* map = reinterpret_cast<Map*>(reinterpret_cast<uint8_t*>(aRtti) + 0xA0);
    const auto name = RED4ext::CName(aName);
    auto plausible = [](const void* aPtr) {
        const auto value = reinterpret_cast<uintptr_t>(aPtr);
        return value > 0x100000000ULL && (value % 8) == 0;
    };
    const bool shape = map->capacity >= 16 && map->capacity <= 100000 && map->size > 0 && map->size < map->capacity &&
                       map->nodeList.stride == sizeof(typename Map::Node) && plausible(map->indexTable) &&
                       plausible(map->nodeList.nodes);
    auto* known = aRtti->GetFunction(RED4ext::CName("GetGameInstance"));
    auto** slot = shape ? map->Get(RED4ext::CName("GetGameInstance")) : nullptr;
    const bool mapOk = shape && known && slot && *slot == known;
    int inserted = 0;
    if (mapOk)
    {
        const auto pair = map->Insert(name, aFunc);
        inserted = pair.second ? 1 : 0;
    }
    auto** cnameSlot = shape ? map->Get(name) : nullptr;
    auto* found = aRtti->GetFunction(name);

    // GetFunction can miss a name that the CName map already holds. The SDK's
    // second table is HashMap<uint64_t, CGlobalFunction*> at +0xD0, keyed by the
    // CName hash. Insert only when that table resolves GetGameInstance to the
    // same pointer the virtual returns.
    using HashMap64 = RED4ext::HashMap<uint64_t, RED4ext::CGlobalFunction*>;
    auto* byHash = reinterpret_cast<HashMap64*>(reinterpret_cast<uint8_t*>(aRtti) + 0xD0);
    const bool hashShape = byHash->capacity >= 16 && byHash->capacity <= 100000 && byHash->size > 0 &&
                           byHash->size < byHash->capacity && byHash->nodeList.stride == sizeof(typename HashMap64::Node) &&
                           plausible(byHash->indexTable) && plausible(byHash->nodeList.nodes);
    const auto knownHash = static_cast<uint64_t>(RED4ext::CName("GetGameInstance"));
    auto** hashKnown = hashShape ? byHash->Get(knownHash) : nullptr;
    const bool hashOk = hashShape && known && hashKnown && *hashKnown == known;
    int hashInserted = 0;
    if (hashOk && !found)
    {
        const auto pair = byHash->Insert(static_cast<uint64_t>(name), aFunc);
        hashInserted = pair.second ? 1 : 0;
        found = aRtti->GetFunction(name);
    }
    MAC_DIAG(
                 "[cw] global %s publish shape=%d map=%d inserted=%d cname=%p func=%p found=%d hashShape=%d hashOk=%d "
                 "hashInserted=%d stride=%u size=%u cap=%u short=%llx\n",
                 aName, shape ? 1 : 0, mapOk ? 1 : 0, inserted, cnameSlot ? static_cast<void*>(*cnameSlot) : nullptr,
                 static_cast<void*>(aFunc), found ? 1 : 0, hashShape ? 1 : 0, hashOk ? 1 : 0, hashInserted,
                 map->nodeList.stride, map->size, map->capacity,
                 static_cast<unsigned long long>(aFunc ? aFunc->shortName.hash : 0));
}
