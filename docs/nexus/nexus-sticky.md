# Nexus sticky / description update draft (0.4.0)

Draft for the Nexus mod page (5208) sticky post and/or description update. Plain text,
no BBCode, meant to be pasted as-is. Headings are lines, not markdown.

---

LET THERE BE FLIGHT 0.4.0 - FOR GAME 2.5

The biggest update since release: an Autopilot, landing pads, flying summons, a new default
flight mode, new sound and new ground effects.

REQUIREMENTS (exact versions, please check before reporting "it doesn't load")

* RED4ext 1.31.0 or newer
* TweakXL 1.12.0 or newer
* ArchiveXL 1.28.3 or newer (optional on game 2.5 and later)
* Redscript 0.5.32 or newer
* Input Loader 0.2.4 or newer
* Mod Settings 0.2.23 or newer (optional, but recommended: this is where LTBF's settings and
  key bindings live. 0.2.29 or newer shows the settings in their intended order, 0.2.31 or newer
  the three choices of Land at Destination)

Most "mod doesn't work" and "no flight button" reports turn out to be one of these being out
of date. If you're not sure, delete and reinstall each dependency fresh rather than trusting
an old copy.

WHAT'S NEW

* Autopilot. While flying, hold the autodrive key (G on keyboard, L3 on a gamepad). The
  vehicle takes off, flies a planned route between the buildings to your tracked destination
  and lands there. With no destination it roams the city, like vanilla autodrive's free roam;
  tap the key to switch between the two. Any flight input takes over for a moment. Hold the
  camera key for the cinematic camera.
* Landing pads. The city's AV pads are on the map and the minimap, discovered by flying near
  them, with supplies the first time you touch down on one. Track a pad and the Autopilot
  lands on it.
* Flying summons. Summon your vehicle on or near a landing pad and it flies in and lands on
  the pad. Where it cannot drive to you (a roof, off the roads, stuck on the way) it flies the
  rest and lands next to you. "Always Fly Summoned Vehicles" (off by default) makes every
  summon fly.
* Standard, the new mode flight starts in. Every control asks for a speed; let go and the
  vehicle comes to rest, level, where it is. Handling (Loose, Default, Tight) and Brake
  Strength set the feel. If flight felt floaty or slippery to you, this is the answer.
* Hover & Fly is still there, second in the mode list, flying as it did up to 0.3.20.
* New sound. The thrusters and the wind are synthesised and play through the game's own
  audio, with echoes off the ground and walls and reverb that follows the space you fly
  through. fmodstudio.dll is gone, and with it the 126 / 127 errors.
* Ground effects. Sparks, dust, trails and a scrape sound for each surface when the vehicle
  touches the ground.
* Drone mode rests and slides on the road on a soft skid instead of bouncing on it.
* A city you can fly through: about 1,200 hidden colliders removed, and solid walls on
  towers that were drawn without any.
* Spanish text for all the new settings.

UPGRADING FROM 0.3.20

* Delete the old `red4ext/plugins/let_there_be_flight` folder before installing, or let your
  mod manager do it. Left behind, `fmodstudio.dll`, `base_sounds.bank` and
  `base_sounds.strings.bank` do no harm, but nothing reads them any more.
* The settings menu was reorganised. Your Hover & Fly on/off choice is kept. These start at
  their defaults again: Brake Strength, Handling, Gamepad Flight Toggle Hold (0: a tap; set it
  again if you used the hold), the Brake key (C: both brakes are one key now), Drone Turn
  Speed and Drone Thrust.
* Hover, Fly and Anti-Gravity Drone are behind one setting, Extra Flight Modes (off). Turn it
  on if you used them. Automatic mode is gone.
* Previous Mode is J by default.
* Gamepad: if you use a hold on the flight toggle, move the toggle off L3 (Key Bindings >
  Toggle Flight (Gamepad)). An L3 hold also starts vanilla autodrive and, in flight, the
  Autopilot.

KNOWN ISSUES

* The Autopilot can wander off its line for the first seconds of a route, and engaging it at
  high speed between buildings can end in a bump before its first route is ready. Engage it
  from a hover where you can.
* A landing on a pad can end turned away from the pad's heading.
* Drone sliding along a road can still be thrown where two road pieces meet at an angle.
* With a destination the Autopilot cannot reach (far outside the city, inside a building) the
  vehicle hovers and no message says why. Pick another destination or let go of the hold.

CRASH ON CRYSTALCOAT / CUSTOMIZATION

If you crash flying a car with CrystalCoat (Caliburn, Aerondight and others), please report it
with your crash dump (below), and say whether a colour was applied before you flew it. To keep
one car out of flight meanwhile, add a tweak like this to `r6/tweaks/` and restart:

Vehicle.v_sport1_rayfield_caliburn_player { bool flightDisabled = true; }

RADIOEXT

LTBF no longer loads FMOD Studio and plays through the game's own audio by default, so the
old clash between the two mods' FMOD copies should be gone. Not tested here with RadioExt
installed: please report what you see.

VORTEX FAILING TO INSTALL INPUT LOADER OR MOD SETTINGS

If Vortex won't deploy Input Loader or Mod Settings correctly, install both by hand: download
the zip from their Nexus/GitHub pages and extract it into your game folder yourself.

HOW TO REPORT A CRASH

Please include the Cyberpunk2077.dmp file from the crash reporter (click "What does this
report contain?" to find it) and your `red4ext/logs/ctd_helper.log` file, copied *before* you
relaunch the game (it gets overwritten on every launch). Full instructions are in the
Troubleshooting section of the readme.

WHERE LOGS LIVE

LTBF's own log is `red4ext/logs/let_there_be_flight.log`. If flight isn't activating at all,
check this file first: it logs every dependency check and hook attach on startup.

SETTINGS

LTBF's settings live under Mod Settings. The settings guide in the repo,
docs/nexus/settings-guide.md, says what each one does and lists the controls.

UNINSTALLATION

Run `red4ext/plugins/let_there_be_flight_uninstall.bat`. It ships in the zip next to the
plugin folder and removes every LTBF file (it leaves your other mods and dependencies alone).
