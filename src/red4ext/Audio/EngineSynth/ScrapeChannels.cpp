#include "ScrapeChannels.hpp"

#include "Audio/Acoustics/Engine.hpp"
#include "Audio/Out/Out.hpp"
#include "Audio/Voices/Voices.hpp"

#include <cmath>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace EngineSynth::ScrapeChannels {

namespace {

struct Generator {
  std::unique_ptr<ScrapeSynth> synth;
  std::mutex lock;
  ScrapeInputs inputs;
  bool fresh = false;
};

struct Channel {
  std::unique_ptr<Generator> generator;
  std::unique_ptr<Out::Voice> out;
  uint64_t voice = 0;
};

std::unordered_map<std::string, Channel> channels;

void Render(void *aUser, float *aOut, uint32_t aFrames, uint32_t) {
  auto *g = static_cast<Generator *>(aUser);
  if (!g->synth) {
    std::memset(aOut, 0, aFrames * sizeof(float));
    return;
  }
  {
    std::unique_lock guard(g->lock, std::try_to_lock);
    if (guard.owns_lock() && g->fresh) {
      g->synth->Set(g->inputs);
      g->fresh = false;
    }
  }
  g->synth->Render(aOut, aFrames);
}

void Release(Channel &aChannel) {
  // the voice first: once it is gone its generator is not run again
  aChannel.out.reset();
  aChannel = {};
}

} // namespace

bool Start(const std::string &aId) {
  Stop(aId);
  Channel c;
  c.generator = std::make_unique<Generator>();
  // seeded as the thruster's synth is, so each thruster's scrape has its own detune
  c.generator->synth = std::make_unique<ScrapeSynth>(float(Out::Rate()), uint32_t(std::hash<std::string>()(aId)));
  Out::Options options;
  options.bus = Buses::Bus::Scrape;
  options.name = "LTBF scrape";
  c.out = Out::Play(Render, c.generator.get(), options);
  c.voice = std::hash<std::string>()(aId + "/scrape");
  // the thruster channel's gain, so the scrape's levels stay as they were set in it
  if (!c.out || !Voices::Register(c.voice, Voices::Kind::PlayerScrape, 0, false, c.out.get(),
                                  std::pow(10.0f, Acoustics::Engine::kChannelGainDb / 20.0f))) {
    Release(c);
    return false;
  }
  channels[aId] = std::move(c);
  return true;
}

void Update(const std::string &aId, const Voices::Vec3 &aAt, const ScrapeInputs &aInputs) {
  auto it = channels.find(aId);
  if (it == channels.end())
    return;
  Channel &c = it->second;
  {
    std::lock_guard guard(c.generator->lock);
    c.generator->inputs = aInputs;
    c.generator->fresh = true;
  }
  // ranked by how hard it scrapes: none, and the pool pauses it
  Voices::Move(c.voice, aAt, {0, 1, 0}, {0, 0, 1}, aInputs.intensity);
}

void Stop(const std::string &aId) {
  auto it = channels.find(aId);
  if (it == channels.end())
    return;
  Voices::Unregister(it->second.voice);
  Release(it->second);
  channels.erase(it);
}

void StopAll() {
  for (auto &[_, c] : channels) {
    Voices::Unregister(c.voice);
    Release(c);
  }
  channels.clear();
}

} // namespace EngineSynth::ScrapeChannels
