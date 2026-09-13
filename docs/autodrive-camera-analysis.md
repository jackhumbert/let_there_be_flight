# Autodrive / cinematic camera broken since 2.3 (LTBF-7, GitHub #93)

Fixed 2026-09-13 in `src/redscript/Extensions/VehicleEventsTransition.reds`. Needs an
in-game check (see the end).

## Root cause

Patch 2.3 changed `VehicleEventsTransition.HandleCameraInput` (vanilla, from
`tools/redmod/scripts/cyberpunk/player/psm/vehicleTransition.script`, 2.3x):

```
if scriptInterface.IsActionJustReleased('ToggleVehCamera') && !IsVehicleCameraChangeBlocked(...) {
  RequestToggleVehicleCamera(scriptInterface);
}
if scriptInterface.IsActionJustHeld('HoldCinematicCamera') && !IsVehicleCameraChangeBlocked(...) {
  RequestVehicleCinematicCamera(scriptInterface);   // queues vehicleCinematicCameraToggleEvent
}
```

`ToggleVehCamera` and `HoldCinematicCamera` are both mapped to `VehicleCameraToggle`
(`r6/config/inputContexts.xml` lines 469 to 515), so a tap cycles presets on release and a
hold toggles the cinematic camera. LTBF shipped an `@replaceMethod` of this function with
the pre-2.3 body: `IsActionJustPressed('ToggleVehCamera')` and no hold branch. With LTBF
installed a press therefore cycled the preset immediately and a hold did nothing, exactly
the "just circling through default camera presets" symptom.

The native camera hooks are not involved:

- `UpdateVehicleCameraInput` (hash 501486464, `sub_1402E6010`, `vehicle::BaseObject::UpdateVehicleCameraInput`)
  only samples input actions into the vehicle input block: CameraX/CameraY (+644/+648),
  CameraMouseX/Y (+652/+656), VehicleInverseCameraToggle (+667/+668), VehicleCameraReset
  (+669), RangedAttack (+670), CameraAim (+671), VisionHold (+672), VisionToggle (+673,
  toggled on press). Action names recovered by matching the FNV1a64 hashes in the
  pseudocode against every `name=` in `inputContexts.xml` / `inputUserMappings.xml`.
  LTBF zeroes `vehicleCameraReset` (+669) while `FlightController.enabled`; nothing
  cinematic goes through here.
- `TPPCameraStats_Update` (4125170300, `sub_142049284`, `TPPCameraComponent::SetVehicleData`),
  `GetLocationFromOffset` (283779224, `sub_14070C4A8`), `GetYaw` (4124321726, `sub_14070AE10`),
  `UpdatePitch` (2719293980, `sub_14070CA10`) and `FPPCameraUpdate` (2531201123, `sub_1404E1638`)
  all pass through to the original unless the flight component / controller is `active`.
  `GetLocationFromOffset` additionally keeps a 500 ms position blend across its own
  mode switches, which is a no-op while flight is inactive.

## Fix

`HandleCameraInput` is now an `@wrapMethod` that calls the vanilla body unless
`FlightController.GetInstance().IsActive()`. While flight is active the `VehicleFlight`
state machine handles `ToggleVehCamera` itself (`HandleFlightCameraInput`, tap only), so the
vanilla toggle stays suppressed there; outside of flight the 2.3 release/hold logic runs
untouched. The wrap also composes with other mods touching the same method (Dark Future,
see LTBF-11) instead of conflicting with them.

## In-game check

1. Vanilla car, flight off: tap the camera key -> presets cycle on release (not on press);
   hold it -> cinematic camera toggles on and off. Repeat with autodrive engaged.
2. Activate flight: tap cycles FPP/custom FPV/TPP as before; hold does nothing (expected).
3. Deactivate flight and repeat step 1.
