// The Wwise backend (Out.hpp): LTBF's voices as game objects of ours in the game's Wwise, their samples handed over
// by the relay (Audio/Wwise), from LTBF's own bank (Audio/Wwise/Bank): a positioned sound with mod_sfx_street's
// spread and no attenuation, and a 2D one. Each voice's gain and low-pass are in its samples (Mix); Wwise positions it
// for a listener of ours.
#include "Backend.hpp"
#include "Mix.hpp"

#include "Audio/Wwise/Axes.hpp"
#include "Audio/Wwise/Bank.hpp"
#include "Audio/Wwise/Ids.hpp"
#include "Audio/Wwise/Link.hpp"
#include "Audio/Wwise/Relay.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <vector>

namespace Out {

namespace {

using namespace Wwise;

constexpr const char *kBank = "ltbf";
constexpr const char *kPositioned = "ltbf_voice_3d";
constexpr const char *kFlat = "ltbf_voice_2d";
constexpr const char *kSpread = "ltbf_spread";
const Bank::Spread kSpreads[] = {{kSpread, Bank::kStreetSpread}};
const Bank::Sound kSounds[] = {{kPositioned, true, Bank::kSfxBus, 0, {}, false, 0, Fnv1(kSpread)}, {kFlat, false}};

// our listener: the first of our game objects; the voices' from kFirstVoice on
const AkGameObjectID kListener = ObjectId(1);
constexpr uint32_t kFirstVoice = 0x100;

class WwiseVoice;

// what the audio thread reads of the backend
struct Shared {
  std::atomic<float> bus[3] = {1.0f, 1.0f, 1.0f};
  std::atomic<bool> paused{false};
  std::atomic<uint32_t> rate{48000};
};

AkVector64 Game(const Voices::Vec3 &v) {
  return {v.x, v.y, v.z};
}

class WwiseBackend;

class WwiseVoice final : public Voice, public Source {
public:
  WwiseVoice(WwiseBackend &aBackend, RenderFn aRender, void *aUser, const Options &aOptions, AkGameObjectID aObject);
  ~WwiseVoice() override;

  void SetPaused(bool aPaused) override { paused.store(aPaused); }
  void SetVolume(float aVolume) override { volume.store(aVolume, std::memory_order_relaxed); }
  void SetLowpass(float aHz) override { cutoff.store(aHz, std::memory_order_relaxed); }
  void Place(const Placement &aPlacement) override;

  // game thread, once a frame: starts or stops the Wwise voice as wanted
  void Sync(bool aReady, AkGameObjectID aListener);
  bool Playing() const { return playing != 0; }

  uint32_t Channels() const override { return options.channels; }
  void Render(float *aData, uint32_t aFrames, uint32_t aChannels) override;

private:
  void StopPlaying();

  WwiseBackend &backend;
  RenderFn render;
  void *user;
  Options options;
  AkGameObjectID object;
  // game thread
  AkPlayingID playing = 0;
  bool registered = false, placed = false;
  AkWorldTransform transform{};
  // set on the game thread, read on the audio thread
  std::atomic<bool> paused{true}, fresh{true};
  std::atomic<float> volume{0}, rolloff{1}, cutoff{0};
  // the audio thread's
  Mix mix;
};

class WwiseBackend final : public Detail::Backend {
public:
  bool Start(std::string &aWhy) {
    const Link &ak = Link::Get();
    if (!ak.missing.empty()) {
      aWhy = "exports not found: " + ak.missing;
      return false;
    }
    // the callbacks go in now (the game may set its own later: Update puts ours back in front); the bank and the
    // listener wait for Wwise to be up, on the first frame
    return InstallRelay(aWhy);
  }

  ~WwiseBackend() override {
    // the plugin is unloading with the game: the relay out first (Wwise's audio thread must not call into a DLL that
    // goes), and Wwise left alone after, as it may be shutting down too. The voices are their owners' and are gone
    // by now (FlightAudio::Unload).
    UninstallRelay();
  }

  std::unique_ptr<Voice> Play(RenderFn aRender, void *aUser, const Options &aOptions) override {
    auto voice = std::make_unique<WwiseVoice>(*this, aRender, aUser, aOptions, ObjectId(kFirstVoice + nextObject++));
    voices.push_back(voice.get());
    return voice;
  }

  void Forget(WwiseVoice *aVoice) { voices.erase(std::remove(voices.begin(), voices.end(), aVoice), voices.end()); }

  void SetBusVolume(Buses::Bus aBus, float aVolume) override {
    shared.bus[int(aBus)].store(aVolume, std::memory_order_relaxed);
  }

  void SetListener(const Pose &l) override {
    listener = ToWwise({l.position.x, l.position.y, l.position.z}, {l.forward.x, l.forward.y, l.forward.z},
                       {l.up.x, l.up.y, l.up.z});
    if (listenerRegistered) {
      lastListenerResult = Link::Get().setPosition(kListener, listener, 3);
    }
  }

  void Update() override {
    if (!ready) {
      Begin();
    }
    KeepRelayInstalled();
    for (WwiseVoice *v : voices) {
      v->Sync(ready, kListener);
    }
  }

  void SetPaused(bool aPaused) override { shared.paused.store(aPaused); }

  int Rate() const override { return int(shared.rate.load()); }

  std::string State() const override {
    const RelayInstall install = RelayInstalled();
    const Relay::Stats s = TheRelay().Read();
    int playingNow = 0;
    for (const WwiseVoice *v : voices) {
      playingNow += v->Playing();
    }
    return fmt::format("{{\"backend\":\"wwise\",\"ready\":{},\"bank\":{},\"listener\":{},\"voices\":{},"
                       "\"playing\":{},\"posts\":{},\"failedPosts\":{},\"relay\":{{\"installed\":{},\"reinstalls\":{},"
                       "\"ours\":{},\"passed\":{},\"unanswered\":{},\"loops\":{},\"frames\":{}}}}}",
                       ready, bankResult, lastListenerResult, voices.size(), playingNow, posts, failedPosts,
                       install.installed, install.reinstalls, s.ours, s.passed, s.unanswered, s.loops, s.frames);
  }

  Shared shared;
  uint64_t posts = 0, failedPosts = 0;

private:
  // Wwise is up by the first game frame: our bank and our listener
  void Begin() {
    const Link &ak = Link::Get();
    if (const uint32_t rate = ak.sampleRate()) {
      shared.rate.store(rate);
      TheRelay().SetRate(rate);
    }
    const std::vector<uint8_t> bytes = Bank::Build(kBank, kSounds, kSpreads);
    uint32_t bank = 0;
    bankResult = ak.loadBankMemoryCopy(bytes.data(), uint32_t(bytes.size()), bank);
    listenerRegistered = ak.registerGameObj(kListener, "LTBF listener") == AK_Success;
    lastListenerResult = ak.setPosition(kListener, listener, 3);
    // a bank already loaded (a second start in one session) is as good
    ready = listenerRegistered && (bankResult == AK_Success || bank == Fnv1(kBank));
    spdlog::info("[Out] Wwise: our bank ({} bytes) {}, listener {}: {}", bytes.size(), bankResult,
                 listenerRegistered ? "registered" : "NOT registered", ready ? "ready" : "NOT ready");
  }

  std::vector<WwiseVoice *> voices;
  uint32_t nextObject = 0;
  bool ready = false, listenerRegistered = false;
  AKRESULT bankResult = 0, lastListenerResult = 0;
  AkWorldTransform listener = ToWwise({0, 0, 0}, {0, 1, 0}, {0, 0, 1});
};

WwiseVoice::WwiseVoice(WwiseBackend &aBackend, RenderFn aRender, void *aUser, const Options &aOptions,
                       AkGameObjectID aObject)
    : backend(aBackend), render(aRender), user(aUser), options(aOptions), object(aObject) {
  options.channels = std::clamp<uint32_t>(options.channels, 1, Mix::kMaxChannels);
}

WwiseVoice::~WwiseVoice() {
  StopPlaying();
  if (registered) {
    Link::Get().unregisterGameObj(object);
  }
  backend.Forget(this);
}

void WwiseVoice::Place(const Placement &p) {
  transform = ToWwise({p.position.x, p.position.y, p.position.z}, {p.forward.x, p.forward.y, p.forward.z},
                      {p.up.x, p.up.y, p.up.z});
  rolloff.store(p.rolloff, std::memory_order_relaxed);
  placed = true;
  if (registered) {
    Link::Get().setPosition(object, transform, 3);
  }
}

void WwiseVoice::StopPlaying() {
  if (!playing) {
    return;
  }
  // out of the relay first: once Unbind returns the audio thread is not in Render
  TheRelay().Unbind(playing);
  Link::Get().stopPlayingID(playing, 0, 4);
  playing = 0;
}

void WwiseVoice::Sync(bool aReady, AkGameObjectID aListener) {
  const Link &ak = Link::Get();
  // a positioned voice waits for its place: one started at the origin, out of range, is dropped for good (spike)
  const bool want = aReady && !paused.load() && (placed || !options.positioned);
  if (!want) {
    StopPlaying();
    return;
  }
  if (playing) {
    return;
  }
  if (!registered) {
    registered = ak.registerGameObj(object, options.name) == AK_Success;
    if (!registered) {
      return;
    }
    ak.setListeners(object, &aListener, 1);
    if (placed) {
      ak.setPosition(object, transform, 3);
    }
  }
  fresh.store(true);
  playing = ak.postEvent(Fnv1(options.positioned ? kPositioned : kFlat), object, 0, nullptr, nullptr, 0, nullptr, 0);
  if (!playing || !TheRelay().Bind(playing, this)) {
    ++backend.failedPosts;
    if (playing) {
      ak.stopPlayingID(playing, 0, 4);
      playing = 0;
    }
    return;
  }
  ++backend.posts;
}

void WwiseVoice::Render(float *aData, uint32_t aFrames, uint32_t aChannels) {
  if (fresh.exchange(false)) {
    mix.Reset();
  }
  if (paused.load(std::memory_order_relaxed) || backend.shared.paused.load(std::memory_order_relaxed)) {
    std::memset(aData, 0, size_t(aFrames) * aChannels * sizeof(float));
    mix.Reset();
    return;
  }
  render(user, aData, aFrames, aChannels);
  // FMOD's Object Spatializer sums a voice's two channels (the generator's mono in both) into its object, so a voice
  // there is 6 dB over its generator: the same here, so the backends sound alike
  const float fold = options.positioned && options.channels == 1 ? 2.0f : 1.0f;
  const float gain = fold * volume.load(std::memory_order_relaxed) * rolloff.load(std::memory_order_relaxed) *
                     backend.shared.bus[int(options.bus)].load(std::memory_order_relaxed);
  mix.Process(aData, aFrames, aChannels, float(backend.shared.rate.load(std::memory_order_relaxed)), gain,
              options.lowpass ? cutoff.load(std::memory_order_relaxed) : 0.0f);
}

} // namespace

namespace Detail {

std::unique_ptr<Backend> MakeWwise(std::string &aWhy) {
  auto backend = std::make_unique<WwiseBackend>();
  if (!backend->Start(aWhy)) {
    return nullptr;
  }
  return backend;
}

} // namespace Detail

} // namespace Out
