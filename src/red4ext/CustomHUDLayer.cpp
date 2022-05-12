#include "FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HudWidgetSpawnEntry.hpp>

namespace vehicle {
namespace flight {

void CopyHUDWidgetSpawnEntries(uintptr_t a1, RED4ext::DynArray<RED4ext::ink::HudWidgetSpawnEntry> *a2);

// 40 53 56 41 56 48 83 EC  30 48 8B F2 4C 89 7C 24 60 8B 52 0C 4C 8B F1 48 8B 0E E8 01 03 00 00 33
constexpr uintptr_t CopyHUDWidgetSpawnEntriesAddr = 0x140820EA0 - RED4ext::Addresses::ImageBase;
decltype(&CopyHUDWidgetSpawnEntries) CopyHUDWidgetSpawnEntries_Original;

void CopyHUDWidgetSpawnEntries(uintptr_t a1, RED4ext::DynArray<RED4ext::ink::HudWidgetSpawnEntry> *a2) {
  CopyHUDWidgetSpawnEntries_Original(a1, a2);

  auto instance = new RED4ext::ink::HudWidgetSpawnEntry();
  instance->hudEntryName = "flight_hud";
  instance->enabled = true;
  instance->contextVisibility.SceneDefault = true;
  instance->contextVisibility.SceneTier1 = true;
  instance->contextVisibility.SceneTier2 = true;
  instance->contextVisibility.SceneTier3 = true;
  instance->contextVisibility.SceneTier4 = true;
  instance->contextVisibility.SceneTier5 = true;
  instance->gameContextVisibility.VehicleMounted = true;
  instance->gameContextVisibility.VehicleRace = true;
  instance->anchorPlace = RED4ext::ink::EAnchor::Centered;
  instance->anchorPoint.X = 0.5;
  instance->anchorPoint.Y = 0.5;
  instance->useSeparateWindow = true;
  instance->slotParams.useSlotLayout = true;
  instance->slotParams.layoutOverride.sizeCoefficient = 1.0;

  //instance->widgetResource.ref = (RED4ext::ink::WidgetLibraryResource *)11046326377887898612; // base\gameplay\gui\widgets\vehicle\huds\hud_car_default.inkwidget
  instance->widgetResource.ref = (RED4ext::ink::WidgetLibraryResource *)2783178642409560840; // base\gameplay\gui\widgets\vehicle\huds\hud_flight_default.inkwidget
  

  //auto rtti = RED4ext::CRTTISystem::Get();
  //auto cls = rtti->GetClass("inkHudWidgetSpawnEntry");
  //auto instance = reinterpret_cast<RED4ext::ink::HudWidgetSpawnEntry *>(cls->AllocInstance());
  //auto pEffectHandle = RED4ext::Handle<RED4ext::ISerializable>(pEffectInstance);

  a2->EmplaceBack(nullptr, instance);
}

struct CustomHUDLayer : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(CopyHUDWidgetSpawnEntriesAddr), &CopyHUDWidgetSpawnEntries,
                          reinterpret_cast<void **>(&CopyHUDWidgetSpawnEntries_Original));
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(CopyHUDWidgetSpawnEntriesAddr));
  }
};

REGISTER_FLIGHT_MODULE(CustomHUDLayer);

} // namespace flight
} // namespace vehicle