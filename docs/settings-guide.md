# Let There Be Flight: Settings Guide

This covers every user-facing flight setting and control, where to find it, what
it does, and the presets that today's Nexus feedback asks for. It is written from
the current `main` branch (0.3.17-era code, pre-0.3.18 SDK bump); numbers here are
the shipped defaults.

## Where the settings live

If you have [Mod Settings](https://github.com/jackhumbert/mod_settings) installed,
open its menu from the pause screen and pick **Let There Be Flight**. Every table
below is one of its categories, in the order they appear in the menu. Without Mod
Settings installed, every value below is just the compiled-in default; there is no
other in-game way to change them (see "Known limits").

Keybindings are a separate system, covered in "Controls" below.

## General Flight Settings

| Setting | Default | Range | What it does | When to change it |
|---|---|---|---|---|
| Enable Auto Activation | Off | Bool | Automatically turns flight on when you get into a vehicle, if the vehicle is already above the height set below. | Turn on if you want flight to kick in automatically after a fall or a scripted drop, instead of pressing the flight toggle every time. |
| Auto Activation Height | 3.0 | 0 to 10, step 0.5 | In-game units above the ground that triggers the auto-activation above. | Raise it if flight activates too eagerly on normal ramps or curbs; lower it if it should catch you sooner after a jump. |

## Flight Control Settings

| Setting | Default | Range | What it does | When to change it |
|---|---|---|---|---|
| Linear Brake Factor | 1.0 | 0 to 5, step 0.1 | How much the linear brake button slows the vehicle's straight-line velocity. | Raise for a snappier stop; this is the "brake" input, not general drag (see Linear Damp Factor below). |
| Angular Brake Factor | 1.0 | 0 to 5, step 0.1 | How much the angular brake button slows the vehicle's rotation. | Raise if the vehicle keeps spinning after you let go of the brake button. |

## Flight Physics Settings

| Setting | Default | Range | What it does | When to change it |
|---|---|---|---|---|
| Apply Flight Physics When Deactivated | Off | Bool | Keeps flight's force/torque model running for a moment after you turn flight off, instead of handing straight back to normal vehicle physics. | Turn on if you want to keep some control (or a graceful fall) right after deactivating mid-air, instead of the vehicle instantly reverting to ground physics. |
| Linear Damp Factor | 0.0 | 0 to 20, step 0.05 | Constant drag applied to all linear movement, independent of the brake button. | Raise this (not the brake factor) if the vehicle drifts and coasts too much and feels "floaty" in a straight line, at any speed. |
| Angular Damp Factor | 2.0 | 0 to 20, step 0.05 | Constant drag applied to all rotation, independent of the brake button. This is one of the two biggest levers on "twitchy vs sluggish" turning. | Raise if the vehicle over-rotates or oscillates when you let go of pitch/roll/yaw; lower if it feels sluggish to turn. |
| Angular Damp Max | 3.5 | 0 to 50, step 0.1 | Upper limit on the angular damping applied while flying. Higher values settle rotation faster (it is a clamp on the combined angular-damp vector, so it only matters once damping is already large). | Raise if the vehicle still tumbles after raising Angular Damp Factor; this is the ceiling on how hard damping can push back. |
| Pitch Aero Factor | 0.0 | 0 to 1, step 0.05 | How much the vehicle is rotated (pitch) towards its direction of travel, like an aircraft naturally pitching into its velocity vector. Off by default. | Raise for a more plane-like, self-leveling-into-travel feel; leave at 0 for a more arcade, nose-independent-of-motion feel. |
| Yaw Aero Factor | 0.0 | 0 to 1, step 0.05 | Same as Pitch Aero Factor, but for yaw: rotates the vehicle to face its direction of travel. Off by default. | Raise if you want the car to weathervane into the direction it is actually moving, e.g. after a sideways drift. |
| Pitch Directionality Factor | 15.0 | 0 to 100, step 1 | How much the vehicle's pitch angle redirects its existing velocity (i.e. how much pitching up bends your current momentum upward, rather than just changing where the nose points). | Raise for flight-sim-like momentum carry-through on pitch; lower for a more hover-car feel where pitch is mostly cosmetic. |
| Yaw Directionality Factor | 5.0 | 0 to 100, step 1 | Same idea as Pitch Directionality Factor, for yaw: how much the vehicle's yaw bends its existing velocity. | This is the "turning resistance" / "too slippery" knob multiple reporters asked about (Kaleb99, DenassaG). Raise it so turning the nose actually redirects your travel instead of sliding sideways. |
| Detachable Thrusters | On | Bool | Whether the visual thruster attachments can detach from the vehicle (e.g. under hard impact). Declared on `IFlightConfiguration`, so a per-vehicle config can still override it in code. | Turn off if you don't want thrusters ever popping off, regardless of impact. |
| Explosion Threshold | 0.5 | 0 to 100, step 0.05 | How hard a flying vehicle has to hit something before it explodes. Higher values take more punishment before exploding. | Raise if vehicles are exploding from minor bumps; lower for a more fragile, high-stakes flight feel. |

## Flight Camera Settings

| Setting | Default | Range | What it does | When to change it |
|---|---|---|---|---|
| Driving Direction Compensation Angle Smoothing | 120.0 | 0 to 180, step 1 | Declared for the TPP camera's driving-direction compensation. effect: see `src/redscript/Extensions/vehicleTPPCameraComponent.reds` (the native fields it would feed are commented out there, so this may not currently change camera behavior). | Not confirmed to do anything right now; see "Known limits" before spending time tuning it. |
| Driving Direction Compensation Speed Coef | 0.1 | 0 to 1, step 0.05 | Same caveat as above. effect: see `src/redscript/Extensions/vehicleTPPCameraComponent.reds`. | Same as above. |
| FPV Camera Pitch Offset | 0.0 | -60 to 60, step 5 | Angle in degrees added to the first-person camera's pitch while flying. | Adjust if the FPV view looks too level or too tilted for your taste. |
| TPP Camera Pitch Offset | 20.0 | -60 to 60, step 1 | Pitch offset in degrees applied to the third-person camera while flying. | Lower it for a flatter chase-cam angle; raise it to look further down from behind the vehicle. |
| Center Camera On Mass | On | Bool | Points the third-person camera at the vehicle's center of mass instead of its geometric origin. | Turn off if the TPP camera looks oddly offset on a particular vehicle. |

## Flight Mode Settings

Enables or disables whole flight modes. Disabled modes are skipped entirely when
cycling modes with Next Mode / Prev Mode.

| Setting | Default | What it does |
|---|---|---|
| Hover & Fly Enabled | On | The default mode: hovers near the ground and flies freely further up, blending between the two (see "Standard (Non-Drone) Mode" below). |
| Drone Mode Enabled | On | The other enabled-by-default mode: point-and-fly with the right stick for pitch/roll, independent of ground distance (see "Drone Mode" below). |
| Automatic Mode Enabled | Off | Ground-following hover that auto-brakes and manages height for you (see "Automatic Mode" below). |
| Fly Mode Enabled | Off | Pure free-flight variant of the standard-mode physics, without the hover-near-ground blending that Hover & Fly does. |
| Hover Mode Enabled | Off | Pure ground-hover variant of the standard-mode physics, holding a fixed height above ground. |
| Anti-Gravity Drone Mode Enabled | Off | Drone mode with gravity fully cancelled out (adds `9.81 * gravityFactor` of lift on top of Drone mode's normal forces). Equivalent to the "Enable Antigrav" quickhack. |

## Standard (Non-Drone) Mode

These apply to Fly, Hover, and Hover & Fly (all three extend the same
`FlightModeStandard` base and share these factors).

| Setting | Default | Range | What it does |
|---|---|---|---|
| Surge Factor | 15.0 | 0 to 200, step 1 | Forward/back thrust multiplier from the surge (accelerate/decelerate) input. |
| Yaw Factor | 2.5 | 0 to 50, step 0.5 | Turning-torque multiplier from the yaw input. |
| Sway Factor | 5.0 | 0 to 50, step 1 | Sideways (strafe) thrust multiplier from the sway input. |
| Pitch Factor | 3.0 | 0 to 20, step 0.5 | Multiplier on the pitch auto-level correction torque. |
| Roll Factor | 3.0 | 0 to 20, step 0.5 | Multiplier on the roll auto-level correction torque. |
| Pitch Input Angle | 45.0 | 0 to 90, step 5 | Maximum pitch angle, in degrees, that full pitch input asks the auto-level system to hold. |
| Roll Input Angle | 45.0 | 0 to 90, step 5 | Maximum roll angle, in degrees, that full roll input asks the auto-level system to hold. |
| Lift Factor | 25.0 | 1 to 500, step 1 | Vertical thrust multiplier from the lift input while in free-flight (used by Fly, and by Hover & Fly when it is in "flying" territory rather than hovering near ground). |
| Hover Factor | 10.0 | 1 to 500, step 1 | Gain on the ground-following hover correction (used by Hover, and by Hover & Fly when it is in "hovering" territory near the ground). |

Two more only apply to Hover & Fly specifically:

| Setting | Default | Range | What it does |
|---|---|---|---|
| Hover And Fly Height Dampening | 1.0 | 0 to 100, step 0.25 | Damps vertical velocity while blending between hover and free flight; higher values resist bobbing. |
| Hover And Fly Height Correction | 0.5 | 0 to 5, step 0.01 | How quickly the lift input changes the held hover height while near the ground. |

## Automatic Mode

| Setting | Default | Range | What it does |
|---|---|---|---|
| Auto Braking Factor | 3.0 | 0 to 10, step 0.1 | How much the vehicle brakes when you let off the gas. |
| Yaw Directionality | 300.0 | 0 to 1000, step 1 | How much your velocity follows your direction (Automatic mode's own, much larger-range version of the Yaw Directionality Factor concept above). |

Automatic Mode has no exposed lift factor. It hovers using the same internal
`hoverFactor` (40.0) that Fly/Hover pull `standardModeHoverFactor` from, but that
particular value is not wired to a `FlightModeAutomatic` field, so it cannot be
tuned from Mod Settings today (see "Known limits").

## Drone Mode

Applies to both Drone Mode and Anti-Gravity Drone Mode (the latter extends the
former and reuses all of these).

| Setting | Default | Range | What it does |
|---|---|---|---|
| Lift Factor | 40.0 | 0 to 200, step 0.5 | Vertical thrust multiplier from the lift input. |
| Pitch Factor | 5.0 | 0 to 200, step 0.1 | Pitch torque multiplier from the right stick / mouse pitch input. |
| Roll Factor | 8.0 | 0 to 200, step 0.1 | Roll torque multiplier from the right stick / mouse roll input. |
| Surge Factor | 15.0 | 0 to 200, step 0.5 | Forward/back thrust multiplier. |
| Yaw Factor | 5.0 | 0 to 200, step 0.1 | Yaw torque multiplier. |
| Sway Factor | 15.0 | 0 to 200, step 0.5 | Sideways thrust multiplier. |

## Flight UI Settings

| Setting | Default | What it does |
|---|---|---|
| Enabled | On | Turns the flight HUD (altitude, speed, mode readout, etc.) on or off entirely. Turn off if it conflicts with another HUD mod (Project E3 HUD has been reported to overlap). |

## Flight Audio Settings

| Setting | Default | Range | What it does |
|---|---|---|---|
| Engine Volume | 1.0 | 0 to 1, step 0.05 | Volume of the flight engine/thruster sound. |
| Wind Volume | 0.6 | 0 to 1, step 0.05 | Volume of the wind/airflow sound. |
| Warning Volume | 0.5 | 0 to 1, step 0.05 | Volume of the collision/imminent-explosion warning sound. |

## Flight Quickhacks

| Setting | Default | What it does |
|---|---|---|
| Enabled | On | Whether the flight-related quickhacks (Enable Flight, Disable Flight, Enable/Disable Antigrav, Initiate Launch, Funhouse) show up on other vehicles' quickhack menus. |

## Key Bindings (in Mod Settings)

Mod Settings also exposes a "Key Bindings" category, but it only covers the
**keyboard** side of the controls below; it works by pointing named,
`overridableUI`-tagged bindings in the input XML at a different key. There is no
equivalent for gamepad sticks or buttons (see "Known limits").

| Setting | Default key |
|---|---|
| Toggle Flight | O |
| Yaw Left / Yaw Right | A / D |
| Pitch Forward / Pitch Backward | L / . (period) |
| Roll Left / Roll Right | , (comma) / / (slash) |
| Lift Up / Lift Down | Space / Left Ctrl |
| Sway Left / Sway Right | Left arrow / Right arrow |
| Linear Brake | C |
| Angular Brake | C |
| Flight Options | R |
| Next Mode | G |
| Prev Mode | (unset by default in code; the shipped input XML binds it to J) |
| Toggle UI | Y |

## Controls

Full default bindings, from `src/input_loader/let_there_be_flight.xml`. These
apply while you are in a vehicle (the flight-specific ones only while flight is
active, except Toggle Flight itself, which works while driving normally).

### Keyboard and mouse

| Action | Key |
|---|---|
| Toggle Flight | O |
| Flight Options | R |
| Toggle Flight UI | Y |
| Next / Prev Mode | G / J |
| Yaw Left / Right | A / D |
| Pitch Forward / Backward | L / . |
| Roll Left / Right | / (slash, right) / , (comma, left) |
| Lift Up / Down | Space / Left Ctrl |
| Sway Left / Right | Left arrow / Right arrow |
| Surge (accelerate/decelerate) | shares the normal drive Acceleration/Deceleration axis |
| Linear / Angular Brake | C |
| Toggle vehicle camera | shares the normal vehicle camera toggle |

### Gamepad

| Action | Input |
|---|---|
| Toggle Flight | **L3 (left stick click)** |
| Right Stick Toggle | R3 (right stick click) |
| Flight Options | Square/X (face button) |
| Toggle Flight UI | D-pad Up |
| Next / Prev Mode | D-pad Right / D-pad Left |
| Yaw | Left stick X |
| Lift | Left stick Y |
| Pitch | Right stick Y |
| Roll | Right stick X |
| Linear / Angular Brake | Cross/A (face button) |

### Rebinding

Everything above is defined in
`src/input_loader/let_there_be_flight.xml` (installed as `inputs.xml` next to the
plugin). Keyboard bindings that are also exposed in Mod Settings' "Key Bindings"
category can be changed there without touching the file. Everything else,
including every gamepad stick and button, requires hand-editing the XML and
reinstalling / restarting the game.

The fix multiple Nexus users have found for inverting the lift axis (aseyabal,
and requested separately by others) is to add `preset="invertAxis"` to the
`Flight_Lift` mapping:

```xml
<mapping name="Flight_Lift" type="Axis" preset="invertAxis">
  <button id="IK_Pad_LeftAxisY" />
  <button id="IK_Space" val="1.0" overridableUI="vehicleFlightLiftUp" />
  <button id="IK_LControl" val="-1.0" overridableUI="vehicleFlightLiftDown" />
</mapping>
```

`invertAxis` is a base-game input preset (defined in the game's own
`inputUserMappings.xml`) that flips both the real and "fake" (keyboard-button)
axis directions, so this inverts lift for gamepad and keyboard/mouse together.

## Presets

These are starting points, not tuned/verified end states; treat them as a base to
nudge from, not a final answer. Every value below is a Mod Settings entry from the
tables above.

### Keyboard and mouse, tighter

Addresses the "floaty on keyboard and mouse" and "too slippery when turning"
reports.

- Flight Physics Settings > Linear Damp Factor: 0.0 -> 1.0
- Flight Physics Settings > Angular Damp Factor: 2.0 -> 4.0
- Flight Physics Settings > Angular Damp Max: 3.5 -> 6.0
- Flight Physics Settings > Yaw Directionality Factor: 5.0 -> 25.0
- Flight Physics Settings > Pitch Directionality Factor: 15.0 -> 30.0
- Flight Control Settings > Linear Brake Factor: 1.0 -> 2.0
- Flight Control Settings > Angular Brake Factor: 1.0 -> 2.0
- Standard (Non-Drone) Mode > Yaw Factor: 2.5 -> 4.0

### Gamepad, less floaty

Addresses "too slippery" and general looseness on stick input without making the
sticks feel stiff to hold.

- Flight Physics Settings > Angular Damp Factor: 2.0 -> 3.0
- Flight Physics Settings > Angular Damp Max: 3.5 -> 5.0
- Flight Physics Settings > Yaw Directionality Factor: 5.0 -> 15.0
- Standard (Non-Drone) Mode > Hover And Fly Height Dampening: 1.0 -> 2.0
- Drone Mode > Pitch Factor: 5.0 -> 4.0
- Drone Mode > Roll Factor: 8.0 -> 6.0

## Known limits

These are gaps between what users have asked for and what the settings system
exposes today; the code half of LTBF-5 should pick these up.

- **L3 (gamepad flight toggle) is not rebindable or disableable in-game.** The
  keyboard Toggle Flight binding (O) is tied to an `overridableUI="vehicleFlight"`
  attribute that Mod Settings' Key Bindings category can repoint, but the gamepad
  `IK_Pad_LeftThumb` binding on the same `Flight_Toggle` mapping in
  `src/input_loader/let_there_be_flight.xml` has no `overridableUI` attribute, so
  nothing in `FlightController.reds` can change it. Same story for R3
  (`Flight_RightStickToggle`): no `overridableUI`, no Mod Settings entry.
- **No gamepad stick rebinding or inversion is exposed.** Yaw/Lift (left stick)
  and Pitch/Roll (right stick) are hardcoded to `IK_Pad_LeftAxisX/Y` and
  `IK_Pad_RightAxisX/Y` in the input XML with no `overridableUI`. Inverting lift
  today means hand-editing the XML with `preset="invertAxis"` (see "Controls"
  above); there is no in-game toggle for it, matching the author's 2025-07-14
  promise ("I'll have this exposed as an option in the next update") that has not
  shipped yet.
- **Automatic Mode has no exposed lift factor.** It reads an internal
  `hoverFactor` value (`FlightSettings.SetFloat("hoverFactor", 40.0)` in
  `src/redscript/Flight/FlightSettings.reds`) that has no `@runtimeProperty` and
  no Mod Settings entry, unlike the near-identical Hover Factor exposed for
  Standard mode. This is the "Automatic lift factor" the author said would be
  exposed.
- **Driving Direction Compensation Angle Smoothing / Speed Coef may be inert.**
  Both are declared as Mod Settings entries in `FlightSettings.reds`, but the only
  other reference to them, in `src/redscript/Extensions/vehicleTPPCameraComponent.reds`,
  is commented out. Whether they currently do anything to the TPP camera needs
  verification in code, not just documentation.
- **A number of physics-tuning values used every frame are not exposed at all**
  and can currently only be changed by editing `FlightSettings.reds` and
  rebuilding: `liftFactor` (8.0, an unused-looking leftover distinct from the
  per-mode Lift Factors above), `hoverFactor` (40.0, see Automatic Mode above),
  `hoverClamp` (10.0), `collisionRecoveryDelay` / `collisionRecoveryDuration`
  (0.8 / 0.8), `defaultHoverHeight` (3.5), `hoverModeMinHoverHeight` /
  `hoverModeMaxHoverHeight` (1.0 / 7.0), `brakeOffset` (0.0), `distanceEase`
  (0.1), `normalEase` (0.3), `lookAheadMin` / `lookAheadMax` (1.0 / 10.0),
  `fwtfCorrection` (0.0), `surgeOffset` (0.5), `swayWithYaw` (0.5), `rollWithYaw`
  (0.15), `pitchWithLift` (0.0), `pitchWithSurge` (0.0), `yawD` (3.0, and appears
  unused: its only read is commented out in `FlightMode_Standard.reds`), and the
  PID tuning vectors `inputPitchPID`, `inputRollPID`, `aeroYawPID`,
  `aeroPitchPID`, `hoverModePID`. This list is what would need
  `@runtimeProperty` tags (and probably category/range choices) to become real
  Mod Settings entries; it is not part of the "turning resistance" or "invert
  lift" asks specifically, but a couple of these (`hoverClamp`, the PID vectors)
  are plausible contributors to the general "floaty" feedback and are worth
  considering for exposure alongside the two items above.
- **No settings guide existed before this file.** Eight Nexus reporters asked for
  one or for keyboard-and-mouse presets; this document and the two presets above
  are the first pass at that.
