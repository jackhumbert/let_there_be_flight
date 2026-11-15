# Scrape effects: sparks, trails and sound per surface

What a flying car leaves behind and sounds like when it scrapes the world: sparks or dust where it touches, a trail
laid along the slide, and a scrape sound that changes with the surface, the slide and how hard the car presses.

## Contacts

`Physics/ScrapeAudio.cpp` hooks `vehicle::BaseObject::NotifyContacts`. The game drops every contact whose summed pair
impulse is under 100 while the vehicle is awake, before any effect, scrape or damage runs. A flying car sliding on
its body makes small impulses every frame (the chassis contact constraint in `ChassisContacts.cpp` keeps them small),
so vanilla never scrapes it. For a flying car, the world contacts the gate dropped are taken by LTBF: the contact's
position, the other shape's physics material (its .physmat CName), the slide speed along the surface, the impulse
and the load. The same file holds the grip table by material (`ScrapeEffects::FrictionOfMaterial`, a factor on
Ground Friction).

The thruster colliders are boxes open at the bottom, so a car on its skid does not touch the road. Scrapes come from
crashes, landings off the skid, kerbs and walls against the pods' sides, and the chassis.

## Surfaces

One table keys every part on the contact's physics material (`Flight::SurfaceOf`, `Flight/Scrape.hpp`, by prefix):

| Surface | What it does |
|---|---|
| Hard | concrete, asphalt, stone, brick, tiles, plaster: sparks and a scratch trail |
| Metal | sparks, more and hotter, and a scratch trail with a dark scuff |
| Glass | sparks, no trail |
| Wood | splinters |
| Soft | dirt, sand, mud: dust and a rut |
| Grass | torn turf and a streak of flattened grass |

Roads report `default_material` (and sometimes asphalt), kerbs and street furniture `metal_hollow` and
`metal_painted`. A character or the car's own chassis is None. `Flight::PeriodOf` gives a surface's repeating
features (a grating's bars, a railing's posts, tiles, planks, slab joints) as a spacing along the slide; the spacings
are the usual ones of such things, not read from the meshes.

## The tracker

The game reports a sliding car's contacts in bursts, a few in a couple of seconds along a kerb, not one a frame, so
anything renewed by contacts flickers. `Flight::ScrapeTracker` (pure, no game types) holds each scrape: where it is on
the car, what it is against, how hard. A contact near a scrape of the same surface renews it (`merge`, 1 m); a scrape
is held `hold` (0.3 s) after its last contact, and a scrape of one contact so far is an impact, held `impactHold`. Each
frame `Follow` sets every scrape's slide from its point's motion now (`SlideAt`, the car's spin included), not from
its last contact. The intensity rises quickly and falls once the scrape is let go. A scrape's loudness and size are
mostly its press (the load as a share of the car's weight), the slide adding the rest, so a slow scrape that presses
hard is not quiet.

## Effects

`Physics/ScrapeEffects.cpp` gives each scrape the tracker holds a spark effect and a trail decal effect for its
surface, spawned through the game's `FxSystem` where it begins and moved with the car every frame, so the trail is
laid along the slide. The effects take the scrape's inputs: their emission scales with its force, their velocity and
size follow its speed and press. A trail comes in three depths (`_light`, plain, `_deep`), picked by the press when it
starts, since changing a parameter on a trail changes the whole line already laid. The loops are broken when the
scrape ends, and all of them when flight is off.

While flight is on the wheels leave no skid marks or tyre tracks (`Hooks/VisualEffects.cpp`, `FindWheelsMaterial`):
the game gets a copy of the material's lookup with its effects emptied.

The effects are vanilla ones edited by `tools/fx/scrape_effects.py`, which unbundles them, edits the JSON and writes
them back into `src/wolvenkit/source/archive/user/jackhumbert/effects/`, so they can be made again after a patch:

- `scrape_trail_hard`, `scrape_trail_metal`: the tyres' sport skid mark (a particle trail drawn as a projected decal)
  on LTBF's groove textures, two gouges near a thruster's corners with their normals;
- `scrape_sparks_hard`, `_metal`, `_glass`: the vanilla scratch sparks with their birth rates scaled;
- `scrape_dust_soft` and `scrape_trail_soft` (a rut with berms), `scrape_trail_grass` and `scrape_turf_grass` (the
  tyre spray on grass, driven by the scrape).

## Sound

`Audio/EngineSynth/Scrape.cpp` is a small physical model of a thruster dragged along a surface, driven by the slide
speed: the surface's texture as pink noise between its largest bumps' rate and the contact's width (so the band slides
up with the slide), grit impacts per metre slid, stick-slip pulses that fade as the slide quickens, the surface's
repeating features, chatter past 6 m/s, and loose grains on soft ground. That excitation rings the surface's own
resonances (metal and glass long, wood short and low), the thruster pod's, and through the pod the car's chassis
(`ChassisModes`, from its size and mass).

Each player thruster has a scrape voice of its own (`EngineSynth::ScrapeChannels`) on the Scrape bus, outside the
engine's channel, limiter and echoes, placed where its scrape is. A scrape sounds from its nearest thruster only
(`Flight::NearestEmitter`): played by every thruster nearby it combed. Each voice is detuned by its seed, so the four
pods scraping at one speed do not beat. `scrape_voice_pitch_follows_slide` checks that the sound's spectral centre
climbs with the slide on every surface.

The game's own scrape loop (`vehicle::Audio::NotifyScrapingCollision`) is fed from the same contacts with a zero
normal, which passes its belly angle gate, and ends on its own when the calls stop.
