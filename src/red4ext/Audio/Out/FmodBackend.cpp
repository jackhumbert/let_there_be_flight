// The FMOD backend (Out.hpp): the plugin's own FMOD Core system and its three buses as channel groups under the
// master (docs/design/audio.md, FMOD), the Engine group's and the master's limiters, and a voice as a channel
// playing a generator DSP with the Object Spatializer after it.
#include "Backend.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <spdlog/spdlog.h>

#include <cstring>
#include <vector>

namespace Out {

namespace {

FMOD::System *core = nullptr;
FMOD::ChannelGroup *master = nullptr;

bool Check(FMOD_RESULT aResult, const char *aWhat) {
  if (aResult != FMOD_OK) {
    spdlog::error("[Out] {}: FMOD error {} - {}", aWhat, int(aResult), FMOD_ErrorString(aResult));
    return false;
  }
  return true;
}

float Dot(const Voices::Vec3 &a, const Voices::Vec3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Voices::Vec3 Minus(const Voices::Vec3 &a, const Voices::Vec3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

// in the listener's own axes, FMOD's right-handed way (x right, y up, z back) as the system is set up
FMOD_VECTOR Relative(const Voices::Vec3 &aVector, const Pose &aListener) {
  return {Dot(aVector, aListener.right), Dot(aVector, aListener.up), -Dot(aVector, aListener.forward)};
}

// the game's world (z up) as FMOD has it
FMOD_VECTOR World(const Voices::Vec3 &v) {
  return {v.x, v.z, -v.y};
}

// the most frames FMOD's mixer asks of a generator at once
constexpr uint32_t kBlock = 8192;

class FmodVoice final : public Voice {
public:
  FmodVoice(RenderFn aRender, void *aUser, uint32_t aChannels)
      : render(aRender), user(aUser), channels(aChannels), scratch(size_t(kBlock) * aChannels) {}
  ~FmodVoice() override {
    // a DSP still on a channel is not released: off first, then the channel stopped
    if (channel) {
      for (FMOD::DSP *dsp : {pan, lowpass}) {
        if (dsp) {
          channel->removeDSP(dsp);
        }
      }
      channel->stop();
    }
    for (FMOD::DSP *dsp : {pan, lowpass, source}) {
      if (dsp) {
        dsp->release();
      }
    }
  }

  bool Start(FMOD::ChannelGroup *aGroup, const Options &aOptions) {
    static FMOD_DSP_DESCRIPTION description = [] {
      FMOD_DSP_DESCRIPTION d{};
      d.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
      std::strncpy(d.name, "LTBF voice", sizeof(d.name) - 1);
      d.version = 1;
      d.numinputbuffers = 0;
      d.numoutputbuffers = 1;
      d.process = Process;
      return d;
    }();
    bool ok = Check(core->createDSP(&description, &source), "createDSP") &&
              Check(source->setUserData(this), "setUserData") &&
              Check(core->playDSP(source, aGroup, true, &channel), "playDSP");
    if (ok && aOptions.positioned) {
      // the Object Spatializer with a linear-squared rolloff over the source's own range
      ok = Check(core->createDSPByType(FMOD_DSP_TYPE_OBJECTPAN, &pan), "createDSPByType") &&
           Check(pan->setParameterInt(FMOD_DSP_OBJECTPAN_3D_ROLLOFF, FMOD_DSP_PAN_3D_ROLLOFF_LINEARSQUARED), "rolloff") &&
           Check(pan->setParameterBool(FMOD_DSP_OBJECTPAN_OVERRIDE_RANGE, true), "override range") &&
           Check(pan->setParameterFloat(FMOD_DSP_OBJECTPAN_3D_MIN_DISTANCE, aOptions.minDistance), "min distance") &&
           Check(pan->setParameterFloat(FMOD_DSP_OBJECTPAN_3D_MAX_DISTANCE, aOptions.maxDistance), "max distance") &&
           Check(pan->setParameterInt(FMOD_DSP_OBJECTPAN_3D_EXTENT_MODE, FMOD_DSP_PAN_3D_EXTENT_MODE_OFF), "extent") &&
           Check(channel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, pan), "addDSP pan");
    }
    // the low-pass at the tail, after the generator and before the fader and the object pan
    if (ok && aOptions.lowpass) {
      ok = Check(core->createDSPByType(FMOD_DSP_TYPE_LOWPASS_SIMPLE, &lowpass), "createDSPByType lowpass") &&
           Check(lowpass->setParameterFloat(FMOD_DSP_LOWPASS_SIMPLE_CUTOFF, 22000.0f), "cutoff") &&
           Check(channel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, lowpass), "addDSP lowpass");
    }
    return ok;
  }

  void SetPaused(bool aPaused) override { channel->setPaused(aPaused); }
  void SetVolume(float aVolume) override { channel->setVolume(aVolume); }
  void SetLowpass(float aHz) override {
    if (lowpass) {
      lowpass->setParameterFloat(FMOD_DSP_LOWPASS_SIMPLE_CUTOFF, aHz);
    }
  }
  void Place(const Placement &p) override {
    if (!pan) {
      return;
    }
    // the rolloff is the Object Spatializer's own (Options' range); Placement::rolloff is for Wwise
    FMOD_DSP_PARAMETER_3DATTRIBUTES_MULTI attributes{};
    attributes.numlisteners = 1;
    attributes.weight[0] = 1.0f;
    attributes.relative[0].position = Relative(Minus(p.position, p.listener.position), p.listener);
    attributes.relative[0].forward = Relative(p.forward, p.listener);
    attributes.relative[0].up = Relative(p.up, p.listener);
    attributes.relative[0].velocity = Relative(Minus(p.velocity, p.listener.velocity), p.listener);
    attributes.absolute.position = World(p.position);
    attributes.absolute.forward = World(p.forward);
    attributes.absolute.up = World(p.up);
    attributes.absolute.velocity = World(p.velocity);
    pan->setParameterData(FMOD_DSP_OBJECTPAN_3D_POSITION, &attributes, sizeof(attributes));
  }

private:
  // the generator, as stereo with the same sound in both for a mono one: the spatializer folds the channels into one
  // object (a mono source came out 6 dB under the event), and the echoes pan their walls left and right
  static FMOD_RESULT F_CALL Process(FMOD_DSP_STATE *aState, unsigned int aLength, const FMOD_DSP_BUFFER_ARRAY *,
                                    FMOD_DSP_BUFFER_ARRAY *aOut, FMOD_BOOL, FMOD_DSP_PROCESS_OPERATION aOperation) {
    if (aOperation == FMOD_DSP_PROCESS_QUERY) {
      if (aOut) {
        aOut->speakermode = FMOD_SPEAKERMODE_STEREO;
        aOut->buffernumchannels[0] = 2;
        aOut->bufferchannelmask[0] = FMOD_CHANNELMASK_STEREO;
      }
      return FMOD_OK;
    }
    void *userData = nullptr;
    aState->functions->getuserdata(aState, &userData);
    auto *v = static_cast<FmodVoice *>(userData);
    float *out = aOut->buffers[0];
    if (!v || aLength > kBlock) {
      std::memset(out, 0, 2 * size_t(aLength) * sizeof(float));
      return FMOD_OK;
    }
    float *s = v->scratch.data();
    v->render(v->user, s, aLength, v->channels);
    const float *right = v->channels > 1 ? s + aLength : s;
    for (unsigned int i = 0; i < aLength; ++i) {
      out[2 * i] = s[i];
      out[2 * i + 1] = right[i];
    }
    return FMOD_OK;
  }

  RenderFn render;
  void *user;
  uint32_t channels;
  std::vector<float> scratch;
  FMOD::DSP *source = nullptr, *pan = nullptr, *lowpass = nullptr;
  FMOD::Channel *channel = nullptr;
};

class FmodBackend final : public Detail::Backend {
public:
  bool Start(std::string &aWhy) {
    if (!Check(FMOD::System_Create(&core), "System_Create")) {
      core = nullptr;
      aWhy = "System_Create";
      return false;
    }
    Check(core->setSoftwareFormat(0, FMOD_SPEAKERMODE_STEREO, 0), "setSoftwareFormat");
    // Due to a bug in WinSonic on Windows, FMOD initialization may fail on some machines. If you get the error
    // "FMOD error 51 - Error initializing output device", try a different output type such as
    // FMOD_OUTPUTTYPE_AUTODETECT
    Check(core->setOutput(FMOD_OUTPUTTYPE_AUTODETECT), "setOutput");
    if (!Check(core->init(1024, FMOD_INIT_3D_RIGHTHANDED, nullptr), "init")) {
      core->release();
      core = nullptr;
      aWhy = "init";
      return false;
    }
    Check(core->getMasterChannelGroup(&master), "getMasterChannelGroup");
    constexpr const char *kNames[Buses::kBusCount] = {"Engine", "Wind", "Warning", "Scrape"};
    for (int i = 0; i < Buses::kBusCount; ++i) {
      Check(core->createChannelGroup(kNames[i], &groups[i]), "createChannelGroup");
      Check(master->addGroup(groups[i]), "addGroup");
    }
    engineLimiter = AddLimiter(groups[int(Buses::Bus::Engine)], Buses::LimiterFor(false));
    core->getSoftwareFormat(&rate, nullptr, nullptr);
    spdlog::info("FMOD loaded");
    return true;
  }

  // the master's limiter goes last before the output: after Acoustics' master DSP, so it is added on the first frame
  void LimitMaster() {
    if (!masterLimiter) {
      masterLimiter = AddLimiter(master, Buses::LimiterFor(true));
    }
  }

  ~FmodBackend() override {
    if (!core) {
      return;
    }
    if (masterLimiter) {
      master->removeDSP(masterLimiter);
      masterLimiter->release();
    }
    if (engineLimiter) {
      groups[int(Buses::Bus::Engine)]->removeDSP(engineLimiter);
      engineLimiter->release();
    }
    core->release();
    core = nullptr;
    master = nullptr;
  }

  std::unique_ptr<Voice> Play(RenderFn aRender, void *aUser, const Options &aOptions) override {
    auto voice = std::make_unique<FmodVoice>(aRender, aUser, aOptions.channels);
    if (!voice->Start(groups[int(aOptions.bus)], aOptions)) {
      spdlog::error("[Out] FMOD refused the voice {}", aOptions.name);
      return nullptr;
    }
    return voice;
  }

  void SetBusVolume(Buses::Bus aBus, float aVolume) override { Check(groups[int(aBus)]->setVolume(aVolume), "setVolume"); }

  void SetListener(const Pose &l) override {
    FMOD_3D_ATTRIBUTES a{};
    a.position = World(l.position);
    a.forward = World(l.forward);
    a.up = World(l.up);
    // FMOD's Doppler takes the listener's velocity
    a.velocity = World(l.velocity);
    Check(core->set3DListenerAttributes(0, &a.position, &a.velocity, &a.forward, &a.up), "set3DListenerAttributes");
  }

  void Update() override {
    LimitMaster();
    // the one FMOD update a frame
    Check(core->update(), "update");
  }

  void SetPaused(bool aPaused) override { Check(master->setPaused(aPaused), "setPaused"); }

  int Rate() const override { return rate; }

  std::string State() const override {
    int channels = 0, real = 0;
    core->getChannelsPlaying(&channels, &real);
    return fmt::format("{{\"backend\":\"fmod\",\"fmodChannels\":{},\"fmodReal\":{}}}", channels, real);
  }

private:
  // an FMOD_DSP_TYPE_LIMITER at the head of aGroup, set from aLimiter; null if FMOD refused (logged)
  static FMOD::DSP *AddLimiter(FMOD::ChannelGroup *aGroup, const Buses::Limiter &aLimiter) {
    FMOD::DSP *dsp = nullptr;
    if (!Check(core->createDSPByType(FMOD_DSP_TYPE_LIMITER, &dsp), "createDSPByType limiter")) {
      return nullptr;
    }
    if (!Check(dsp->setParameterFloat(FMOD_DSP_LIMITER_CEILING, aLimiter.ceilingDb), "ceiling") ||
        !Check(dsp->setParameterFloat(FMOD_DSP_LIMITER_RELEASETIME, aLimiter.releaseMs), "release") ||
        !Check(aGroup->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, dsp), "addDSP limiter")) {
      dsp->release();
      return nullptr;
    }
    return dsp;
  }

  FMOD::ChannelGroup *groups[Buses::kBusCount] = {};
  FMOD::DSP *engineLimiter = nullptr, *masterLimiter = nullptr;
  int rate = 48000;
};

} // namespace

namespace Detail {

std::unique_ptr<Backend> MakeFmod(std::string &aWhy) {
  auto backend = std::make_unique<FmodBackend>();
  if (!backend->Start(aWhy)) {
    return nullptr;
  }
  return backend;
}

} // namespace Detail

FMOD::System *FmodSystem() {
  return core;
}

FMOD::ChannelGroup *FmodMaster() {
  return master;
}

} // namespace Out
