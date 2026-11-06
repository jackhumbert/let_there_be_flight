#include "Acoustics.hpp"
#include "Materials.hpp"
#include "Occlusion.hpp"
#include "Planes.hpp"
#include "Reflections.hpp"
#include "Stems.hpp"

#include "Audio/Out/Out.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
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
  Clock::time_point liveTime{};
};

// the last PhysX cast (PhysXRaysDue), in steady clock ticks; for State(): calls from the caster,
// casts, and the last candidate's distance from the reported car
std::atomic<int64_t> lastPhysXCast{0};
std::atomic<uint64_t> physxCalls{0}, physxCasts{0};
std::atomic<float> physxGap{-1.0f};
std::atomic<float> physxMs{0.0f}, physxMsMax{0.0f};

// Planes fitted to the live hits (SetPlanes); the last fit's inlier counts and the cache's size, for State()
std::atomic<bool> planes{true};
std::array<int, Planes::kSides> lastInliers{};
std::atomic<size_t> cacheSize{0};
// the cache's clock
const Clock::time_point kEpoch = Clock::now();

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
Clock::time_point lastFeaturesTime{};
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
  // the stems' scratch for the master DSP's block
  std::vector<float> stemScratch;
  // the test source's state and the block it writes
  struct {
    uint32_t phase = 0, rng = 22222;
    float envelope = 0;
    double sine = 0;
    float b[7] = {};
  } test;
  std::vector<float> testBlock = std::vector<float>(size_t(1) << 16);
};

// The test source (SetTestSource): the master DSP plays it in place of what reaches it, at the mixer's rate.
std::atomic<int> testSource{0};
// Tuning::airAbsorption for the worker's Targets (SetAirAbsorption)
std::atomic<bool> airAbsorption{true};

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

// the master group's DSP
FMOD::ChannelGroup *attachedGroup = nullptr;
FMOD::DSP *attachedDsp = nullptr;

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
  const auto test = static_cast<TestSource>(testSource.load(std::memory_order_relaxed));
  if (test != TestSource::Off && size_t(aLength) * aInChannels <= i.testBlock.size()) {
    Generate(i, test, i.testBlock.data(), aLength, aInChannels);
    aIn = i.testBlock.data();
  }
  if (test != TestSource::Off) {
    // the test sound's echoes; everything else LTBF plays reaches the master with its echoes already
    // (the car's Engine and its echo voices), and would be echoed twice
    Render(i, aIn, aOut, aLength, aInChannels);
  } else {
    std::memcpy(aOut, aIn, size_t(aLength) * aInChannels * sizeof(float));
    i.idle = true;
  }
  if (Stems::Recording()) {
    unsigned long long clock = 0;
    unsigned int offset = 0, length = 0;
    aState->functions->getclock(aState, &clock, &offset, &length);
    Stems::AddBus(clock, aIn, aOut, aLength, aInChannels, i.rate, test != TestSource::Off, i.stemScratch);
  }
  return FMOD_OK;
}

FMOD_RESULT F_CALL DspShouldIProcess(FMOD_DSP_STATE *aState, FMOD_BOOL aInputsIdle, unsigned int, FMOD_CHANNELMASK,
                                     int, FMOD_SPEAKERMODE) {
  auto &i = *static_cast<Instance *>(aState->plugindata);
  // on: always; off: while the echoes and reverb still ring; always while stems record (the DSP keeps
  // their time)
  if (!aInputsIdle || level.load(std::memory_order_relaxed) > 0 || Stems::Recording() || testSource.load() ||
      (i.amount > 0 && i.reflections.TailLevel() > 1e-12f)) {
    return FMOD_OK;
  }
  silentBlocks.fetch_add(1, std::memory_order_relaxed);
  return FMOD_ERR_DSP_SILENCE;
}

void Run();

void StartWorker() {
  if (!running) {
    running = true;
    worker = std::thread(Run);
  }
}

void Run() {
  std::optional<Probe> probe;
  // the car's velocity from probe to probe (for the echoes' Doppler shifts), smoothed
  Vec3 lastPosition{}, velocity{};
  auto lastTime = Clock::now();
  bool haveLast = false;
  Tuning tuning;
  Planes::Cache cache;
  Clock::time_point lastCast{};
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
    Features f = (*probe)(p.position, p.forward, p.up, live ? &p.live : nullptr);
    std::array<int, Planes::kSides> inliers{};
    if (planes.load(std::memory_order_relaxed)) {
      const float now = std::chrono::duration<float>(Clock::now() - kEpoch).count();
      // each cast's hits once, though a few probes may use them
      if (live && p.liveTime != lastCast) {
        lastCast = p.liveTime;
        for (int k = 0; k < kNearRays; ++k) {
          if (p.live.distance[k] >= 0) {
            cache.Add({p.live.position[k], p.live.normal[k], now});
          }
        }
      }
      cache.Expire(now, p.position);
      // a fitted plane's distance and normal replace the probe's, on its side
      float *distance[Planes::kSides] = {&f.floor, &f.ceiling, &f.left, &f.right};
      Vec3 *outward[Planes::kSides] = {&f.floorNormal, &f.ceilingNormal, &f.leftNormal, &f.rightNormal};
      bool *flag[Planes::kSides] = {&f.floorPlane, &f.ceilingPlane, &f.leftPlane, &f.rightPlane};
      for (int s = 0; s < Planes::kSides; ++s) {
        if (auto plane = cache.Fit(Planes::Side(s), p.position, f.rightDir)) {
          const Vec3 &n = plane->normal;
          *distance[s] = (p.position.x - plane->point.x) * n.x + (p.position.y - plane->point.y) * n.y +
                         (p.position.z - plane->point.z) * n.z;
          *outward[s] = {-n.x, -n.y, -n.z};
          *flag[s] = true;
          inliers[s] = plane->inliers;
        }
      }
      cacheSize.store(cache.Size(), std::memory_order_relaxed);
    }
    tuning.gain = p.gain;
    tuning.airAbsorption = airAbsorption.load(std::memory_order_relaxed);
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
      lastInliers = inliers;
      lastFeaturesTime = Clock::now();
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
                     p.position.z, live ? f.liveRays : -1, live ? int(RaySource::PhysX) : 0, f.floorMap, f.floor, material(0).c_str(), f.ceilingMap,
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

} // namespace

bool Attach(FMOD::System *aCore, FMOD::ChannelGroup *aMaster) {
  FMOD::DSP *dsp = nullptr;
  // at the output end, after the fader: the buses' volumes are in the sound already
  const bool ok = Check(aCore->createDSP(Description(), &dsp), "createDSP") &&
                  Check(aMaster->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, dsp), "addDSP");
  if (!ok) {
    if (dsp) {
      dsp->release();
    }
    return false;
  }
  attachedGroup = aMaster;
  attachedDsp = dsp;
  StartWorker();
  spdlog::info("[Acoustics] on the master group");
  return true;
}

namespace {

// Under Wwise there is no master to sit on: the test source is a 2D stereo voice of its own, with its echoes (the
// rest has its echoes from the car's Engine already, as under FMOD)
std::unique_ptr<Instance> testInstance;
std::unique_ptr<Out::Voice> testVoice;

void TestRender(void *aUser, float *aOut, uint32_t aFrames, uint32_t aChannels) {
  auto &i = *static_cast<Instance *>(aUser);
  const auto test = static_cast<TestSource>(testSource.load(std::memory_order_relaxed));
  const size_t samples = size_t(aFrames) * 2;
  if (test == TestSource::Off || aChannels != 2 || 2 * samples > i.testBlock.size()) {
    std::memset(aOut, 0, size_t(aFrames) * aChannels * sizeof(float));
    return;
  }
  // interleaved in testBlock's halves as the master DSP has it, then apart into aOut's channels
  float *in = i.testBlock.data(), *out = in + samples;
  Generate(i, test, in, aFrames, 2);
  Render(i, in, out, aFrames, 2);
  for (uint32_t k = 0; k < aFrames; ++k) {
    aOut[k] = out[2 * k];
    aOut[aFrames + k] = out[2 * k + 1];
  }
}

} // namespace

bool AttachVoice() {
  testInstance = std::make_unique<Instance>(float(Out::Rate()));
  Out::Options options;
  options.positioned = false;
  options.channels = 2;
  options.name = "LTBF test source";
  testVoice = Out::Play(TestRender, testInstance.get(), options);
  if (testVoice) {
    testVoice->SetVolume(1.0f);
    testVoice->SetPaused(testSource.load() == int(TestSource::Off));
  }
  StartWorker();
  spdlog::info("[Acoustics] the test source as a voice of its own");
  return testVoice != nullptr;
}

void Detach() {
  testVoice.reset();
  testInstance.reset();
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
  if (attachedDsp) {
    attachedGroup->removeDSP(attachedDsp);
    attachedDsp->release();
    attachedDsp = nullptr;
    attachedGroup = nullptr;
  }
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
                 const std::array<std::string, kNearRays> &aMaterial, const std::array<Vec3, kNearRays> &aPosition,
                 const std::array<Vec3, kNearRays> &aNormal, const std::vector<Vec3> &aSphereDir,
                 const std::vector<float> &aSphereDistance, const std::vector<float> &aSphereWeight) {
  LiveHits hits;
  hits.origin = aOrigin;
  hits.distance = aDistance;
  hits.position = aPosition;
  hits.normal = aNormal;
  hits.sphereDir = aSphereDir;
  hits.sphereDistance = aSphereDistance;
  hits.sphereWeight = aSphereWeight;
  for (int k = 0; k < kNearRays; ++k) {
    hits.reflectivity[k] = aDistance[k] >= 0 ? Materials::Find(aMaterial[k]).value_or(Bands{}) : Bands{};
  }
  std::lock_guard guard(poseLock);
  pose.live = hits;
  pose.liveMaterial = aMaterial;
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

void SetPlanes(bool aOn) {
  planes.store(aOn);
  spdlog::info("[Acoustics] planes {}", aOn);
}

void SetAirAbsorption(bool aOn) {
  airAbsorption.store(aOn);
  spdlog::info("[Acoustics] air absorption {}", aOn);
}

void SetOcclusion(bool aOn) {
  Occlusion::enabled.store(aOn);
  spdlog::info("[Acoustics] occlusion {}", aOn);
}

void SetTestSource(TestSource aSource) {
  testSource.store(int(aSource));
  if (testVoice) {
    testVoice->SetPaused(aSource == TestSource::Off);
  }
  spdlog::info("[Acoustics] test source {}", int(aSource));
}

void SetListener(const Vec3 &aPosition) {
  std::lock_guard guard(poseLock);
  pose.listener = aPosition;
  pose.haveListener = true;
}

std::string Summary() {
  float gain;
  bool fresh;
  RaySource rays;
  {
    std::lock_guard guard(poseLock);
    gain = pose.gain;
    fresh = Clock::now() - pose.carTime < kStale;
    rays = pose.rays;
  }
  constexpr const char *kTests[] = {"off", "clicks", "pink noise", "440 Hz sine"};
  const int test = std::clamp(testSource.load(), 0, 3);
  if (!fresh) {
    return fmt::format("echoes: not flying (air absorption {}, planes {}, test sound {}, stems {})",
                       airAbsorption.load() ? "on" : "off", planes.load() ? "on" : "off", kTests[test],
                       Stems::Recording() ? "recording" : "off");
  }
  return fmt::format("echoes {} (level {:.2f}), air absorption {}, planes {}, test sound {}, near rays from {}, stems {}",
                     gain > 0 ? "on" : "off", level.load(), airAbsorption.load() ? "on" : "off",
                     planes.load() ? "on" : "off", kTests[test],
                     rays == RaySource::PhysX ? "PhysX" : "the nav map only",
                     Stems::Recording() ? "recording" : "off");
}

bool LastParams(Params &aParams) {
  if (paramsSeq.load(std::memory_order_acquire) == 0) {
    return false;
  }
  std::lock_guard guard(stateLock);
  aParams = lastParams;
  return true;
}

bool NewestParams(uint32_t &aSeq, Params &aParams) {
  if (paramsSeq.load(std::memory_order_acquire) == aSeq) {
    return false;
  }
  std::unique_lock guard(paramsLock, std::try_to_lock);
  if (!guard.owns_lock()) {
    return false;
  }
  aParams = params;
  aSeq = paramsSeq.load(std::memory_order_relaxed);
  return true;
}

float Level() {
  return level.load(std::memory_order_relaxed);
}

bool LastFeatures(Features &aFeatures) {
  std::lock_guard guard(stateLock);
  if (Clock::now() - lastFeaturesTime > std::chrono::milliseconds(500)) {
    return false;
  }
  aFeatures = lastFeatures;
  return true;
}

std::string State() {
  Features f;
  Params p;
  float ms;
  std::array<int, Planes::kSides> inliers;
  {
    std::lock_guard guard(stateLock);
    f = lastFeatures;
    inliers = lastInliers;
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
  // per side: whether a fitted plane answers it, the hits that agree, its distance and outward normal
  auto side = [&](bool aOn, int aInliers, float aDistance, const Vec3 &aNormal) {
    return fmt::format("{{\"on\":{},\"inliers\":{},\"distance\":{},\"normal\":[{},{},{}]}}", aOn ? "true" : "false",
                       aInliers, num(aDistance), num(aNormal.x), num(aNormal.y), num(aNormal.z));
  };
  const std::string planesJson = fmt::format(
      "{{\"on\":{},\"cache\":{},\"floor\":{},\"ceiling\":{},\"left\":{},\"right\":{}}}", planes.load() ? "true" : "false",
      cacheSize.load(), side(f.floorPlane, inliers[0], f.floor, f.floorNormal),
      side(f.ceilingPlane, inliers[1], f.ceiling, f.ceilingNormal), side(f.leftPlane, inliers[2], f.left, f.leftNormal),
      side(f.rightPlane, inliers[3], f.right, f.rightNormal));
  return fmt::format(
      "{{\"buses\":{},\"flying\":{},\"gain\":{},\"inside\":{},\"level\":{},\"probes\":{},\"probeMs\":{},"
      "\"blocks\":{},\"soundBlocks\":{},\"silentBlocks\":{},\"floor\":{},\"ceiling\":{},\"left\":{},\"right\":{},\"enclosure\":{},\"enclosureMap\":{},\"meanFreePathMap\":{},\"sphereLive\":{},"
      "\"meanFreePath\":{},\"physxCalls\":{},\"physxCasts\":{},\"physxGap\":{},\"physxMs\":{},\"physxMsMax\":{},\"liveRays\":{},\"floorMap\":{},\"leftMap\":{},\"rightMap\":{},\"floorDelay\":{},\"floorGain\":{},\"leftDelay\":{},\"rightDelay\":{},"
      "\"floorFeedback\":{},\"wallFeedback\":{},\"rt60\":{},\"wet\":{},\"testSource\":{},\"air\":{},\"planes\":{},\"occlusionCasts\":{},\"stems\":{}}}",
      attachedDsp ? 1 : 0, fresh ? "true" : "false", num(gain), num(inside), num(level.load()), probes.load(), num(ms),
      blocks.load(), soundBlocks.load(), silentBlocks.load(), num(f.floor), num(f.ceiling), num(f.left), num(f.right), num(f.enclosure), num(f.enclosureMap), num(f.meanFreePathMap), f.sphereLive,
      num(f.meanFreePath), physxCalls.load(), physxCasts.load(), num(physxGap.load()), num(physxMs.load()), num(physxMsMax.load()), f.liveRays, num(f.floorMap), num(f.leftMap), num(f.rightMap), num(p.floor.delay), num(p.floor.gain), num(p.left.delay), num(p.right.delay),
      num(p.floorFeedback), num(p.wallFeedback), num(p.rt60), num(p.wet), testSource.load(), airAbsorption.load(), planesJson, Occlusion::Casts(), Stems::State());
}

} // namespace Acoustics
