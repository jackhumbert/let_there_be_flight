# CrystalCoat / customization crash (LTBF-2): code-level analysis

Status: 2026-09-13, code analysis only (no in-game reproduction yet). Covers the ~30 Nexus
reports (2025-07 to 2026-04) of crashes when entering or recoloring CrystalCoat-capable cars
(Caliburns, Aerondight, Hella, Type 66 variants, Emperor, Outlaw, Galena, Alvarado, Riptide
Terrier, Trophy Tanishi, Johnny's Porsche, quest cars). See
`docs/nexus-feedback-2026-09/report_5208.md`, issue 2.

Crash signatures from user dumps:

- `Detected integer overflow for offset: 276192813200 aligned: 276192813200 capacity: 3337213872 elementSize: 11 File: ...\redContainers\src\dynamicBuffer.cpp(96)` (skyrimsasuke, 2025-07-22)
- `NULL_CLASS_PTR_READ_INVALID_POINTER_READ_c0000005_let_there_be_flight.dll!Unknown` (Zylenxx, 2025-07-28)

## 1. What is on the customization path

Order of events when a vehicle entity is built (all in the plugin unless noted):

1. `src/red4ext/Hooks/Entity_InitializeComponents.cpp` hooks `ent::Entity::Initialize`
   (hash 3490519617). For every `vehicleBaseObject` it runs the redscript
   `VehicleObject.CanEnterFlight()` and, if true:
   - creates a `FlightComponent` and appends it to `componentsStorage.components`,
   - finds the `vehicle_slots` `entSlotComponent`,
   - picks a configuration class (`IFlightConfiguration::GetConfigurationClass`, which walks
     every component and casts each `parentTransform` to `entHardTransformBinding`),
   - runs the redscript `OnSetup`, which builds the thrusters: each `FlightThrusterXX.Create`
     calls `VehicleObject.AddSlot(...)` (native, `EntityExt::AddSlot`) and
     `IFlightConfiguration.CreateMesh` (a `PhysicalMeshComponent` whose mesh and appearance
     come from `Vehicle.<record>.thrusterMesh` / `.thrusterAppearance` tweak flats),
   - for each thruster with a mesh calls `EntityExt::AddComponent(mesh)`,
   - adds the `CustomFlightCamera` slot and a `WidgetHudComponent` (flight HUD).
2. `src/red4ext/Extensions/MeshComponent.cpp`, `EntityExt::AddComponent`:
   - `componentsStorage.components.PushBack(component)` (raw push, not the engine's
     `ComponentsStorage::AddComponent`, so the engine's own bookkeeping for the new
     component is skipped),
   - looks up the `entVisualControllerComponent` and appends a
     `VisualControllerDependency` to `appearanceDependency` and the mesh path to
     `resourcePaths` (so the thruster's appearance resolves),
   - looked up the `entEffectSpawnerComponent` named `vehicleVisualCustomization` (this is
     the CrystalCoat component) and patched its effect descriptors. **This is the part
     7a1a51d disabled.**
   - sets a fresh `physicsFilterData` on `entPhysicalMeshComponent`s.
3. `src/red4ext/Hooks/VehicleDetachPart.cpp` hooks part detachment (hash 2150896908) through
   a hand-written `Manager` struct and indexes `self.parts[index]`.
4. `src/red4ext/Extensions/VehicleObject.cpp` exposes `GetComponentsUsingSlot`,
   `GetWeapons`, `GetWeaponPlaceholderOrientation`, inertia getters, `TurnOffAirControl`;
   all read hand-written `vehicle::BaseObject` fields.
5. `src/red4ext/Hooks/VehicleCustomization.cpp` is entirely commented out (an old
   `VehicleCustomizationLayer_Create` hook, hash 3955119979, never active).
6. `src/red4ext/FlightThruster.cpp` is only a destructor; `IFlightThruster` holds the
   `Handle<ent::MeshComponent>` at 0x90 that redscript reads.
7. Redscript touching appearance: `IFlightConfiguration.OnSetup` classifies the vehicle by
   `GetCurrentAppearanceName()` substring, `CreateMesh` reads the two tweak flats,
   `Flight/Thruster/Meshes.reds` has the corpo/nomad mesh factories. Nothing in redscript
   talks to CrystalCoat (`vehicleVisualCustomizationHotkeyController` wrap is commented out).

### What 7a1a51d ("disable customizations for now", v0.3.16, 2025-08-29) changed

Two things:

- `MeshComponent.cpp`: `if (customization)` became `if (customization && false)`. The dead
  block iterated `customization->effectDescs`, and for the `vvc_color`, `vvc_color_instant`
  and `vvc_damage_glitch` descriptors did `compiledEffectInfo.componentNames.PushBack(name)`
  and OR'd a new bit into every event's `componentIndexMask`. Those descriptors live in the
  vehicle's `.ent` resource (shared, resource-allocated `DynArray`s, 64-bit component masks
  with a 16-entry cap that the earlier hotfix 94362f0 tried to respect). Growing a
  resource-owned `DynArray` in place is exactly the kind of write that makes the engine
  assert in `dynamicBuffer.cpp` with a garbage `capacity`.
- SDK submodule bump a8cb29c -> 60f12bc ("more effect spawner stuff"): the hand-written
  `entEffectSpawnerComponent` layout was extended (`componentCache`, `components`,
  `transformProvider`, `effectPointers`, ...), asserted at 0x230.

Still active after 7a1a51d on a CrystalCoat car (all of this runs for every vehicle):

- the `vehicleVisualCustomization` lookup itself (`GetComponent<EffectSpawnerComponent>`),
- the `entVisualControllerComponent` mutation (`appearanceDependency`, `resourcePaths`),
- the raw `componentsStorage.components.PushBack` of our `PhysicalMeshComponent`s,
- `entSlotComponent.slots` / `slotIndexLookup` growth in `AddSlot` / `AddSlots`,
- the `VehicleDetachPart` filter, `GetComponentsUsingSlot`, colliders on activation.

So the disable removed the one write into CrystalCoat's own data, but the thruster meshes are
still injected into the entity behind the engine's back, and CrystalCoat still enumerates the
entity's visual components when a color is applied. Reports continuing into 2026 are
consistent with either old builds (0.3.15 and earlier still circulate in collections) or a
second path where the engine's recolor pass meets our half-registered mesh component.

## 2. Hand-written SDK layouts on this path

SDK fork: `deps/red4ext.sdk`, branch `new-types`, pinned at b5ea59195 (2.31 cherry-pick on top of
the 2.3 update). "Updated for 2.3" means touched by cc5f6b030 (2025-07-17) or later.

| Struct (file under `include/RED4ext/Scripting/Natives/`) | Asserted size / offsets used | Last change | 2.3+ | Used by |
|---|---|---|---|---|
| `vehicle::BaseObject` (`vehicleBaseObject.hpp`, hand-written) | 0xBA0; `physicsData` 0x2D0, `airControl` ~0x5E0, `meshParamsRegistry` 0x610, `weapons` 0xAF0, `componentsStorage` via `ent::Entity` | 101fd18cb 2025-07-17 | yes | `VehicleObject.cpp`, `VehicleDetachPart.cpp`, `FlightConfiguration.cpp` |
| `ent::Entity` (`entEntity.hpp`, hand-written) | `componentsStorage` at 0x70 (`ComponentsStorage` 0x40, `components` at +0x30), `currentAppearance` | 7bbfd467b 2025-07-10 | yes | everything (`GetComponent<T>` template lives here and dereferences every handle without a null check) |
| `ent::ComponentsStorage` (`entComponentsStorage.hpp`) | 0x40, `components` DynArray at 0x30 | 64e3a4c57 2025-07-16 | yes | raw `PushBack`/`EmplaceBack` in `AddComponent`, `Entity_InitializeComponents` |
| `ent::EffectSpawnerComponent` (`entEffectSpawnerComponent.hpp`, hand-written) | 0x230; `effectDescs` 0x140, `components` map 0x198 | 60f12bc08 2025-08-29 (the disable commit) | yes | lookup only now; the `#if 0` block wrote `effectDescs[i]->compiledEffectInfo` |
| `ent::EffectDesc` (Generated) | 0xC0; `compiledEffectInfo` 0x48 | 2b84fa2d6 2023-04-11 (1.62) | **no** | disabled block |
| `world::CompiledEffectInfo` / `CompiledEffectEventInfo` (Generated) | 0x68; `componentNames` DynArray at 0x10, `eventsSortedByRUID` at 0x50 | 64e3a4c57 2025-07-16 | yes | disabled block |
| `ent::VisualControllerComponent` (Generated, hand-edited) | 0x128; `appearanceDependency` DynArray 0xA8, `resourcePaths` DynArray 0xC8 | 2b84fa2d6 2023-04-11 (1.62) | **no** | `AddComponent` (still active, EmplaceBack/Emplace into both arrays) |
| `ent::VisualControllerDependency` (Generated) | 0x18 | 2023-04-11 (1.62) | **no** | `AddComponent` |
| `ent::SlotComponent` (Generated, hand-edited) | 0x1A0; `slots` DynArray 0x120, `slotIndexLookup` HashMap 0x150 | d7137688f 2025-07-02 | yes (pre-2.3 by two weeks, 2.3 commit did not touch it) | `AddSlot`, `AddSlots`, `AddColliders` |
| `ent::MeshComponent` (Generated, hand-edited) | 0x1E0; `mesh` 0x150, `meshAppearance` 0x190 | 8b963051b 2025-07-02 | same as above | `CreateMesh`, `AddComponent` |
| `ent::IComponent` (`entIComponent.hpp`) | 0x90; `name` 0x40, `entity` 0x50 | 438ed1476 2025-06-06 | pre-2.3 | everywhere |
| `Manager` in `Hooks/VehicleDetachPart.cpp` (plugin-local, hand-written) | `vehicle` 0x00, `parts` DynArray 0x80, `PartData` 0x250 with two 16-entry handle arrays | 6320167 2025-07-12 | pre-2.3 | `VehicleDetachPart` hook |
| `vehicle::PhysicsData` (`vehiclePhysicsData.hpp`) | 0x1E0 | 2c9eb3388 2025-06-09 | pre-2.3 | inertia / gravity getters |
| `vehicle::AirControl` (`vehicleAirControl.hpp`) | size assert commented out (0x290 vs 0x2A8 unresolved) | 2025-06 | pre-2.3 | `TurnOffAirControl` |
| `vehicle::Weapon` (`vehicleWeapon.hpp`) | 0x68 | 2025-06 | pre-2.3 | `GetWeapons`, `GetWeaponPlaceholderOrientation` |
| `physics::PhysicalSystemProxy` | `bodies` DynArray<void*> | 2023-12 | **no** | `AddColliders` / `RemoveColliders` (`bodies.entries[0]` with no size check) |

Note on `elementSize: 11`: no `DynArray` on this path has an 11-byte element (`CName` 8,
`ResourcePath` 8, `VisualControllerDependency` 0x18, `Slot` larger, handles 0x10). Together
with the nonsense `capacity: 3337213872` and offset `276192813200`, this says the engine read a
`DynArray` header that is not a header: either a struct offset drifted (a field that is not a
`DynArray` at that offset any more) or the object was already freed. The hand-written layouts
that were never regenerated after 1.62 and that we still write through are
`entVisualControllerComponent` (0xA8 / 0xC8) and `entEffectDesc` / `worldCompiledEffectInfo`
(the latter only in the now-disabled block). Both are prime suspects and neither can be
confirmed without an RTTI dump of 2.31 (`tools/` has no dumper; see CLAUDE.md "Reverse
engineering a new game binary").

Why only CrystalCoat cars: those are exactly the vehicles whose `.ent` carries the
`vehicleVisualCustomization` effect spawner and a `VehicleCustomMultilayer` record (the 2.3
SDK update touched `VehicleCustomMultilayer.hpp` and `VehicleCustomMultilayer_Record.hpp`).
Applying a color runs the `vvc_color` effect, which resolves component names to mesh
components on the entity and writes material parameters through the vehicle's
`meshParamsRegistry`. Our thruster `PhysicalMeshComponent`s are in `componentsStorage` but
were never registered with the engine (`ComponentsStorage::AddComponent` was bypassed, no
`VisualControllerComponent` cooked appearance, `visualScale` 0, toggled off), so the recolor
pass can meet a component in a state the engine never produces itself. The null read inside
`let_there_be_flight.dll` is most plausibly one of the unchecked dereferences below hit while
the engine rebuilds the entity for the new appearance (`ScheduleAppearanceChange` re-runs
`Entity::Initialize`, so our hook runs again on a vehicle that already has our components).

## 3. What this commit changes (defensive only)

No hook hashes changed. `tools/check_hashes.py` passes.

C++ (`src/red4ext`):

- `Utils/Utils.hpp`: `LTBF_WARN_ONCE(...)` (log once per site) and
  `Utils::LooksLikeDynArray(arr, maxCapacity)` (rejects `size > capacity`, absurd capacity,
  non-null capacity with null `entries`), used before every write through a hand-written
  `DynArray`.
- `Extensions/MeshComponent.hpp`: `EntityExt::FindComponent<T>(name)`, a null-safe
  replacement for the SDK's `Entity::GetComponent<T>` (skips null handles, validates the
  `componentsStorage` header, no change to the SDK submodule). `EntityExt::From(entity)`.
- `Extensions/MeshComponent.cpp`: `AddComponent` rejects null components and a corrupt
  `componentsStorage`; only touches `entVisualControllerComponent` arrays when they look
  sane; the CrystalCoat lookup is now diagnostic only (one warning naming the tweak flag) and
  the old descriptor patch is under `#if 0`. `AddSlot` validates `slots` before growing it.
- `FlightConfiguration.cpp`: `GetConfigurationClass` null-checks every component, uses
  `CClass::IsA` instead of walking `parent`, only casts `parentTransform` when it really is
  an `entHardTransformBinding`, and tolerates a null `CName::ToString()`. `AddSlots`,
  `AddColliders`, `RemoveColliders` check the slot component, the locked flight component,
  the physics proxy and `bodies.size` before indexing `bodies.entries[0]`.
- `Hooks/Entity_InitializeComponents.cpp`: null entity, corrupt `componentsStorage`, failed
  configuration instantiation, null thruster handles all fall through to the original.
- `Hooks/VehicleDetachPart.cpp`: bounds-checks `index` against `parts.size` (and the header),
  null `vehicle`, missing `AreThrustersDetachable`, null thrusters; on any doubt calls the
  original unfiltered.
- `Extensions/VehicleObject.cpp`: null `physicsData` / `airControl`, negative or out-of-range
  weapon index, corrupt `weapons` header, null components and non-hard-binding transforms in
  `GetComponentsUsingSlot` (also fixes the `"endHardTransformBinding"` typo that made the old
  class lookup a no-op).

Redscript (`src/redscript`):

- `Extensions/VehicleObject.reds`: new `VehicleObject.IsFlightDisabledByTweak()` reading
  `Vehicle.<record>.flightDisabled`; `CanEnterFlight()` returns false when set, which the
  C++ `Entity_InitializeComponents` hook already consults, so a blacklisted vehicle gets no
  flight component, thruster meshes, slots, colliders or HUD.
- `Extensions/DefaultTransition.reds`: `IsPlayerAllowedToEnterFlight` honors the flag and
  null-checks the owner.
- `Flight/FlightComponent.reds`: `OnVehicleOnPartDetached` returns early without a
  configuration.
- `Flight/Thruster/IFlightThruster.reds`, `Flight/FlightThrusterFX.reds`: null `meshComponent`
  / `instance` guards (no effect spawn without a mesh to attach to).

Tweaks (`src/tweaks/vehicleFlight.tweak`): documented `flightDisabled` with the reported
records as commented examples. Nothing is disabled by default. Users add their own
`r6/tweaks/ltbf_blacklist.yaml`:

```yaml
Vehicle.v_sport1_rayfield_caliburn_player:
  flightDisabled: true
```

or in native syntax `Vehicle.v_sport1_rayfield_caliburn_player { bool flightDisabled = true; }`.
The pre-existing `Vehicle.<record>.canEnterFlight = false` keeps working.

## 4. What to test in game (2.31, 0.3.18 + this change)

Watch `red4ext/logs/let_there_be_flight.log` for `[EntityExt]`, `[FlightConfiguration]`,
`[VehicleDetachPart]`, `[Entity_InitializeComponents]` warnings; each fires once per site and
tells which guard tripped (and prints the DynArray size/capacity it saw).

1. Baseline: Quadra Turbo-R (non-CrystalCoat). Enter, fly, land, exit. No warnings expected
   except possibly the CrystalCoat notice on cars that have the component.
2. Caliburn (`v_sport1_rayfield_caliburn_player`) with no CrystalCoat color: summon, enter,
   fly, exit. Then open CrystalCoat, apply a color, apply another, reset. Then re-enter.
3. Same Caliburn with a CrystalCoat color already applied from a save made without LTBF.
4. Johnny's Porsche (`v_sport2_porsche_911turbo_player`): load a save just before
   "You know my name" (or "Chippin' In" where the car is handed over), play through the
   handover, enter the car. Note the Porsche has a custom `thrusterMesh` /
   `thrusterAppearance` in `vehicles.tweak`.
5. Type-66 "Jen Rowley" / Aerondight: recolor while flight is active and while inactive.
6. Detach test: on a CrystalCoat car, crash into a wall until a bumper detaches; confirm no
   `[VehicleDetachPart]` warning and thrusters still detach or stay per the setting.
7. Blacklist: add `Vehicle.v_sport1_rayfield_caliburn_player { bool flightDisabled = true; }`
   to `r6/tweaks/ltbf_blacklist.tweak`, restart, summon the Caliburn: no flight HUD, L3 does
   nothing, no `[FlightComponent]` lines for it in the log, CrystalCoat recolor works.
   Remove the file, restart, flight is back.
8. If step 2 or 4 still crashes: the dump's faulting module tells the next move. If it is in
   `let_there_be_flight.dll`, the PDB from `game_dir_debug` gives the line; if it is the
   engine's `dynamicBuffer.cpp` assert, set `ignored = ["let_there_be_flight"]` in
   `red4ext/config.ini` to confirm it is us, then compare `entVisualControllerComponent`
   against a 2.31 RTTI dump (offsets 0xA8 / 0xC8 / size 0x128) and, if they moved, stop
   registering the thruster mesh with the visual controller for CrystalCoat cars.

## 5. Follow-ups not done here

- Verify `entVisualControllerComponent`, `entEffectDesc`, `worldCompiledEffectInfo` and
  `physicsPhysicalSystemProxy` against 2.31 RTTI (the four layouts on this path never
  regenerated after 1.62 / 2023).
- Consider a `thrusterMeshDisabled` flag (keep flight, skip the mesh injection) once the
  redscript thruster `Create` functions tolerate a null mesh; today every `Create` writes
  `meshComponent.name`, so it needs ten small edits in `Flight/Thruster/*.reds`.
- Use the engine's `ComponentsStorage::AddComponent` (the SDK already declares it, pattern
  only, needs a hash) instead of the raw `PushBack` so the engine sees our components the
  normal way.
- The SDK `Entity::GetComponent<T>` template should null-check handles; that is a submodule
  change and is left for the stage-2 SDK sync.
