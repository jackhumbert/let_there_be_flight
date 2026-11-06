#include "Link.hpp"

#include "Pattern.hpp"
#include "Relay.hpp"

#include <spdlog/spdlog.h>
#include <windows.h>

#include <cstring>
#include <vector>

namespace Wwise {

namespace {

template <typename F> void Get(F &aFn, const char *aName, std::string &aMissing) {
  aFn = reinterpret_cast<F>(GetProcAddress(GetModuleHandleW(nullptr), aName));
  if (!aFn) {
    aMissing += std::string(aMissing.empty() ? "" : ", ") + aName;
  }
}

using SetCallbacksFn = void (*)(ExecuteFn, FormatFn, GainFn);

RelayInstall state;
SetCallbacksFn setter = nullptr;

Callbacks Current() {
  const auto *g = reinterpret_cast<const uintptr_t *>(state.globals);
  return {reinterpret_cast<ExecuteFn>(g[0]), reinterpret_cast<FormatFn>(g[1]), reinterpret_cast<GainFn>(g[2])};
}

} // namespace

const Link &Link::Get() {
  static const Link link = [] {
    Link l;
    std::string &m = l.missing;
    Wwise::Get(l.registerGameObj, "?RegisterGameObj@SoundEngine@AK@@YA?AW4AKRESULT@@_KPEBD@Z", m);
    Wwise::Get(l.unregisterGameObj, "?UnregisterGameObj@SoundEngine@AK@@YA?AW4AKRESULT@@_K@Z", m);
    Wwise::Get(l.setPosition,
               "?SetPosition@SoundEngine@AK@@YA?AW4AKRESULT@@_KAEBVAkWorldTransform@@W4AkSetPositionFlags@@@Z", m);
    Wwise::Get(l.setListeners, "?SetListeners@SoundEngine@AK@@YA?AW4AKRESULT@@_KPEB_KI@Z", m);
    Wwise::Get(l.resetListenersToDefault, "?ResetListenersToDefault@SoundEngine@AK@@YA?AW4AKRESULT@@_K@Z", m);
    Wwise::Get(l.postEvent,
               "?PostEvent@SoundEngine@AK@@YAII_KIP6AXW4AkCallbackType@@PEAUAkCallbackInfo@@@ZPEAXIPEAUAkExternalSourceInfo@@I@Z",
               m);
    Wwise::Get(l.stopPlayingID, "?StopPlayingID@SoundEngine@AK@@YAXIHW4AkCurveInterpolation@@@Z", m);
    Wwise::Get(l.idFromString, "?GetIDFromString@SoundEngine@AK@@YAIPEBD@Z", m);
    Wwise::Get(l.sampleRate, "?GetSampleRate@SoundEngine@AK@@YAIXZ", m);
    Wwise::Get(l.audioSettings, "?GetAudioSettings@SoundEngine@AK@@YA?AW4AKRESULT@@AEAUAkAudioSettings@@@Z", m);
    Wwise::Get(l.loadBankMemoryCopy, "?LoadBankMemoryCopy@SoundEngine@AK@@YA?AW4AKRESULT@@PEBXIAEAI@Z", m);
    Wwise::Get(l.listeners, "?GetListeners@Query@SoundEngine@AK@@YA?AW4AKRESULT@@_KPEA_KAEAI@Z", m);
    Wwise::Get(l.listenerPosition,
               "?GetListenerPosition@Query@SoundEngine@AK@@YA?AW4AKRESULT@@_KAEAVAkWorldTransform@@@Z", m);
    Wwise::Get(l.position, "?GetPosition@Query@SoundEngine@AK@@YA?AW4AKRESULT@@_KAEAVAkWorldTransform@@@Z", m);
    if (!m.empty()) {
      spdlog::warn("[Wwise] exports not found: {}", m);
    }
    return l;
  }();
  return link;
}

bool InstallRelay(std::string &aWhy) {
  if (state.installed) {
    return true;
  }
  auto *base = reinterpret_cast<uint8_t *>(GetModuleHandleW(nullptr));
  auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + reinterpret_cast<IMAGE_DOS_HEADER *>(base)->e_lfanew);
  const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(nt);
  std::vector<SetterMatch> matches;
  uint64_t text = 0;
  for (int s = 0; s < nt->FileHeader.NumberOfSections; ++s, ++section) {
    if (std::memcmp(section->Name, ".text", 5) == 0) {
      text = reinterpret_cast<uint64_t>(base + section->VirtualAddress);
      matches = FindSetters({base + section->VirtualAddress, section->Misc.VirtualSize}, text);
      break;
    }
  }
  state.matches = int(matches.size());
  if (matches.size() != 1) {
    aWhy = "SetAudioInputCallbacks: " + std::to_string(matches.size()) + " matches";
    spdlog::warn("[Wwise] {}", aWhy);
    return false;
  }
  state.setter = text + matches[0].offset;
  state.globals = matches[0].globals;
  setter = reinterpret_cast<SetCallbacksFn>(state.setter);
  if (const auto rate = Link::Get().sampleRate) {
    TheRelay().SetRate(rate());
  }
  const Callbacks ours = Trampolines();
  TheRelay().Observe(Current(), ours);
  setter(ours.execute, ours.format, ours.gain);
  state.installed = Current().execute == ours.execute;
  const Callbacks passed = TheRelay().Passed();
  spdlog::info("[Wwise] relay {}: setter {:#x}, globals {:#x}, passing to {:#x} {:#x} {:#x}",
               state.installed ? "installed" : "NOT installed", state.setter, state.globals,
               reinterpret_cast<uintptr_t>(passed.execute), reinterpret_cast<uintptr_t>(passed.format),
               reinterpret_cast<uintptr_t>(passed.gain));
  if (!state.installed) {
    aWhy = "the setter did not take ours";
  }
  return state.installed;
}

bool KeepRelayInstalled() {
  if (!state.installed) {
    return false;
  }
  const Callbacks ours = Trampolines();
  if (!TheRelay().Observe(Current(), ours)) {
    return false;
  }
  setter(ours.execute, ours.format, ours.gain);
  ++state.reinstalls;
  const Callbacks passed = TheRelay().Passed();
  spdlog::info("[Wwise] the Audio Input callbacks were set after ours (now {:#x}): ours back in front ({} times)",
               reinterpret_cast<uintptr_t>(passed.execute), state.reinstalls);
  return true;
}

void UninstallRelay() {
  if (!state.installed) {
    return;
  }
  // straight into the globals: the setter refuses a null callback, and without REDmod sounds the game's are null
  const Callbacks passed = TheRelay().Passed();
  auto *g = reinterpret_cast<uintptr_t *>(state.globals);
  g[0] = reinterpret_cast<uintptr_t>(passed.execute);
  g[1] = reinterpret_cast<uintptr_t>(passed.format);
  g[2] = reinterpret_cast<uintptr_t>(passed.gain);
  state.installed = false;
  spdlog::info("[Wwise] relay uninstalled, the game's callbacks back");
}

RelayInstall RelayInstalled() {
  return state;
}

} // namespace Wwise
