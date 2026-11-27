#include <Utils/FlightModule.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/ResourceLoader.hpp>
#include <RED4ext/ResourcePath.hpp>
#include <RED4ext/Scripting/Natives/Generated/DynamicTexture.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HudEntriesResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/IWidgetController.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ImageWidget.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/VirtualWindow.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/Widget.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WidgetLibraryResource.hpp>
#include <mutex>
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>

// Adds the flight HUD's entry to each HUD entries resource (.inkhud) with the game's car HUD in it as it loads (not
// the HUDs of cameras, turrets and drones the player looks through), so the flight HUD is part of
// whatever HUD the player has: V's own (prototype_hud.inkhud), a HUD mod's replacement, or a quest's own player
// with its own HUD (Johnny's, the tutorial's, the Arasaka Tower 3D arcade game's, whose HUD is the game's screen).
// Giving the player hud_flight.inkhud in place of its own HUD left that arcade game a black screen under V's HUD.
//
// The entry is copied from hud_flight.inkhud as that file loads (its context flags as the game reads them), into
// every other HUD resource loaded before or after it. The game spawns a HUD's entries when the player attaches,
// long after its resource has loaded, so one that loaded before ours still gets the entry in time.

using namespace RED4ext;

namespace {
// the serializer's state for one resource load; its finished objects at 0x230 (ArchiveXL's and Codeware's layout,
// as Mod Settings uses it for the menu registries)
struct ResourceSerializerContext {
  uint8_t unk00[0x230];
  DynArray<Handle<ISerializable>> serializables; // 230
};

constexpr ResourcePath kFlightHud = "user\\jackhumbert\\widgets\\hud_flight.inkhud";
constexpr ResourcePath kCarHud = "base\\gameplay\\gui\\widgets\\vehicle\\huds\\hud_car_default.inkwidget";
const CName kFlightEntry = "hud flight";

std::mutex lock;
std::optional<ink::HudWidgetSpawnEntry> flightEntry;
// HUD resources loaded before hud_flight.inkhud, to be given the entry once it has loaded
std::vector<WeakHandle<ISerializable>> waiting;
// nothing else asks for hud_flight.inkhud: the first other HUD to load starts it
SharedPtr<ResourceToken<ink::HudEntriesResource>> flightHudToken;
bool flightHudRequested = false;

bool HasFlightEntry(ink::HudEntriesResource *aHud) {
  for (const auto &entry : aHud->entries) {
    if (entry.hudEntryName == kFlightEntry) {
      return true;
    }
  }
  return false;
}

// a HUD the player drives with: it has the game's car HUD (V's, Johnny's, a HUD mod's replacement), not a camera's
// or a turret's
bool HasCarHud(ink::HudEntriesResource *aHud) {
  for (const auto &entry : aHud->entries) {
    if (entry.widgetResource.path == kCarHud) {
      return true;
    }
  }
  return false;
}

void AddFlightEntry(ink::HudEntriesResource *aHud) {
  if (HasFlightEntry(aHud) || !HasCarHud(aHud)) {
    return;
  }
  // first, where hud_flight.inkhud has always had it
  aHud->entries.Emplace(aHud->entries.begin(), *flightEntry);
  spdlog::info("[HUD] flight HUD entry added to {:016x} ({} entries)", aHud->path.hash, aHud->entries.size);
}
} // namespace

// bool ResourceSerializer::OnDependenciesReady(ResourceSerializerContext *)
REGISTER_FLIGHT_HOOK_HASH(bool, 3892979844, ResourceSerializer_OnDependenciesReady, ResourceSerializerContext *aContext) {
  static auto *hudType = CRTTISystem::Get()->GetClass(ink::HudEntriesResource::NAME);
  bool loadFlightHud = false;
  for (const auto &serializable : aContext->serializables) {
    if (!serializable || serializable->GetNativeType() != hudType) {
      continue;
    }
    auto *hud = reinterpret_cast<ink::HudEntriesResource *>(serializable.GetPtr());
    std::lock_guard guard(lock);
    if (hud->path == kFlightHud) {
      for (const auto &entry : hud->entries) {
        if (entry.hudEntryName == kFlightEntry) {
          flightEntry = entry;
        }
      }
      if (!flightEntry) {
        spdlog::warn("[HUD] hud_flight.inkhud has no '{}' entry: no flight HUD", kFlightEntry.ToString());
        continue;
      }
      for (const auto &weak : waiting) {
        if (auto earlier = weak.Lock()) {
          AddFlightEntry(reinterpret_cast<ink::HudEntriesResource *>(earlier.GetPtr()));
        }
      }
      waiting.clear();
    } else if (flightEntry) {
      AddFlightEntry(hud);
    } else {
      waiting.emplace_back(serializable);
      loadFlightHud = !flightHudRequested;
      flightHudRequested = true;
    }
  }
  // outside the lock: the load comes back through this hook
  if (loadFlightHud) {
    flightHudToken = ResourceLoader::Get()->LoadAsync<ink::HudEntriesResource>(kFlightHud);
  }
  return ResourceSerializer_OnDependenciesReady_Original(aContext);
}
