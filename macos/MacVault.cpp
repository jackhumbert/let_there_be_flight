// LTBF R41 (macOS only). Let There Be Flight's own copy of the SDK stand-ins every Mac plugin carries (the memory vault, the
// IScriptable vtable slots, the RTTI async id and serializable counters), copied from cp2077-codeware/macos/MacSupport.cpp (the part
// before its own Codeware code; itself a copy of TweakXL's MacRegisterNatives.cpp). Symbols stay hidden so each dylib binds its own
// copy. Changed: the diagnostics macro, the image name in the vslot check, and the id bands (disjoint from TweakXL, Codeware and
// ArchiveXL). The pool RVAs are the game's PoolStorageProxy exports.
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Memory/Allocators.hpp>
#include <RED4ext/RTTISystem.hpp>

#include <cstdlib>

#define MAC_DIAG(...)                                       \
    do                                                      \
    {                                                       \
        static const bool s_diag = std::getenv("LTBF_MAC_DIAG") != nullptr; \
        if (s_diag)                                         \
        {                                                   \
            std::fprintf(stderr, __VA_ARGS__);              \
        }                                                   \
    } while (0)



#include <RED4ext/Hashing/FNV1a.hpp>
#include <RED4ext/Memory/Vault.hpp>

#include <atomic>
#include <thread>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>

// Local stand-in. TweakXL 0x7FFF0000, Codeware 0x7FFE0000, ArchiveXL 0x7FFD0000; LTBF takes 0x7FFC0000.
extern "C" __attribute__((visibility("hidden"))) uint32_t tweakxl_rtti_async_id = 0x7FFC0000;

// Local stand-in. TweakXL 1<<62, Codeware +1<<61, ArchiveXL +1<<60; LTBF one band further (+1<<59).
extern "C" __attribute__((visibility("hidden"))) volatile int64_t tweakxl_serializable_counter =
    (1LL << 62) + (1LL << 61) + (1LL << 60) + (1LL << 59);

namespace
{
// Exported PoolStorageProxy functions. Static. Block comes back in x0/x1.
// dlsym wants one leading underscore. dyld_info prints two.
constexpr uint64_t kTagRtti = 0x10;
constexpr uint64_t kTagFunction = 0x20;
constexpr uint64_t kTagProperty = 0x30;
// R14. Scripted classes live in PoolScript. A C++ Handle to a scripted instance (a ScriptableTweak or a
// ScriptableService subclass written in redscript) frees it through Allocator<PoolScript>, which looks the pool up
// in this vault. Without the entry the lookup returns null and the free faults at +0x10.
constexpr uint64_t kTagScript = 0x40;
// R42c (from ArchiveXL R22). Engine containers allocate from the game's own pools: Memory::EngineAllocator from PoolEngine,
// reference counts (MakeShared, Handle) from PoolRefCount, DefaultAllocator users (HashMap, DynArray) from PoolDefault. A pool missing
// from the vault is a null lookup and a fault at +0x10 (Boot B3: FlightSettings::SetVector3 -> HashMap::Reserve).
constexpr uint64_t kTagEngine = 0x50;
constexpr uint64_t kTagRefCount = 0x60;
constexpr uint64_t kTagDefault = 0x70;

struct GameBlock
{
    void* memory;
    uint64_t size;
};

struct PoolOps
{
    const char* name;
    uint64_t tag;
    uintptr_t expect[5];
    GameBlock (*alloc)(uint64_t);
    GameBlock (*allocAligned)(uint64_t, uint32_t);
    GameBlock (*realloc)(GameBlock&, uint64_t);
    GameBlock (*reallocAligned)(GameBlock&, uint64_t, uint32_t);
    void (*freeFn)(GameBlock&);
};

PoolOps g_pools[] = {
    {"8PoolRTTI", kTagRtti, {0x2177D04, 0x2177E28, 0x2177F68, 0x21780B4, 0x217819C}, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"16PoolRTTIFunction", kTagFunction, {0x217844C, 0x2178570, 0x21786B0, 0x21787FC, 0x21788E4}, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"16PoolRTTIProperty", kTagProperty, {0x2178B94, 0x2178CB8, 0x2178DF8, 0x2178F44, 0x217902C}, nullptr, nullptr, nullptr, nullptr, nullptr},
    // R14. Exports checked with nm: Allocate 0x344DCAC, AllocateAligned 0x344DDD0, Reallocate 0x344DF10,
    // ReallocateAligned 0x344E05C, Free 0x344E144.
    {"10PoolScript", kTagScript, {0x344DCAC, 0x344DDD0, 0x344DF10, 0x344E05C, 0x344E144}, nullptr, nullptr, nullptr, nullptr, nullptr},
    // R42c (ArchiveXL R22 rows). Exports checked with nm: PoolEngine 0x1F8F8.., PoolRefCount 0x20040.., PoolDefault 0x2263C..
    {"10PoolEngine", kTagEngine, {0x1F8F8, 0x1FA1C, 0x1FB5C, 0x1FCA8, 0x1FD90}, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"12PoolRefCount", kTagRefCount, {0x20040, 0x20164, 0x202A4, 0x203F0, 0x204D8}, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"11PoolDefault", kTagDefault, {0x2263C, 0x22808, 0x229DC, 0x22BC8, 0x22CB0}, nullptr, nullptr, nullptr, nullptr, nullptr},
};

const char* kPoolSuffixes[5] = {
    "8AllocateEy",
    "15AllocateAlignedEyj",
    "10ReallocateERNS0_5BlockEy",
    "17ReallocateAlignedERNS0_5BlockEyj",
    "4FreeERNS0_5BlockE",
};

bool g_vaultSymbolsOk = false;

RED4ext::Memory::Vault g_vault;
RED4ext::Memory::PoolStorage g_rttiStorage;
RED4ext::Memory::PoolStorage g_functionStorage;
RED4ext::Memory::PoolStorage g_propertyStorage;
RED4ext::Memory::PoolStorage g_scriptStorage; // R14
RED4ext::Memory::PoolStorage g_engineStorage;   // R42c
RED4ext::Memory::PoolStorage g_refCountStorage; // R42c
RED4ext::Memory::PoolStorage g_defaultStorage;  // R42c

// R42c (from ArchiveXL R22, group c). Every pool the game exports gets a vault node (macos/MacPools.inc, written by ArchiveXL's
// tools/mac/gen-pools.py from the game executable); its five functions are resolved with dlsym once, at the first use of the pool.
struct GeneratedPool
{
    const char* name;
    const char* type; // mangled template argument, closing E included
};

constexpr GeneratedPool kGeneratedPools[] = {
#include "MacPools.inc"
};
constexpr size_t kGeneratedCount = sizeof(kGeneratedPools) / sizeof(kGeneratedPools[0]);
constexpr uint64_t kGeneratedTagBase = 0x1000;
constexpr uint64_t kGeneratedTagStep = 0x10;
constexpr uint32_t kFirstGeneratedNode = 7;

RED4ext::Memory::PoolStorage g_generatedStorage[kGeneratedCount];
PoolOps g_generatedOps[kGeneratedCount];
std::atomic<uint8_t> g_generatedState[kGeneratedCount]; // 0 not resolved, 1 resolving, 2 ready
bool g_vaultReady = false;

uintptr_t GameBase()
{
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
}

void FillPool(uint32_t aIndex, const char* aName, RED4ext::Memory::PoolStorage& aStorage, uint64_t aTag)
{
    auto& node = g_vault.poolRegistry.nodes[aIndex];
    std::memset(&node, 0, sizeof(node));
    node.handle = RED4ext::FNV1a32(aName);
    node.storage = &aStorage;
    aStorage.allocatorStorage = aTag;
    std::strncpy(node.name, aName, sizeof(node.name) - 1);
}

bool AddressInImage(uint32_t aIndex, uintptr_t aAddr)
{
    const auto* header = reinterpret_cast<const mach_header_64*>(_dyld_get_image_header(aIndex));
    if (!header)
    {
        return false;
    }
    const intptr_t slide = _dyld_get_image_vmaddr_slide(aIndex);
    auto* cmd = reinterpret_cast<const load_command*>(header + 1);
    for (uint32_t i = 0; i < header->ncmds; ++i)
    {
        if (cmd->cmd == LC_SEGMENT_64)
        {
            const auto* seg = reinterpret_cast<const segment_command_64*>(cmd);
            const auto start = static_cast<uintptr_t>(seg->vmaddr) + static_cast<uintptr_t>(slide);
            const auto end = start + seg->vmsize;
            if (aAddr >= start && aAddr < end)
            {
                return true;
            }
        }
        cmd = reinterpret_cast<const load_command*>(reinterpret_cast<const uint8_t*>(cmd) + cmd->cmdsize);
    }
    return false;
}

int ImageIndex(const char* aNamePart, uint32_t aFileType)
{
    const uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i)
    {
        const auto* header = reinterpret_cast<const mach_header_64*>(_dyld_get_image_header(i));
        const char* name = _dyld_get_image_name(i);
        if (!header)
        {
            continue;
        }
        if (aFileType != 0 && header->filetype != aFileType)
        {
            continue;
        }
        if (aNamePart && (!name || !std::strstr(name, aNamePart)))
        {
            continue;
        }
        return static_cast<int>(i);
    }
    return -1;
}

void InitVault()
{
    if (g_vaultReady)
    {
        return;
    }
    std::memset(&g_vault, 0, sizeof(g_vault));
    std::memset(&g_rttiStorage, 0, sizeof(g_rttiStorage));
    std::memset(&g_functionStorage, 0, sizeof(g_functionStorage));
    std::memset(&g_propertyStorage, 0, sizeof(g_propertyStorage));
    std::memset(&g_scriptStorage, 0, sizeof(g_scriptStorage)); // R14
    std::memset(&g_engineStorage, 0, sizeof(g_engineStorage));     // R42c
    std::memset(&g_refCountStorage, 0, sizeof(g_refCountStorage)); // R42c
    std::memset(&g_defaultStorage, 0, sizeof(g_defaultStorage));   // R42c
    FillPool(0, "PoolRTTI", g_rttiStorage, kTagRtti);
    FillPool(1, "PoolRTTIFunction", g_functionStorage, kTagFunction);
    FillPool(2, "PoolRTTIProperty", g_propertyStorage, kTagProperty);
    FillPool(3, "PoolScript", g_scriptStorage, kTagScript); // R14
    FillPool(4, "PoolEngine", g_engineStorage, kTagEngine);       // R42c
    FillPool(5, "PoolRefCount", g_refCountStorage, kTagRefCount); // R42c
    FillPool(6, "PoolDefault", g_defaultStorage, kTagDefault);    // R42c

    // R42c. Generated pools: registry nodes only; the functions are resolved at the first allocation (ResolveGenerated).
    std::memset(g_generatedStorage, 0, sizeof(g_generatedStorage));
    {
        uint32_t node = kFirstGeneratedNode;
        for (size_t i = 0; i < kGeneratedCount && node < RED4ext::Memory::PoolRegistry::MaxPoolCount; ++i)
        {
            const char* name = kGeneratedPools[i].name;
            bool known = false;
            for (size_t k = 0; k < kFirstGeneratedNode; ++k)
            {
                // The first seven pools were registered above under these names.
                static const char* const kKnown[kFirstGeneratedNode] = {"PoolRTTI",  "PoolRTTIFunction", "PoolRTTIProperty", "PoolScript",
                                                                        "PoolEngine", "PoolRefCount",       "PoolDefault"};
                known = known || std::strcmp(name, kKnown[k]) == 0;
            }
            if (known)
            {
                continue;
            }
            FillPool(node++, name, g_generatedStorage[i], kGeneratedTagBase + i * kGeneratedTagStep);
        }
    }

    const uintptr_t base = GameBase();
    g_vaultSymbolsOk = true;
    for (auto& pool : g_pools)
    {
        void* slots[5] = {};
        for (int i = 0; i < 5; ++i)
        {
            char symbol[160];
            std::snprintf(symbol, sizeof(symbol), "_ZN3red6memory16PoolStorageProxyINS_%sEE%s", pool.name, kPoolSuffixes[i]);
            void* found = dlsym(RTLD_MAIN_ONLY, symbol);
            const auto addr = reinterpret_cast<uintptr_t>(found);
            const auto rva = found ? addr - base : 0;
            MAC_DIAG( "[vault] %s %s rva=%llx expected=%llx\n", pool.name, kPoolSuffixes[i],
                         static_cast<unsigned long long>(rva), static_cast<unsigned long long>(pool.expect[i]));
            if (!found || rva != pool.expect[i])
            {
                std::fprintf(stderr, "[vault] MISMATCH %s %s\n", pool.name, kPoolSuffixes[i]);
                g_vaultSymbolsOk = false;
                slots[i] = nullptr;
            }
            else
            {
                slots[i] = found;
            }
        }
        pool.alloc = reinterpret_cast<GameBlock (*)(uint64_t)>(slots[0]);
        pool.allocAligned = reinterpret_cast<GameBlock (*)(uint64_t, uint32_t)>(slots[1]);
        pool.realloc = reinterpret_cast<GameBlock (*)(GameBlock&, uint64_t)>(slots[2]);
        pool.reallocAligned = reinterpret_cast<GameBlock (*)(GameBlock&, uint64_t, uint32_t)>(slots[3]);
        pool.freeFn = reinterpret_cast<void (*)(GameBlock&)>(slots[4]);
    }
    MAC_DIAG( "[vault] ok=%d\n", g_vaultSymbolsOk ? 1 : 0);
    g_vaultReady = true;
}

PoolOps* ResolveGenerated(size_t aIndex)
{
    auto& state = g_generatedState[aIndex];
    uint8_t expected = 0;
    if (state.compare_exchange_strong(expected, 1))
    {
        auto& pool = g_generatedOps[aIndex];
        void* slots[5] = {};
        bool ok = true;
        for (int i = 0; i < 5; ++i)
        {
            char symbol[200];
            std::snprintf(symbol, sizeof(symbol), "_ZN3red6memory16PoolStorageProxyI%sE%s", kGeneratedPools[aIndex].type, kPoolSuffixes[i]);
            slots[i] = dlsym(RTLD_MAIN_ONLY, symbol);
            ok = ok && slots[i];
        }
        pool.name = kGeneratedPools[aIndex].name;
        pool.tag = kGeneratedTagBase + aIndex * kGeneratedTagStep;
        if (ok)
        {
            pool.alloc = reinterpret_cast<GameBlock (*)(uint64_t)>(slots[0]);
            pool.allocAligned = reinterpret_cast<GameBlock (*)(uint64_t, uint32_t)>(slots[1]);
            pool.realloc = reinterpret_cast<GameBlock (*)(GameBlock&, uint64_t)>(slots[2]);
            pool.reallocAligned = reinterpret_cast<GameBlock (*)(GameBlock&, uint64_t, uint32_t)>(slots[3]);
            pool.freeFn = reinterpret_cast<void (*)(GameBlock&)>(slots[4]);
        }
        else
        {
            std::fprintf(stderr, "[vault] pool %s: symbols not found, its allocations return null\n", pool.name);
        }
        MAC_DIAG("[vault] generated pool %s resolved=%d\n", pool.name, ok ? 1 : 0);
        state.store(2);
    }
    else
    {
        while (state.load() != 2)
        {
            std::this_thread::yield();
        }
    }
    return &g_generatedOps[aIndex];
}

PoolOps* PoolFor(uintptr_t aTag)
{
    if (aTag >= kGeneratedTagBase)
    {
        const auto index = (aTag - kGeneratedTagBase) / kGeneratedTagStep;
        if (index < kGeneratedCount && (aTag - kGeneratedTagBase) % kGeneratedTagStep == 0)
        {
            return ResolveGenerated(index);
        }
    }

    for (auto& pool : g_pools)
    {
        if (pool.tag == aTag)
        {
            return &pool;
        }
    }
    std::fprintf(stderr, "[vault] unknown tag 0x%llx\n", static_cast<unsigned long long>(aTag));
    return nullptr;
}
}

extern "C" __attribute__((visibility("hidden"))) void* tweakxl_memory_vault()
{
    InitVault();
    return &g_vault;
}

extern "C" __attribute__((visibility("hidden"))) void tweakxl_vault_alloc(void* aStorage, void* aResult, uint64_t aSize)
{
    InitVault();
    auto* result = static_cast<RED4ext::Memory::AllocationResult*>(aResult);
    result->memory = nullptr;
    result->size = 0;
    auto* pool = PoolFor(reinterpret_cast<uintptr_t>(aStorage));
    if (!pool || !pool->alloc)
    {
        return;
    }
    const GameBlock block = pool->alloc(aSize);
    result->memory = block.memory;
    result->size = block.memory ? block.size : 0;
}

extern "C" __attribute__((visibility("hidden"))) void tweakxl_vault_alloc_aligned(void* aStorage, void* aResult, uint64_t aSize, uint32_t aAlign)
{
    InitVault();
    auto* result = static_cast<RED4ext::Memory::AllocationResult*>(aResult);
    result->memory = nullptr;
    result->size = 0;
    auto* pool = PoolFor(reinterpret_cast<uintptr_t>(aStorage));
    if (!pool || !pool->allocAligned)
    {
        return;
    }
    const GameBlock block = pool->allocAligned(aSize, aAlign);
    result->memory = block.memory;
    result->size = block.memory ? block.size : 0;
}

extern "C" __attribute__((visibility("hidden"))) void tweakxl_vault_realloc(void* aStorage, void* aResult, void* aInput, uint64_t aSize)
{
    InitVault();
    auto* result = static_cast<RED4ext::Memory::AllocationResult*>(aResult);
    result->memory = nullptr;
    result->size = 0;
    auto* pool = PoolFor(reinterpret_cast<uintptr_t>(aStorage));
    if (!pool || !pool->realloc || !aInput)
    {
        return;
    }
    const GameBlock block = pool->realloc(*static_cast<GameBlock*>(aInput), aSize);
    result->memory = block.memory;
    result->size = block.memory ? block.size : 0;
}

extern "C" __attribute__((visibility("hidden"))) void tweakxl_vault_realloc_aligned(void* aStorage, void* aResult, void* aInput, uint64_t aSize, uint32_t aAlign)
{
    InitVault();
    auto* result = static_cast<RED4ext::Memory::AllocationResult*>(aResult);
    result->memory = nullptr;
    result->size = 0;
    auto* pool = PoolFor(reinterpret_cast<uintptr_t>(aStorage));
    if (!pool || !pool->reallocAligned || !aInput)
    {
        return;
    }
    const GameBlock block = pool->reallocAligned(*static_cast<GameBlock*>(aInput), aSize, aAlign);
    result->memory = block.memory;
    result->size = block.memory ? block.size : 0;
}

extern "C" __attribute__((visibility("hidden"))) void tweakxl_vault_free(void* aStorage, void* aInput)
{
    InitVault();
    auto* pool = PoolFor(reinterpret_cast<uintptr_t>(aStorage));
    if (!pool || !pool->freeFn || !aInput)
    {
        return;
    }
    pool->freeFn(*static_cast<GameBlock*>(aInput));
}

// R19: tweakxl_destruct_value_holder (the stand-in that logged "[vh] unexpected holder" and leaked the holder) is gone.
// IScriptable_DestructValueHolder resolves to the game function (0x22384BC) through MacSdkRows.inc.

extern "C" __attribute__((visibility("hidden"))) uintptr_t tweakxl_class_vslot(uint32_t aByteOffset)
{
    static bool ready = false;
    static uintptr_t slots[11] = {};
    static const uint32_t kOffsets[11] = {0x68, 0x70, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB8, 0xC8, 0xD0, 0xD8};
    if (!ready)
    {
        auto* rtti = RED4ext::CRTTISystem::Get();
        auto* cls = rtti ? rtti->GetClass(RED4ext::CName("IScriptable")) : nullptr;
        if (!cls)
        {
            std::fprintf(stderr, "[vslot] IScriptable missing\n");
            return 0;
        }
        auto* vt = *reinterpret_cast<uintptr_t**>(cls);
        const uintptr_t base = GameBase();
        const int game = ImageIndex(nullptr, MH_EXECUTE);
        const int tweak = ImageIndex("TweakXL.dylib", 0);
        const int codeware = ImageIndex("Codeware.dylib", 0);
        const int ltbf = ImageIndex("let_there_be_flight.dylib", 0);
        MAC_DIAG( "[vslot] vtable=%p\n", static_cast<void*>(vt));
        for (int i = 0; i < 11; ++i)
        {
            const uintptr_t fn = vt[kOffsets[i] / 8];
            const bool inGame = game >= 0 && AddressInImage(static_cast<uint32_t>(game), fn);
            const bool inTweak = tweak >= 0 && AddressInImage(static_cast<uint32_t>(tweak), fn);
            const bool inCodeware = codeware >= 0 && AddressInImage(static_cast<uint32_t>(codeware), fn);
            const bool inLtbf = ltbf >= 0 && AddressInImage(static_cast<uint32_t>(ltbf), fn);
            MAC_DIAG( "[vslot] off=%x rva=%llx\n", kOffsets[i],
                         static_cast<unsigned long long>(fn - base));
            if (!inGame || inTweak || inCodeware || inLtbf)
            {
                std::fprintf(stderr, "[vslot] reject off=%x inGame=%d inTweakXL=%d inCodeware=%d inLtbf=%d\n", kOffsets[i],
                             inGame ? 1 : 0, inTweak ? 1 : 0, inCodeware ? 1 : 0, inLtbf ? 1 : 0);
                slots[i] = 0;
            }
            else
            {
                slots[i] = fn;
            }
        }
        ready = true;
    }
    for (int i = 0; i < 11; ++i)
    {
        if (kOffsets[i] == aByteOffset)
        {
            return slots[i];
        }
    }
    std::fprintf(stderr, "[vslot] unknown offset %x\n", aByteOffset);
    return 0;
}

// R42c. Self-test, run once before any type is registered: for every pool in g_pools, allocate 64 bytes through the game's
// PoolStorageProxy and free them again, one log line each. Also checks that the vault lookup (FNV1a32 of the pool name) finds the
// storage FillPool wrote, which is the path Allocator<Pool...> takes. Returns false if any pool failed.
extern "C" __attribute__((visibility("hidden"))) bool ltbf_vault_selftest()
{
    InitVault();
    bool allOk = g_vaultSymbolsOk;
    for (auto& pool : g_pools)
    {
        // pool.name is the mangled length + name ("11PoolDefault"); the registry key is the bare name.
        const char* bare = pool.name;
        while (*bare >= '0' && *bare <= '9')
        {
            ++bare;
        }
        const auto hash = RED4ext::FNV1a32(bare);
        bool inVault = false;
        for (const auto& node : g_vault.poolRegistry.nodes)
        {
            if (node.handle == hash && node.storage && node.storage->allocatorStorage == pool.tag)
            {
                inVault = true;
                break;
            }
        }
        bool ok = inVault && pool.alloc && pool.freeFn;
        void* memory = nullptr;
        if (ok)
        {
            GameBlock block = pool.alloc(64);
            memory = block.memory;
            ok = block.memory != nullptr; // the granted size is not checked: the SDK path ignores it too
            if (block.memory)
            {
                pool.freeFn(block);
            }
        }
        std::fprintf(stderr, "[vault] self-test %s: %s (inVault=%d alloc=%p)\n", bare, ok ? "ok" : "FAILED", inVault ? 1 : 0,
                     memory);
        allOk = allOk && ok;
    }
    return allOk;
}
