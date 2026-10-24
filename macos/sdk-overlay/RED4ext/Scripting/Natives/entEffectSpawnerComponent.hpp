#pragma once
// LTBF R41 (macOS): copied from the jackhumbert fork (deps/red4ext.sdk, branch new-types); macOS changes, if any, are marked LTBF R41.

// This file is generated from the Game's Reflection data

#include <cstdint>
#include <RED4ext/Common.hpp>
#include <RED4ext/DynArray.hpp>
#include <RED4ext/Handle.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IVisualComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/Effect.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/CompiledEffectInfo.hpp>

namespace RED4ext
{

namespace ent { struct EffectDesc; }


struct EffectSpawnerComponentTransformProvider {
    virtual void GetMemoryPool() {}

    Handle<ISerializable> handle; // 08 (LTBF R41: the macOS SDK's Handle needs an ISerializable type; same 16 bytes)
    ent::IComponent * component; // 18
};

namespace ent { 
struct EffectSpawnerComponent : IVisualComponent
{
    static constexpr const char* NAME = "entEffectSpawnerComponent";
    static constexpr const char* ALIAS = NAME;

    void UpdateCache(world::CompiledEffectInfo const & compiledEffectInfo) {
        IHookable::StaticHook<void, 3519030832>(this, compiledEffectInfo);
    }

    void StopAllEffects(bool a2) {
        IHookable::StaticHook<void, 518525837>(this, a2);
    }

    struct RunningEffectEntry {
        uint64_t unk00;
        uint64_t unk08;
        WeakHandle<EffectDesc> effect;
        uint64_t unk20;
        uint64_t unk28;
    };

    struct SlotComponentCache {
        uint64_t unk00;
        uint64_t unk08;
        uint64_t unk10;
    };
    DynArray<Handle<EffectDesc>> effectDescs; // 140
    DynArray<RunningEffectEntry> activeEffects; // 150
    DynArray<RaRef<RED4ext::world::Effect>> resources; // 160
    // uint8_t unk150[0x230 - 0x170]; // 170
    Map<CName, SlotComponentCache> componentCache; // 170
    Map<CName, WeakHandle<ent::IVisualComponent>> components; // 198
    WeakHandle<ISerializable> unk1C0; // 1C0 animatedComponentCache
    DynArray<void *> unk1D0;
    DynArray<void *> unk1E0;
    uint32_t unk1F0;
    uint32_t unk1F4;
    WeakHandle<ISerializable> transformProvider; // LTBF R41: EffectSpawnerComponentTransformProvider is not ISerializable
    DynArray<EffectDesc const *> effectPointers; // 208
    uint32_t unk218;
    uint32_t unk21C;
    void * effectSpawnerSaveSystem; // 220
    uint64_t unk228;
};
RED4EXT_ASSERT_SIZE(EffectSpawnerComponent, 0x230);
} // namespace ent
} // namespace RED4ext
