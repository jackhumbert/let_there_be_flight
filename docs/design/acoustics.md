# Acoustics

The player's flying car heard in the space round it (`src/red4ext/Audio/Acoustics/`): the echo off the floor that
sweeps as the car climbs or dives, slapback and flutter between street walls, a floor to ceiling comb under
overpasses, slap back off far facades, and a late reverb that grows as the city closes in. Each echo is a voice of
its own, heard from its image source, so it comes from where it bounces. NPC cars get no echoes; they are muffled
when a building is between them and the listener (`Occlusion`). How the voices are played is `audio.md`.

## Threads

- **Game thread**: each frame the player's car flies, `Inputs::Gather` calls `FlightAudio.UpdateAcoustics` with the
  car's matrix, the echoes' level and how far the view is inside the cabin (`Acoustics::SetCar`); PrePhysics sets
  the listener (`Acoustics::SetListener`) and moves the echo voices (`Engine::Frame`). Both go into a pose slot under
  a mutex.
- **Vehicle fixed update**: the PhysX rays (`Live rays`), cast where the game runs its own wheel raycasts.
- **Worker** (`Acoustics.cpp`): 20 times a second (`kProbeHz`) it probes round the car (`Probe`), fits planes
  (`Planes`), turns the result into `Params` (`Targets`) and publishes them, numbered.
- **Audio thread**: the acoustics engine's bed (`Engine`) takes the newest Params at the start of a block without
  waiting on the worker (`NewestParams`), and `Reflections` ramps to them over one probe interval.

The echoes fade in and out over 50 ms: out when `SetCar` has not been called for a quarter of a second (the car
stopped flying) or its level is 0. In first person (`inside` 1) the echoes drop about 10 dB; the cabin muffle is on
the dry sound already. `FlightAudio.acoustics` and `acousticsGain` switch them and set their level.

## Probe

`Probe` (the port of `tools/acoustics/probe.py`) answers, for the car's position and axes:

- **floor and ceiling**: the gap of the nav map (`navmap.md`) the car is in, straight from its column;
- **left and right**: the nearest hit in a 15 ray fan round the car's horizontal right (yaw -30 to 30 degrees in 15
  degree steps, pitch -10, 0, 10). The minimum over a fan is the perpendicular distance to a wall facing it;
- **enclosure and mean free path**: the share of a sphere's rays that hit within 200 m, and their mean distance.

These 32 rays (down, up, two fans: `NearRays`) and a 48 ray Fibonacci sphere are marched through the nav map in 1 m
steps out to 200 m. The map's solids are grown by the car's shape, so a hit is pushed back out along the ray by that
ellipsoid's radius; a ray starting inside the grown floor (a car on the ground) hits only after it has been in free
space. Columns with no data (outside the city) count as open, an empty column inside the map as solid. The map has
no materials and no traffic; interiors are solid, which is right for sound outside them.

The nav map is the fallback. Where the game's own collision answers a ray (`Live rays`), its hit replaces the map's;
a live miss keeps the map's answer, since it is often collision not streamed in yet or a building with none up high.
Live hits are used only within 200 ms and 10 m of the probe.

The sphere also gives the four **quarters** of the horizon (world +x, +y, -x, -y; rays steeper than 50 degrees belong
to none): how enclosed each is, and its facade 20 to 200 m out (the weighted median of its hits there, with the share
of its rays within 15% of it).

## Live rays

`LiveRays.cpp` casts through the game's PhysX scene (`PxScene::raycast` by vtable slot) from the player's car's
fixed update, once a probe interval when `FlightAudio.acousticsRaySource` is 2 (the default; 0 leaves the nav map
alone):

- **Near rays**: the 32 of the probe, to 100 m, keeping each hit's distance, position, surface normal and physics
  material. The fans turn by -4.5, -1.5, 1.5 or 4.5 degrees from cast to cast, so successive casts find different
  spots of a surface.
- **Live sphere**: the developer setting `acousticsRays` (1024 by default) less the near rays, to 200 m, for the
  enclosure and mean free path in place of the map's 48. The rays bunch where the car is going (`SphereRays`: a
  Fibonacci sphere whose cosine to the velocity is warped by `SphereBias`, uniform at rest, a third of the rays within
  about 40 degrees ahead from 60 m/s), so the place it is about to be in is sampled finest. Each ray is weighed by
  its band's area, so the weighted enclosure is still the whole sphere's. The sphere turns about the velocity each
  cast. A miss takes the nav map's answer for the nearest of its 48 rays.
- **Occlusion rays**: up to 4 a cast (`Occlusion`).

Only static simulation shapes are hit: buildings, terrain, blockers, not the car itself, traffic, triggers or
query-only volumes (which a ray from inside reports at 0 m). The scene's origin is shifted near the player, so the car
is found by its world position and the rays leave from the chassis body's place in the scene. A hit's material name is
the CName the game keeps with its `PxMaterial`.

## Planes

`Planes` makes the walls steady. Each cast's live hits go into a world space cache of the last second (1024 hits,
dropped past 150 m). Each side of the car (floor and ceiling within 45 degrees of straight down or up, else left or
right) takes the plane most of its hits agree on: every hit is tried as a seed, hits within 1 m of its plane with
normals within 30 degrees agree, and at least 4 must. The plane's normal is the agreeing hits' mean, its point their
mean position. So a pole or a sign cannot stand in for a wall, and a cast that finds nothing (collision not streamed
in) does not drop it. A fitted plane's distance and normal replace the probe's on its side. `FlightAudio.AcousticsPlanes`
switches them; off, each surface is the probe's nearest hit seen square on.

## Materials

`Materials` gives each physics material, matched by name prefix, a reflectivity in three bands: low (below 250 Hz),
mid and high (above 2.5 kHz). Hard smooth surfaces reflect most (metal 0.9 flat, concrete 0.9, 0.85, 0.8); soft ones
keep their lows and lose their highs (foliage 0.35, 0.2, 0.1; fabric 0.35, 0.2, 0.05); glass the other way (0.8,
0.95, 0.95). An unknown material, or a surface the nav map answered, takes the defaults: 0.8 for walls and ceilings,
0.7 for the floor, flat. An echo's tone is its surface's bands against the mid (`BandSplit`: the sound split by
one-pole filters at 250 Hz and 2.5 kHz, each band scaled).

## Image sources

A surface is a plane (a point and its normal). The echo off it is heard from the car's mirror image behind the plane
(`Images`), along the path car to image to listener, so a wall at an angle gives the right delay and direction, and a
third person camera hears a different floor echo than the car would. For an echo:

- **delay**: the extra path over the direct sound, over 343 m/s;
- **rate**: with the car and listener moving at v past a still surface, the path to the image changes at
  2 (v.n) (L - I).n / |L - I|, so its Doppler shift comes from the car's velocity (smoothed from probe to probe), not
  from the distance changing between probes;
- **gain**: the reflectivity times spherical spreading against the dry sound heard at 3 m (`Tuning::nearDistance`),
  none past 249 m.

The echo voices are placed at these images.

## Echoes

`Reflections` (the port of `tools/acoustics/render.py`) renders, per sample:

- **Two echo pairs**: floor and ceiling, left and right wall. One echo off each surface, then cross feedback between
  the two, the second bounce taking the other side's round trip: the flutter between parallel walls. Feedback is 0.5
  times the square root of the two reflectivities (the walls' fourth root), full while the two surfaces are within
  60 m of each other and gone by 120 m. The floor alone is the comb that sweeps as the car climbs or dives.
- **Taps**: each echo is a voice of two taps on a delay line. Between probes a tap moves at its echo's rate, steered to
  reach the next target, and its pitch follows (1 - rate). A target further than 4 ms beyond the motion's share from
  the tap is another reflection (the nearest ray now on another building): the other tap fades in at it over 80 ms
  while the old one fades out. 0.3 s/s bounds how fast a delay may move. Delays and ramps are doubles: a delay of tens
  of thousands of samples moving a thousandth of a sample a sample would stand still in float.
- **Reads** are smeared over three samples (absorption at the surface), except the floor's, which is read unsmeared
  so its echo combs with the direct sound near the ground. The shortest delay is 0.5 ms, short enough for the floor's
  comb with the car low under a third person camera.
- **Air absorption** (on by default, `FlightAudio.AcousticsAir`): a one-pole low-pass per echo by its path, about
  16 kHz near, 8 kHz by 60 m, 4 kHz by 180 m.

A value that is not a number is refused at the Params, and one that turns up in a sample resets everything to
silence, so the delay lines cannot be poisoned.

## Scatter

Every other near ray that hit is a weak echo back along it (0.12 of a main echo's gain), off the patch of wall it
found: the rest of the walls round the car, a cluster of arrivals that smears the pairs' single echoes into a decay.
They play on two voices, one a side, each heard from its echoes' images weighted by gain.

## Distant echoes

A quarter's facade 20 to 200 m out is an echo seen square on, off a wall of the default reflectivity, times the share
of the quarter's rays on that facade: the slap back off far buildings. It plays on the quarter's voice with the
quarter's tail.

## Late reverb

Two 8 line Householder feedback delay networks with fixed delays, so the tail never warbles: a small one (7 to 20 ms)
for an underpass or a narrow street and a large one (30 to 73 ms). They share the input by the space's size at equal
power: the small alone up to a mean free path of 8 m, the large alone from 25 m.

- **Decay**: Eyring with the mean free path as 4V/S. The absorption is the open sky's share (the sky absorbs
  everything) plus the rest times the surfaces' (0.5). Two RT60s, 0.2 to 2 s: the lows' from the near rays'
  materials' low bands against the mid, and the highs' from their high bands, the highs also lost to the air
  (0.01 per m, Sabine's 4mV term). Each line's loop is a one-pole with the lows' gain at DC and the highs' at Nyquist,
  worked out again every 64 samples.
- **Send**: 0.15 at full enclosure, starting from enclosure 0.35, since the lower half of the sphere is mostly ground.
- **Direction**: the tail leaves by quarter of the horizon. Each quarter's voice plays two of the lines, scaled by how
  enclosed that quarter is, from its facade's image or a point 30 m that way. The four together are as loud as a
  stereo bed of four lines a side: the lines all take the same input and add partly in phase, so a pair counts
  sqrt(2).

## Acoustics engine

`Engine` is where the player's echoes are made, one engine for the car rather than one per thruster:

- **Bed**: a 2D voice in the Engine bus, never paused while the player flies. It renders the player's thruster synths
  (up to 8 slots), writes each to its dry ring (`audio.md`, Rings) and adds it to the dry sum, runs
  `Reflections::ProcessStreams` on the sum, and writes each echo stream to an echo ring. Its own output is silent.
  Under Wwise it also runs the Engine bus's limiter on what the thruster and echo voices will play.
- **Thrusters**: each player thruster's voice reads its dry ring and is placed by the voice pool as any other.
- **Echo voices**: ten voices in the Engine bus (floor, ceiling, left, right, scatter right, scatter left, the four
  quarters), ranked in the pool after the player's own sounds and before NPCs, placed each frame at their images. They
  have no rolloff: the spreading is in the echo's gain, the Doppler in its delay's rate.

Every ring starts one block behind, the dry and the echoes alike, so the echoes' delays against the thrusters stay
exact whichever of the bed and the voices the backend runs first.

## Occlusion

NPC cars are heard through buildings (`Occlusion`, game-free). Each frame the voice pool publishes the NPC cars that
play, nearest 16 first, into slots. The PhysX caster casts a ray from the listener to up to 4 of them a cast, round
robin, through the static shapes, to 100 m at most: a hit nearer than the car less 3 m (its own size) is blocked. The
game thread smooths that per car (0.1 s) into 0 (open) to 1 (blocked), and the car's voices get a low-pass, log-spaced
from 22 kHz to 1.2 kHz, and a duck of up to 8 dB. `FlightAudio.AcousticsOcclusion` switches it; off, no rays are cast
and the cars fade back to open.

## Stems and the test source

`Stems` writes LTBF's sound to WAVs while the game runs (`FlightAudio.AcousticsStems`, to
`red4ext/logs/ltbf_stems_<time>`): `_dry` the player's thrusters summed before the echoes, `_wet` the same with the
echoes and the reverb panned as `Reflections` pans them, `_bus` the FMOD master's output, a `.json` with the mixer
rate and the first sample's wall clock, and `_params.csv` with each thruster's inputs. Stereo 32-bit float, mono
sources in both channels, unspatialized. A writer thread saves what is half a second old and fixes the headers on
every write. Under FMOD the master group's DSP keeps the timeline; under Wwise the bed does and the bus stem is
silent.

`FlightAudio.AcousticsTestSource` plays a plain sound in place of LTBF's to hear the echoes on: clicks (a 3 ms burst
twice a second), pink noise or a 440 Hz sine, at the mixer's rate, not spatialized. Under FMOD the master DSP plays
it; under Wwise it is a 2D stereo voice of its own, with its echoes.

`FlightAudio.AcousticsLog` writes the nav map's and the live distances side by side, a row a probe, to
`red4ext/logs/ltbf_acoustics_<time>.csv`. `AcousticsState` and `AudioSummary` read the state out.

## Offline tools

`Probe` and `Reflections` take no game or FMOD types, so the offline target `acoustics`
(`tools/acoustics/acoustics.cpp`, not built by default) runs the plugin's code over Autopilot telemetry and a
recording. `tools/acoustics/` also has the Python reference (`flyby.py` with `probe.py` and `render.py`), the checks
of the port against it (`compare_features.py`, `compare_render.py`), `compare_live.py` for the comparison log, and
`stems_video.py`, which puts a run's stems under its video. Unit tests: `probe_`, `reflections_`, `images_`,
`planes_`, `materials_` and `occlusion_` in `tests/`.
