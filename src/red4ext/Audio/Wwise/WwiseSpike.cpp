#include "WwiseSpike.hpp"

#include <spdlog/spdlog.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <numbers>
#include <string>
#include <vector>

namespace {

// --- Wwise 2023.1's types, as far as the spike needs them (the layouts read from the exe: research note) ---
using AkGameObjectID = uint64_t;
using AkPlayingID = uint32_t;
using AkUniqueID = uint32_t;
using AKRESULT = int32_t;
constexpr AKRESULT AK_Success = 1;
constexpr int32_t AK_DataReady = 45;

struct AkAudioBuffer {
  float *pData; // non-interleaved: channel c at pData + c * uMaxFrames
  uint32_t channelConfig;
  int32_t eState;
  uint16_t uMaxFrames;
  uint16_t uValidFrames;
};
static_assert(offsetof(AkAudioBuffer, eState) == 0xC && offsetof(AkAudioBuffer, uMaxFrames) == 0x10 &&
              offsetof(AkAudioBuffer, uValidFrames) == 0x12);

// the game's format callback writes 12 bytes: the rate, the channel config, and the bit fields (bits per sample 6,
// block align 10, type 2, interleave 1)
struct AkAudioFormat {
  uint32_t sampleRate;
  uint32_t channelConfig;
  uint32_t bits;
};
// one channel, a standard config, the centre speaker: channels 8 bits, type 4, mask 20
constexpr uint32_t kMono = 1u | (1u << 8) | (0x4u << 12);
// 32-bit float (type 1), non-interleaved (1), a block of 4 bytes
constexpr uint32_t kFloatBits = 32u | (4u << 6) | (1u << 16) | (1u << 18);

struct AkVector {
  float x, y, z;
};
struct AkVector64 {
  double x, y, z;
};
// GetListenerPosition copies 48 bytes: two float vectors and a double position (which comes first: State() shows)
struct AkWorldTransform {
  AkVector front, top;
  AkVector64 position;
};
static_assert(sizeof(AkWorldTransform) == 48);

struct AkAudioSettings {
  uint32_t samplesPerFrame, samplesPerSecond;
};

using ExecuteFn = void (*)(AkPlayingID, AkAudioBuffer *);
using FormatFn = void (*)(AkPlayingID, AkAudioFormat &);
using GainFn = float (*)(AkPlayingID);
using SetCallbacksFn = void (*)(ExecuteFn, FormatFn, GainFn);

// --- the exe's exports, by their mangled names (research note: 350 AK:: exports) ---
template <typename F> F Export(const char *aName) {
  return reinterpret_cast<F>(GetProcAddress(GetModuleHandleW(nullptr), aName));
}
struct Ak {
  AKRESULT (*registerGameObj)(AkGameObjectID, const char *) = nullptr;
  AKRESULT (*unregisterGameObj)(AkGameObjectID) = nullptr;
  AKRESULT (*setPosition)(AkGameObjectID, const AkWorldTransform &, int) = nullptr;
  AkPlayingID (*postEvent)(AkUniqueID, AkGameObjectID, uint32_t, void *, void *, uint32_t, void *, AkPlayingID) = nullptr;
  AkUniqueID (*idFromString)(const char *) = nullptr;
  uint32_t (*sampleRate)() = nullptr;
  AKRESULT (*audioSettings)(AkAudioSettings &) = nullptr;
  AKRESULT (*listeners)(AkGameObjectID, AkGameObjectID *, uint32_t &) = nullptr;
  AKRESULT (*listenerPosition)(AkGameObjectID, AkWorldTransform &) = nullptr;
  AKRESULT (*position)(AkGameObjectID, AkWorldTransform &) = nullptr;
  void (*stopPlayingID)(AkPlayingID, int32_t, int) = nullptr;
  AKRESULT (*loadBankMemoryCopy)(const void *, uint32_t, uint32_t &) = nullptr;
  AKRESULT (*setListeners)(AkGameObjectID, const AkGameObjectID *, uint32_t) = nullptr;
  std::string missing;

  Ak() {
    auto get = [&](auto &aFn, const char *aName) {
      aFn = Export<std::remove_reference_t<decltype(aFn)>>(aName);
      if (!aFn) {
        missing += std::string(missing.empty() ? "" : ", ") + aName;
      }
    };
    get(registerGameObj, "?RegisterGameObj@SoundEngine@AK@@YA?AW4AKRESULT@@_KPEBD@Z");
    get(unregisterGameObj, "?UnregisterGameObj@SoundEngine@AK@@YA?AW4AKRESULT@@_K@Z");
    get(setPosition, "?SetPosition@SoundEngine@AK@@YA?AW4AKRESULT@@_KAEBVAkWorldTransform@@W4AkSetPositionFlags@@@Z");
    get(postEvent, "?PostEvent@SoundEngine@AK@@YAII_KIP6AXW4AkCallbackType@@PEAUAkCallbackInfo@@@ZPEAXIPEAUAkExternalSourceInfo@@I@Z");
    get(idFromString, "?GetIDFromString@SoundEngine@AK@@YAIPEBD@Z");
    get(sampleRate, "?GetSampleRate@SoundEngine@AK@@YAIXZ");
    get(audioSettings, "?GetAudioSettings@SoundEngine@AK@@YA?AW4AKRESULT@@AEAUAkAudioSettings@@@Z");
    get(listeners, "?GetListeners@Query@SoundEngine@AK@@YA?AW4AKRESULT@@_KPEA_KAEAI@Z");
    get(listenerPosition, "?GetListenerPosition@Query@SoundEngine@AK@@YA?AW4AKRESULT@@_KAEAVAkWorldTransform@@@Z");
    get(position, "?GetPosition@Query@SoundEngine@AK@@YA?AW4AKRESULT@@_KAEAVAkWorldTransform@@@Z");
    get(stopPlayingID, "?StopPlayingID@SoundEngine@AK@@YAXIHW4AkCurveInterpolation@@@Z");
    get(loadBankMemoryCopy, "?LoadBankMemoryCopy@SoundEngine@AK@@YA?AW4AKRESULT@@PEBXIAEAI@Z");
    get(setListeners, "?SetListeners@SoundEngine@AK@@YA?AW4AKRESULT@@_KPEB_KI@Z");
  }
};
const Ak &AK() {
  static const Ak ak;
  return ak;
}

// --- SetAudioInputCallbacks, found by its bytes: test rcx,rcx / je / test rdx,rdx / je / three rip-relative movs
// of rcx, rdx and r8 into consecutive globals / ret ---
struct Setter {
  SetCallbacksFn fn = nullptr;
  uintptr_t globals = 0; // the execute callback's; format and gain follow 8 and 16 bytes on
  int matches = 0;
};
Setter FindSetter() {
  Setter out;
  auto *base = reinterpret_cast<uint8_t *>(GetModuleHandleW(nullptr));
  auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + reinterpret_cast<IMAGE_DOS_HEADER *>(base)->e_lfanew);
  const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(nt);
  for (int s = 0; s < nt->FileHeader.NumberOfSections; ++s, ++section) {
    if (std::memcmp(section->Name, ".text", 5) != 0) {
      continue;
    }
    const uint8_t *p = base + section->VirtualAddress;
    const size_t n = section->Misc.VirtualSize;
    for (size_t i = 0; i + 32 <= n; ++i) {
      const uint8_t *q = p + i;
      if (q[0] != 0x48 || q[1] != 0x85 || q[2] != 0xC9 || q[3] != 0x74 || q[5] != 0x48 || q[6] != 0x85 || q[7] != 0xD2 ||
          q[8] != 0x74 || q[10] != 0x48 || q[11] != 0x89 || q[12] != 0x0D || q[17] != 0x48 || q[18] != 0x89 ||
          q[19] != 0x15 || q[24] != 0x4C || q[25] != 0x89 || q[26] != 0x05 || q[31] != 0xC3) {
        continue;
      }
      // each mov is 7 bytes; its target is the next instruction plus the displacement
      auto target = [&](int aAt) {
        int32_t d;
        std::memcpy(&d, q + aAt + 3, 4);
        return reinterpret_cast<uintptr_t>(q + aAt + 7) + intptr_t(d);
      };
      const uintptr_t a = target(10), b = target(17), c = target(24);
      if (b != a + 8 || c != a + 16) {
        continue;
      }
      ++out.matches;
      out.fn = reinterpret_cast<SetCallbacksFn>(const_cast<uint8_t *>(q));
      out.globals = a;
    }
  }
  return out;
}

// --- our callbacks, in front of the game's ---
std::atomic<ExecuteFn> gameExecute{nullptr};
std::atomic<FormatFn> gameFormat{nullptr};
std::atomic<GainFn> gameGain{nullptr};
std::atomic<AkPlayingID> ourPlaying{0};
std::atomic<float> frequency{440.0f}, gain{0.2f};
std::atomic<uint32_t> rate{48000};
std::atomic<uint64_t> oursCalls{0}, passedCalls{0}, formatCalls{0}, ourFormats{0};
std::atomic<uint32_t> lastFrames{0}, lastConfig{0}, defaultRate{0}, defaultConfig{0}, defaultBits{0};
double phase = 0; // the audio thread's

void OurExecute(AkPlayingID aId, AkAudioBuffer *aBuffer) {
  if (aId != 0 && aId == ourPlaying.load(std::memory_order_relaxed)) {
    oursCalls.fetch_add(1, std::memory_order_relaxed);
    const uint32_t frames = aBuffer->uMaxFrames, channels = aBuffer->channelConfig & 0xFF;
    lastFrames.store(frames, std::memory_order_relaxed);
    lastConfig.store(aBuffer->channelConfig, std::memory_order_relaxed);
    const double step = 2.0 * std::numbers::pi * frequency.load(std::memory_order_relaxed) /
                        double(rate.load(std::memory_order_relaxed));
    const float g = gain.load(std::memory_order_relaxed);
    for (uint32_t i = 0; i < frames; ++i) {
      const float v = g * float(std::sin(phase));
      phase += step;
      for (uint32_t c = 0; c < std::max(channels, 1u); ++c) {
        aBuffer->pData[size_t(c) * frames + i] = v;
      }
    }
    phase = std::fmod(phase, 2.0 * std::numbers::pi);
    aBuffer->uValidFrames = uint16_t(frames);
    aBuffer->eState = AK_DataReady;
    return;
  }
  passedCalls.fetch_add(1, std::memory_order_relaxed);
  if (auto fn = gameExecute.load(std::memory_order_relaxed)) {
    fn(aId, aBuffer);
  }
}

void OurFormat(AkPlayingID aId, AkAudioFormat &aFormat) {
  formatCalls.fetch_add(1, std::memory_order_relaxed);
  if (aId != 0 && aId == ourPlaying.load(std::memory_order_relaxed)) {
    ourFormats.fetch_add(1, std::memory_order_relaxed);
    // what the plugin had before us, for the report
    defaultRate.store(aFormat.sampleRate);
    defaultConfig.store(aFormat.channelConfig);
    defaultBits.store(aFormat.bits);
    aFormat.sampleRate = rate.load();
    aFormat.channelConfig = kMono;
    aFormat.bits = kFloatBits;
    return;
  }
  if (auto fn = gameFormat.load(std::memory_order_relaxed)) {
    fn(aId, aFormat);
  }
}

float OurGain(AkPlayingID aId) {
  if (aId != 0 && aId == ourPlaying.load(std::memory_order_relaxed)) {
    return 1.0f;
  }
  auto fn = gameGain.load(std::memory_order_relaxed);
  return fn ? fn(aId) : 1.0f;
}

// our game object: "LTBF" and 1, far from the game's own IDs (a guess the spike checks)
constexpr AkGameObjectID kObject = 0x4C54424600000001ull;
// a listener of ours, for the camera listener (spike question 6)
constexpr AkGameObjectID kListener = 0x4C54424600000002ull;
bool installed = false, registered = false, listenerRegistered = false;
Setter setter;
std::string lastEvent;
AKRESULT lastPlace = 0;

std::string Hex(uintptr_t v) {
  char b[32];
  std::snprintf(b, sizeof(b), "0x%llx", static_cast<unsigned long long>(v));
  return b;
}

bool Install(std::string &aWhy) {
  if (installed) {
    return true;
  }
  setter = FindSetter();
  if (setter.matches != 1) {
    aWhy = "SetAudioInputCallbacks: " + std::to_string(setter.matches) + " matches";
    return false;
  }
  const auto *globals = reinterpret_cast<const uintptr_t *>(setter.globals);
  gameExecute = reinterpret_cast<ExecuteFn>(globals[0]);
  gameFormat = reinterpret_cast<FormatFn>(globals[1]);
  gameGain = reinterpret_cast<GainFn>(globals[2]);
  if (AK().sampleRate) {
    rate = AK().sampleRate();
  }
  setter.fn(&OurExecute, &OurFormat, &OurGain);
  installed = globals[0] == reinterpret_cast<uintptr_t>(&OurExecute);
  spdlog::info("[WwiseSpike] SetAudioInputCallbacks at {}, globals at {}; the game's: {} {} {}; ours {}",
               Hex(reinterpret_cast<uintptr_t>(setter.fn)), Hex(setter.globals),
               Hex(reinterpret_cast<uintptr_t>(gameExecute.load())), Hex(reinterpret_cast<uintptr_t>(gameFormat.load())),
               Hex(reinterpret_cast<uintptr_t>(gameGain.load())), installed ? "in" : "NOT in");
  if (!installed) {
    aWhy = "the setter did not take ours";
  }
  return installed;
}

std::string Transform(const AkWorldTransform &t) {
  return fmt::format("{{\"front\":[{:.3f},{:.3f},{:.3f}],\"top\":[{:.3f},{:.3f},{:.3f}],\"position\":[{:.2f},{:.2f},{:.2f}]}}",
                     t.front.x, t.front.y, t.front.z, t.top.x, t.top.y, t.top.z, t.position.x, t.position.y,
                     t.position.z);
}

// the same 48 bytes read as twelve floats and six doubles: which layout is right shows in which reading is sensible
std::string Raw(const AkWorldTransform &t) {
  float f[12];
  double d[6];
  std::memcpy(f, &t, 48);
  std::memcpy(d, &t, 48);
  std::string s = "{\"floats\":[";
  for (int i = 0; i < 12; ++i) {
    s += fmt::format("{}{:.3f}", i ? "," : "", f[i]);
  }
  s += "],\"doubles\":[";
  for (int i = 0; i < 6; ++i) {
    s += fmt::format("{}{:.3f}", i ? "," : "", d[i]);
  }
  return s + "]}";
}

} // namespace

RED4ext::CString WwiseSpike::Start(RED4ext::CString aEvent, float aFrequency, float aGain) {
  const Ak &ak = AK();
  if (!ak.missing.empty()) {
    return ("{\"error\":\"exports not found: " + ak.missing + "\"}").c_str();
  }
  std::string why;
  if (!Install(why)) {
    return ("{\"error\":\"" + why + "\"}").c_str();
  }
  frequency = aFrequency;
  gain = aGain;
  AKRESULT reg = AK_Success;
  if (!registered) {
    reg = ak.registerGameObj(kObject, "LTBF spike");
    registered = reg == AK_Success;
  }
  if (const AkPlayingID old = ourPlaying.exchange(0)) {
    ak.stopPlayingID(old, 0, 4);
  }
  lastEvent = aEvent.c_str();
  const AkUniqueID event = ak.idFromString(aEvent.c_str());
  const AkPlayingID id = ak.postEvent(event, kObject, 0, nullptr, nullptr, 0, nullptr, 0);
  ourPlaying = id;
  spdlog::info("[WwiseSpike] {} (id {}) on our object: register {}, playing {}", lastEvent, event, reg, id);
  return fmt::format("{{\"event\":\"{}\",\"eventId\":{},\"register\":{},\"playingId\":{},\"sampleRate\":{}}}", lastEvent,
                     event, reg, id, rate.load())
      .c_str();
}

RED4ext::CString WwiseSpike::LoadBank(RED4ext::CString aPath) {
  if (!AK().loadBankMemoryCopy) {
    return "{\"error\":\"LoadBankMemoryCopy not found\"}";
  }
  std::ifstream in(aPath.c_str(), std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.empty()) {
    return "{\"error\":\"could not read the file\"}";
  }
  uint32_t bank = 0;
  const AKRESULT r = AK().loadBankMemoryCopy(bytes.data(), uint32_t(bytes.size()), bank);
  spdlog::info("[WwiseSpike] LoadBankMemoryCopy {} ({} bytes): result {}, bank {}", aPath.c_str(), bytes.size(), r, bank);
  return fmt::format("{{\"result\":{},\"bank\":{},\"bytes\":{}}}", r, bank, bytes.size()).c_str();
}

RED4ext::CString WwiseSpike::Place(float aX, float aY, float aZ, float aFx, float aFy, float aFz) {
  if (!AK().setPosition || !registered) {
    return "{\"error\":\"not started\"}";
  }
  // in Wwise's axes (y up; the game's x, z, y): the facing flattened to the horizontal and up straight up, as Wwise
  // wants them at right angles (the first try's up was not: SetPosition refused it, result 31)
  AkWorldTransform t{};
  const float h = std::sqrt(aFx * aFx + aFz * aFz);
  t.front = h > 1e-4f ? AkVector{aFx / h, 0, aFz / h} : AkVector{0, 0, 1};
  t.top = {0, 1, 0};
  t.position = {aX, aY, aZ};
  lastPlace = AK().setPosition(kObject, t, 3);
  AkWorldTransform back{};
  const AKRESULT r = AK().position ? AK().position(kObject, back) : 0;
  return fmt::format("{{\"set\":{},\"get\":{},\"readBack\":{}}}", lastPlace, r, Transform(back)).c_str();
}

RED4ext::CString WwiseSpike::Listen(bool aOurs, float aX, float aY, float aZ, float aFx, float aFy, float aFz) {
  const Ak &ak = AK();
  if (!registered || !ak.setListeners) {
    return "{\"error\":\"not started\"}";
  }
  if (!aOurs) {
    // back to the game's own (its default listeners): the listener the game set when it registered our object
    AkGameObjectID ids[8]{};
    uint32_t count = 0;
    const AKRESULT r = ak.setListeners(kObject, ids, count);
    return fmt::format("{{\"listener\":\"the game's\",\"set\":{}}}", r).c_str();
  }
  AKRESULT reg = AK_Success;
  if (!listenerRegistered) {
    reg = ak.registerGameObj(kListener, "LTBF listener");
    listenerRegistered = reg == AK_Success;
  }
  AkWorldTransform t{};
  const float h = std::sqrt(aFx * aFx + aFz * aFz);
  t.front = h > 1e-4f ? AkVector{aFx / h, 0, aFz / h} : AkVector{0, 0, 1};
  t.top = {0, 1, 0};
  t.position = {aX, aY, aZ};
  const AKRESULT p = ak.setPosition(kListener, t, 3);
  const AKRESULT l = ak.setListeners(kObject, &kListener, 1);
  return fmt::format("{{\"listener\":\"ours\",\"register\":{},\"place\":{},\"set\":{}}}", reg, p, l).c_str();
}

RED4ext::CString WwiseSpike::State() {
  const Ak &ak = AK();
  AkAudioSettings settings{};
  const AKRESULT s = ak.audioSettings ? ak.audioSettings(settings) : 0;
  AkGameObjectID ids[8]{};
  uint32_t count = 8;
  const AKRESULT l = ak.listeners && registered ? ak.listeners(kObject, ids, count) : 0;
  std::string listeners = "[";
  for (uint32_t i = 0; l == AK_Success && i < count && i < 8; ++i) {
    AkWorldTransform t{};
    const AKRESULT p = ak.listenerPosition(ids[i], t);
    listeners += fmt::format("{}{{\"id\":\"{}\",\"result\":{},\"as\":{},\"raw\":{}}}", i ? "," : "", Hex(ids[i]), p,
                             Transform(t), Raw(t));
  }
  listeners += "]";
  AkWorldTransform ours{};
  const AKRESULT o = ak.position && registered ? ak.position(kObject, ours) : 0;
  return fmt::format(
             "{{\"installed\":{},\"matches\":{},\"setter\":\"{}\",\"gameExecute\":\"{}\",\"playingId\":{},"
             "\"oursCalls\":{},\"passedCalls\":{},\"formatCalls\":{},\"ourFormats\":{},\"frames\":{},\"config\":\"{}\","
             "\"defaultFormat\":{{\"rate\":{},\"config\":\"{}\",\"bits\":\"{}\"}},\"sampleRate\":{},"
             "\"settings\":{{\"result\":{},\"samplesPerFrame\":{},\"samplesPerSecond\":{}}},\"listeners\":{},"
             "\"ours\":{{\"result\":{},\"as\":{}}}}}",
             installed, setter.matches, Hex(reinterpret_cast<uintptr_t>(setter.fn)),
             Hex(reinterpret_cast<uintptr_t>(gameExecute.load())), ourPlaying.load(), oursCalls.load(),
             passedCalls.load(), formatCalls.load(), ourFormats.load(), lastFrames.load(), Hex(lastConfig.load()),
             defaultRate.load(), Hex(defaultConfig.load()), Hex(defaultBits.load()), rate.load(), s,
             settings.samplesPerFrame, settings.samplesPerSecond, listeners, o, Transform(ours))
      .c_str();
}

RED4ext::CString WwiseSpike::Stop() {
  if (const AkPlayingID old = ourPlaying.exchange(0); old && AK().stopPlayingID) {
    AK().stopPlayingID(old, 0, 4);
  }
  AKRESULT u = 0;
  if (registered && AK().unregisterGameObj) {
    u = AK().unregisterGameObj(kObject);
    registered = false;
  }
  return fmt::format("{{\"stopped\":true,\"unregister\":{}}}", u).c_str();
}
