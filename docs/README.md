# Docs

- `history.md`: decisions and findings, newest first, append-only. Write an entry for every finding
  or decision (date, title, decision, why, numbers, tried and dropped, commits).
- `design/`: how each subsystem works now, edited in place when it changes.
  - `autopilot.md`: the Autopilot flight mode, a flight from engage to landing, tuning.
  - `planner.md`: route search, doorways, the line's passes and repair, navplan.
  - `vehicle-limits.md`: the car's limits (VehicleSpec) and who reads them.
  - `navmap.md`: the baked nav map, how it is made and shipped.
  - `city-collision.md`: hidden colliders removed, proxy colliders added.
  - `landing-pads.md`: pads, pins, supplies, real decks for visual-only pads.
  - `physx-flight.md`: the experimental PhysX-owned flight physics.
  - `acoustics.md`: echoes and reverb from the nav map (offline prototype).
  - `engine-synth.md`: a C++ thruster sound on the FMOD event's parameters (behind a switch).
- `testing.md`: driving the game for tests, the test drivers and bridge helpers.
- `release.md`: cutting a release, Nexus Mods.
- `game-patch.md`: what a game patch breaks, the update checklist, reverse engineering.
- `research/`: dated analyses and research notes, kept as written.
- `archive/`: superseded plans and logs, kept as written (`autodrive-flight-plan.md`,
  `helipads.md`, `organization_plan.md`).
- `nexus/`: user-facing texts (sticky, settings guide) and the comment feedback queue.
