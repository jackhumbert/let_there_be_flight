#include "Acoustics.hpp"
#include "Reflections.hpp"
#include "Stems.hpp"

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
#include <unordered_map>

namespace Acoustics {

namespace {

using Clock = std::chrono::steady_clock;

constexpr float kProbeHz = 20.0f;
// SetCar() older than this: the car is not flying any more
constexpr auto kStale = std::chrono::milliseconds(250);
constexpr float kFadeSeconds = 0.05f;
// in first person the echoes drop by this much (10 dB); the cabin muffle is on the dry sound already
constexpr float kInsideDrop = 0.68f;

// live hits older than this, or cast further than this from where the car is, are not used
constexpr auto kLiveStale = std::chrono::milliseconds(200);
constexpr float kLiveDrift = 10.0f;

struct Pose {
  Vec3 position, forward, up, listener;
  bool haveListener = false;
  float gain = 0, inside = 0;
  Clock::time_point carTime{};
  RaySource rays = RaySource::None;
  LiveHits live{};
  std::array<std::string, kNearRays> liveMaterial;
  RaySource liveSource = RaySource::None;
  Clock::time_point liveTime{};
};

// the last PhysX cast (PhysXRaysDue), in steady clock ticks; for State(): calls from the caster,
// casts, and the last candidate's distance from the reported car
std::atomic<int64_t> lastPhysXCast{0};
std::atomic<uint64_t> physxCalls{0}, physxCasts{0};
std::atomic<float> physxGap{-1.0f};
std::atomic<float> physxMs{0.0f}, physxMsMax{0.0f};

// Reflectivity by physics material (the .physmat names the game reports, matched by prefix as
// Physics/ScrapeAudio.cpp does): a first guess, hard and smooth surfaces high, soft and broken ones
// low. Unknown: -1 (the Tuning defaults).
struct MaterialReflectivity {
  const char *prefix;
  float reflectivity;
};
constexpr MaterialReflectivity kMaterialReflectivity[] = {
    {"glass", 0.95f}, {"metal", 0.9f},  {"concrete", 0.85f}, {"stone", 0.85f},   {"brick", 0.85f},
    {"asphalt", 0.75f}, {"plastic", 0.7f}, {"wood", 0.6f},   {"water", 0.8f},    {"dirt", 0.45f},
    {"mud", 0.4f},    {"sand", 0.35f},  {"trash", 0.3f},     {"cardboard", 0.3f}, {"grass", 0.3f},
    {"leaves", 0.2f}, {"foliage", 0.2f}, {"fabric", 0.2f},   {"upholstery", 0.2f},
};

float Reflectivity(const std::string &aMaterial) {
  for (const auto &entry : kMaterialReflectivity) {
    if (aMaterial.rfind(entry.prefix, 0) == 0) {
      return entry.reflectivity;
    }
  }
  return -1.0f;
}

// the comparison log (SetLog): one row a probe
std::mutex logLock;
FILE *logFile = nullptr;
Clock::time_point logStart{};

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
// blocks rendered, those with sound coming in, and those skipped as silent
std::atomic<uint64_t> probes{0}, blocks{0}, soundBlocks{0}, silentBlocks{0};

std::mutex wakeLock;
std::condition_variable wake;
bool running = false;
std::thread worker;

struct Instance {
  explicit Instance(float aRate)
      : reflections(aRate, int(std::lround(aRate / kProbeHz))), fadeStep(1.0f / (kFadeSeconds * aRate)), rate(int(aRate)) {}
  Reflections reflections;
  uint32_t seq = 0;
  float amount = 0;
  float fadeStep;
  int rate;
  bool idle = true;
  // an event DSP's place in the stems (the bus DSP uses its scratch)
  Stems::EventCursor stems;
  // the test source's state and the block it writes (bus DSPs)
  struct {
    uint32_t phase = 0, rng = 22222;
    float envelope = 0;
    double sine = 0;
    float b[7] = {};
  } test;
  std::vector<float> testBlock = std::vector<float>(size_t(1) << 16);
};

// a bus DSP's user data (an event DSP has none): which stem it records
const char kBusDsp = 0;

// The test source (SetTestSource): the bus DSPs play it in place of what reaches them, the event
// DSPs mute the thrusters. The bus runs at the mixer's rate; an event's DSPs run at its pitch.
std::atomic<int> testSource{0};

// clicks: a 3 ms decaying noise burst every half second; a 440 Hz sine; pink noise: Paul Kellet's
// filter on white
void Generate(Instance &i, TestSource aKind, float *aOut, unsigned int aLength, int aChannels) {
  auto &t = i.test;
  const uint32_t period = uint32_t(i.rate / 2), burst = uint32_t(i.rate * 0.003f);
  const float decay = std::exp(-1.0f / (0.0006f * i.rate));
  for (unsigned int s = 0; s < aLength; ++s) {
    t.rng = t.rng * 1664525u + 1013904223u;
    const float white = float(int32_t(t.rng)) / 2147483648.0f;
    float v = 0;
    if (aKind == TestSource::Clicks) {
      if (t.phase == 0) {
        t.envelope = 0.5f;
      }
      v = t.phase < burst ? white * t.envelope : 0.0f;
      t.envelope *= decay;
      t.phase = (t.phase + 1) % period;
    } else if (aKind == TestSource::Sine) {
      v = 0.25f * float(std::sin(t.sine));
      t.sine += 2.0 * 3.14159265358979 * 440.0 / i.rate;
      if (t.sine > 2.0 * 3.14159265358979) {
        t.sine -= 2.0 * 3.14159265358979;
      }
    } else {
      t.b[0] = 0.99886f * t.b[0] + white * 0.0555179f;
      t.b[1] = 0.99332f * t.b[1] + white * 0.0750759f;
      t.b[2] = 0.96900f * t.b[2] + white * 0.1538520f;
      t.b[3] = 0.86650f * t.b[3] + white * 0.3104856f;
      t.b[4] = 0.55000f * t.b[4] + white * 0.5329522f;
      t.b[5] = -0.7616f * t.b[5] - white * 0.0168980f;
      v = 0.03f * (t.b[0] + t.b[1] + t.b[2] + t.b[3] + t.b[4] + t.b[5] + t.b[6] + white * 0.5362f);
      t.b[6] = white * 0.115926f;
    }
    // front left and right (or the one channel)
    for (int c = 0; c < aChannels; ++c) {
      aOut[size_t(s) * aChannels + c] = c < 2 ? v : 0.0f;
    }
  }
}

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

void Render(Instance &i, float *aIn, float *aOut, unsigned int aLength, int aInChannels) {
  const size_t samples = size_t(aLength) * aInChannels;
  const float target = level.load(std::memory_order_relaxed);
  if (target <= 0 && i.amount <= 0) {
    // off: passed through, and started afresh when on again
    std::memcpy(aOut, aIn, samples * sizeof(float));
    i.idle = true;
    return;
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
    return;
  }
  float peak = 0;
  for (size_t k = 0; k < samples; ++k) {
    peak = std::max(peak, std::abs(aIn[k]));
  }
  if (peak > 1e-6f) {
    soundBlocks.fetch_add(1, std::memory_order_relaxed);
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
}

FMOD_RESULT F_CALL DspRead(FMOD_DSP_STATE *aState, float *aIn, float *aOut, unsigned int aLength, int aInChannels,
                           int *aOutChannels) {
  auto &i = *static_cast<Instance *>(aState->plugindata);
  *aOutChannels = aInChannels;
  void *userData = nullptr;
  aState->functions->getuserdata(aState, &userData);
  const bool bus = userData == &kBusDsp;
  const auto test = static_cast<TestSource>(testSource.load(std::memory_order_relaxed));
  if (test != TestSource::Off && bus && size_t(aLength) * aInChannels <= i.testBlock.size()) {
    Generate(i, test, i.testBlock.data(), aLength, aInChannels);
    aIn = i.testBlock.data();
  }
  Render(i, aIn, aOut, aLength, aInChannels);
  if (test != TestSource::Off && !bus) {
    std::memset(aOut, 0, size_t(aLength) * aInChannels * sizeof(float));
  }
  if (bus) {
    if (Stems::Recording()) {
      unsigned long long clock = 0;
      unsigned int offset = 0, length = 0;
      aState->functions->getclock(aState, &clock, &offset, &length);
      Stems::AddBus(clock, aIn, aOut, aLength, aInChannels, i.rate, test != TestSource::Off, i.stems.scratch);
    }
  } else if (test == TestSource::Off) {
    Stems::AddEvent(i.stems, aIn, aOut, aLength, aInChannels);
  }
  return FMOD_OK;
}

FMOD_RESULT F_CALL DspShouldIProcess(FMOD_DSP_STATE *aState, FMOD_BOOL aInputsIdle, unsigned int, FMOD_CHANNELMASK,
                                     int, FMOD_SPEAKERMODE) {
  auto &i = *static_cast<Instance *>(aState->plugindata);
  // on: always (in the game the buses reported idle inputs while the thrusters played); off:
  // while the echoes and reverb still ring; always while stems record (the bus DSP keeps their time)
  if (!aInputsIdle || level.load(std::memory_order_relaxed) > 0 || Stems::Recording() || testSource.load() ||
      (i.amount > 0 && i.reflections.TailLevel() > 1e-12f)) {
    return FMOD_OK;
  }
  silentBlocks.fetch_add(1, std::memory_order_relaxed);
  return FMOD_ERR_DSP_SILENCE;
}

void Run() {
  std::optional<Probe> probe;
  // the car's velocity from probe to probe (for the echoes' Doppler shifts), smoothed
  Vec3 lastPosition{}, velocity{};
  auto lastTime = Clock::now();
  bool haveLast = false;
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
    const float drift = std::hypot(p.live.origin.x - p.position.x, p.live.origin.y - p.position.y,
                                   p.live.origin.z - p.position.z);
    const bool live = Clock::now() - p.liveTime < kLiveStale && drift < kLiveDrift;
    const Features f = (*probe)(p.position, p.forward, p.up, live ? &p.live : nullptr);
    tuning.gain = p.gain;
    {
      const float dt = std::chrono::duration<float>(p.carTime - lastTime).count();
      if (haveLast && dt > 0.01f) {
        Vec3 v{(p.position.x - lastPosition.x) / dt, (p.position.y - lastPosition.y) / dt,
               (p.position.z - lastPosition.z) / dt};
        // a teleport is not a speed
        if (std::hypot(v.x, v.y, v.z) > 200.0f) {
          v = {};
        }
        velocity = {velocity.x + 0.5f * (v.x - velocity.x), velocity.y + 0.5f * (v.y - velocity.y),
                    velocity.z + 0.5f * (v.z - velocity.z)};
      }
      if (dt > 0.01f || !haveLast) {
        lastPosition = p.position;
        lastTime = p.carTime;
        haveLast = true;
      }
    }
    const Params prm = Targets(f, p.position, p.haveListener ? p.listener : p.position, tuning, velocity);
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
    {
      std::lock_guard guard(logLock);
      if (logFile) {
        auto material = [&](int aRay) { return live && aRay >= 0 && p.live.distance[aRay] >= 0 ? p.liveMaterial[aRay] : ""; };
        std::fprintf(logFile, "%.3f,%.2f,%.2f,%.2f,%d,%d,%.3f,%.3f,%s,%.3f,%.3f,%.3f,%.3f,%s,%.3f,%.3f,%s,%.3f,%.3f\n",
                     std::chrono::duration<double>(Clock::now() - logStart).count(), p.position.x, p.position.y,
                     p.position.z, live ? f.liveRays : -1, live ? int(p.liveSource) : 0, f.floorMap, f.floor, material(0).c_str(), f.ceilingMap,
                     f.ceiling, f.leftMap, f.left, material(f.leftRay).c_str(), f.rightMap, f.right,
                     material(f.rightRay).c_str(), f.enclosure, ms);
        std::fflush(logFile); // tests end by killing the game
      }
    }
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

namespace {

FMOD_DSP_DESCRIPTION *Description() {
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
  return &description;
}

// DSPs inserted into event instances, by instance (the game thread starts and stops events)
std::mutex eventsLock;
std::unordered_map<FMOD::Studio::EventInstance *, std::vector<Attached>> events;

// A DSP just before every Object Spatializer under aGroup: those hand the sound to the output as
// a 3D object, past every bus
void InsertBeforeObjectPans(FMOD::System *aCore, FMOD::ChannelGroup *aGroup, std::vector<Attached> &aOut) {
  int dsps = 0;
  aGroup->getNumDSPs(&dsps);
  for (int i = 0; i < dsps; ++i) {
    FMOD::DSP *dsp = nullptr;
    FMOD_DSP_TYPE type = FMOD_DSP_TYPE_UNKNOWN;
    if (aGroup->getDSP(i, &dsp) != FMOD_OK || dsp->getType(&type) != FMOD_OK || type != FMOD_DSP_TYPE_OBJECTPAN) {
      continue;
    }
    // index 0 is the output end: one further from it is just before the spatializer
    Attached a{nullptr, aGroup};
    if (Check(aCore->createDSP(Description(), &a.dsp), "createDSP") && Check(aGroup->addDSP(i + 1, a.dsp), "addDSP")) {
      aOut.push_back(a);
      ++i;
      ++dsps;
    } else if (a.dsp) {
      a.dsp->release();
    }
  }
  int groups = 0;
  aGroup->getNumGroups(&groups);
  for (int g = 0; g < groups; ++g) {
    FMOD::ChannelGroup *child = nullptr;
    if (aGroup->getGroup(g, &child) == FMOD_OK) {
      InsertBeforeObjectPans(aCore, child, aOut);
    }
  }
}

} // namespace

FMOD::DSP *CreateDsp(FMOD::System *aCore) {
  FMOD::DSP *dsp = nullptr;
  return Check(aCore->createDSP(Description(), &dsp), "createDSP") ? dsp : nullptr;
}

int AttachToEvent(FMOD::Studio::System *aStudio, FMOD::Studio::EventInstance *aInstance) {
  FMOD::System *core = nullptr;
  FMOD::ChannelGroup *group = nullptr;
  // the instance's DSP graph exists once its start has gone through
  aStudio->flushCommands();
  if (!Check(aStudio->getCoreSystem(&core), "getCoreSystem") || !Check(aInstance->getChannelGroup(&group), "event getChannelGroup")) {
    return 0;
  }
  std::vector<Attached> inserted;
  InsertBeforeObjectPans(core, group, inserted);
  const int count = int(inserted.size());
  if (count) {
    std::lock_guard guard(eventsLock);
    auto &list = events[aInstance];
    list.insert(list.end(), inserted.begin(), inserted.end());
  }
  return count;
}

void DetachFromEvent(FMOD::Studio::EventInstance *aInstance) {
  std::vector<Attached> list;
  {
    std::lock_guard guard(eventsLock);
    auto it = events.find(aInstance);
    if (it == events.end()) {
      return;
    }
    list = std::move(it->second);
    events.erase(it);
  }
  for (auto &a : list) {
    a.group->removeDSP(a.dsp);
    a.dsp->release();
  }
}

bool Attach(FMOD::Studio::System *aStudio, const std::vector<std::string> &aBuses) {
  FMOD_DSP_DESCRIPTION &description = *Description();
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
        // at the output end: Studio connects its events into the bus past the tail, so a DSP there
        // heard nothing in the game; after the fader, the bus volume is in the sound already
        Check(a.dsp->setUserData((void *)&kBusDsp), "setUserData") &&
        Check(a.group->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, a.dsp), "addDSP")) {
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
  Stems::Stop();
  std::vector<FMOD::Studio::EventInstance *> instances;
  {
    std::lock_guard guard(eventsLock);
    for (auto &[instance, _] : events) {
      instances.push_back(instance);
    }
  }
  for (auto *instance : instances) {
    DetachFromEvent(instance);
  }
  for (auto &a : attached) {
    a.group->removeDSP(a.dsp);
    a.dsp->release();
    a.bus->unlockChannelGroup();
  }
  attached.clear();
}

void SetCar(const Vec3 &aPosition, const Vec3 &aForward, const Vec3 &aUp, float aGain, float aInside,
            RaySource aRays) {
  std::lock_guard guard(poseLock);
  pose.position = aPosition;
  pose.forward = aForward;
  pose.up = aUp;
  pose.gain = aGain;
  pose.inside = aInside;
  pose.rays = aRays;
  pose.carTime = Clock::now();
}

bool PhysXRaysDue(const Vec3 &aCandidate, Vec3 &aPosition, Vec3 &aForward, Vec3 &aUp) {
  const auto now = Clock::now();
  const int64_t ticks = now.time_since_epoch().count();
  const int64_t interval = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(1.0f / kProbeHz)).count();
  int64_t last = lastPhysXCast.load(std::memory_order_relaxed);
  if (ticks - last < interval) {
    return false;
  }
  physxCalls.fetch_add(1, std::memory_order_relaxed);
  {
    std::lock_guard guard(poseLock);
    const float gap =
        std::hypot(aCandidate.x - pose.position.x, aCandidate.y - pose.position.y, aCandidate.z - pose.position.z);
    physxGap.store(gap, std::memory_order_relaxed);
    if (pose.rays != RaySource::PhysX || pose.gain <= 0 || now - pose.carTime > kStale || gap > 5.0f) {
      return false;
    }
    aPosition = pose.position;
    aForward = pose.forward;
    aUp = pose.up;
  }
  // one caster per interval, whichever thread gets here first
  if (!lastPhysXCast.compare_exchange_strong(last, ticks, std::memory_order_relaxed)) {
    return false;
  }
  physxCasts.fetch_add(1, std::memory_order_relaxed);
  return true;
}

void SetLiveHits(const Vec3 &aOrigin, const std::array<float, kNearRays> &aDistance,
                 const std::array<std::string, kNearRays> &aMaterial, RaySource aSource) {
  LiveHits hits;
  hits.origin = aOrigin;
  hits.distance = aDistance;
  for (int k = 0; k < kNearRays; ++k) {
    hits.reflectivity[k] = aDistance[k] >= 0 ? Reflectivity(aMaterial[k]) : -1.0f;
  }
  std::lock_guard guard(poseLock);
  pose.live = hits;
  pose.liveMaterial = aMaterial;
  pose.liveSource = aSource;
  pose.liveTime = Clock::now();
}

void SetPhysXCastTime(float aMilliseconds) {
  physxMs.store(aMilliseconds, std::memory_order_relaxed);
  if (aMilliseconds > physxMsMax.load(std::memory_order_relaxed)) {
    physxMsMax.store(aMilliseconds, std::memory_order_relaxed);
  }
}

void SetLog(const std::string &aPath) {
  std::lock_guard guard(logLock);
  if (logFile) {
    std::fclose(logFile);
    logFile = nullptr;
  }
  if (aPath.empty()) {
    return;
  }
  logFile = std::fopen(aPath.c_str(), "w");
  if (!logFile) {
    spdlog::warn("[Acoustics] cannot write {}", aPath);
    return;
  }
  logStart = Clock::now();
  std::fputs("t,x,y,z,live_rays,source,floor_map,floor,floor_material,ceiling_map,ceiling,left_map,left,left_material,"
             "right_map,right,right_material,enclosure,probe_ms\n",
             logFile);
  spdlog::info("[Acoustics] logging to {}", aPath);
}

void SetTestSource(TestSource aSource) {
  testSource.store(int(aSource));
  spdlog::info("[Acoustics] test source {}", int(aSource));
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
  size_t eventDsps = 0;
  {
    std::lock_guard guard(eventsLock);
    for (auto &[_, list] : events) {
      eventDsps += list.size();
    }
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
      "{{\"buses\":{},\"eventDsps\":{},\"flying\":{},\"gain\":{},\"inside\":{},\"level\":{},\"probes\":{},\"probeMs\":{},"
      "\"blocks\":{},\"soundBlocks\":{},\"silentBlocks\":{},\"floor\":{},\"ceiling\":{},\"left\":{},\"right\":{},\"enclosure\":{},"
      "\"meanFreePath\":{},\"physxCalls\":{},\"physxCasts\":{},\"physxGap\":{},\"physxMs\":{},\"physxMsMax\":{},\"liveRays\":{},\"floorMap\":{},\"leftMap\":{},\"rightMap\":{},\"floorDelay\":{},\"floorGain\":{},\"leftDelay\":{},\"rightDelay\":{},"
      "\"floorFeedback\":{},\"wallFeedback\":{},\"rt60\":{},\"wet\":{},\"testSource\":{},\"stems\":{}}}",
      attached.size(), eventDsps, fresh ? "true" : "false", num(gain), num(inside), num(level.load()), probes.load(), num(ms),
      blocks.load(), soundBlocks.load(), silentBlocks.load(), num(f.floor), num(f.ceiling), num(f.left), num(f.right), num(f.enclosure),
      num(f.meanFreePath), physxCalls.load(), physxCasts.load(), num(physxGap.load()), num(physxMs.load()), num(physxMsMax.load()), f.liveRays, num(f.floorMap), num(f.leftMap), num(f.rightMap), num(p.floor.delay), num(p.floor.gain), num(p.left.delay), num(p.right.delay),
      num(p.floorFeedback), num(p.wallFeedback), num(p.rt60), num(p.wet), testSource.load(), Stems::State());
}

} // namespace Acoustics
