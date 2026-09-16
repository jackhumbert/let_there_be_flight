# Destroyed vehicle wreck falls through the map (LTBF-14, GitHub #89)

Analysis 2026-09-15 (read-only, from the code and `git show v0.3.5`). Report: JayeConcord, 2025-06-19, v0.3.5 (first seen v0.3.4): "As soon as you destroy a vehicle on the road, the burnt shell falls through the map as if collisions were removed. All the pieces, such as the bumpers and doors, will collide with the ground and remain there as normal." Plain traffic cars the player never flew, so flight state is not the trigger.

## Mechanism

Every vehicle gets a FlightComponent and configuration, not just the player's (`CanEnterFlight()` defaults to true, `src/redscript/Extensions/VehicleObject.reds`; `Entity_InitializeComponents.cpp` attaches the component, configuration, four thruster `PhysicalMeshComponent`s and the HUD).

LTBF wraps the explode path for all vehicles (`src/redscript/Extensions/VehicleComponent.reds`):

```reds
@wrapMethod(VehicleComponent)
private final func ExplodeVehicle(instigator: wref<GameObject>) -> Void {
  wrappedMethod(instigator);
  let flight = this.GetVehicle().GetFlightComponent();
  if IsDefined(flight) { flight.HandleExplosion(); }
}
```

`HandleExplosion` calls `Deactivate(true)` with no `this.active` check, and `Deactivate` ends in `configuration.OnDeactivationCore()`, which calls `IFlightConfiguration::RemoveColliders()`.

At v0.3.5 `originalShapeCount` was uninitialized (zero) and `RemoveColliders` was unguarded, so on every exploding vehicle the chassis `PxRigidDynamic` had all of its shapes detached from index 0. The body keeps simulating with no collision and sinks; detached parts are separate actors with their own proxies, which is why they still land. `detachShape`/`getShapes` only appear in `src/red4ext/FlightConfiguration.cpp`, nothing in the archive or tweaks touches base vehicle physics. Confidence high for the historic cause.

## Current code: partly fixed, holes left

`int32_t originalShapeCount = -1` (7af9e6a, 2025-07-10) and the early return `if (originalShapeCount == -1) return;` in `RemoveColliders` (54b4c27, 2025-07-12, "prevent remove collider crash") cover the never-armed case. Remaining:

1. The sentinel gets armed on vehicles that never fly. `FlightComponent.reds` `OnVehicleOnPartDetached`: when a detached part is one of our thrusters it calls `RemoveColliders()` then `AddColliders()` with no `this.active` check, and `AddColliders` records `originalShapeCount = body->getNbShapes()`. Since 1859d17/2f9747e (July 2025) `src/tweaks/vehicles.yaml` appends ThrusterFL/FR/BL/BR to `VehicleDestructionParamsDefault_4w` and friends (threshold 0.999, bikes 0.5), so thrusters detach on every car that is destroyed, including traffic. That arms the count mid-destruction and the explosion's `RemoveColliders` detaches everything from that index.
2. Index-range bookkeeping is not safe across destruction. `RemoveColliders` assumes our spheres are still the tail of the shape array; PhysX compacts on removal and the destruction path removes chassis shapes and can add wreck shapes. `originalShapeCount` is never reset to -1 after removal, and `OnUpdate` calls `Deactivate` every frame while a destroyed vehicle still has `hasUpdate`, so any shape the game attaches later is stripped on the next frame. Likely the current-day variant of #89 and a candidate for LTBF-9 "undrivable after landing" (#101).
3. `-1` only holds if the C++ ctor runs for the redscript-derived configuration classes (`CreateInstance(true)` + `ConstructCls` chain). Evidence says yes (red_lib forwards `ConstructCls` to the parent), but `Setup()` does not set it.

| Path | Arms `originalShapeCount` | Runs `RemoveColliders` |
|---|---|---|
| Destroyed while flying | yes (Activate) | `OnDeath`, `HandleExplosion`, `OnUpdate` every frame; never reset |
| Destroyed after landing | yes (first activation, never cleared) | `HandleExplosion` |
| Never flown, thruster part detached | yes (`OnVehicleOnPartDetached`) | `HandleExplosion` |
| Never flown, no thruster detach | no | early-returns today; stripped everything before 0.3.10 |
| NPC vehicle | as above, the wrap is unconditional | as above |
| Player exits mid-air | `Deactivate` via controller | yes; chassis shapes assumed intact |

## Fix

Minimal (redscript plus one C++ line):

- `OnVehicleOnPartDetached`: only call `RemoveColliders`/`AddColliders` when `this.active`. Keep the `thruster.Detach()` loop unconditional (FX/audio only, keeps `thrusterTensor` correct).
- `Deactivate`: early return when `!this.active`. This skips the destroyed-vehicle audio stop and `sys.ctlr.Deactivate` on a non-active component; neither matters when flight was never on, but check the player's own car exploding mid-flight.
- `HandleExplosion`: set the flags, then `Deactivate(true)` only if active.
- `IFlightConfiguration::Setup()`: set `originalShapeCount = -1` so the guard does not depend on the C++ ctor.

Real fix (after): stop using an index range. Record the `PxShape*` we attach in `AddColliders` in a member array; in `RemoveColliders` re-query `getShapes`, detach only shapes whose pointer is in that set, clear the set, reset the count. Intersect against the live shape list, since we `release2()` our shapes after attaching and stored pointers can dangle if the proxy is rebuilt. Keep the `proxyHelper.mutex` lock and `UpdateProxyCache()` structure; detaching a shape the proxy already dropped is the crash 54b4c27 was aimed at. Do not remove the `ExplodeVehicle` wrap: `HandleExplosion` also stops the destroyed-vehicle alarm loop for the player's car.

## Repro

1. Current LTBF on 2.31 with current RED4ext/ArchiveXL/TweakXL.
2. Pick a parked or traffic car never entered (Cortes, Galena), stand on the sidewalk.
3. Make it explode (two frag grenades under it, or shoot until it burns and blows). The symptom is tied to `ExplodeVehicle`, not first damage.
4. The shell should sink through the road while a bumper or door stays on the asphalt.
5. Confirm the mod: `red4ext/config.ini` `ignored = ["let_there_be_flight"]`, relaunch, repeat.
6. Which hole is live: repeat with Mod Settings, Flight Physics Settings, Detachable Thrusters off (`VehicleDetachPart.cpp` then blocks thruster detaches, so `AddColliders` is never armed). Wreck survives off and falls on means hole 1. `[FlightComponent] Part detached: ThrusterFL` in `red4ext/logs/let_there_be_flight.log` on a car you never drove proves the arming path fires on traffic.
7. Bike variant arms faster: thruster threshold 0.5 on 2-wheelers, so a Kusanagi sheds thrusters on moderate damage.
