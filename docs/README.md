# Docs

- `design/`: how each subsystem works now, edited in place when it changes.
  - `flight-modes.md`: the flight modes (Standard, Hover, Fly, Hover & Fly, Drone, the Autopilot as a mode), the native
    FlightMode base, FindGround, Handling, the Mod Settings and developer values, the feel spec.
  - `flightsim.md`: the offline simulator (`tools/flightsim`): the feel tests, the fleet, the city's collision, terrain runs.
  - `autopilot.md`: the Autopilot in C++, a flight from engage to landing, tuning, the offline trip and its checks.
  - `trajectory.md`: the timed trajectory over a route and the follower that flies it.
  - `planner.md`: route search, doorways, the line's passes and repair, navplan.
  - `guidance.md`: the flight route published for other mods to draw (`FlightGuidance`), from the Autopilot or
    planned for flying by hand.
  - `vehicle-limits.md`: the car's limits (VehicleSpec) and who reads them.
  - `navmap.md`: the baked nav map, how it is made and shipped.
  - `world-boundaries.md`: the areas the game keeps the player out of, gated by quest facts, laid over the nav map.
  - `city-collision.md`: hidden colliders removed, proxy colliders added.
  - `cinematic-camera.md`: the Autopilot's cinematic camera in flight: level attached shots, framing with room ahead,
    flybys beside the route, shot choice.
  - `vehicle-summon.md`: the pad summon (a summoned vehicle flies the Autopilot to the pad) and its fallback.
  - `landing-pads.md`: pads, pins, supplies, real decks for visual-only pads.
  - `physx-flight.md`: the experimental PhysX-owned flight physics.
  - `scrape-effects.md`: sparks, trails and the scrape sound per surface.
  - `audio.md`: the sound: the Wwise and FMOD backends, buses, the voice pool, samples, wind, the per-frame inputs.
  - `acoustics.md`: the echoes and late reverb round the player's car, from live rays and the nav map.
  - `engine-synth.md`: the thruster sound, made in C++.
- `release.md`: cutting a release, Nexus Mods.
- `game-patch.md`: what a game patch breaks, the update checklist, reverse engineering, the SDK v1 migration.
- `nexus/`: the Nexus page texts (sticky, settings guide).
