#include "Addresses.hpp"
#include "Utils/FlightModule.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectSet.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectSystem.hpp>

// #include "LoadResRef.hpp"

// 1.52 RVA : 0x1478200
// 1.6  RVA: 0x148ED00 / 21556480
// 1.61 RVA: 0x148F3F0
/// @pattern 48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 48 89 4C 24 08 55 41 54 41 55 41 56 41 57 48 8D 6C 24 D0 48 81 EC 30 01 00 00 4C 8B AD 80 00 00 00
// using CreateStaticEffect = uintptr_t (RED4ext::game::EffectSystem *, uintptr_t, uint64_t, uint64_t, uintptr_t, uintptr_t);

/// @pattern 48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 48 89 4C 24 08 55 41 54 41 55 41 56 41 57 48 8D 6C 24 D0 48 81
/// EC 30 01 00 00 4C 8B AD 80 00 00 00
uintptr_t CreateStaticEffect(RED4ext::game::EffectSystem *, uintptr_t, uint64_t, uint64_t, uintptr_t, uintptr_t);

REGISTER_FLIGHT_HOOK(uintptr_t, CreateStaticEffect, RED4ext::game::EffectSystem *es, uintptr_t effectRef,
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