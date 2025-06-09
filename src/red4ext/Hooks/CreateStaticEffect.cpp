#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectSet.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectSystem.hpp>

// #include "LoadResRef.hpp"

/// @hash 247932756
// uintptr_t CreateStaticEffect(RED4ext::game::EffectSystem *, uintptr_t, uint64_t, uint64_t, uintptr_t, uintptr_t);

REGISTER_FLIGHT_HOOK_HASH(uintptr_t, 247932756, CreateStaticEffect, RED4ext::game::EffectSystem *es, uintptr_t effectRef,
                     uint64_t effectNameHash, uint64_t effectTagHash, uintptr_t instigator, uintptr_t weapon) {
  bool exists = false;
  for (const auto hash : es->effectNameHashes) {
    if (hash == effectNameHash) {
      exists = true;
    }
  }

  if (!exists) {
    // auto handle = new RED4ext::ResourceHandle<RED4ext::game::EffectSet>();
    // LoadResRef<RED4ext::game::EffectSet>(&effectNameHash, handle, true);
    auto resource = RED4ext::RaRef<RED4ext::game::EffectSet>(effectNameHash);
    es->effectNameHashes.EmplaceBack(effectNameHash);
    es->effectResources.EmplaceBack(resource);
    es->unkA0 = 1; // needs sorting maybe?
  }

  return CreateStaticEffect_Original(es, effectRef, effectNameHash, effectTagHash, instigator, weapon);
}