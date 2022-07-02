#include "FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>
#include <RED4ext/Relocation.hpp>

// 40 55 48 83 EC 40 80 39  00 48 8B EA 0F 84 C5 00 00 00 48 89 7C 24 60 48 8B 79 18 44 8B 47 0C 44
void __fastcall DebugPrint(uintptr_t, RED4ext::CString *);
constexpr uintptr_t DebugPrintAddr = 0x140A7E8E0 - RED4ext::Addresses::ImageBase;
decltype(&DebugPrint) DebugPrint_Original;

void __fastcall DebugPrint(uintptr_t a1, RED4ext::CString *a2) {
  spdlog::info(a2->c_str());
  DebugPrint_Original(a1, a2);
}

struct DebugPrintModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(DebugPrintAddr), &DebugPrint,
                                  reinterpret_cast<void **>(&DebugPrint_Original)))
      ;
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(DebugPrintAddr));
  }
};

REGISTER_FLIGHT_MODULE(DebugPrintModule);