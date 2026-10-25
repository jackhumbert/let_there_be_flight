# Nexus sticky / description update draft (0.3.18, LTBF-1)

Draft for the Nexus mod page (5208) sticky post and/or description update. Plain text,
no BBCode, meant to be pasted as-is. Headings are lines, not markdown.

---

LET THERE BE FLIGHT 0.3.18 - FOR GAME 2.31

0.3.18 targets game version 2.31 (the current PC patch as of this post). If you are on an
older LTBF version and just updated the game, update LTBF too before reporting a crash.

REQUIREMENTS (exact versions, please check before reporting "it doesn't load")

* RED4ext 1.29.0 or newer
* ArchiveXL 1.27.0 or newer
* TweakXL 1.11.0 or newer
* Redscript 0.5.31 or newer
* Input Loader v0.2.3 or newer
* Mod Settings v0.2.21 or newer (optional, but recommended - this is where LTBF's settings
  live, see below)

Most "mod doesn't work" and "no flight button" reports I've seen turn out to be one of these
being out of date. If you're not sure, delete and reinstall each dependency fresh rather than
trusting an old copy.

WHAT'S FIXED IN 0.3.18

* Supports game patch 2.31.
* CrystalCoat and customization crashes (Caliburn, Aerondight, Hella, Type 66, Emperor,
  Outlaw, Galena, Alvarado, Riptide Terrier, Trophy Tanishi, and quest/story cars like
  Johnny's Porsche): added null and bounds guards on the customization and mesh code path,
  plus a per-vehicle blacklist so a specific car can be excluded from flight entirely if it
  still gives you trouble (needs confirmation - if you still crash on one of these cars,
  please report it with your crash dump, see below).
* Flight audio no longer goes silent after a scripted pause (Chipping In, the act 2
  transition, races) (needs confirmation).
* The autodrive cinematic camera works again, including alongside Dark Future (needs
  confirmation).
* The flight input hint no longer shows on vehicles you're not allowed to fly (quest cars,
  Delamain, anything with the VehicleBlockFlight status effect) (needs confirmation).
* Destroyed vehicles no longer fall through the map when LTBF is installed (needs
  confirmation).
* uninstall.bat is back in the release zip (see Uninstallation below).

If you can blacklist a car yourself before I get to it: add a tweak like this to
`r6/tweaks/` and restart:

Vehicle.v_sport1_rayfield_caliburn_player { bool flightDisabled = true; }

LIMITED HUD

Limited HUD works again with current versions of both mods. If the mod page description
still calls it incompatible anywhere, that text is stale, ignore it.

FMODSTUDIO.DLL ERROR 126 / 127

If you see an error mentioning fmodstudio.dll (error code 126 or 127, "the specified
procedure could not be found" or similar), it means you have stale FMOD dlls sitting around,
or the plugin folder got renamed (it must be named exactly
`red4ext/plugins/let_there_be_flight`, not something like `zzzlet_there_be_flight` to force
load order). The fix: delete the whole `red4ext/plugins/let_there_be_flight` folder and
reinstall LTBF fresh. Don't just overwrite files on top of an old install.

VORTEX FAILING TO INSTALL INPUT LOADER OR MOD SETTINGS

If Vortex won't deploy Input Loader or Mod Settings correctly, install both by hand: download
the zip from their Nexus/GitHub pages and extract it into your game folder yourself. Both are
required (well, Mod Settings is optional but recommended) and LTBF will refuse to load
without the ones it needs.

RADIOEXT

RadioExt and LTBF both use FMOD, and versions of the two can conflict if their FMOD copies
don't match. I know it's being worked on. If you use RadioExt, keep both mods on their
current versions and update together when either one releases.

CRASH ON CRYSTALCOAT / CUSTOMIZATION (SEE ABOVE)

Covered above under "What's fixed" - guards and a blacklist flag are in, but please still
report it if you hit a crash on one of the listed cars, especially with a color already
applied on the vehicle before you flew it.

HOW TO REPORT A CRASH

Please include the Cyberpunk2077.dmp file from the crash reporter (click "What does this
report contain?" to find it) and your `red4ext/logs/ctd_helper.log` file, copied *before* you
relaunch the game (it gets overwritten on every launch). Both together make it much easier
for me to track down what happened. Full instructions are in the Troubleshooting section of
the readme.

WHERE LOGS LIVE

LTBF's own log is `red4ext/logs/let_there_be_flight.log`. If flight isn't activating at all,
check this file first, it logs every dependency check and hook attach on startup.

SETTINGS

LTBF's settings (lift, turn resistance, flight UI, etc.) live under Mod Settings, not a
separate menu. Install Mod Settings and open it in-game to find them. A settings guide is in
the repo at docs/settings-guide.md if you want details on what each option does.

UNINSTALLATION

Run `red4ext/plugins/let_there_be_flight_uninstall.bat`, it ships in the zip next to the
plugin folder and removes every LTBF file (leaves your other mods and dependencies alone).
