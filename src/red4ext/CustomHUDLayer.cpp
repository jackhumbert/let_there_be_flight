#include "Utils/FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HudWidgetSpawnEntry.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WidgetLibraryResource.hpp>
//#include "LoadResRef.hpp"

namespace vehicle {
namespace flight {

struct HUDLayerUnknown {
  uint64_t unk[11];
  RED4ext::DynArray<RED4ext::ink::HudWidgetSpawnEntry> spawnEntries;
};

void CopyHUDWidgetSpawnEntries(HUDLayerUnknown *a1, RED4ext::DynArray<RED4ext::ink::HudWidgetSpawnEntry> *a2);

// 40 53 56 41 56 48 83 EC  30 48 8B F2 4C 89 7C 24 60 8B 52 0C 4C 8B F1 48 8B 0E E8 01 03 00 00 33
constexpr uintptr_t CopyHUDWidgetSpawnEntriesAddr = 0x140820EA0 - RED4ext::Addresses::ImageBase;
decltype(&CopyHUDWidgetSpawnEntries) CopyHUDWidgetSpawnEntries_Original;

//// 40 53 48 83 EC 60 33 C0  48 8B DA 48 89 44 24 20 89 44 24 28 88 44 24 2C 48 39 01 74 2D 48 8B 11
//constexpr uintptr_t CreateResRef08Addr = 0x140200110 - RED4ext::Addresses::ImageBase;
//using CreateResRef08Sig = RED4ext::ResourceWrapper<RED4ext::ink::WidgetLibraryResource>* (*) (uint64_t *, RED4ext::ResourceWrapper<RED4ext::ink::WidgetLibraryResource> *a2);
//
//// 40 53 48 83 EC 40 8B 41  58 48 8B D9 0F 29 74 24 30 0F 29 7C 24 20 85 C0 74 0A 80 79 5C 00 0F 84
//constexpr uintptr_t WaitUntilLoadedAddr = 0x1402476B0 - RED4ext::Addresses::ImageBase;
//using WaitUntilLoadedSig = void * (*) (RED4ext::ResourceWrapper<RED4ext::ink::WidgetLibraryResource> *a2);


void CopyHUDWidgetSpawnEntries(HUDLayerUnknown *a1, RED4ext::DynArray<RED4ext::ink::HudWidgetSpawnEntry> *a2) {

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

  //RED4ext::RelocFunc<WaitUntilLoadedSig> WaitUntilLoaded(WaitUntilLoadedAddr);

  //auto handle = new RED4ext::ResourceHandle<RED4ext::ink::WidgetLibraryResource>();
  uint64_t hash = 2783178642409560840; // flight
  //uint64_t hash = 11046326377887898612; // car
  //LoadResRef<RED4ext::ink::WidgetLibraryResource>(&hash, handle, true);
  //WaitUntilLoaded(handle->self);

  instance->widgetResource.path = hash;
  //instance->widgetResource.Fetch();
  //instance->widgetResource.token.instance = *handle;
  //instance->widgetResource.token.refCount->IncRef();

  //for (const auto &entry : a1->spawnEntries) {
  //  if (entry.hudEntryName == "car hud") {
  //    instance->widgetResource.hash = entry.widgetResource.hash;
  //    instance->widgetResource.wrapper= entry.widgetResource.wrapper;
  //    instance->widgetResource.refCount = entry.widgetResource.refCount;
  //    instance->widgetResource.refCount->IncRef();
  //  }
  //}

  //instance->widgetResource.ref = (RED4ext::ink::WidgetLibraryResource *)11046326377887898612; // base\gameplay\gui\widgets\vehicle\huds\hud_car_default.inkwidget
  //instance->widgetResource.ref = (RED4ext::ink::WidgetLibraryResource *)2783178642409560840; // base\gameplay\gui\widgets\vehicle\huds\hud_flight_default.inkwidget
  

  //auto rtti = RED4ext::CRTTISystem::Get();
  //auto cls = rtti->GetClass("inkHudWidgetSpawnEntry");
  //auto instance = reinterpret_cast<RED4ext::ink::HudWidgetSpawnEntry *>(cls->AllocInstance());
  //auto pEffectHandle = RED4ext::Handle<RED4ext::ISerializable>(pEffectInstance);

  CopyHUDWidgetSpawnEntries_Original(a1, a2);
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

//REGISTER_FLIGHT_MODULE(CustomHUDLayer);

} // namespace flight
} // namespace vehicle