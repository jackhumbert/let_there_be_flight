# In-World Navigation (mod 4583) comment analysis

## 1. Numbers
- Comments analyzed: 191 (thread total 1217). Range 2022-10-27 to 2026-09-11; 186 fall in 2025-01-07 to 2026-09-11.
- Author (jackhumbert) comments: 7 (1 sticky from 2022, 6 replies 2025-06-16 to 2025-07-24). None after 2025-07-24.
- User top-level posts: 100; without an author reply: 94.

## 2. Issues (ranked by frequency)

**A. Mod not showing / no arrows / no settings entry** (19 reporters; 2025-01-07 to 2026-07-20; spikes after 2.3 and 2.31, not version-specific)
- bloodmoon96, 2025-01-07: "Mod just isn't showing up for me at all. I have all required mods installed and updated to lasted."
- pp179795, 2025-10-06: "Doesn't work anymore, needs update for 2.31." then 2025-10-10: "problem solved by update \"Mod Settings\"."
- OasisUnkown, 2026-04-01: "you have to try one of those and pray if it works. Worst mod ever."
- Fixes posted: install/update Mod Settings (jackhumbert 2025-07-22, tazmaster, slowchu, zzc300); Vortex purge + redeploy (TheLunaLeo); settings sit under "Mod Settings" not "Mods" (einyel18); pick walking/driving/both (Kiwiteepee2). Six users confirm it works on 2.31.

**B. REDScript compilation error on packed.reds** (12 reporters; 2025-04-06 to 2026-06-13; spike 2025-07-17 to 07-19, Mod Settings "being out of date for 2 days" per Bigmonkey23)
- XZodio12345, 2025-04-06: "I'm getting a redscript compilation error when launching the game that points to the packed.reds file from this mod."
- eter9401, 2025-12-02: "method 'GetInkSystem' not found on 'GameInstance'" (after GVINSH's manual edit; fixed by mod 7780).
- Fix: jackhumbert 2025-07-22: "you probably need to install Mod Settings."

**C. Crashes** (8 reporters; 2025-03-21 to 2026-09-11)
- Pre-2.3 exit crash: kleecels 2025-03-21: "the crash error to pop up when you quit the game or after prolonged(maybe 2hrs+) gameplay". jackhumbert 2025-07-01: "This should be fixed in version 0.1.19".
- Post-2.31: GamerChan09 2026-07-08: "crashing my game on any save I tried to load... Uninstalling In-World Navigation and Vehicle Navigation System fixed my issue."; AnonymousGameZ 2026-08-28: "Crashing whenever I open the map"; MootsMods 2026-06-13 hash error on 0.1.19; liq1337 2026-09-11: "please update for 3.0.80game crashes because the latest version is for 2.3".
- Workarounds: No Crash Reporter (dframed), lock ReportQueue folder (1Cyanide101).

**D. FPS loss while driving** (4 reporters; 2025-02-05 to 2025-09-11)
- HodTogMann, 2025-02-05: "around a 20%-25% performance drop upon entering a vehicle"
- Meroshiro, 2025-09-11: "From 75-90 to 55-65 - driving (higher speed -> higher fps loss. Drop to 50-55 when speed is 300+ km/h)"

**E. Cannot hide with HUD toggles** (see section 3). 833LZ38U8, 2025-11-16: "Limited HUD, Simple HUD Toggle, even AMM - none of them are disabling in-world navigation." GVINSH's inkHUDLayer edit works only with HUD fully off; eter9401's `this.mmcc.lhud_isVisibleNow` edit works with Limited HUD. Untrack Quest Ultimate suggested as a toggle (40teeth, bonzopellmo).

**F. Wrong / stale / misplaced path** (5 reporters)
- chiefs4322, 2025-03-14: "seems to reset the path often now with the guide line even vanishing for my mini map"
- uecasm, 2025-08-15: "even on \"driving only\" mode the arrows still appear (usually in midair) when riding the metro"
- 1RuggedGamer 2026-01-30 (2.31a, waypoint line sometimes missing; Arnoschnitzel calls it vanilla), axbhub (not centered), ref2335 (map filter icons gone).

**G. Conflicts / misc**: VNS (GamerChan09); packed.reds name clash with Mod Settings suspected (xnadler); MO2 RootBuild does not hook (sickorider76); distance fade slider not working (1Cyanide101). Circlemap Widgets confirmed compatible; HUD Painter asked.

## 3. Feature requests (deduplicated)
| Request | Count |
|---|---|
| Toggle hotkey / hide with HUD, scanner or zoom | 15 |
| Flat, flush-to-road or race-style arrows | 9 (author 2025-07-22: "computationally expensive") |
| Change arrow color (match Project E3) | 6 |
| Opacity / brightness slider | 4 (+3 user patches) |
| Smaller arrows on foot / size setting | 4 (author 2025-07-24: "I'll see what I can do") |
| Only show at turns / turn indicator | 4 |
| Hide during autodrive | 3 (WitaraSP fork) |
| Hide in photo mode | 1 (author: "I can make this an option") |
| Vertical offset slider | 1 (MustDmrll, code posted) |
| Hide on metro / as passenger, larger spacing, XYZ tweak, alt styles, Native Settings UI, HUD Painter compat | 1 each |

## 4. Sentiment (184 non-author comments)
About 35% positive, 35% neutral (questions, +1, how-tos), 30% negative. Jan to Jun 2025 negative (exit crash, compile errors, "seems to be dead"); Jun to Aug 2025 strongly positive after 0.1.19/0.1.20; Sep to Nov 2025 split between "needs update for 2.31" and "works just fine on 2.31"; 2026 mostly positive plus requests, but crashes and "the MOD is DEAD and there is no way to contact Jack" (SUNYTECH, 2026-09-03) resurface. The community is self-patching packed.reds (MustDmrll, GVINSH, eter9401, Velgath, WitaraSP, SUNYTECH).

## 5. Open questions to the author
- liq1337: update for "3.0.80"; MootsMods: hash failure on 0.1.19.
- AnonymousGameZ: crash on map open; GamerChan09: crash on save load alongside VNS.
- aleczapka: "what is \"in_world_navigation.dll\" for under plugins?"; xnadler: packed.reds name clash?
- 1Cyanide101: is distance fade working?
- Promised: walking size, flush option, photo mode hide.
- MustDmrll, SUNYTECH: messages closed; copyright clause blocks sharing tweaks.

## 6. Actionable list
1. Verify the "3.0.80" build claim (2026-09-11) and the 0.1.19 hash error; rebuild and republish if hashes moved.
2. Pin a sticky: works on 2.31, needs latest Mod Settings, settings live under "Mod Settings" (covers A and B, 31 reporters).
3. Add a toggle keybind plus HUD-visibility gating via `lhud_isVisibleNow` (15 requesters).
4. Ship opacity and vertical offset sliders (MustDmrll and guruhoro patches exist).
5. Add flat/flush style and per-mode walking size, both promised July 2025.
6. Hide arrows during autodrive, photo mode, metro and passenger rides.
7. Profile the driving path update; four reports of 20 to 45% FPS loss at speed.
8. Reopen a contact channel (GitHub issues or Nexus messages) and clarify the redistribution clause.
