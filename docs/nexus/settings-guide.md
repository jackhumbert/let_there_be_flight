# Let There Be Flight: Settings Guide

Every flight setting a player can change, where to find it and what it does, then the controls. Numbers are the
shipped defaults. The settings were cut from 90 rows to 46 in the 2026-11 re-org (`docs/design/settings-audit.md`);
"What the re-org changed" at the end says what that means for values saved before it.

## Where the settings live

With [Mod Settings](https://github.com/jackhumbert/mod_settings) installed, open its menu from the pause screen and
pick **Let There Be Flight**. Each table below is one of its categories, in the menu's order. Without Mod Settings
every value is the default. Key bindings are in the menu too (the last category); the controls they move are listed
under "The controls".

## General

| Setting | Default | What it does |
|---|---|---|
| Flight Availability | All Vehicles | Which vehicles you can fly: All Vehicles, Owned Vehicles (your summoned vehicle and anything unlocked in your garage, matched by record, so an identical traffic car counts too), Player Vehicle Only (the vehicle you summon), Whitelist Only (the records you tag with `canEnterFlight: true` in an `r6/tweaks/*.yaml`, see the readme). Every vehicle keeps its flight component; only the toggle, its hint and the flight quickhacks are gated. Applies live. |
| Start Flying in Mid-Air | Off | Turns flight on by itself when you get into a vehicle more than 3 m above the ground. |
| Flight Quickhacks | On | Offers the flight quickhacks (Enable Flight, Antigrav, Launch and others) on other vehicles. |

## Controls

| Setting | Default | Range | What it does |
|---|---|---|---|
| Invert Lift | Off | | Flips the lift axis (pulling the stick down or pressing lift-down climbs), gamepad and keyboard together. |
| Handling | Default | Loose / Default / Tight | How flying feels. Loose: the car drifts and keeps turning longer (Standard coasts about 134 m from full speed and slips up to 12 deg through a turn; a Drone spin lasts longer). Tight: it stops sliding sooner and follows its nose (90 m, 6 deg; a Drone spin stops sooner). The top speed is the same in all three. |
| Brake Strength | 1.0 | 0.5 to 3 | How hard the brake slows the vehicle and stops it turning (both brakes). |
| Gamepad Flight Toggle Hold (s) | 0 | 0 to 3 | How long to hold the gamepad flight toggle; 0 is a tap. The keyboard toggle stays a tap. Keep it above 0.5 s or move the toggle off L3: vanilla autodrive is a 0.5 s hold on L3. |

## Flight Modes

The modes the mode switch (Next / Prev Mode) cycles, in this order: Standard, Hover & Fly, then Hover, Fly and Anti-Gravity Drone
with Extra Flight Modes on, then Drone and the Autopilot. A mode switched off is skipped.

| Setting | Default | Range | What it does |
|---|---|---|---|
| Standard | On | | The mode flight starts in (it was called Hover & Fly). Every control asks for a speed; let go and the vehicle comes to rest, level, where it is. Near the ground it hovers at a height you raise and lower with lift; higher up it holds its altitude. |
| Hover & Fly | On | | Flight as it was up to version 0.3.20, when Hover & Fly was the mode flight started in: the vehicle pitches and rolls with your input and keeps its momentum. Near the ground (up to 7 m) it hovers at a height you raise and lower with lift and tilts with the ground; higher up, lift pushes it up and down and it slows its climb or fall when you let go. |
| Standard Turn Speed (deg/s) | 90 | 30 to 180 | How fast the vehicle turns while you hold a turn key. Hover and Fly use it too. |
| Standard Climb and Descent Speed (m/s) | 8 | 2 to 20 | How fast it climbs or descends while you hold the lift keys. Hover and Fly use it too. |
| Drone Mode | On | | Point-and-fly: the right stick pitches and rolls the vehicle and thrust follows its attitude. |
| Drone Turn Speed | 1.0 | 0.25 to 3 | How fast the drone pitches, rolls and turns. |
| Drone Thrust | 1.0 | 0.25 to 3 | How hard the drone pushes up, forward and sideways. |
| Extra Flight Modes | Off | | Adds Hover (always holds its height over the ground, at any height: a vehicle above the hover band sinks into it), Fly (ignores the ground and holds its height) and Anti-Gravity Drone (Drone with gravity cancelled) to the mode switch. Takes effect for vehicles you get into after changing it. |
| Autopilot | On | | While flying, hold the autodrive key to fly to the tracked destination on its own; any flight input takes over for a moment. |
| Land at Destination | On | | The Autopilot touches down at a destination on the ground and switches flight off. Off: it hovers over the destination and hands the controls back. |
| Land on a Nearby Pad (m) | 0 | 0 to 500 | The Autopilot lands on an AV pad within this many metres of the destination instead of on the street. 0: off. |

## Crashes

| Setting | Default | Range | What it does |
|---|---|---|---|
| Crash Toughness | 0.5 | 0.05 to 1 | How hard a flying vehicle you are not in (a quickhacked one) can hit something before it explodes. At 1 it never explodes. |
| Thrusters Break Off in Crashes | On | | Lets the thrusters come off a vehicle in a hard crash. |

## Camera

| Setting | Default | Range | What it does |
|---|---|---|---|
| First-Person Camera Tilt | 0 | -60 to 60 deg | Tilt of the flight first-person camera. Up is positive. |
| Third-Person Camera Tilt | 20 | -60 to 60 deg | How far the chase camera looks down at the vehicle while flying. |

## Landing Pads and Summons

| Setting | Default | What it does |
|---|---|---|
| Show Landing Pads on the Map | On | AV pads as map pins. |
| Only Discovered Pads | On | Only the pads you have been near (within 150 m). |
| Fly Summoned Vehicles to Pads | On | A vehicle you summon near a pad flies there and lands. |
| Fly Summons That Cannot Drive | On | A summoned vehicle that cannot be driven to you (on a roof, off the roads, stuck) flies the rest of the way and lands next to you. |

## HUD and Sound

| Setting | Default | Range | What it does |
|---|---|---|---|
| Show Flight HUD | On | | The flight HUD while flying. Turn it off if it overlaps another HUD mod. |
| Engine Volume | 1.0 | 0 to 1 | The thrusters' engine sound. |
| Wind Volume | 0.6 | 0 to 1 | The wind. |
| Warning Volume | 0.5 | 0 to 1 | The warning beeps. |

## Key Bindings

Toggle Flight, Toggle Flight (Gamepad), Yaw Left and Right, Pitch Forward and Backward, Roll Left and Right, Lift Up
and Down, Sway Left and Right, Brake, Flight Options, Next Mode, Prev Mode, Toggle UI. Their defaults are under
"The controls" below.

## The controls

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
| Brake | C |
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
| Brake | Cross/A (face button) |

### Rebinding

Everything above is defined in
`src/input_loader/let_there_be_flight.xml` (installed as `inputs.xml` next to the
plugin). Bindings exposed in Mod Settings' "Key Bindings" category can be changed
there without touching the file: every keyboard binding in the table above, plus
the gamepad flight toggle. The remaining gamepad buttons and both sticks still
require hand-editing the XML and reinstalling / restarting the game.

Two things that used to need an XML edit are now settings:

- **Inverting lift.** Use **Controls > Invert Lift**. It flips the
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
  **Key Bindings > Toggle Flight (Gamepad)**, and/or set **Controls > Gamepad
  Flight Toggle Hold** above 0 so a stray stick click cannot activate flight. L3 is also vanilla autodrive (hold 0.5 s to start or stop it,
  tap while autodriving to switch free roam / destination); flight cannot be
  activated while autodrive is driving, so that tap no longer toggles flight too.
  With a hold set, keep the toggle off L3: when a GPS
  route is set, an L3 hold reaches vanilla's 0.5 s autodrive threshold before
  LTBF's hold time, autodrive starts, and flight is then refused. Tap mode (the
  default) is unaffected. Checked in game on 2.5 with a virtual pad (2026-10-25).

## What the re-org changed

Values are saved by the setting's internal name, so a setting that was only renamed or moved keeps its value. The
rest:

- **Back to the default for everyone:** the settings that left the menu (PhysX Flight Physics, Ground Friction, Auto
  Activation Height, Apply Flight Physics When Deactivated, the ten Flight Physics factors, the two Driving Direction
  Compensation values, Center Camera On Mass, Pad Summon Range, HUD attached to player, the Autopilot's Braking).
  Anyone who used the old "tighter" or "less floaty" presets, or switched PhysX Flight Physics off, flies the
  defaults again; Handling is the setting for that now.
- **New settings start at their default:** Brake Strength (1: the old brake defaults), Gamepad Flight Toggle Hold (0:
  a tap; set it again if you used the hold), Extra Flight Modes (off: turn it on if you used Hover or Fly), Drone Turn
  Speed and Drone Thrust (1: today's drone), the Brake key (C: rebind it if you had moved either brake), Handling
  (Default).
- **Hover & Fly, as it flew:** the mode that was Hover & Fly is Standard now, with a new feel; the old feel is back
  as its own mode, Hover & Fly, second in the list. Its old Height Dampening and Height Correction settings stay at
  their defaults (1 and 0.5) and are not in the menu.
- **Gone:** the Automatic mode, the Standard (Non-Drone) Mode and Automatic Mode factors, the Drone factors (now
  Drone Turn Speed and Drone Thrust), Top Speed and Cornering (they did nothing).
- **Kept:** Standard's switch, turn and climb speed (Hover & Fly's), everything else in the tables above.
- **Prev Mode** defaults to J, as the input file always bound it. If you had saved settings before, it may still be
  stored as no key: set it in Key Bindings.
- **For mod authors:** the first mode is `FlightModeStandard` (was `FlightModeHoverFlyAssist`) in
  `FlightModes.classes`; `FlightModeHover` and `FlightModeFly` are its C++ subclasses. The old Hover & Fly is the
  C++ `FlightModeHoverFlyClassic`, second. A mod that extended the old
  script `FlightModeStandard` base or `FlightModeAutomatic` no longer compiles.

## Known limits

- **R3 (right stick toggle) is not rebindable.** `Flight_RightStickToggle` has no `overridableUI`, so R3 can only be
  changed in the XML.
- **No gamepad stick rebinding.** The sticks are bound in the input XML with no `overridableUI`; inverting pitch,
  roll or yaw still needs an XML edit (lift has Invert Lift).
- **The menu's order** depends on Mod Settings: up to 0.2.28 it lists Flight Modes first and the rows inside a
  category in its own order; the next Mod Settings release lists them as the tables above have them.
- **Handling's Loose and Tight** were tuned in the offline simulator over every surveyed vehicle, not yet in game.
- **Spanish** has no text for the new settings yet.
