# Audio

Every sound LTBF makes is made in the plugin (`src/red4ext/Audio/`): the thruster synth (`engine-synth.md`), the
scrapes, the echoes and the late reverb (`acoustics.md`), the wind and three samples. Each one is a generator, a
function that renders a block of samples on the audio thread, and plays through a voice of the audio backend. The
game's Wwise is the backend by default; LTBF's own FMOD Core system is the fallback. Redscript only reports events
(mount, activation, destruction, water, flight on and off); the per-frame audio logic is C++.

## Backends

`Audio/Out` is the one place sound leaves the plugin. `Out::Play` takes a generator (`RenderFn`) and `Options` (its
bus, positioned or 2D, mono or stereo, the rolloff's range, an optional low-pass, whether it takes the game's area
reverb) and returns a voice, paused. A voice is paused, given a volume and a low-pass cutoff, and placed each frame
(`Placement`: its pose, the listener's, the rolloff at its distance). A paused voice's generator is not run.

The backend is chosen once, when the plugin loads, by the developer setting `wwiseAudio` (on by default). Everything
in `Out` runs on the game thread; generators run on the backend's audio thread and never lock or allocate.

## Wwise

LTBF plays through the game's own Wwise (2023.1 in the exe) by the Audio Input source plugin, the one the game uses
for REDmod sounds. The game's master and SFX volumes, its pause, ducking and output act on LTBF as on the game's own
sounds, and there is one output stream.

- **The link** (`Wwise/Link`): the exe's exported `AK::SoundEngine` and `AK::SoundEngine::Query` functions, looked
  up once by mangled name. A missing one is named in the log and Wwise does not start.
- **The relay** (`Wwise/Relay`, `Wwise/Pattern`): `SetAudioInputCallbacks` is not exported, so it is found by its
  bytes (the three stores into consecutive globals). Our callbacks go in front of the ones that were there. Ours
  answer the playing IDs bound to a voice of ours and pass every other ID on, so REDmod sounds keep playing. The
  game sets its callbacks only when it has REDmod sounds, possibly after ours, so `KeepRelayInstalled` checks the
  globals once a frame and puts ours back in front. On unload the relay writes the old callbacks back before the
  DLL goes.
- **The bank** (`Wwise/Bank`): written in code in `mod.bnk`'s layout (bank version 150) and loaded with
  `LoadBankMemoryCopy` on the first game frame, when Wwise is up. Nothing is shipped. Each sound is an Audio Input
  Sound started by its own event on the game's `sfx` bus: `ltbf_voice_3d` (positioned), `ltbf_voice_2d`, and
  `ltbf_voice_3d_room` (positioned, taking the game's aux sends). The positioned ones carry `ltbf_spread`, an
  attenuation with only a spread curve, taken from `mod_sfx_street`: with no spread Wwise pans a point source hard
  to one side. There is no volume curve: the loudness is ours.
- **Voices** (`Out/WwiseBackend`): a voice is a game object of ours (`Wwise/Ids`: "LTBF" in the high 32 bits) that
  posts its event while the pool lets it play and stops it when paused. A positioned voice is placed before it is
  posted, since Wwise drops a 3D voice started out of range for good. A playing voice Wwise has not rendered for a
  second (the game stops sounds when the world goes) is posted again.
- **Mix** (`Out/Mix`): Wwise only positions. Each voice's gain (its own, its bus's, the rolloff's) ramps over each
  block and an optional one-pole low-pass runs, in the samples. A positioned mono voice gets 6 dB over its generator,
  as FMOD's Object Spatializer gives a voice by summing its two channels, so the two backends sound alike.
- **Axes** (`Wwise/Axes`): the game is z up, Wwise y up: the game's (x, y, z) is Wwise's (x, z, y). `SetPosition`
  needs the up square to the facing, so `ToWwise` makes it so.
- **The listener**: a game object of ours, set as every voice's listener and placed each frame where `Listener`
  below says.
- **The game's reverb** (`Wwise/Sends`, `SendsHook`): the game puts its own sounds into its area reverbs by aux
  sends it sets on its game objects. The setter is hooked to learn which objects get sends; the one nearest the
  player's car is taken as the car's, and its sends are copied onto our voices with `Options::gameReverb` (the
  scrapes), so they ring in the same tunnel or room as the car's tyres.

`WwiseSpike` is a native class that plays a test tone through these pieces; nothing calls it in play.

## FMOD

The fallback (`Out/FmodBackend`): the plugin's own FMOD Core system, stereo, with each bus a channel group under the
master. A voice is a channel playing a generator DSP. A positioned voice has FMOD's Object Spatializer after it, with
a linear-squared rolloff over the voice's own range; a voice with a low-pass has FMOD's simple low-pass at the tail.
The generator writes the same sound in both channels and the spatializer folds them into one object. FMOD's
`update()` runs once a frame. The Engine group and the master each have a limiter (`Buses::LimiterFor`). The
acoustics' master DSP (the stems' bus track and the test source, `acoustics.md`) exists only here.

## Fallback

When Wwise cannot start, `Out::Init` logs why and starts FMOD instead: an exe whose callback setter is not found
exactly once, or whose exports are missing (a game patch with a new Wwise or a new mangling). `Out::Summary` then
reads "audio: FMOD, Wwise could not start: <why>". If FMOD cannot start either, LTBF plays nothing. `docs/game-patch.md`
has the step to find the setter and exports again after a patch.

## Buses

Four buses (`Buses.hpp`), each with its volume worked out each frame by `Buses::BusVolume`:

| Bus | What plays on it | Volume |
|---|---|---|
| Engine | the thrusters, the acoustics engine's bed and echo voices, the startup and off samples | Engine slider times the game's, plus 5 dB (`kEngineGainDb`) |
| Scrape | the player's thrusters' scrapes | as Engine, without the Engine limiter, so a loud engine does not duck the scrape |
| Wind | the wind | Wind slider (0.6 by default) times the game's |
| Warning | the destroyed alarm | Warning slider (0.5 by default) times the game's |

The sliders are Mod Settings on `FlightAudio` (`engineVolume`, `windVolume`, `warningVolume`). The game's own
volume (`Inputs::GameVolume`) is 0 in menus, in the hub menu's, world map's and radial menu's time dilations, and
otherwise the master and SFX sliders; they are read 4 times a second. Under Wwise the master and SFX sliders are
left out, since the game applies them to our voices already (`Out::GameVolumeIncluded`). The pause is
`FlightAudio.Pause` and `Resume`, which pause the backend and every voice.

Under FMOD the limiters sit on the Engine group and the master (ceiling -12 dB). Wwise has no bus of ours, so the
acoustics engine's bed runs the same limiter (`Out::Limiter`) on the sum it hands the player's thruster and echo
voices and scales them all by its gain.

## Voices

Every 3D sound registers with one pool (`Voices/Voices`, the ranking in `Voices/Pool`, game-free): its id, kind, car,
whether it is a thruster synth, its voice, its own gain and its rolloff range. Each frame it says where it is and how
loud it is before rolloff (`Voices::Move`). Once a frame `Voices::Frame` ranks every source by its level at the
listener (loudness times FMOD's linear-squared rolloff, `Voices::Rolloff`) and decides who plays:

- **Tiers**: the player's thrusters, samples, wind and scrapes first; then the player's echoes; then everything else
  (NPC thrusters, looping samples). Within a tier, by level.
- **Budget**: at most 16 voices play (`kMaxObjects`, under Windows Sonic's dynamic object limit) and at most 16 of
  them are thruster synths (`kMaxSynth`, about 1% of a core each). A source under -60 dB plays nothing.
- **Hysteresis**: a source that played last frame ranks 3 dB louder than it is, so voices do not flap at the edge.
  A newcomer waits for a fading voice to give its place up rather than going over the budget.
- **Fades**: a voice fades in and out over 50 ms.
- **Folding**: an NPC car with a thruster out of the set plays as one voice at its thrusters' centre, at their
  summed level (power sum, at most 4 times the lead's), carried by its thruster with the lowest id.
- **Occlusion**: an NPC car that plays gets a low-pass and a duck from how blocked it is (`acoustics.md`, Occlusion).

A voice that loses is paused, so a thruster synth that loses costs nothing. `FlightAudio.VoicesBudget` sets the budget
and `VoicesState` reads it out (sources, paths, folded cars, the synth's core share).

## Rings

`Voices/Ring` is a single producer, single consumer ring of mono samples between generators: the acoustics engine's
bed writes a block, a thruster or echo voice reads it. It starts with a block of zeros in it, so the reader is one
block behind from the start and the echoes keep their delays against the dry sound whichever the backend runs first.
A short read fills with zeros and counts an underrun; a full write drops the newest and counts an overrun. Nothing
waits.

## Samples

Three sounds are WAVs (`src/audio/`, installed to `red4ext/plugins/let_there_be_flight/audio/` and loaded once at
start, `Samples/Samples`): `vehicle3_on` (`vehicle3_startup.wav`), `vehicle3_off` (`vehicle3_off.wav`) and the
destroyed alarm `vehicle3_destroyed` (`brake-1.wav`, looping, on the Warning bus at -4 dB). `Samples/Events` keeps
each one's level, pitch curve, pitch wobble, random pitch, tremolo and distortion as the old FMOD Studio events had
them, curves taken as linear.

`Samples/Player` decodes a WAV (16-bit PCM or 32-bit float) to mono once and reads it at the pitch ratio with linear
interpolation, through LTBF's own tremolo and soft clip with the events' parameters. The game thread sets the pitch
and effects each frame (`Samples::Frame`) and places the sound at its car. Redscript calls
`FlightAudio.PlayOneShot(name, vehicle)`, `StartLoop(id, name, vehicle)` and `StopLoop(id)`. A one-shot ends at the
end of its sound and is released.

## Wind

The wind as the listener hears it (`Wind/Model` game-free, `Wind/Wind` its two voices on the Wind bus). Each frame the
player's car flies, `FlightAudio.UpdateWind` gets the car's matrix, velocity and view; after a quarter of a second
without one the wind goes silent.

- **The air** is the listener's velocity minus the city's wind (`Field`). The mean wind grows with height over the
  floor by the urban power law (6 m/s at 10 m, exponent 0.3, capped at 300 m) and drops between buildings: a street
  canyon with no open sky keeps 35%, a side wall within 30 m halves it again. Its direction drifts up to 20 degrees
  over two minutes. Turbulence has intensity 0.25 (vertical half that) in three Ornstein-Uhlenbeck components whose
  time scale is the eddies' size (0.7 of the height, 10 to 80 m) over the speed they are crossed at (Taylor's
  frozen turbulence): gusts pass quickly flying fast and slowly hovering. A change of shelter (a building edge) stirs
  three times its size of extra turbulence, which settles over 5 m of travel. The probe's open sky, floor and wall
  distances feed it.
- **Flow noise** (`Model`): 100 dB SPL at 28 m/s for an exposed listener, then U^6 (Curle's dipole law, 60 dB a
  decade). The real range cannot be played, so the mix compresses it 2.5:1 from 158 dB SPL, at most -3 dB. Its
  frequencies scale with U by a Strouhal number of 0.2: the rumble over the listener's head (0.2 m), the hiss off the
  car's edges (2 cm, 10 dB under), and an aeolian tone off small parts (1.5 cm, 28 dB under, 8 dB more in full
  crossflow).
- **First person**: the panels by the mass law (10 kg/m2, 6 dB more loss an octave), the seals' leak at -32 dB, a
  cabin boom at 45 Hz (-24 dB), and a seal gap's whistle (1100 Hz resonance, 3 cm gap) that sounds only where one of
  the gap's shear layer modes (Rossiter) crosses the resonance, so it comes and goes with speed. One voice at each
  front window (`SetWindowSlots`, or a default place in the car's axes), louder on the side the air comes from.
- **Third person**: one voice 2 m ahead of the listener along the car's velocity.
- **Cinematic camera**: the camera's own wind ahead of it, and the car's wind 6 dB under as a second voice at the car.

`Render` makes the sound from noise through state variable filters, its gains smoothed on the audio thread.
`WindOn` switches it, `WindState` reads it out.

## Inputs

The per-frame values the synth and the scrapes take (`FlightAudioUpdate`'s fields: speed, surge, yawDiff, lift, yaw,
pitchDiff, brake, inside, damage, roll, pitch, sway, scrape) are worked out in C++ (`Inputs/Gather`, the rules in
`Inputs/Inputs`, game-free). `Gather::Component` runs once a frame per flight component, after its script's update
(`Flight/Component.cpp`), and skips while the photo mode is on. It reads the vehicle, `FlightStats` and the script
fields (the controller's inputs, the mode, the Autopilot's commands, the collision timer, the health stat) through
RTTI into an `Inputs::Frame`; `Inputs::Update` applies the rules for the player, the drone, the Autopilot, NPCs, a
destroyed car and collision recovery, and fades `inside` at 4 per second. Then:

- each attached thruster (up to 8) gets the values, its volume (a front thruster always full, the others coming in
  with speed), its place and its cone axes (`UpdateThruster`), and its scrape voice the car's scrapes nearest it;
- for the player's car, the camera listener is set outside the car, and `UpdateAcoustics` and `UpdateWind` get the
  car's matrix, velocity, view and the echoes' switches (`FlightAudio.acoustics`, `acousticsGain`,
  `acousticsRaySource`).

Thrusters start and stop with `FlightAudio.StartEngine` and `StopEngine` from redscript; `SetWater` drives the synth's
water low-pass.

## Listener

Each frame `FlightSystem`'s PrePhysics sets the listener (`FlightAudio::UpdateListenerMatrix`): the player's
soundListener component in first person, the active camera in third person and on the Autopilot's cinematic shots
(`SetCameraListener`, held for 200 ms). The soundListener stays with V but turns with the camera, so third person
heard from it swung the thrusters round a listener sitting among them. The listener's velocity is differentiated
and smoothed over 0.1 s (a jump over 500 m/s counts as a teleport), for the wind.

## Frame

Once a game frame, in PrePhysics: the listener goes to the backend, then `Samples::Frame`, `Wind::Tick`, the
acoustics engine's `Frame` (echo voices to their image sources), `Voices::Frame` (rank, place, play and pause), and the
acoustics' listener. Then `FlightAudio::UpdateVolume` sets the buses' volumes and runs the backend's frame
(`Out::Update`: FMOD's update; under Wwise the bank and listener on the first frame, the relay kept in front, the
voices started and stopped, the game's reverb copied). The flight components' `Gather::Component` calls feed the
inputs during the frame.

Pure parts (`Pool`, `Wind/Model`, `Inputs`, `Samples/Events`, `Samples/Player`, `Out/Mix`, `Buses`, `Wwise/*` but
`Link`) have no game or FMOD types and are tested offline in `tests/` (`voices_`, `wind_`, `inputs_`, `samples_`,
`buses_`, `out_`, `wwise_` tests).
