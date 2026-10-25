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

## Confirmed mechanism (2026-09-16, IDA on the 2.31 binary)

Candidate 1 is not "the flight HUD's 377 widgets respawn per mount". It is worse: **the
entire HUD respawns per mount and again per unmount.**

`inkHUDLayer` holds exactly one `inkHudEntriesResource` at a time.

- `game::ui::RootHudGameController::GetHudComponent(THandle<game::Object>)` (`0x140AECDCC`)
  walks an entity's components, returns the `hudEntriesResource` of the **first**
  `WidgetHudComponentInterface` it finds, and a null resource when there is none.
- `game::ui::RootHudGameController::OnEntityHudChange(EntityID)` (`0x140AED3A0`) runs that
  on the entity the player mounted; with an empty entity id it falls back to the player
  controlled object. It only broadcasts `ink::RequestNewHudEvent` when the resource it got
  is non-null.
- `ink::HUDLayer::OnRequestNewHud` (`0x141529A90`) forwards to
  `ink::HUDLayer::SpawnFrom` (`0x141C676A0`), which opens with
  `if (resource != current || force)` and then `DespawnHierarchy()`,
  `ClearSpawnedWidgets()`, `CopyEntries()`, and a fresh spawn job per entry.

So a vanilla car, which carries no `WidgetHudComponent`, produces a null resource on mount
and the HUD is left alone. LTBF's `FlightHUD` component made every flight-capable vehicle
answer with `hud_flight.inkhud`, which differs from the player's resource, so:

- mount -> despawn all ~72 HUD entries, respawn all of them from `hud_flight.inkhud`;
- unmount -> `OnEntityHudChange(0)` -> the player's resource -> despawn and respawn again.

That is the enter/exit hitch and it is exactly the "HUD reset" users describe: every HUD
controller in the game is destroyed and re-initialized twice per car trip.

`hud_flight.inkhud` is not a one-entry file. It is a copy of the base game's hud entries
resource (72 widget entries) with one extra `hud flight` entry pointing at
`user\jackhumbert\widgets\flight_hud.inkwidget`, which is why swapping to it does not blank
the rest of the HUD.

## Shipped 2026-09-16

New Mod Settings switch: **Let There Be Flight > Flight UI Settings > HUD attached to
player (experimental)**, default ON. It is read from the `hudFlightController.attachToPlayer`
class default by `Entity_InitializeComponents.cpp`, the same way `enabled` already was.

ON (new default):

- `Entity_InitializeComponents` sees the `PlayerPuppet` entity, finds the puppet's own
  `WidgetHudComponent` and repoints its `hudEntriesResource` at `hud_flight.inkhud`. Log
  line: `[Entity_InitializeComponents] FlightHUD attached to player ('<name>' -> ...)`.
  If the puppet somehow has no hud component, one named `FlightHUD` is appended instead.
- No `FlightHUD` component is added to vehicles, so `GetHudComponent` returns null on mount
  and `SpawnFrom` never sees a different resource. Zero HUD rebuilds: no
  `hudFlightController OnInitialize` / `OnUninitialize` pair per mount, and no
  `OnPlayerAttach` storm across every other HUD controller.
- `hudFlightController` now lives for the whole session, so it was made vehicle-agnostic:
  `OnInitialize` / `OnPlayerAttach` no longer dereference
  `FlightSystem.playerComponent` (there is no mounted vehicle at that point), the health
  stat pool listener and the vehicle blackboard moved into `BindVehicle` / `UnbindVehicle`,
  and `FlightComponent.OnMountingEvent` / `OnUnmountingEvent` call
  `FlightSystem.NotifyHudVehicleChanged(vehicle | null)` to rebind them. The root widget
  stays hidden until flight activates, exactly as before; unmounting hides it again.
  Log lines: `FlightHUD bound to <vehicle>`, `FlightHUD unbound`, `FlightHUD shown`,
  `FlightHUD hidden`.

OFF: the pre-0.3.19 path, one `FlightHUD` `WidgetHudComponent` per flight-capable vehicle.
Log line once: `FlightHUD component added per vehicle (HUD attached to player is off)`.

"Flight UI Settings > Enabled" off still means no flight HUD at all, on either path.

Both switches are read when an entity spawns, so **reload a save after changing them**.

### In-game A/B

1. Default settings (switch ON). Load a save, summon a car, enter, wait, exit; three times.
   In `red4ext/logs/let_there_be_flight.log` you should see `FlightHUD attached to player`
   once at load, then per mount only `[FlightComponent] OnMountingEvent` ...
   `FlightHUD bound to <vehicle>` and **no** `[hudFlightController] OnInitialize` /
   `OnUninitialize`. Watch for the hitch and for other HUD elements (minimap, quest
   tracker, health bar) flickering or resetting.
2. Turn "HUD attached to player" OFF, reload the save, repeat. The log should show
   `FlightHUD component added per vehicle`, an `OnInitialize` about 0.4 s after every mount
   and an `OnUninitialize` after every unmount. If the hitch and the HUD flicker come back
   here and are gone in step 1, the fix is confirmed.
3. Also worth checking in step 1: the flight HUD still shows the right vehicle name and
   health when you activate flight in a second car without reloading, and the crosshair,
   pitch ladder and mode text still update.

### Wolvenkit follow-up for the user

With the switch ON, `hud_flight.inkhud` is the HUD entries resource for the whole session,
not just while driving, so any HUD entry the base game gained after that file was last
copied is now missing on foot as well. The copy in
`src/wolvenkit/source/archive/user/jackhumbert/widgets/hud_flight.inkhud` is dated
2025-06-10 (game 2.3), and 2.31 is a minor patch, so it is probably still complete. Worth
one check before release: export the current base game hud entries resource in Wolvenkit,
diff its entry list against the 72 entries in `hud_flight.inkhud`, and if anything is
missing, re-copy the vanilla resource and re-add the single `hud flight` entry
(`hudEntryName` "hud flight", `widgetResource`
`user\jackhumbert\widgets\flight_hud.inkwidget`, `gameContextVisibility` Default /
QuickHack / Scanning / VehicleMounted, no slot attachment). No inkhud change is required
for the code shipped here.

One new localization key pair was added to
`src/wolvenkit/source/raw/user/jackhumbert/localization/let_there_be_flight.en-us.json.json`
(`UI-Settings-Hud-Attached-To-Player` and `-Description`); re-import and repack or the new
option shows raw keys.
