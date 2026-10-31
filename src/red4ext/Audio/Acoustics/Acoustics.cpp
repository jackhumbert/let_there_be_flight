#include "Acoustics.hpp"
#include "Reflections.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_studio.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <cstring>
#include <mutex>
#include <optional>
#include <thread>

namespace Acoustics {

namespace {

using Clock = std::chrono::steady_clock;

constexpr float kProbeHz = 20.0f;
// SetCar() older than this: the car is not flying any more
constexpr auto kStale = std::chrono::milliseconds(250);
constexpr float kFadeSeconds = 0.05f;
// in first person the echoes drop by this much (10 dB); the cabin muffle is on the dry sound already
constexpr float kInsideDrop = 0.68f;

struct Pose {
  Vec3 position, forward, up, listener;
  bool haveListener = false;
  float gain = 0, inside = 0;
  Clock::time_point carTime{};
};

std::mutex poseLock;
Pose pose;

// worker -> mixer: the newest Params, numbered from 1 (0: none yet)
std::mutex paramsLock;
Params params;
std::atomic<uint32_t> paramsSeq{0};
// the echoes' level the DSPs fade to (0: off)
std::atomic<float> level{0.0f};

// for State()
std::mutex stateLock;
Features lastFeatures{};
Params lastParams{};
float lastProbeMs = 0;
std::atomic<uint64_t> probes{0}, blocks{0}, silentBlocks{0};

std::mutex wakeLock;
std::condition_variable wake;
bool running = false;
std::thread worker;

struct Instance {
  explicit Instance(float aRate)
      : reflections(aRate, int(std::lround(aRate / kProbeHz))), fadeStep(1.0f / (kFadeSeconds * aRate)) {}
  Reflections reflections;
  uint32_t seq = 0;
  float amount = 0;
  float fadeStep;
  bool idle = true;
};

struct Attached {
  FMOD::Studio::Bus *bus = nullptr;
  FMOD::ChannelGroup *group = nullptr;
  FMOD::DSP *dsp = nullptr;
};
std::vector<Attached> attached;

bool Check(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    spdlog::error("[Acoustics] {}: FMOD error {} - {}", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

FMOD_RESULT F_CALL DspCreate(FMOD_DSP_STATE *aState) {
  int rate = 48000;
  aState->functions->getsamplerate(aState, &rate);
  aState->plugindata = new Instance(float(rate));
  return FMOD_OK;
}

FMOD_RESULT F_CALL DspRelease(FMOD_DSP_STATE *aState) {
  delete static_cast<Instance *>(aState->plugindata);
  aState->plugindata = nullptr;
  return FMOD_OK;
}

FMOD_RESULT F_CALL DspReset(FMOD_DSP_STATE *aState) {
  auto &i = *static_cast<Instance *>(aState->plugindata);
  i.reflections.Reset();
  i.seq = 0;
  i.amount = 0;
  i.idle = true;
  return FMOD_OK;
}

FMOD_RESULT F_CALL DspRead(FMOD_DSP_STATE *aState, float *aIn, float *aOut, unsigned int aLength, int aInChannels,
                           int *aOutChannels) {
  auto &i = *static_cast<Instance *>(aState->plugindata);
  *aOutChannels = aInChannels;
  const size_t samples = size_t(aLength) * aInChannels;
  const float target = level.load(std::memory_order_relaxed);
  if (target <= 0 && i.amount <= 0) {
    // off: passed through, and started afresh when on again
    std::memcpy(aOut, aIn, samples * sizeof(float));
    i.idle = true;
    return FMOD_OK;
  }
  if (i.idle) {
    i.reflections.Reset();
    i.seq = 0;
    i.idle = false;
  }
  // the newest Params, never waiting on the worker
  const uint32_t seq = paramsSeq.load(std::memory_order_acquire);
  if (seq != i.seq) {
    std::unique_lock guard(paramsLock, std::try_to_lock);
    if (guard.owns_lock()) {
      Params p = params;
      i.seq = paramsSeq.load(std::memory_order_relaxed);
      guard.unlock();
      i.reflections.SetTargets(p);
    }
  }
  if (i.seq == 0) {
    std::memcpy(aOut, aIn, samples * sizeof(float));
    return FMOD_OK;
  }
  i.reflections.Process(aIn, aOut, aLength, aInChannels);
  // fade the echoes (out - in) to the level
  for (unsigned int s = 0; s < aLength; ++s) {
    i.amount += std::clamp(target - i.amount, -i.fadeStep, i.fadeStep);
    for (int c = 0; c < aInChannels; ++c) {
      size_t k = size_t(s) * aInChannels + c;
      aOut[k] = aIn[k] + i.amount * (aOut[k] - aIn[k]);
    }
  }
  blocks.fetch_add(1, std::memory_order_relaxed);
  return FMOD_OK;
}

FMOD_RESULT F_CALL DspShouldIProcess(FMOD_DSP_STATE *aState, FMOD_BOOL aInputsIdle, unsigned int, FMOD_CHANNELMASK,
                                     int, FMOD_SPEAKERMODE) {
  auto &i = *static_cast<Instance *>(aState->plugindata);
  if (!aInputsIdle) {
    return FMOD_OK;
  }
  // nothing coming in: keep going while the echoes and reverb still ring
  if (i.amount > 0 && i.reflections.TailLevel() > 1e-12f) {
    return FMOD_OK;
  }
  silentBlocks.fetch_add(1, std::memory_order_relaxed);
  return FMOD_ERR_DSP_SILENCE;
}

void Run() {
  std::optional<Probe> probe;
  Tuning tuning;
  const auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(1.0f / kProbeHz));
  auto next = Clock::now();
  auto lastLog = next;
  std::unique_lock wakeGuard(wakeLock);
  while (running) {
    next = std::max(next + period, Clock::now() - period);
    wake.wait_until(wakeGuard, next, [] { return !running; });
    if (!running) {
      break;
    }
    Pose p;
    {
      std::lock_guard guard(poseLock);
      p = pose;
    }
    auto &map = Navigation::NavMap::Get();
    if (p.gain <= 0 || Clock::now() - p.carTime > kStale || !map.IsLoaded()) {
      level.store(0.0f, std::memory_order_relaxed);
      continue;
    }
    wakeGuard.unlock();
    if (!probe) {
      probe.emplace(map);
    }
    const auto t0 = Clock::now();
    const Features f = (*probe)(p.position, p.forward, p.up);
    tuning.gain = p.gain;
    const Params prm = Targets(f, p.position, p.haveListener ? p.listener : p.position, tuning);
    const float ms = std::chrono::duration<float, std::milli>(Clock::now() - t0).count();
    {
      std::lock_guard guard(paramsLock);
      params = prm;
      uint32_t seq = paramsSeq.load(std::memory_order_relaxed) + 1;
      paramsSeq.store(seq ? seq : 1, std::memory_order_release);
    }
    level.store(1.0f - kInsideDrop * std::clamp(p.inside, 0.0f, 1.0f), std::memory_order_relaxed);
    {
      std::lock_guard guard(stateLock);
      lastFeatures = f;
      lastParams = prm;
      lastProbeMs = ms;
    }
    probes.fetch_add(1, std::memory_order_relaxed);
    if (Clock::now() - lastLog > std::chrono::seconds(1)) {
      lastLog = Clock::now();
      spdlog::debug("[Acoustics] floor {:.1f} ceiling {:.1f} left {:.1f} right {:.1f} enclosure {:.2f} rt60 {:.2f} "
                    "wet {:.3f}, probe {:.2f} ms",
                    f.floor, f.ceiling, f.left, f.right, f.enclosure, prm.rt60, prm.wet, ms);
    }
    wakeGuard.lock();
  }
}

} // namespace

bool Attach(FMOD::Studio::System *aStudio, const std::vector<std::string> &aBuses) {
  static FMOD_DSP_DESCRIPTION description = [] {
    FMOD_DSP_DESCRIPTION d{};
    d.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
    std::strncpy(d.name, "LTBF acoustics", sizeof(d.name) - 1);
    d.version = 1;
    d.numinputbuffers = 1;
    d.numoutputbuffers = 1;
    d.create = DspCreate;
    d.release = DspRelease;
    d.reset = DspReset;
    d.read = DspRead;
    d.shouldiprocess = DspShouldIProcess;
    return d;
  }();
  FMOD::System *core = nullptr;
  if (!Check(aStudio->getCoreSystem(&core), "getCoreSystem")) {
    return false;
  }
  // a bus's channel group exists once it is locked and the command has gone through
  std::vector<FMOD::Studio::Bus *> buses;
  for (const auto &name : aBuses) {
    FMOD::Studio::Bus *bus = nullptr;
    if (Check(aStudio->getBus(name.c_str(), &bus), name.c_str()) && Check(bus->lockChannelGroup(), "lockChannelGroup")) {
      buses.push_back(bus);
    }
  }
  Check(aStudio->flushCommands(), "flushCommands");
  bool ok = buses.size() == aBuses.size();
  for (auto *bus : buses) {
    Attached a{bus};
    if (Check(bus->getChannelGroup(&a.group), "getChannelGroup") && Check(core->createDSP(&description, &a.dsp), "createDSP") &&
        Check(a.group->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, a.dsp), "addDSP")) {
      attached.push_back(a);
    } else {
      if (a.dsp) {
        a.dsp->release();
      }
      bus->unlockChannelGroup();
      ok = false;
    }
  }
  if (!attached.empty() && !running) {
    running = true;
    worker = std::thread(Run);
  }
  spdlog::info("[Acoustics] on {} of {} buses", attached.size(), aBuses.size());
  return ok;
}

void Detach() {
  {
    std::lock_guard guard(wakeLock);
    running = false;
  }
  wake.notify_all();
  if (worker.joinable()) {
    worker.join();
  }
  level.store(0.0f);
  for (auto &a : attached) {
    a.group->removeDSP(a.dsp);
    a.dsp->release();
    a.bus->unlockChannelGroup();
  }
  attached.clear();
}

void SetCar(const Vec3 &aPosition, const Vec3 &aForward, const Vec3 &aUp, float aGain, float aInside) {
  std::lock_guard guard(poseLock);
  pose.position = aPosition;
  pose.forward = aForward;
  pose.up = aUp;
  pose.gain = aGain;
  pose.inside = aInside;
  pose.carTime = Clock::now();
}

void SetListener(const Vec3 &aPosition) {
  std::lock_guard guard(poseLock);
  pose.listener = aPosition;
  pose.haveListener = true;
}

std::string State() {
  Features f;
  Params p;
  float ms;
  {
    std::lock_guard guard(stateLock);
    f = lastFeatures;
    p = lastParams;
    ms = lastProbeMs;
  }
  float gain, inside;
  bool fresh;
  {
    std::lock_guard guard(poseLock);
    gain = pose.gain;
    inside = pose.inside;
    fresh = Clock::now() - pose.carTime < kStale;
  }
  // JSON has no infinity: a miss is null
  auto num = [](float x) { return std::isfinite(x) ? fmt::format("{:.3f}", x) : std::string("null"); };
  return fmt::format(
      "{{\"buses\":{},\"flying\":{},\"gain\":{},\"inside\":{},\"level\":{},\"probes\":{},\"probeMs\":{},"
      "\"blocks\":{},\"silentBlocks\":{},\"floor\":{},\"ceiling\":{},\"left\":{},\"right\":{},\"enclosure\":{},"
      "\"meanFreePath\":{},\"floorDelay\":{},\"floorGain\":{},\"leftDelay\":{},\"rightDelay\":{},"
      "\"floorFeedback\":{},\"wallFeedback\":{},\"rt60\":{},\"wet\":{}}}",
      attached.size(), fresh ? "true" : "false", num(gain), num(inside), num(level.load()), probes.load(), num(ms),
      blocks.load(), silentBlocks.load(), num(f.floor), num(f.ceiling), num(f.left), num(f.right), num(f.enclosure),
      num(f.meanFreePath), num(p.floor.delay), num(p.floor.gain), num(p.left.delay), num(p.right.delay),
      num(p.floorFeedback), num(p.wallFeedback), num(p.rt60), num(p.wet));
}

} // namespace Acoustics
