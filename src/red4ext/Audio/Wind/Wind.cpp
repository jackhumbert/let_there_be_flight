#include "Wind.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>

namespace Wind {

std::atomic<bool> on{true};

namespace {

using Clock = std::chrono::steady_clock;

// the voices' ids in the pool
constexpr uint64_t kVoiceId = 0x57494E4400000000ull;
// where the windows are in a car's own axes when the scripts have none
constexpr Vec3 kDefaultLeft{-0.8f, 0.3f, 1.0f}, kDefaultRight{0.8f, 0.3f, 1.0f};
// the TPP and cinematic wind sit this far from the listener, along the velocity
constexpr float kAheadMetres = 2.0f;
// the car's wind heard by the cinematic camera, under the camera's own
constexpr float kCarDb = -6.0f;
constexpr float kStale = 0.25f;

bool Check(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    spdlog::error("[Wind] {}: FMOD error {} - {}", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

// what a generator DSP plays: the noise, and the newest params from the game thread
struct Generator {
  Generator(float aRate, uint32_t aSeed) : render(aRate, aSeed) {}
  Render render;
  std::mutex lock;
  Params params;
  bool fresh = false;
};

struct Channel {
  std::unique_ptr<Generator> generator;
  FMOD::DSP *source = nullptr;
  FMOD::Channel *channel = nullptr;
  uint64_t voice = 0;
};

Channel channels[2];
Model model;
// one wind field for the city round the listener
Field field(31);
FMOD::System *core = nullptr;
bool ready = false;
bool active = false;
Clock::time_point lastUpdate, lastFrame;
// for State(): the first voice's
View lastView = View::Tpp;
Params lastParams;
float lastWind = 0, lastSigma = 0, lastStir = 0;

FMOD_RESULT F_CALL Process(FMOD_DSP_STATE *aState, unsigned int aLength, const FMOD_DSP_BUFFER_ARRAY *,
                           FMOD_DSP_BUFFER_ARRAY *aOut, FMOD_BOOL, FMOD_DSP_PROCESS_OPERATION aOperation) {
  void *userData = nullptr;
  aState->functions->getuserdata(aState, &userData);
  auto *g = static_cast<Generator *>(userData);
  if (aOperation == FMOD_DSP_PROCESS_QUERY) {
    // stereo with the same sound in both, as the engine synth's: the spatializer folds it into one object
    if (aOut) {
      aOut->speakermode = FMOD_SPEAKERMODE_STEREO;
      aOut->buffernumchannels[0] = 2;
      aOut->bufferchannelmask[0] = FMOD_CHANNELMASK_STEREO;
    }
    return FMOD_OK;
  }
  float *out = aOut->buffers[0];
  if (!g) {
    std::memset(out, 0, 2 * aLength * sizeof(float));
    return FMOD_OK;
  }
  {
    std::unique_lock guard(g->lock, std::try_to_lock);
    if (guard.owns_lock() && g->fresh) {
      g->render.Set(g->params);
      g->fresh = false;
    }
  }
  g->render.Process(out, aLength);
  return FMOD_OK;
}

FMOD_DSP_DESCRIPTION *Description() {
  static FMOD_DSP_DESCRIPTION description = [] {
    FMOD_DSP_DESCRIPTION d{};
    d.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
    std::strncpy(d.name, "LTBF wind", sizeof(d.name) - 1);
    d.version = 1;
    d.numinputbuffers = 0;
    d.numoutputbuffers = 1;
    d.process = Process;
    return d;
  }();
  return &description;
}

void Release(Channel &c) {
  if (c.voice) {
    Voices::Unregister(c.voice);
  }
  if (c.channel) {
    c.channel->stop();
  }
  if (c.source) {
    c.source->release();
  }
  c = {};
}

float Length(const Vec3 &v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

bool IsZero(const Vec3 &v) {
  return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}

Vec3 Plus(const Vec3 &a, const Vec3 &b, float aScale = 1.0f) {
  return {a.x + aScale * b.x, a.y + aScale * b.y, a.z + aScale * b.z};
}

// along aDirection (unit), or aFallback when it is shorter than aMin
Vec3 Direction(const Vec3 &aVelocity, const Vec3 &aFallback, float aMin) {
  const float length = Length(aVelocity);
  return length < aMin ? aFallback : Vec3{aVelocity.x / length, aVelocity.y / length, aVelocity.z / length};
}

// a window's place in the world
Vec3 Slot(const Inputs &in, const Vec3 &aLocal) {
  return Plus(Plus(Plus(in.carPosition, in.carRight, aLocal.x), in.carForward, aLocal.y), in.carUp, aLocal.z);
}

// every gain times aScale (a voice's side, or its share)
Params Scaled(Params p, float aScale) {
  p.level *= aScale;
  p.edgeLevel *= aScale;
  p.aeolianLevel *= aScale;
  p.boomLevel *= aScale;
  p.whistleLevel *= aScale;
  return p;
}

Wind::Vec ToVec(const Vec3 &v) {
  return {v.x, v.y, v.z};
}

void Send(int aVoice, const Params &p) {
  Generator &g = *channels[aVoice].generator;
  std::lock_guard guard(g.lock);
  g.params = p;
  g.fresh = true;
}

void Silence() {
  for (int i = 0; i < 2; ++i) {
    Send(i, Params{.level = 0});
    Voices::Move(channels[i].voice, {}, {0, 1, 0}, {0, 0, 1}, 0.0f);
  }
}

} // namespace

bool Init(FMOD::System *aCore, FMOD::ChannelGroup *group) {
  Shutdown();
  core = aCore;
  Voices::Init(core);
  int rate = 48000;
  core->getSoftwareFormat(&rate, nullptr, nullptr);
  for (int i = 0; i < 2; ++i) {
    Channel &c = channels[i];
    c.generator = std::make_unique<Generator>(float(rate), 11 + i);
    c.voice = kVoiceId + uint64_t(i);
    const bool ok = Check(core->createDSP(Description(), &c.source), "createDSP") &&
                    Check(c.source->setUserData(c.generator.get()), "setUserData") &&
                    Check(core->playDSP(c.source, group, true, &c.channel), "playDSP") &&
                    Voices::Register(c.voice, Voices::Kind::Wind, 0, false, c.channel, 1.0f, 0.0f, 100.0f);
    if (!ok) {
      c.voice = 0;
      Shutdown();
      return false;
    }
  }
  ready = true;
  spdlog::info("[Wind] two channels in the Wind group at {} Hz", rate);
  return true;
}

void Shutdown() {
  ready = false;
  active = false;
  for (Channel &c : channels) {
    Release(c);
  }
}

void Update(const Inputs &in) {
  if (!ready) {
    return;
  }
  const auto now = Clock::now();
  const float dt = lastFrame == Clock::time_point{} ? 0.0f : std::clamp(std::chrono::duration<float>(now - lastFrame).count(), 0.0f, 0.1f);
  lastFrame = now;
  lastUpdate = now;
  active = true;
  if (!on.load(std::memory_order_relaxed)) {
    Silence();
    return;
  }

  // the city's wind where the listener is, its eddies crossed at the listener's speed (the cinematic
  // camera's own; the car's is heard in the same field)
  FieldInput air;
  air.dt = dt;
  air.crossing = Length(in.view == View::Cinematic ? in.listenerVelocity : in.carVelocity);
  if (in.haveProbe) {
    air.openSky = in.openSky;
    air.floorDistance = in.floorDistance;
    air.wallDistance = in.wallDistance;
  }
  const Wind::Vec wind = field.Update(air);

  Input input;
  input.view = in.view;
  input.wind = wind;
  input.right = ToVec(in.carRight);
  input.dt = dt;
  const Vec3 up = in.carUp, forward = in.carForward;

  if (in.view == View::Fpp) {
    input.velocity = ToVec(in.carVelocity);
    const Params p = model.Update(input);
    const Vec3 left = Slot(in, IsZero(in.windowLeft) ? kDefaultLeft : in.windowLeft);
    const Vec3 right = Slot(in, IsZero(in.windowRight) ? kDefaultRight : in.windowRight);
    Send(0, Scaled(p, p.left));
    Send(1, Scaled(p, p.right));
    Voices::Move(channels[0].voice, left, forward, up, p.level * p.left);
    Voices::Move(channels[1].voice, right, forward, up, p.level * p.right);
    lastParams = p;
  } else if (in.view == View::Tpp) {
    input.velocity = ToVec(in.carVelocity);
    const Params p = model.Update(input);
    const Vec3 at = Plus(in.listenerPosition, Direction(in.carVelocity, forward, 0.1f), kAheadMetres);
    Send(0, p);
    Send(1, Params{});
    Voices::Move(channels[0].voice, at, forward, up, p.level);
    Voices::Move(channels[1].voice, at, forward, up, 0.0f);
    lastParams = p;
  } else {
    // the camera's wind, and the car's as it passes
    input.velocity = ToVec(in.listenerVelocity);
    const Params camera = model.Update(input);
    input.view = View::Tpp;
    input.velocity = ToVec(in.carVelocity);
    const float share = std::pow(10.0f, kCarDb / 20.0f);
    const Params car = Scaled(model.Update(input), share);
    const Vec3 at = Plus(in.listenerPosition, Direction(in.listenerVelocity, forward, 0.1f), kAheadMetres);
    Send(0, camera);
    Send(1, car);
    Voices::Move(channels[0].voice, at, forward, up, camera.level);
    Voices::Move(channels[1].voice, in.carPosition, forward, up, car.level, in.carVelocity);
    lastParams = camera;
  }
  lastView = in.view;
  lastWind = std::sqrt(wind.x * wind.x + wind.y * wind.y + wind.z * wind.z);
  lastSigma = field.Sigma();
  lastStir = field.Stir();
}

void Tick() {
  if (!ready || !active) {
    return;
  }
  if (std::chrono::duration<float>(Clock::now() - lastUpdate).count() > kStale) {
    active = false;
    Silence();
  }
}

std::string State() {
  const char *view = lastView == View::Tpp ? "tpp" : lastView == View::Fpp ? "fpp" : "cinematic";
  const float levelDb = 20.0f * std::log10(std::max(lastParams.level, 1e-10f));
  const float whistleDb = 20.0f * std::log10(std::max(lastParams.whistleLevel, 1e-10f));
  return fmt::format("{{\"on\":{},\"view\":\"{}\",\"airspeed\":{:.1f},\"wind\":{:.1f},\"sigma\":{:.2f},"
                     "\"stir\":{:.2f},\"levelDb\":{:.1f},\"flowHz\":{:.0f},\"edgeHz\":{:.0f},"
                     "\"aeolianHz\":{:.0f},\"whistleDb\":{:.1f},\"left\":{:.3f},\"right\":{:.3f}}}",
                     on.load(), view, lastParams.airspeed, lastWind, lastSigma, lastStir, levelDb,
                     lastParams.flowHz, lastParams.edgeHz, lastParams.aeolianHz, whistleDb, lastParams.left,
                     lastParams.right);
}

std::string Summary() {
  return on.load() ? "wind on" : "wind off";
}

} // namespace Wind
