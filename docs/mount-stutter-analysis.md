# Enter/exit vehicle stutter and HUD reset (LTBF-3)

Status 2026-09-13: analysis from source, IDA (game 2.31) and the 2026-09-13 smoke-test log.
Not yet measured in-game. Two safe changes and one experiment toggle shipped, see the end.

## What LTBF does when the player mounts a flight-capable vehicle

Nothing native runs on mount. `FlightComponent::OnAttach` (C++) runs at vehicle spawn, and
`Entity_InitializeComponents` (hook 3490519617, `sub_1408B0190` in the 2.31 IDB) also runs at
spawn: it appends the `FlightComponent`, the thruster `entMeshComponent`s, extra slots and a
`WidgetHudComponent` named `FlightHUD` (`user\jackhumbert\widgets\hud_flight.inkhud`) to the
vehicle. Everything on the mount/unmount path is redscript plus what the engine does in
response to those components.

Timeline from `red4ext/logs/let_there_be_flight.log` (Shion MZ1, 2026-09-13 11:06; spdlog
stamps are milliseconds, so the log is the profiler):

| t (s) | event | who |
|---|---|---|
| 0.000 | `[FlightComponent] OnMountingEvent` | `FlightComponent.reds` `OnMountingEvent` |
| 0.000 | `[FlightSystem] Player component set` | same handler |
| 0.092 | `VehicleFlightDisabled` -> 0.130 `VehicleFlightEnabled` | engine adds the on-demand `VehicleFlight` PSM queued by `OnMountingEvent` |
| 0.466 | `[hudFlightController] OnInitialize / OnPlayerAttach / ActivateUI` | engine spawns the vehicle's `FlightHUD` hud entries |
| 3.125 | `[InputContext] vehicleDriverContext` | vanilla |
| 3.843 | `[FlightComponent] OnVehicleFinishedMountingEvent`, `[FlightController] Enable` | `OnVehicleFinishedMountingEvent` -> `FlightController.Enable` -> `SetupActions` |

On unmount the mirror image: `OnUnmountingEvent` queues `PSMRemoveOnDemandStateMachine`,
`FlightController.Disable` -> `SetupActions`, and the engine calls
`hudFlightController.OnPlayerDetach` then `OnUninitialize` (0.7 s apart in the log, but that
sample was the world detach at quit, not a normal exit).

## Candidates, ranked

1. **HUD entries spawned and destroyed per mount** (`Entity_InitializeComponents.cpp`,
   `hud_flight.inkhud` -> `flight_hud.inkwidget`). The engine treats a `WidgetHudComponent`
   on the mounted vehicle the way it treats the Basilisk/turret HUDs: its entries are
   spawned when the vehicle becomes the player's mount and torn down when it stops being
   one. The log proves the cycle (`OnInitialize` 466 ms after the mount event, `OnUninitialize`
   after detach). `flight_hud.inkwidget` is 3.3 MB of raw JSON: 377 widgets (164 images,
   61 texts, 75 canvases, 9 masks), 307 property bindings, plus `flight_hud_animations.inkanim`.
   inkwidget spawning is synchronous on the main thread, so this is the best match for a
   hitch that scales with widget count rather than CPU speed, and it is the only LTBF code
   that touches the HUD layer on mount, so it is also the only candidate for the "HUD reset"
   reports (the vehicle entry spawn goes through the same `inkHUDLayer` player attach path
   that fires `OnPlayerAttach` on every HUD controller; `sub_1409099C8` is the native that
   invokes scripted `OnPlayerAttach`, name hash `0xE71EF3A4F0CF1EE4`).
   Estimated cost: tens of ms on a fast machine, more with many HUD mods.
2. **On-demand `VehicleFlight` player state machine** added on mount and removed on unmount
   (`PSMAddOnDemandStateMachine` / `PSMRemoveOnDemandStateMachine` in `FlightComponent.reds`).
   The PSM picks it up ~90 ms later and runs `VehicleFlightDisabled` -> `VehicleFlightEnabled`
   (`src/tweaks/stateMachine.tweak`). Cost unknown; it instantiates the 6 scripted
   transition classes and re-evaluates the InputContext machine. Cheap to time: the
   `[VehicleFlightEventsTransition]` lines already exist.
3. **`FlightController.SetupActions`** (twice per mount/unmount: `Enable`/`Disable`, and
   again on activate). Unregisters/re-registers ~17 input listeners and queues one
   `UpdateInputHintMultipleEvent` with 16 hints. Sub-millisecond; not the hitch.
4. **`OnVehicleFinishedMountingEvent`** ran `FindGround` (4 synchronous raycasts) on every
   player mount even with auto activation off. Fixed below; microseconds anyway.
5. **`OnMountingEvent` for every occupant of every flight-capable vehicle** (NPC drivers too)
   recomputes `thrusterTensor` (4 slot transforms) and does one `FindEntityByID`. Cheap.
   Left alone because quickhacked NPC vehicles rely on the tensor computed at their driver's
   mount.
6. Audio: nothing starts on mount. FMOD banks load once at plugin start
   (`FlightAudio.cpp`); events start only on flight activation (`vehicle3_on`, thrusters).
7. Thruster meshes: created at spawn, not at mount. `entVisualControllerComponent`
   registration happens in `EntityExt::AddComponent` at spawn.

Nothing on the mount path loads a resource synchronously, iterates all vehicles, or
re-registers settings.

## Shipped in this change

- `FlightComponent.reds`: `FindGround` only runs when `autoActivationEnabled`; timing
  markers `OnMountingEvent done`, `OnVehicleFinishedMountingEvent done`,
  `OnUnmountingEvent` / `OnUnmountingEvent done`.
- `Entity_InitializeComponents.cpp`: the `FlightHUD` `WidgetHudComponent` is only added when
  Mod Settings "Let There Be Flight > Flight UI Settings > Enabled" is on (read from the
  class default of `hudFlightController.enabled`, which Mod Settings keeps updated). This
  is the experiment switch for candidate 1 and a free win for users who turned the flight
  UI off. Applies to vehicles spawned after the setting changes (reload a save).

## In-game measurement

1. Baseline: default settings, summon a car, enter, wait, exit; repeat 3 times. In the log,
   measure `OnMountingEvent` -> `hudFlightController OnInitialize` and watch for the frame
   hitch relative to those lines (RED4ext logs and the game share the wall clock; a
   `Ctrl+Shift+F` style frame-time overlay or just feel).
2. Disable "Flight UI Settings > Enabled", load the save (so `Entity_InitializeComponents`
   re-runs without the HUD component; the log prints one
   `flight UI disabled in Mod Settings; FlightHUD component not added` warning), repeat.
   If the hitch and the HUD flicker are gone, candidate 1 is confirmed and the fix is to
   move the HUD entry to the player puppet (spawn once, show/hide on mount) or to a
   Codeware/inkSystem external spawn kept alive across mounts.
3. If the hitch survives step 2, set `ignored = ["let_there_be_flight"]` in
   `red4ext/config.ini` to confirm it is LTBF at all, then compare the
   `[VehicleFlightEventsTransition]` timings (candidate 2); the remaining experiment is to
   comment out the two `PSM*OnDemandStateMachine` queues in `FlightComponent.reds`
   (flight cannot be activated then, but mount/unmount still works).

## IDA references (2.31 IDB, image base 0x140000000)

- `Entity_InitializeComponents` hook target: `sub_1408B0190` (addresses.json hash 3490519617,
  offset 0001:008af190).
- `WidgetHudComponent` RTTI name string at 0x142D26868 (used at 0x1415FA20F),
  `hudEntriesResource` property registration at 0x1405AC057, `inkHUDLayer` at 0x14176A1FF,
  `inkHudEntrySpawnedEvent` at 0x14176A36B, scripted `OnPlayerAttach` dispatcher `sub_1409099C8`.
