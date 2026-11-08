#include "Out.hpp"

#include "Backend.hpp"

#include <spdlog/spdlog.h>

#include <atomic>

namespace Out {

namespace {

std::unique_ptr<Detail::Backend> backend;
Backend current = Backend::Fmod;
std::string fallenWhy;
std::atomic<float> busVolumes[3] = {1.0f, 1.0f, 1.0f};

} // namespace

bool Init(Backend aBackend) {
  Shutdown();
  fallenWhy.clear();
  std::string why;
  if (aBackend == Backend::Wwise) {
    backend = Detail::MakeWwise(why);
    if (backend) {
      current = Backend::Wwise;
      spdlog::info("[Out] LTBF plays through the game's Wwise");
      return true;
    }
    // an exe whose callback setter or exports were not found: FMOD as before
    fallenWhy = why;
    spdlog::warn("[Out] Wwise could not start ({}): LTBF plays through FMOD", why);
  }
  backend = Detail::MakeFmod(why);
  current = Backend::Fmod;
  if (!backend) {
    spdlog::error("[Out] FMOD could not start ({}): LTBF plays nothing", why);
    return false;
  }
  spdlog::info("[Out] LTBF plays through its own FMOD");
  return true;
}

void Shutdown() {
  backend.reset();
}

Backend Current() {
  return current;
}

bool Fallen() {
  return !fallenWhy.empty();
}

std::unique_ptr<Voice> Play(RenderFn aRender, void *aUser, const Options &aOptions) {
  return backend ? backend->Play(aRender, aUser, aOptions) : nullptr;
}

void SetBusVolume(Buses::Bus aBus, float aVolume) {
  busVolumes[int(aBus)].store(aVolume, std::memory_order_relaxed);
  if (backend) {
    backend->SetBusVolume(aBus, aVolume);
  }
}

float BusVolume(Buses::Bus aBus) {
  return busVolumes[int(aBus)].load(std::memory_order_relaxed);
}

void SetListener(const Pose &aListener) {
  if (backend) {
    backend->SetListener(aListener);
  }
}

void SetReverbFrom(const Voices::Vec3 &aCar) {
  if (backend) {
    backend->SetReverbFrom(aCar);
  }
}

void Update() {
  if (backend) {
    backend->Update();
  }
}

void SetPaused(bool aPaused) {
  if (backend) {
    backend->SetPaused(aPaused);
  }
}

int Rate() {
  return backend ? backend->Rate() : 48000;
}

bool GameVolumeIncluded() {
  return backend && current == Backend::Wwise;
}

std::string State() {
  return backend ? backend->State() : "{\"backend\":\"none\"}";
}

std::string Summary() {
  if (!backend) {
    return "audio: none";
  }
  if (current == Backend::Wwise) {
    return "audio: Wwise";
  }
  return Fallen() ? "audio: FMOD, Wwise could not start: " + fallenWhy : "audio: FMOD";
}

} // namespace Out
