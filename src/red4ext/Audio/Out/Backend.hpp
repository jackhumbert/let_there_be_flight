#pragma once

// The two backends behind Out.hpp (FmodBackend.cpp, WwiseBackend.cpp). Internal to Audio/Out.

#include "Out.hpp"

namespace FMOD {
class ChannelGroup;
class System;
} // namespace FMOD

namespace Out::Detail {

class Backend {
public:
  virtual ~Backend() = default;
  virtual std::unique_ptr<Voice> Play(RenderFn aRender, void *aUser, const Options &aOptions) = 0;
  virtual void SetBusVolume(Buses::Bus aBus, float aVolume) = 0;
  virtual void SetListener(const Pose &aListener) = 0;
  // the player's car, for the game's reverb on the voices that take it (Wwise; FMOD has none)
  virtual void SetReverbFrom(const Voices::Vec3 &) {}
  virtual void Update() = 0;
  virtual void SetPaused(bool aPaused) = 0;
  virtual int Rate() const = 0;
  virtual std::string State() const = 0;
};

// null and why when the backend cannot start
std::unique_ptr<Backend> MakeFmod(std::string &aWhy);
std::unique_ptr<Backend> MakeWwise(std::string &aWhy);

} // namespace Out::Detail

namespace Out {

// the FMOD backend's system and master group, for what only FMOD has (Acoustics' master DSP: the test source and
// the stems' bus track); null under Wwise
FMOD::System *FmodSystem();
FMOD::ChannelGroup *FmodMaster();

} // namespace Out
