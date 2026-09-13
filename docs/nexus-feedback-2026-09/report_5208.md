# Let There Be Flight (5208) Nexus comment analysis

## 1. Numbers

- Comments analyzed: 651 (3 stickies and 1 user post from 2022; the rest 2025-06-15 to 2026-09-09). Jul 2025 = 206, Aug 2025 = 111, then 10 to 46 per month.
- By the author (jackhumbert): 21 (3 stickies, 18 replies). Last author reply: 2025-07-16.
- Top-level user posts: 360. Author replied to 15. No author reply: 345 (every top-level post after 2025-07-14).

## 2. Issues (ranked by distinct reporters)

**1. Mod does not load / flight never activates / no Mod Settings entry (about 50 reporters, 2025-06-25 to 2026-09-09; dominant after 2.31).**
"Doesn't seem to be working ever since patch 2.31. Am I alone on this one?" (MayaTheCatboy, 2025-12-01). "Could not load plugin 'fmodstudio'. Error code: 127, msg: 'The specified procedure could not be found.'" (Deseoso, 2025-09-09; Pte00 2025-09-20 hit a getVersion@System@FMOD entry point error). "no button options to start the flight to begin with" (spacebrain97, 2026-09-09). Error 126 seen by SPEEDRUN165hz (2026-01-04, folder named zzzlet_there_be_flight).
User fixes: install Input Loader and Mod Settings manually, not via Vortex (CozmicsNonsense); uninstall in manager, launch once, reinstall (ritualdevice); clean reinstall of redscript and Codeware and clear r6/cache (SuperiiorX); "usually the culprit is outdated red4ext" (djkovrik). Several confirm it works on 2.31 with updated requirements (jojo1241, sh4dOw911, mattmesmer, DZxLA). No author reply.

**2. CrystalCoat / customization crash on entering or recoloring specific cars (about 30 reporters, 2025-07-13 to 2026-04-23; began with 2.3).**
Cars: all Caliburns, Aerondight, Hella, Type 66 variants, Emperor, Outlaw, Galena, Alvarado, Riptide Terrier, Trophy Tanishi, Johnny's Porsche (full list by TenebrisT1000, 2025-08-27). "Detected integer overflow for offset: 276192813200 aligned: 276192813200 capacity: 3337213872 elementSize: 11 File: E:\R6.Release\dev\src\common\redContainers\src\dynamicBuffer.cpp(96)" (skyrimsasuke, 2025-07-22). "NULL_CLASS_PTR_READ_INVALID_POINTER_READ_c0000005_let_there_be_flight.dll!Unknown" (Zylenxx, 2025-07-28).
Author (2025-07-14): "Yeah, it only seems to crash with particular cars - I haven't sorted out why yet." Workarounds: uninstall LTBF, reset CrystalCoat colors, reinstall (BerserkerDre); rename the dll before summoning quest cars and add a vehicle blacklist config (Charly30, 2026-03-28); uninstall before "You know my name" (Avnic).

**3. Startup crash before main menu (about 25 reporters, 2025-07-14 to 2026-08-20; peak after 2.3, continuing in 2026).**
"it makes my game instantly crash before even getting to the main menu when loading" (anodreth, 2025-07-18). "the scripts needed for let there be flight could not be loaded - was redscript compilation succesful?" (rejectiing, 2026-04-26). "i crashed when i updated the requioments, but when i go back to older versions, this mod works agine" (Darcy2000, 2026-08-20).
User-found culprits: SynthDose (WTCN collection), SE7EN Belt Dress clothing, spawn0 body mod, JB TPP, Input Loader together with LTBF (FILJOEN), Codeware (qyc2424711328, disputed). Syndroid: delete the plugin folder, launch, re-add clean files.

**4. Controls too floaty, hard on keyboard and mouse, settings undocumented (about 24 reporters, 2025-06-28 to 2026-07-10).**
"Could we get some type of wiki article or free website explaining what each mod setting does?" (RealCryterion, 2025-08-27). "could we get a setting to increase or decrease the amount of turning resistances, its too slippery" (Kaleb99, 2026-06-02). Tips: "Yaw Directionality Factor" (DenassaG), lower surge in Automatic (MercyFlusher288), use Automatic mode (elminister).

**5. RadioExt conflict (about 22 reporters, 2025-07-05 to 2026-01-03).**
"the game crashes as soon as you select a radio station from radioext while using the Flight Mode" (RogerWhatever, 2025-07-21). Author (2025-07-13): "they both use FMOD, and I had to update my copy, which causes a version mismatch. I know they're working on updating it". Sirandar888: "you should at least say that you have a plan".

**6. Stutter on entering and exiting vehicles, plus HUD reset (about 17 reporters, 2025-07-14 to 2026-08-08; mostly 2026).**
"Bisected down to just this mod. When enabled, entering and exiting a vehicle causes a noticeable hitch" (marcellobuddy, 2026-04-20, 9800X3D + 5090). "resets the HUD anytime you enter or exit a vehicle" (tomdepain, 2026-03-11). No fix found.

**7. Controller and keybind problems (about 15 reporters, 2025-06-27 to 2026-08-17).**
L3 click toggles flight and cannot be disabled or rebound (69DennySilverBat69, MichalsAvatar, FreiTheDerg); "you can't remap the stick and trigger" (Onewithmisery, 2025-08-29); lift on triggers fails (Phidiax). Fix: preset="invertAxis" on Flight_Lift (aseyabal).

**8. Limited HUD confusion (about 12 reporters, 2025-08-30 to 2026-08-07).** Description says incompatible; bra1L (2025-12-04): "Now compatible with Limited HUD!"; DZxLA confirms 0.3.17 with LHUD 2.21.4. B9s2n (2026-04-12): "can you make it clear that it works again?"

**9. Physics glitches (about 14).** "When I get into the car, it suddenly flies into space" (Voshak, 2026-06-18); cannot drive after landing (PanPan66); traffic vanishes (StreetKidV2, 2026-06-23). Fix for undrivable car: toggle flight on then off (XirlioMerazul).

**10. Other HUD conflicts (about 10).** Project E3 HUD layout breaks (4 reporters), 3D HUD missing (2), waypoint marker vanishes (OnsenBoss), Firestarter behavioral imprint UI blocked (KarateChaise, 2025-09-10).

**11. Flight audio lost after quests or races (about 9, 2025-07-24 to 2026-08-16).** After Chipping In (TyetheGuy420), act 2 (PunishedWolfOG), Santo Domingo race (tfrosteeee). Fix: "Game.GetTimeSystem():SetPausedState(false, '')" in CET (fronom). The June 2025 ambient sound bug was fixed in 0.3.10.

**12. uninstall.bat missing (9 reporters, 2025-07-18 to 2026-07-28).** "There is no Uninstall.Bat file anywhere" (mark4020).

**13. Autodrive cinematic camera broken since 2.3 (about 8, 2025-07-18 to 2026-05-22).**

**14. Other named conflicts.** Night City Alive, Dark Future (rayzm0nd1201: HandleCameraInput in DFVehicleSleepSystem.reds vs packed.reds), Car Modification Shop, Vehicle Navigation System, Night City Allies, EVS, Need More Smoke FX. Also redscript "@addField ... already defined" warnings (5), 3 missing localization strings (8LOMU8), archive.xl "The target sector has 243 node(s), but the mod expects 242" (battleyurika).

## 3. Feature requests

- Limit flight to certain, luxury, or purchased vehicles: 11 requests plus 4 "+1" (2022 to 2026-09-07). Author 2025-06-16: "some roleplay/gameplay ideas like this I'd like to add down the road".
- Settings guide or presets for keyboard and mouse: 8.
- Mouse-aimed flight or BTTF-style scheme: 5.
- Hide or choose thruster style per car: 4 (author posted a custom_flight_config.reds snippet 2025-06-17).
- Rebindable or hold-to-toggle L3: 4.
- Invert vertical axis, Automatic lift factor: 2 (author 2025-07-14: "I'll have this exposed as an option in the next update").
- Lower thruster volume: 2. Flying NPC or police vehicles: 2. One each: mute honk in flight, call car from the air, "Vantage Point" hover quickhack, quickhacks-only version, level "DeLorean" mode, disable vehicle health HUD.
- Compat patches: RadioExt, Night City Allies (3), Project E3 HUD (2), Immersive Odometer, VNS.

## 4. Sentiment

Rough split of user comments: 30% positive, 25% neutral, 45% problem reports, many still grateful ("Love this mod but...").
- Jun to mid-Jul 2025: euphoric comeback ("I cannot express the amount of joy I felt seeing the words, "Hello! I am back." on this page.", Frank4Fingers); audio fix praised.
- Late Jul to Aug 2025: crash wave after 2.3, tone still warm; donations mentioned.
- Sep to Dec 2025: repeated "does it work on 2.31?", peers answer yes; LHUD fix celebrated.
- Jan to Mar 2026: "No updates or replies/posts since 8/2025 so I assume this mod is dead" (MajDamage, 2026-03-13), countered by "This mod isn't dead. It works." (AnUnfathomableRy).
- Apr to Sep 2026: sparse and frustrated: "the mod author hasn't been on the Nexus since the start of the year" (AbeBatJes, 2026-07-30); one hostile post ("Pretty sure this mod just sucks", guitarishard); users redirect to mod 13842.

## 5. Open questions to the author

1. Is Limited HUD supported now? The description contradicts users.
2. Any plan for RadioExt?
3. How to uninstall cleanly without uninstall.bat?
4. Will the CrystalCoat and quest-vehicle crash be fixed, or can vehicles be blacklisted?
5. Cause of the enter/exit stutter and HUD reset?
6. Why does flight audio die after Chipping In or races?
7. Autodrive cinematic camera fix?
8. What does each setting do; recommended keyboard and mouse preset?
9. Which requirement versions does 0.3.18 need?
10. Vehicle whitelist status?

## 6. Actionable list

1. Post a 2.31 sticky with 0.3.18: exact requirement versions, "works with Limited HUD", what errors 126/127 mean, manual-install note for Input Loader and Mod Settings (themes 1, 8; 60+ reporters).
2. Fix the CrystalCoat crash: null guard in the vehicle setup hook, skip quest or reward vehicles, add a blacklist (theme 2; author-acknowledged; dumps show a dynamicBuffer overflow and a null read inside let_there_be_flight.dll).
3. Ship uninstall instructions and a fresh-install checklist; note stale fmod DLLs cause error 127 (themes 1, 12).
4. Profile vehicle mount/unmount; the hitch and HUD reset are bisected to LTBF alone (theme 6).
5. Investigate audio loss tied to time-system pause state after quests and races (theme 11).
6. Document the RadioExt FMOD status in one sentence on the description (theme 5).
7. Add the promised options: invert lift, Automatic lift factor, turn resistance, L3 hold or rebind; write a short settings guide (themes 4, 7).
8. Restore the Autodrive cinematic camera (theme 13).
9. Implement a vehicle whitelist or unlock mechanic, the top request since 2022.
10. Silence the @addField warnings, add the 3 localization strings, fix the archive.xl node count.
