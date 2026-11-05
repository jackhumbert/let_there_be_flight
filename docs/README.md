# Docs

- `history.md`: decisions and findings, newest first, append-only. Write an entry for every finding
  or decision (date, title, decision, why, numbers, tried and dropped, commits).
- `design/`: how each subsystem works now, edited in place when it changes.
  - `autopilot.md`: the Autopilot flight mode, a flight from engage to landing, tuning.
  - `planner.md`: route search, doorways, the line's passes and repair, navplan.
  - `hover-fly-feel.md`: how Hover & Fly should feel (the draft spec), the offline simulator that
    measures it (`tools/flightsim`), and what the mode does today.
  - `hover-fly-native.md`: plan, not built: the Hover & Fly redesign as the first flight mode written wholly in
    C++, its Mod Settings registered from C++.
  - `settings-audit.md`: proposal, not built: every Mod Setting, what it changes, and a smaller player-facing set.
  - `vehicle-limits.md`: the car's limits (VehicleSpec) and who reads them.
  - `navmap.md`: the baked nav map, how it is made and shipped.
  - `city-collision.md`: hidden colliders removed, proxy colliders added.
  - `vehicle-summon.md`: the pad summon (a summoned vehicle flies the Autopilot to the pad).
  - `summon-edge-hover.md`: plan, not built: a summon with no room to land waits at the roof's edge, driver door to V.
  - `landing-pads.md`: pads, pins, supplies, real decks for visual-only pads.
  - `paraglide.md`: the shelved glider for leaving a car in flight, and the plan to finish it.
  - `physx-flight.md`: the experimental PhysX-owned flight physics.
  - `acoustics.md`: echoes and reverb from the nav map (offline prototype).
  - `audio-native.md`: every sound in the plugin on FMOD Core: the voice system, samples, wind, echoes as voices.
  - `engine-synth.md`: the thruster sound, made in C++ on the old FMOD event's parameters.
  - `native-ports.md`: the plan for moving the per-frame redscript (follower, force path, modes,
    thrusters) to C++, and M6, the Autopilot as a C++ mode (steps 2 to 4 built).
  - `autopilot-regressions.md`: every Autopilot and summon problem found in game, and the test that
    should catch it again offline.
- `testing.md`: driving the game for tests, the test drivers and bridge helpers.
- `release.md`: cutting a release, Nexus Mods.
- `game-patch.md`: what a game patch breaks, the update checklist, reverse engineering.
- `research/`: dated analyses and research notes, kept as written.
- `archive/`: superseded plans and logs, kept as written (`autodrive-flight-plan.md`,
  `helipads.md`, `organization_plan.md`).
- `nexus/`: user-facing texts (sticky, settings guide) and the comment feedback queue.
