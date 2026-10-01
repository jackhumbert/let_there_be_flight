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
| Flight Availability | All Vehicles | All Vehicles / Owned Vehicles / Player Vehicle Only / Whitelist Only | Which vehicles you are allowed to fly. **Owned Vehicles** means your summoned vehicle plus anything unlocked in your garage (it matches by record, so an identical traffic car counts too). **Player Vehicle Only** means just the vehicle you summon. **Whitelist Only** means the vehicle records you tag yourself with `canEnterFlight: true` in an `r6/tweaks/*.yaml`, see the readme's Configuration section. Every vehicle keeps its flight component in all four modes; only the flight toggle, its input hint and the flight quickhacks are gated. Applies live. | Set it to Owned Vehicles or Whitelist Only if you want flight to be a property of your own cars instead of every car in Night City, which is the most-requested LTBF feature. |
| Enable Auto Activation | Off | Bool | Automatically turns flight on when you get into a vehicle, if the vehicle is already above the height set below. | Turn on if you want flight to kick in automatically after a fall or a scripted drop, instead of pressing the flight toggle every time. |
| Auto Activation Height | 3.0 | 0 to 10, step 0.5 | In-game units above the ground that triggers the auto-activation above. | Raise it if flight activates too eagerly on normal ramps or curbs; lower it if it should catch you sooner after a jump. |

## Flight Control Settings

| Setting | Default | Range | What it does | When to change it |
|---|---|---|---|---|
| Linear Brake Factor | 1.0 | 0 to 5, step 0.1 | How much the linear brake button slows the vehicle's straight-line velocity. | Raise for a snappier stop; this is the "brake" input, not general drag (see Linear Damp Factor below). |
| Angular Brake Factor | 1.0 | 0 to 5, step 0.1 | How much the angular brake button slows the vehicle's rotation. | Raise if the vehicle keeps spinning after you let go of the brake button. |
| Invert Lift | Off | Bool | Flips the lift axis, so pulling the stick down (or pressing the lift-down key) climbs and pushing up descends. Applied where the controller reads the `Lift` action, so it covers the gamepad stick and the keyboard lift keys together. | Turn on if you fly with an inverted vertical axis. This replaces the `preset="invertAxis"` XML edit that used to be the only way. |
| Gamepad Toggle Requires Hold | Off | Bool | Makes the gamepad flight toggle need a hold (length set by Gamepad Toggle Hold Time) instead of a tap. The keyboard toggle stays a tap either way. | Turn on if L3 (or whatever you rebound it to) keeps activating flight by accident while you steer. |
| Gamepad Toggle Hold Time | 1.0 | 0.2 to 3.0 | Seconds the gamepad toggle has to be held when Gamepad Toggle Requires Hold is on. | Keep it above 0.5: vanilla autodrive is a 0.5 s hold on the same button (L3), so at 0.5 or below one hold toggles both |

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
| Yaw Damp Factor | 1.0 | 0 to 5, step 0.05 | Extra resistance on yaw alone. Angular damping is normally cut to zero while the stick is deflected and comes back when you let go; this multiplies just the yaw part of it, before Angular Damp Max clamps the result. 1.0 is the shipped behaviour. | Raise it if the vehicle keeps swinging round after you let go of yaw. Use this instead of Angular Damp Factor when pitch and roll already feel right. |
| Sway With Yaw | 0.0 | 0 to 2, step 0.05 | Feeds yaw input into the Yaw Directionality Factor term above, so while you are actually turning, more of your existing velocity gets redirected into the new heading. 0.0 is the shipped behaviour. | The second half of the "too slippery" fix: raise it (with Yaw Directionality Factor) if the car turns its nose but keeps drifting the old way. |
| Roll With Yaw | 0.15 | 0 to 1, step 0.01 | How far the vehicle banks into a turn. Applies to Hover & Fly, Fly, Hover and Automatic (everything that auto-levels to a target normal). | Raise for a more plane-like banked turn; set to 0 for a flat, helicopter-style turn. |
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
| Lift Factor | 25.0 | 0.5 to 50, step 0.5 | How fast the lift input raises and lowers the hover height Automatic mode holds. Automatic used to borrow Standard mode's Lift Factor for this; it now has its own with the same default, so lower it if climbing feels too fast. |
| Hover Factor | 40.0 | 5 to 100, step 1 | Gain on Automatic mode's whole lift force: how hard it pushes to reach and hold the target height. The minimum is above zero because at zero the vehicle gets no lift at all and simply falls. |

Raise Lift Factor if Automatic mode climbs and descends too slowly for you;
raise Hover Factor if it sags on the way up or feels mushy holding a height, and
lower it if it snaps to height too hard and bobs.

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

Mod Settings also exposes a "Key Bindings" category. It works by pointing named,
`overridableUI`-tagged bindings in the input XML at a different key. It covers
the keyboard bindings below plus the gamepad flight toggle; the gamepad sticks
and the other pad buttons are still XML-only (see "Known limits").

Clearing a binding (unbind it in the Mod Settings key-binding row) disables that
action, which is how you turn the gamepad flight toggle off entirely.

| Setting | Default key |
|---|---|
| Toggle Flight | O |
| Toggle Flight (Gamepad) | L3 (left stick click) |
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
| Toggle Flight | **L3 (left stick click)**, rebindable in Mod Settings > Key Bindings > Toggle Flight (Gamepad) |
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
plugin). Bindings exposed in Mod Settings' "Key Bindings" category can be changed
there without touching the file: every keyboard binding in the table above, plus
the gamepad flight toggle. The remaining gamepad buttons and both sticks still
require hand-editing the XML and reinstalling / restarting the game.

Two things that used to need an XML edit are now settings:

- **Inverting lift.** Use **Flight Control Settings > Invert Lift**. It flips the
  lift axis where the flight controller reads it, so gamepad and keyboard invert
  together. The old workaround, still valid if you would rather do it in the
  file, is `preset="invertAxis"` on the `Flight_Lift` mapping (aseyabal):

  ```xml
  <mapping name="Flight_Lift" type="Axis" preset="invertAxis">
    <button id="IK_Pad_LeftAxisY" />
    <button id="IK_Space" val="1.0" overridableUI="vehicleFlightLiftUp" />
    <button id="IK_LControl" val="-1.0" overridableUI="vehicleFlightLiftDown" />
  </mapping>
  ```

  `invertAxis` is a base-game input preset (defined in the game's own
  `inputUserMappings.xml`) that flips both the real and "fake" (keyboard-button)
  axis directions. The shipped XML does not use it.
- **Moving, disabling or hold-gating the L3 flight toggle.** Rebind or clear
  **Key Bindings > Toggle Flight (Gamepad)**, and/or turn on **Flight Control
  Settings > Gamepad Toggle Requires Hold** so a stray stick click cannot
  activate flight. L3 is also vanilla autodrive (hold 0.5 s to start or stop it,
  tap while autodriving to switch free roam / destination); flight cannot be
  activated while autodrive is driving, so that tap no longer toggles flight too.

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
- Flight Physics Settings > Yaw Damp Factor: 1.0 -> 2.0
- Flight Physics Settings > Sway With Yaw: 0.0 -> 0.5
- Standard (Non-Drone) Mode > Yaw Factor: 2.5 -> 4.0

### Gamepad, less floaty

Addresses "too slippery" and general looseness on stick input without making the
sticks feel stiff to hold.

- Flight Physics Settings > Angular Damp Factor: 2.0 -> 3.0
- Flight Physics Settings > Angular Damp Max: 3.5 -> 5.0
- Flight Physics Settings > Yaw Directionality Factor: 5.0 -> 15.0
- Flight Physics Settings > Yaw Damp Factor: 1.0 -> 1.5
- Flight Physics Settings > Sway With Yaw: 0.0 -> 0.5
- Standard (Non-Drone) Mode > Hover And Fly Height Dampening: 1.0 -> 2.0
- Drone Mode > Pitch Factor: 5.0 -> 4.0
- Drone Mode > Roll Factor: 8.0 -> 6.0

## Known limits

These are gaps between what users have asked for and what the settings system
exposes today.

- **R3 (right stick toggle) is still not rebindable.** The gamepad flight toggle
  now carries `overridableUI="vehicleFlightPad"` and shows up in Mod Settings'
  Key Bindings category, but `Flight_RightStickToggle` still has no
  `overridableUI` and no Mod Settings entry, so R3 can only be changed in the XML.
- **No gamepad stick rebinding is exposed.** Yaw/Lift (left stick) and
  Pitch/Roll (right stick) are hardcoded to `IK_Pad_LeftAxisX/Y` and
  `IK_Pad_RightAxisX/Y` in the input XML with no `overridableUI`, so they cannot
  be moved to other axes or triggers from the menu. Inverting *lift* no longer
  needs an XML edit (Flight Control Settings > Invert Lift); inverting pitch,
  roll or yaw still does.
- **Driving Direction Compensation Angle Smoothing / Speed Coef may be inert.**
  Both are declared as Mod Settings entries in `FlightSettings.reds`, but the only
  other reference to them, in `src/redscript/Extensions/vehicleTPPCameraComponent.reds`,
  is commented out. Whether they currently do anything to the TPP camera needs
  verification in code, not just documentation.
- **A number of physics-tuning values used every frame are still not exposed**
  and can only be changed by editing `FlightSettings.reds` and rebuilding:
  `hoverClamp` (10.0), `collisionRecoveryDelay` / `collisionRecoveryDuration`
  (0.8 / 0.8), `defaultHoverHeight` (3.5), `hoverModeMinHoverHeight` /
  `hoverModeMaxHoverHeight` (1.0 / 7.0), `brakeOffset` (0.0), `distanceEase`
  (0.1), `normalEase` (0.3), `lookAheadMin` / `lookAheadMax` (1.0 / 10.0),
  `fwtfCorrection` (0.0), `surgeOffset` (0.5), `pitchWithLift` (0.0),
  `pitchWithSurge` (0.0), `yawD` (3.0, and appears unused: its only read is
  commented out in `FlightMode_Standard.reds`), and the PID tuning vectors
  `inputPitchPID`, `inputRollPID`, `aeroYawPID`, `aeroPitchPID`, `hoverModePID`.
  A couple of these (`hoverClamp`, the PID vectors) are plausible contributors to
  the general "floaty" feedback and are worth considering next.
- **The new options are built but unverified in game.** Invert Lift, Yaw Damp
  Factor, Sway With Yaw, Roll With Yaw, the two Automatic Mode factors, the
  rebindable gamepad toggle and the hold option all compile and lint, but nobody
  has flown with them yet, and the ranges above are first guesses.
- **The new strings need a Wolvenkit repack.** Their `UI-Settings-*` entries are
  in `src/wolvenkit/source/raw/.../let_there_be_flight.en-us.json.json`, but the
  built `.archive` is committed separately; until it is re-imported and repacked,
  the new rows show raw keys in the Mod Settings menu. es-es has no entries for
  them at all.
