// The game's SetGameObjectAuxSendValues, hooked to learn which of its game objects it gives reverb sends (Sends.hpp).
// The export is found by its mangled name, as Link finds the others; the call goes on to the game's unchanged.

#include "Sends.hpp"

#include "Utils/FlightModule.hpp"

#include <spdlog/spdlog.h>
#include <windows.h>

#include <atomic>

namespace {

using SetAuxSendsFn = Wwise::AKRESULT (*)(Wwise::AkGameObjectID, Wwise::AkAuxSendValue *, uint32_t);
SetAuxSendsFn original = nullptr;
void *address = nullptr;
// set by the Wwise backend once it reads the book (Sends::Listen): until then, and on the FMOD backend for the whole
// session, nothing is noted, since nothing would ever prune the book
std::atomic<bool> listening{false};

Wwise::AKRESULT Hook(Wwise::AkGameObjectID aObject, Wwise::AkAuxSendValue *aValues, uint32_t aCount) {
  if (listening.load(std::memory_order_relaxed)) {
    Wwise::Sends::TheBook().Note(aObject, aValues, aCount);
  }
  return original(aObject, aValues, aCount);
}

struct SendsHook : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) override {
    address = reinterpret_cast<void *>(GetProcAddress(
        GetModuleHandleW(nullptr), "?SetGameObjectAuxSendValues@SoundEngine@AK@@YA?AW4AKRESULT@@_KPEAUAkAuxSendValue@@I@Z"));
    if (!address) {
      spdlog::warn("[Reverb] SetGameObjectAuxSendValues not exported: no game reverb on LTBF's voices");
      return;
    }
    if (!aSdk->hooking->Attach(aHandle, address, reinterpret_cast<void *>(&Hook), reinterpret_cast<void **>(&original))) {
      spdlog::warn("[Reverb] could not hook SetGameObjectAuxSendValues: no game reverb on LTBF's voices");
      address = nullptr;
    }
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) override {
    if (address)
      aSdk->hooking->Detach(aHandle, address);
    address = nullptr;
  }
};

REGISTER_FLIGHT_MODULE(SendsHook);

} // namespace

void Wwise::Sends::Listen() {
  listening.store(true, std::memory_order_relaxed);
}
