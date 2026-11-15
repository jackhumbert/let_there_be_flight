#pragma once

// Where LTBF's sound goes (docs/design/audio.md, Backends): every sound LTBF makes is a generator, a function
// that renders a block on the audio thread, and plays through a voice of one of two backends, chosen once at load by
// the developer setting wwiseAudio:
//
//   FMOD    the plugin's own FMOD Core system (docs/design/audio.md, FMOD): a voice is a channel playing a generator
//           DSP with the Object Spatializer (and an NPC car's low-pass) after it, in its bus's channel group; the
//           buses' volumes and limiters, the listener and the pause are the system's.
//   Wwise   the game's Wwise through the Audio Input plugin (Audio/Wwise): a voice is a game object of ours playing
//           one of LTBF's bank's events, its samples from the generator through the relay; its gain (the bus's, the
//           voice's, the rolloff's) and its low-pass are applied in the samples (Mix), Wwise only positions it, for a
//           listener of ours. Wwise is not up when the plugin loads: the backend starts on the first game frame and
//           the voices made before then start with it.
//
// Game thread: everything here. The generators run on the backend's audio thread and must not lock or allocate.

#include "Audio/Buses.hpp"
#include "Audio/Voices/Pool.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace Out {

enum class Backend { Fmod, Wwise };

// aFrames samples of each of aChannels channels, channel c at aOut + c * aFrames
using RenderFn = void (*)(void *aUser, float *aOut, uint32_t aFrames, uint32_t aChannels);

struct Options {
  Buses::Bus bus = Buses::Bus::Engine;
  // a 3D voice placed each frame (Place), or a 2D one
  bool positioned = true;
  // what the generator renders: 1, or 2 for a 2D stereo sound
  uint32_t channels = 1;
  // the rolloff's range: FMOD's Object Spatializer applies it, Wwise's voices take Placement::rolloff
  float minDistance = 0, maxDistance = 100;
  // a low-pass after the generator (an NPC car's occlusion, SetLowpass)
  bool lowpass = false;
  // takes the game's area reverb where the player's car is (Wwise only, SetReverbFrom; Audio/Wwise/Sends.hpp): the
  // reverb the game gives its own sounds there
  bool gameReverb = false;
  // for the log and the backend's state
  const char *name = "";
};

// a pose in the game's world (z up)
struct Pose {
  Voices::Vec3 position, right, forward, up;
  Voices::Vec3 velocity;
};

// a 3D voice's place for this frame: its own pose, the listener's, and the rolloff at its distance (Voices::Rolloff)
struct Placement {
  Voices::Vec3 position, forward, up, velocity;
  Pose listener;
  float rolloff = 1;
};

class Voice {
public:
  virtual ~Voice() = default;
  // a paused voice renders nothing and costs nothing; voices start paused
  virtual void SetPaused(bool aPaused) = 0;
  // linear, before the bus's volume and (FMOD) the rolloff
  virtual void SetVolume(float aVolume) = 0;
  // the low-pass's cutoff (Options::lowpass)
  virtual void SetLowpass(float aHz) = 0;
  virtual void Place(const Placement &aPlacement) = 0;
};

// The backend; false (logged) when it could not start, and LTBF then plays nothing. aWwise falls back to FMOD when
// the relay cannot be installed (the first frame finds out: Fallen()).
bool Init(Backend aBackend);
void Shutdown();
Backend Current();
// the Wwise backend gave up and FMOD plays instead (an exe whose callback setter or exports were not found)
bool Fallen();

// A voice of aRender (with aUser) in its bus, paused; null when the backend refused (logged). The generator and aUser
// must outlive the voice.
std::unique_ptr<Voice> Play(RenderFn aRender, void *aUser, const Options &aOptions);

// each frame: the buses' volumes, the listener, and the backend's frame (FMOD's update; Wwise's start, the relay
// kept in front, the voices' starts and stops)
void SetBusVolume(Buses::Bus aBus, float aVolume);
// what SetBusVolume set last (any thread)
float BusVolume(Buses::Bus aBus);
void SetListener(const Pose &aListener);
// each frame the player is in a car: where it is, so the voices with gameReverb take the reverb the game gives the car
void SetReverbFrom(const Voices::Vec3 &aCar);
void Update();
// every voice silent (the game's pause), until SetPaused(false)
void SetPaused(bool aPaused);

// the rate the generators render at
int Rate();
// the Wwise backend renders the game's own volumes in its voices already (they are under the game's SFX bus): the
// buses' volumes leave the game's master and SFX sliders out
bool GameVolumeIncluded();

// JSON: the backend, and its figures (FMOD's channels; Wwise's relay, voices, starts)
std::string State();
// "audio: Wwise" / "audio: FMOD" (/ "audio: FMOD, Wwise could not start: <why>")
std::string Summary();

} // namespace Out
