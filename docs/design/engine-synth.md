# Engine synth

A plain thruster sound made in C++, driven by the same parameters the FMOD event `vehicle3_TPP`
takes, so the acoustics (`acoustics.md`) do the space round the car instead of FX baked into the
event. Built behind a switch (`FlightAudio.engineSource`, 1 = the synth; `LtbfBridge.EngineSource`);
the FMOD event stays the default.

## What FMOD does now

Each thruster (`IFlightThruster`) starts `vehicle3_TPP` with `StartWithPitch(id, ..., pitch)`, pitch
`clamp(700 / mass + 0.5, 0.5, 1.5)`, and every frame `UpdateEvent` sets the event's 3D place, a
volume (front 1, rear `speed / 100`) and every field of `FlightAudioUpdate` as a parameter of the
same name: `speed` (m/s, 0 to 300), `surge`, `lift`, `pitch`, `roll`, `sway`, `yaw` (-1 to 1, the
player's inputs or the Autopilot's commands and body rates), `brake`, `damage` (0 to 0.25), `inside`
(0 to 1), `scrape` (per thruster, the wheel's spring force), plus the global `water`. `yawDiff`,
`pitchDiff` are sent but nothing uses them; the event's `load` is never set. The Cone return makes
the level depend on where the camera is against each thruster's facing (up to about +7 dB facing it).

The event (read from `src/fmod_studio/Metadata`):

| Track | Source | Pitch (semitones) | Level | FX in the track |
|---|---|---|---|---|
| Surge | `2077-soft-5.wav` (8 s loop, about 46 Hz fundamental) | surge ±1: +4, 0: -8; speed 0..300: -4..+16; damage: +6 | surge ±1: +5 dB, 0: -12 | 2 EQs (yaw and lift add 10 dB bands), distortion 0.56..1 with speed, LFO gain stutter with damage, chorus at rest, flanger (bypassed), 2 more EQs, SFX reverb 240 ms; send -13.5 dB to Engine |
| Lift, Pitch, Roll, Sway | `2077-soft-5/6.wav` (about 46 / 66 Hz) | axis ±1: +9 to +11, 0: -1 to -10; speed -4..+16; damage +6 | axis ±1: 0 dB, 0: -27 | gain LFO 6.4 Hz, EQ, flanger, distortion with speed and axis, EQ, distortion 0.83, EQ whose top follows speed; sends -7.5 to -24 dB to Engine |
| Brake | `brake-1.wav` x3, `white.wav` | fixed | brake 0..1: -80..0 dB; speed 0..300: -80..+1 dB | EQ with the band following brake, tremolo 2..20 Hz with speed, distortion falling with speed |
| Audio 1 (damage) | `whirling.wav` | speed: -15..0 | damage: -80..0 dB | |
| Scrape | `thruster-scrape.wav` | speed 0..300: 0..+12, random ±3.5 | scrape and speed > 10 m/s | EQ following speed |
| returns | | | | Reverb (SFX reverb 240 ms, -16 dB); Engine (EQ, distortion whose level follows speed 0.11..0.42, a low-pass following `inside` 2700..3500 Hz, a high shelf -15.8 dB above 240 Hz, a low shelf: everything but brake and damage goes through it); Cone (Engine sends +1 dB to it outside the cabin; its gain follows FMOD's built-in Event Cone Angle, 0 dB facing the listener to -80 dB facing away) |
| master | | | inside: -5.5..-3.5 dB | water low-pass 22 kHz..500 Hz, Object Spatializer |

The reverbs, chorus and flangers are space and movement made by hand; the distortions and EQs are
the instrument's tone. The synth keeps the tone and the parameter responses and leaves the space to
the acoustics.

## The synth

`src/red4ext/Audio/EngineSynth/` with no game or FMOD types (like `Reflections`): `Curves.hpp` (the
event's curves, one home), `Synth` (one thruster), `Channels` (the plugin's FMOD side).

- Axis voices (surge, lift, pitch, roll, sway): two detuned PolyBLEP saws and a sine at the loop's
  fundamental (46 or 66 Hz) times the car's pitch and the curves' semitones, then each track's
  chain in FMOD's order: its EQ peaks, a distortion by speed and axis, the gain by axis, a second
  fixed distortion, the low-pass following speed. FMOD's distortion is `(1 + k) x / (1 + k |x|)`,
  `k = 2a / (1 - a)`: a high gain for quiet sound that clips loud sound, so the gains before it are
  squeezed (surge's 26 dB curve comes out about 13). Yaw and lift lift two bands of surge.
- The Engine return on their sum (EQ, the speed distortion, the low-pass and shelves), and the cone
  path: `1 + Db(send(inside) + cone(angle))` times it.
- Air (pink noise band-passed, opening with thrust and speed; -30 dB, a little texture), brake (two
  bands of hiss, tremolo 2..20 Hz with speed), damage (a whirl at 392 Hz rising with speed), scrape
  (sparse filtered clicks), inside level and the water low-pass.
- Inputs smoothed (20 ms), curves read every 32 samples. No reverb, chorus or flanger.
- `Tuning` holds each voice's level against the event, fitted with `engine_fit.py`: source -6 dB,
  surge +1, lift -1, pitch +3, roll -3.5, sway -1, brake +10, master -7.2 dB.

## In the plugin

- `FlightAudio.StartEngine(id, pitch)`, `UpdateEngine(id, matrix, volume, update)`, `StopEngine(id)`
  take what `StartWithPitch` and `UpdateEvent` take; `IFlightThruster` calls one pair or the other by
  `engineSource` when it starts (flight off and on to switch).
- Each thruster is a Core channel (`System::playDSP`, in the Engine bus's channel group): the synth as a
  generator DSP (stereo, the same in both, as the event's loops: the spatializer folds the pair into
  one object, 6 dB over a mono source), the acoustics DSP (`Acoustics::CreateDsp`, the echoes, at the mixer's rate: no
  event pitch), the fader, an Object Spatializer (linear-squared rolloff 0..100 m, as Studio's) whose
  3D position the plugin sets from the thruster's matrix and the listener's, in listener space. The
  channel's fader is +4.7 dB: the event still reached the speakers that much louder than the same stem
  from a channel, for a reason not found (measured, then set; not chased further).
- `UpdateEngine` works out the Event Cone Angle as FMOD does (thruster forward against the way to the
  listener) and passes it in. Stems record the channels as they do the events (dry: the synth, wet:
  with the echoes); the parameter log works for both.
- Cost in game: 1.05% of a core per thruster on the mixer (4% for four), 112 us a block, worst block
  0.9 to 1.7 ms. Offline about 200 ns a thruster frame.

## Switches: which FMOD effects to keep

The event's effects were made by hand to sound like a space; with the acoustics doing that from the
city, some work against it. `Character` (Synth.hpp) keeps or drops each, all on by default (the event):

- `cone`: the Cone return, up to +7 dB when a thruster faces the listener, from the camera's angle.
- `distortion`: FMOD's three distortion stages. Off, each keeps its level behaviour as a gain following
  the sound's envelope (the same squeeze of the swings) without reshaping the wave.
- `darkness`: the Engine return's high shelf (-16 dB over 240 Hz) and 2.7 kHz low-pass. Off: a mild
  tilt (-4 dB over 800 Hz) and an 8 kHz low-pass, for distance and the echoes to darken instead.
- `grit`: speed driving the distortion and opening the low-pass. Off: held at a 40 m/s cruise's
  (pitch still follows speed).

And on the echoes, `Tuning::airAbsorption` (Reflections): each echo low-passed by its path (about
16 kHz near, 8 kHz by 60 m, 4 kHz by 180 m), the reverb's lines at 5 kHz.

In game, at once: `LtbfBridge.EngineCharacter("clean")`, `("fmod")` or `("darkness=0,grit=0")`, and
`LtbfBridge.AcousticsAir(true)`. These and the other audio setters (`EngineSource`, `Acoustics`,
`AcousticsLive`, `AcousticsTest`, `AcousticsStems`) answer with what they set and then the audio as the
plugin has it (`LtbfBridge.AudioStatus`: who plays the thrusters, the character, echoes, air, test sound,
ray source, stems), or a line starting `error:` when the input was wrong and nothing changed. Offline, for any recorded run with stems: `tools/acoustics/
engine_variants.py <run dir>` renders each variant from the run's parameter log with the echoes along its
own path (the car's place and axes from the four thrusters' logged positions, the nav map probed 20
times a second), at the event's loudness, as tracks of the run's video with a 10 s tour first.

On the FMOD free roam (variants matched to the event's loudness): dark as the event, the dry sound's
centroid 333 Hz and the echoes' 284; darkness off 594 and 513; clean 614 and 523; air absorption takes the
echoes from 284 to 243 Hz (clean: 523 to 470); distortion and grit move them a little; the cone did
nothing there (the camera behind and above, the thrusters never facing it).

## Tools

- The stems' `<base>_params.csv`: each thruster's parameters per frame (wall clock, id, start pitch,
  volume, position, every `FlightAudioUpdate` field, the cone angle).
- `../cp2077_tools/mods/let_there_be_flight/axis_sweep.py`: each control held in turn, recorded with stems (`--call
  LtbfBridge.EngineSource,1` for the synth).
- `tools/acoustics/engine_synth.cpp` (target `engine_synth`): a params log through the synth on the
  stems' timeline, `--voices` for each voice, `--tune` to try levels, `--character` for the switches,
  `--acoustics navmap.bin [--air]` for the echoes along the run's path.
- `tools/acoustics/engine_variants.py`: a run's video with each switch's variant as a track.
- `tools/acoustics/engine_fit.py`: the synth against the FMOD dry stem of the same run, per control
  held (level, centroid, lowest partial) and the loudness correlation.
- `tools/acoustics/stems_video.py --extra synth=<render>.wav`: the render as another audio track of the
  run's video.

## Where it stands

On the FMOD axis sweep (the inputs it was fitted on): every control held within 1.6 dB rms of the
event, brightness close (centroids 260..1440 Hz, within about 100 Hz on most), the same lowest
partial, loudness r 0.89. On a free roam it was not fitted on: 2.4 dB quieter, r 0.86. Left: at speed
with nothing held FMOD is about 3 dB louder and brighter; pitch, roll and yaw centroids run low. The
FMOD event stays the default until it is heard in game.
