#include "WwiseSpike.hpp"

#include "Bank.hpp"
#include "Ids.hpp"
#include "Link.hpp"
#include "Relay.hpp"

#include <spdlog/spdlog.h>

#include <atomic>
#include <bit>
#include <cmath>
#include <fstream>
#include <numbers>
#include <string>
#include <vector>

using namespace Wwise;

namespace {

// the tone, rendered on Wwise's audio thread
struct Sine : Source {
  std::atomic<float> frequency{440.0f}, gain{0.2f};
  std::atomic<uint32_t> rate{48000};
  double phase = 0; // the audio thread's

  void Render(float *aData, uint32_t aFrames, uint32_t aChannels) override {
    const double step = 2.0 * std::numbers::pi * frequency.load(std::memory_order_relaxed) /
                        double(rate.load(std::memory_order_relaxed));
    const float g = gain.load(std::memory_order_relaxed);
    for (uint32_t i = 0; i < aFrames; ++i) {
      const float v = g * float(std::sin(phase));
      phase += step;
      for (uint32_t c = 0; c < aChannels; ++c) {
        aData[size_t(c) * aFrames + i] = v;
      }
    }
    phase = std::fmod(phase, 2.0 * std::numbers::pi);
  }
};
Sine sine;

// the spike's sounds in LTBF's own bank (LoadOurBank): positioned with no attenuation, and 2D; then mod_sfx_street's
// copies, whole and less one thing each, to find what makes the first 40 dB quieter than mod_sfx_street (the first
// try in game)
constexpr const char *kBank = "ltbf_spike";
uint32_t Bits(float aValue) {
  return std::bit_cast<uint32_t>(aValue);
}
// mod_sfx_street's: its actor mixer, two properties and its attenuation (by ID, in mod.bnk), its aux send
constexpr AkUniqueID kStreetParent = 453120594, kStreetAttenuation = 463889900, kStreetAux = 0x37ec68c6;
const Bank::Prop kStreetProps[] = {{0x0c, Bits(-16.0f)}, {0x29, Bits(100.0f)}, {0x55, kStreetAttenuation}};
const Bank::Prop kStreetPropsNoAttenuation[] = {{0x0c, Bits(-16.0f)}, {0x29, Bits(100.0f)}};
const Bank::Sound kSounds[] = {
    {"ltbf_spike_3d"},
    {"ltbf_spike_2d", false},
    {"ltbf_spike_street", true, Bank::kSfxBus, kStreetParent, kStreetProps, true, kStreetAux},
    {"ltbf_spike_street_no_attenuation", true, Bank::kSfxBus, kStreetParent, kStreetPropsNoAttenuation, false,
     kStreetAux},
    {"ltbf_spike_street_no_parent", true, Bank::kSfxBus, 0, kStreetProps, true, kStreetAux},
    {"ltbf_spike_street_no_aux", true, Bank::kSfxBus, kStreetParent, kStreetProps, true, 0},
    {"ltbf_spike_3d_props", true, Bank::kSfxBus, 0, kStreetPropsNoAttenuation},
};

const AkGameObjectID kObject = ObjectId(1);
// a listener of ours, for the camera listener (spike question 6)
const AkGameObjectID kListener = ObjectId(2);
bool registered = false, listenerRegistered = false;
std::atomic<AkPlayingID> playing{0};
AKRESULT lastPlace = 0;

std::string Hex(uint64_t v) {
  return fmt::format("{:#x}", v);
}

std::string Transform(const AkWorldTransform &t) {
  return fmt::format("{{\"front\":[{:.3f},{:.3f},{:.3f}],\"top\":[{:.3f},{:.3f},{:.3f}],\"position\":[{:.2f},{:.2f},{:.2f}]}}",
                     t.front.x, t.front.y, t.front.z, t.top.x, t.top.y, t.top.z, t.position.x, t.position.y,
                     t.position.z);
}

// in Wwise's axes (y up; the game's x, z, y): the facing flattened to the horizontal and up straight up
AkWorldTransform Flat(float aX, float aY, float aZ, float aFx, float aFz) {
  AkWorldTransform t{};
  const float h = std::sqrt(aFx * aFx + aFz * aFz);
  t.front = h > 1e-4f ? AkVector{aFx / h, 0, aFz / h} : AkVector{0, 0, 1};
  t.top = {0, 1, 0};
  t.position = {aX, aY, aZ};
  return t;
}

} // namespace

RED4ext::CString WwiseSpike::Start(RED4ext::CString aEvent, float aFrequency, float aGain) {
  const Link &ak = Link::Get();
  if (!ak.missing.empty()) {
    return ("{\"error\":\"exports not found: " + ak.missing + "\"}").c_str();
  }
  std::string why;
  if (!InstallRelay(why)) {
    return ("{\"error\":\"" + why + "\"}").c_str();
  }
  KeepRelayInstalled();
  sine.frequency = aFrequency;
  sine.gain = aGain;
  sine.rate = ak.sampleRate();
  AKRESULT reg = AK_Success;
  if (!registered) {
    reg = ak.registerGameObj(kObject, "LTBF spike");
    registered = reg == AK_Success;
  }
  if (const AkPlayingID old = playing.exchange(0)) {
    TheRelay().Unbind(old);
    ak.stopPlayingID(old, 0, 4);
  }
  const AkUniqueID event = ak.idFromString(aEvent.c_str());
  const AkPlayingID id = ak.postEvent(event, kObject, 0, nullptr, nullptr, 0, nullptr, 0);
  TheRelay().Bind(id, &sine);
  playing = id;
  spdlog::info("[WwiseSpike] {} (id {}) on our object: register {}, playing {}", aEvent.c_str(), event, reg, id);
  return fmt::format("{{\"event\":\"{}\",\"eventId\":{},\"register\":{},\"playingId\":{},\"sampleRate\":{}}}",
                     aEvent.c_str(), event, reg, id, sine.rate.load())
      .c_str();
}

RED4ext::CString WwiseSpike::LoadBank(RED4ext::CString aPath) {
  if (!Link::Get().loadBankMemoryCopy) {
    return "{\"error\":\"LoadBankMemoryCopy not found\"}";
  }
  std::ifstream in(aPath.c_str(), std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.empty()) {
    return "{\"error\":\"could not read the file\"}";
  }
  uint32_t bank = 0;
  const AKRESULT r = Link::Get().loadBankMemoryCopy(bytes.data(), uint32_t(bytes.size()), bank);
  spdlog::info("[WwiseSpike] LoadBankMemoryCopy {} ({} bytes): result {}, bank {}", aPath.c_str(), bytes.size(), r, bank);
  return fmt::format("{{\"result\":{},\"bank\":{},\"bytes\":{}}}", r, bank, bytes.size()).c_str();
}

RED4ext::CString WwiseSpike::LoadOurBank() {
  if (!Link::Get().loadBankMemoryCopy) {
    return "{\"error\":\"LoadBankMemoryCopy not found\"}";
  }
  static bool loaded = false;
  static AKRESULT result = 0;
  static uint32_t bank = 0;
  const std::vector<uint8_t> bytes = Bank::Build(kBank, kSounds);
  if (!loaded) {
    result = Link::Get().loadBankMemoryCopy(bytes.data(), uint32_t(bytes.size()), bank);
    loaded = result == AK_Success;
    spdlog::info("[WwiseSpike] our bank ({} bytes): result {}, bank {}", bytes.size(), result, bank);
  }
  return fmt::format("{{\"result\":{},\"bank\":{},\"expected\":{},\"bytes\":{}}}", result, bank, Fnv1(kBank),
                     bytes.size())
      .c_str();
}

RED4ext::CString WwiseSpike::Place(float aX, float aY, float aZ, float aFx, float aFy, float aFz) {
  if (!Link::Get().setPosition || !registered) {
    return "{\"error\":\"not started\"}";
  }
  lastPlace = Link::Get().setPosition(kObject, Flat(aX, aY, aZ, aFx, aFz), 3);
  AkWorldTransform back{};
  const AKRESULT r = Link::Get().position(kObject, back);
  return fmt::format("{{\"set\":{},\"get\":{},\"readBack\":{}}}", lastPlace, r, Transform(back)).c_str();
}

RED4ext::CString WwiseSpike::Listen(bool aOurs, float aX, float aY, float aZ, float aFx, float aFy, float aFz) {
  const Link &ak = Link::Get();
  if (!registered || !ak.setListeners) {
    return "{\"error\":\"not started\"}";
  }
  if (!aOurs) {
    // the game's default listeners again (SetListeners with none, the first try, left it none)
    const AKRESULT r = ak.resetListenersToDefault(kObject);
    return fmt::format("{{\"listener\":\"the game's\",\"set\":{}}}", r).c_str();
  }
  AKRESULT reg = AK_Success;
  if (!listenerRegistered) {
    reg = ak.registerGameObj(kListener, "LTBF listener");
    listenerRegistered = reg == AK_Success;
  }
  const AKRESULT p = ak.setPosition(kListener, Flat(aX, aY, aZ, aFx, aFz), 3);
  const AKRESULT l = ak.setListeners(kObject, &kListener, 1);
  return fmt::format("{{\"listener\":\"ours\",\"register\":{},\"place\":{},\"set\":{}}}", reg, p, l).c_str();
}

RED4ext::CString WwiseSpike::State() {
  const Link &ak = Link::Get();
  KeepRelayInstalled();
  AkAudioSettings settings{};
  const AKRESULT s = ak.audioSettings ? ak.audioSettings(settings) : 0;
  AkGameObjectID ids[8]{};
  uint32_t count = 8;
  const AKRESULT l = ak.listeners && registered ? ak.listeners(kObject, ids, count) : 0;
  std::string listeners = "[";
  for (uint32_t i = 0; l == AK_Success && i < count && i < 8; ++i) {
    AkWorldTransform t{};
    const AKRESULT p = ak.listenerPosition(ids[i], t);
    listeners += fmt::format("{}{{\"id\":\"{}\",\"result\":{},\"as\":{}}}", i ? "," : "", Hex(ids[i]), p, Transform(t));
  }
  listeners += "]";
  AkWorldTransform ours{};
  const AKRESULT o = ak.position && registered ? ak.position(kObject, ours) : 0;
  const RelayInstall install = RelayInstalled();
  const Relay::Stats stats = TheRelay().Read();
  return fmt::format(
             "{{\"installed\":{},\"matches\":{},\"setter\":\"{}\",\"gameExecute\":\"{}\",\"reinstalls\":{},"
             "\"playingId\":{},\"oursCalls\":{},\"passedCalls\":{},\"unanswered\":{},\"loops\":{},\"ourFormats\":{},"
             "\"frames\":{},\"config\":\"{}\",\"sampleRate\":{},"
             "\"settings\":{{\"result\":{},\"samplesPerFrame\":{},\"samplesPerSecond\":{}}},\"listeners\":{},"
             "\"ours\":{{\"result\":{},\"as\":{}}}}}",
             install.installed, install.matches, Hex(install.setter),
             Hex(reinterpret_cast<uintptr_t>(TheRelay().Passed().execute)), install.reinstalls, playing.load(),
             stats.ours, stats.passed, stats.unanswered, stats.loops, stats.formats, stats.frames, Hex(stats.config),
             sine.rate.load(), s, settings.samplesPerFrame, settings.samplesPerSecond, listeners, o, Transform(ours))
      .c_str();
}

RED4ext::CString WwiseSpike::Stop() {
  if (const AkPlayingID old = playing.exchange(0); old && Link::Get().stopPlayingID) {
    TheRelay().Unbind(old);
    Link::Get().stopPlayingID(old, 0, 4);
  }
  AKRESULT u = 0;
  if (registered && Link::Get().unregisterGameObj) {
    u = Link::Get().unregisterGameObj(kObject);
    registered = false;
  }
  return fmt::format("{{\"stopped\":true,\"unregister\":{}}}", u).c_str();
}
