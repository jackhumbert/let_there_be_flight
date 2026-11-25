# Flight modes

How a flying car is steered: the flight modes, the ground they read, the player's few settings and the values only
a developer changes. The modes' maths is pure C++ (`src/red4ext/Flight/Modes.hpp`), so the game and the offline
simulator (`tools/flightsim`, `docs/design/flightsim.md`) run the same code. Only the gathering of inputs differs.

## The mode list

`FlightComponent` builds each car's modes from TweakDB's `FlightModes.classes` (`src/tweaks/flight_modes.yaml`), in
the order the controller cycles them. The first enabled one is the mode flight starts in:

| Class | Name in the HUD | Code | In the list when |
|---|---|---|---|
| `FlightModeStandard` | Standard | C++ | its switch, Standard (on) |
| `FlightModeHoverFlyClassic` | Hover & Fly | C++ | its switch, Hover & Fly (on) |
| `FlightModeHover` | Hover | C++ | Extra Flight Modes (off) |
| `FlightModeFly` | Fly | C++ | Extra Flight Modes |
| `FlightModeDroneAntiGravity` | Anti-Gravity Drone | redscript class, C++ maths | Extra Flight Modes |
| `FlightModeDrone` | Drone | redscript class, C++ maths | its switch, Drone Mode Enabled (on) |
| `FlightModeAutopilot` | Autopilot | C++ | its switch, Autopilot (on) |

`FlightMode.Instantiate` makes each class by name in C++, the way the flight configurations are made, so a
redscript class gets its own overrides and fields. The script keeps an instance when its `IsEnabled` says so.

A mod adds a mode, or swaps one for its subclass, with its own yaml in `r6/tweaks`. TweakXL reads `r6/tweaks`
before the tweaks LTBF registers, so the mod gives the flat's type and puts its change under `$value`
(`- !append-once MyMode`). The yaml's header has both forms.

## FlightMode, the native base

`FlightMode` is a native class (`Flight/Mode.hpp`), the shape of CDPR's scriptable systems and of LTBF's flight
configurations. Its `force` and `torque` (car frame) are native fields. Its redscript `Initialize`, `IsEnabled`,
`Activate`, `Deactivate`, `Teleported`, `Update` and `GetDescription` call final native entry points
(`NativeUpdate` and the rest). Each runs the C++ mode's virtual step (`OnUpdateNative` and so on), then the
redscript hook (`OnUpdate`, `OnActivate`, ...) when the object's class overrides it. `Enabled` and `Description`
hooks replace the C++ answer. Hooks are found by name once per class.

Why: redscript calling a function by name on a native object does not reach a C++ override of it. With final entry
points no redscript call has to. A mod extends a C++ mode by subclassing it in redscript and overriding the hooks.
Redscript modes that override `Update` and the rest themselves (Drone) work as before.

`FlightModeNativeProbe` is a test mode: it counts its calls and gives only a small sideways marker force. It joins
the list only with the developer setting `nativeModeProbe`.

## Standard

`FlightModeStandard` (`Flight/FlightModeStandard.hpp`) over `Flight::Standard` (`Modes.hpp`). Every control asks for
a speed; let go of everything and the car comes to rest, level, where it was.

- **Attitude:** a PD on the rotation from the body's up to a target up. The target is world up blended towards the
  ground's normal by the hover weight, tilted by the pitch and roll inputs. It holds at any tilt, upside down too,
  and has no integral to wind up while a collision's recovery scales the torque down.
- **Yaw:** a turn rate about world up (input x Standard Turn Speed), not a torque about the body's own up. At speed
  it turns no faster than the sideways grip can turn the velocity with it.
- **Travel:** surge and sway accelerate along the level forward and right. Let go, the speed along each brakes to
  0, the sideways part harder, so the velocity follows the nose through a turn.
- **Height:** lift asks for a vertical speed (input x Standard Climb and Descent Speed). Let go, the car stops and
  holds that height (`referenceZ`, taken once it has stopped climbing or sinking).
- **Hover band:** between `hoverModeMinHoverHeight` and `hoverModeMaxHoverHeight` (1 and 7 m) the car holds a hover
  height over the ground, weighted 1 at the bottom of the band and 0 at the top. Lift moves the hover height at the
  same rate. The band's spring is damped on the car's vertical speed less the ground's own vertical speed under it
  (`Flight::GroundRate`: from FindGround's normal and the horizontal velocity, smoothed over `groundRateTime`;
  faces steeper than 30 degrees count as none). So the car does not settle under its hover height while climbing a
  slope, and a step under the car does not kick it. After `kRegainHoverHeight` (0.5 s) out of the band the hover
  height is taken from the ground again on entering it, so a moment without ground does not lose it.
- **Activate:** holds the height the car is at, and a hover height of the ground's distance, kept within the band.

The attitude and yaw terms ask for an angular acceleration and divide it by the car's own response
(`Flight::TorqueResponse`): `kEngineTorqueGain` x mass x thruster tensor / inertia, per body axis. Over the surveyed
cars that response spans about 30 times on each axis, so no fixed torque gain suits them all.

The gains are `Flight::StandardGains`, the shared factors `Flight::StandardParams`. Surge and sway are scaled by the
car's thrust (`FlightModeControl::Thrust`), the linear brake by Brake Strength, and the braking and side grip by
Handling.

## Hover and Fly

C++ subclasses of Standard that pick its hover band (`Flight::HoverBand`, `Band()`):

- **Hover** (`Always`): follows the ground wherever the rays find it, at any height, so a car above the band sinks
  into it.
- **Fly** (`Never`): ignores the ground and holds its height wherever lift leaves it.

Both join the list with Extra Flight Modes.

## Hover & Fly

`FlightModeHoverFlyClassic` over `Flight::HoverFlyClassic`: Hover & Fly as releases up to 0.3.20 flew it, step for
step. It is the old standard modes' attitude PIDs and lift (`Flight::AttitudeLift`) under a hover band. The car
pitches and rolls with the input and keeps its momentum. Near the ground the hover PID holds the hover height and
the attitude follows the ground's normal; higher up lift pushes straight, damped by the vertical speed, and the car
levels. Holding lift moves the hover height by `heightCorrection` each frame, per frame as the old script did.

It reads the component's pitch, roll and hover PIDs, the mode's `gravityFactor`, the standard factors at their code
defaults, the player's Handling (the roll with yaw it sets) and Brake Strength.

Its class name is new. The old script class's name, `FlightModeHoverFly`, keys Standard's switch in Mod Settings'
`user.ini`, so the value players saved there carries over to Standard (see Settings).

## Drone

`FlightModeDrone` is a redscript class; its maths is C++ (`FlightModeControl.Drone`, `Flight::Drone`). Its force is
an acceleration (the force tail scales it by mass), so lift and surge are the same on every car but for air drag. Its
torque is input x factor, so a held key spins each car at its own response until PhysX's 7 rad/s cap.

Near the ground a skid (`Flight::DroneSkidPoints`) holds each thruster a few centimetres off the ground from its own
ray, so a car let down onto a road does not slide on it or get thrown at flush mesh joints. Lift down near the
ground fades to a gentle press (`Flight::GroundedLift`), so a car set down slides along.

`FlightModeDroneAntiGravity` is Drone with gravity cancelled (`d_localUp` x 9.81 x `gravityFactor`).

Drone Turn Speed and Drone Thrust multiply its six factors (`droneMode*Factor`, developer values).

`Flight::DroneAssist` (`Modes.hpp`) is a proposal the game does not call: a rate command per axis divided by the
car's response, as Standard does. The simulator flies it (`--mode droneassist`).

## Autopilot

`FlightModeAutopilot` is a native mode around `Navigation::Autopilot`, under the class name the script mode had. A
frame the player has taken over is flown with `Flight::AttitudeLift`. It uses the code's own standard factors, not
any player setting, for its thrust limits. `docs/design/autopilot.md` describes it.

## FindGround

The ground under the car (`Flight/FlightGround.hpp`, `FlightGroundNative::Find`), read by every C++ mode:

- four rays, one per wheel emitter (`fl_tire`, `fr_tire`, `bl_tire`, `br_tire`), each straight down to
  `maxHoverHeight + 10` m under the emitter (`Flight::GroundRayEnd`);
- each starts `kGroundRayRise` (0.5 m) over its emitter (`Flight::GroundRayStart`). A bike's emitters sit at the
  bottom of its tyres, under the road collider's surface, and rays cast from there found no ground. Started higher
  they would hit the car itself, so the native rays see only the static world;
- cast through `SyncRaycastByQueryFilter` with the `VehicleBlocker` and `Water` groups, the engine call the script
  makes;
- averaged by `Flight::GroundFromHits`: found only when every ray hits; the distance is the mean of the emitters'
  heights over their hits, the normal the normalised mean of the hit normals. When not found the component's
  `distance` keeps its last value.

The redscript `FlightComponent.FindGround` keeps its own four rays from the emitters, for auto activation.
`FlightGroundNative::Touches` turns the same four hits into each thruster's clearance and slide, for the Drone skid
and the scrape sounds.

## Handling

One choice, Loose / Default / Tight (`Flight/Handling.hpp`, `FlightSettings.handling`), in place of the ten
handling values that used to be Mod Settings. It is applied where the values are read (the force tail, Standard,
Hover, Fly, Hover & Fly, the Autopilot), never written into the fields, so the developer values stay what they were
set to. The modes feel through different values, so each choice has two parts:

- **The force tail's damping and directionality** (`Flight::HandlingFactors`): the Drone modes' feel, how long a
  spin or a slide lasts. Default is `FlightSettings`' fields as they are. Tight is the settings guide's "Gamepad,
  less floaty" preset; Loose goes the other way. On Standard they change little, since its own controller sets the
  turn rate and the slide.
- **Standard's braking and side grip** (`StandardGains` `brake`, `sideBrake`, `maxBrake`): how far it coasts and
  how much it slips in a turn. Loose brakes softer, Tight harder; the top speed is the same.

The brake at high speed (`ForceParams` `brakeKneeSpeed` 50 m/s, `brakeHighSpeedRate` 1.5 /s, in every mode's
force tail): the brakes alone take the speed off exponentially, so the distance to stop grows with the speed, about
2 km from 450 m/s. Past the knee each m/s over it is braked at the rate too, so a fast car sheds the extra speed in a
second or two; under the knee nothing changes (`force_tail_high_speed_brake`).

All three keep Standard's feel spec (`feel_standard_fleet`, `feel_handling_loose`, `feel_handling_tight`). The
guide's keyboard preset is not used: its linear damping held Standard's top speed to a fraction of the default.

## Settings

The player sees few settings, in plain words, each one a thing a player can judge by feel. Engine terms (damping,
directionality, PID gains) are developer values. Why so few: the old ninety rows were engine terms that interacted
and applied to every mode at once, and a player could not tell what to set. A Mod Setting also pins its value: on
Accept, Mod Settings writes every registered field to `user.ini`, so a better default in a later release never
reaches a player who once pressed Accept, unless the property is renamed.

The menu, by category:

- **General:** Flight Availability, Start Flying in Mid-Air, Flight Quickhacks.
- **Controls:** Invert Lift, Handling, Brake Strength (scales both brakes), Gamepad Flight Toggle Hold (0: a tap).
- **Flight Modes:** Standard, Standard Turn Speed (90 deg/s, 30 to 180), Standard Climb and Descent Speed (8 m/s,
  2 to 20), Hover & Fly, Drone Mode Enabled, Drone Turn Speed, Drone Thrust, Extra Flight Modes, Autopilot, Land at
  Destination, Land on a Nearby Pad.
- **Crashes:** Crash Toughness (`explosionThreshold`, 0.05 to 1: a flying vehicle V is not in explodes when a hit
  changes a damage grid cell by more than it; a cell's change is 0 to 1, so 1 is never), Thrusters Break Off in
  Crashes.
- **Camera:** the first-person and third-person camera tilts.
- **Landing Pads and Summons:** Show Landing Pads on the Map, Only Discovered Pads, Fly Summoned Vehicles to Pads,
  Fly Summons That Cannot Drive.
- **HUD and Sound:** Show Flight HUD, Minimal Input Hints, Engine, Wind and Warning Volume.
- **Key Bindings:** the flight keys, one Brake for both brakes.

Where they live:

- The C++ modes' settings are registered from C++ with Mod Settings' `AddVariable` (`Flight/Settings.hpp`,
  `Flight/FlightSettingsLink.cpp`), into `Flight::PlayerSettings` and `Flight::AutopilotSettings`. Without Mod
  Settings they keep their defaults. Hover & Fly's switch keeps the class and name the script mode had up to
  0.3.20, `[FlightModeHoverFly] enabled`, so a saved value still switches Hover & Fly. Standard is new and has
  its own: `[FlightModeStandard] enabled`, and its two speeds `[LTBF_HoverFly]` `hoverFlyTurnRate` and
  `hoverFlyClimbRate`. The Autopilot's are `[FlightModeAutopilot]`. The rows' places in Flight Modes are fixed
  in `Settings.hpp`.
- The rest are redscript fields with `ModSettings` attributes, mostly in `FlightSettings.reds` and
  `FlightModeDrone.reds`.

A Mod Setting field is written back from its stored value when its mode is engaged, so a value set on that field
before engaging is lost.

## Developer values

Values only a developer changes, never in the menu. Three kinds:

- **`Flight::DeveloperSettings`** (`Flight/DeveloperSettings.hpp`): a C++ struct with the code defaults, overridden
  at plugin load by an optional `developer.ini` in the plugin folder (releases ship none), and at run time by
  `FlightDeveloperSettings.Set(name, value)`. `Get` reads one, `Describe` lists those off their defaults. An
  unknown name or a value that does not parse is refused, a number out of range is clamped, and each is logged;
  each setting off its default is logged once at load. `autopilot.<name>` overrides a field of the Autopilot's
  tuning (`Navigation::AutopilotParams`).
- **Redscript fields with no Mod Settings attributes**: `FlightSettings`' handling values (Handling's Default set),
  brake factors, `physxFlightPhysics`, `groundFriction`, `autoActivationHeight` and camera internals; Drone's six
  factors; `padSummonRange`; `hudFlightController.attachToPlayer`. With no attributes they are not stored, so the
  code default holds for every player, and they are not written back when a mode engages. `FlightSettings.OnAttach`
  sets the rest with `SetFloat` and `SetVector3` (the hover band's heights, the input PIDs, the collision recovery).
- **C++ code defaults**: `StandardGains`, `StandardParams` (the old standard factors, read by Hover & Fly and the
  Autopilot), `HoverFlyClassicParams` (Hover & Fly's old Height Dampening and Height Correction), `DroneSkidParams`,
  `kEngineTorqueGain`.

The Autopilot's planned braking (`brakeAccel`) is a developer value too: out of the menu, so a saved value is no
longer read, and `autopilot.brakeAccel` overrides it.

## Feel spec

What Standard should feel like, for a beginner on any car: let go of everything at any time and the car comes to
rest, level, about where it was let go. Each control asks for a speed, not an acceleration. `tests/feel_tests.cpp`
flies the simulator's scenarios over every car of `tools/flightsim/vehicles.tsv` and checks, on Standard at each
Handling choice:

| Scenario | Check |
|---|---|
| `yaw_hold` | a held turn turns at a steady 85 to 95 deg/s, at 1 s and at 3 s |
| `knock_roll_yaw`, `knock_pitch_yaw` | knocked with a turn held, level within 2.5 s |
| `tilt_recovery`, `tilt_recovery_yaw` | let go at any tilt, a turn held or not, level within 3 s, never flipped over |
| `knock_low` | knocked while hovering low, back above the band's bottom within 1.5 s |
| `surge_turn` | a turn at speed keeps the velocity within 15 deg of the nose |
| `climb_short`, `climb_long` | a climb let go stops within 2 m |

The other scenarios (`hands_off`, `surge_release`, `descend_land`, `bump`,
`roof_edge_off`, ...) are measured by `flightsim` but not checked.

Drone is held to what it does now (`feel_drone_fleet`, `feel_antigravity_drone_fleet`): no NaN, never over the
spin cap, the slowest car no slower and the longest coast no longer, the angular brake never slower than letting
go, lift and surge within the fleet's spread. `feel_drone_assist_fleet` checks the proposal's rates on every car.
Hover & Fly has no feel test: it is kept as the old mode flew.
