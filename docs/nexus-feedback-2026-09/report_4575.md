# Input Loader (Nexus 4575) comment analysis

## 1. Numbers
- Comments analyzed: 167 (thread total 764; this fetch covers 2024-12-27 to 2026-09-03 plus the 2023-04-22 sticky).
- Author (jackhumbert) comments: 1, the 2023 sticky ("shouldn't need to be updated with future game versions"). No author reply anywhere in the window.
- Top-level posts: 70 (69 by users). Top-level user posts without an author reply: 69 (100%).
- Most useful community responder: djkovrik (5 comments, technical rebuttals).

## 2. Issues (ranked by frequency)

**A. Crash to desktop / freeze blamed on IL (about 35 distinct reporters, 2025-01-07 to 2026-08-06).** Spikes after 2.3 (Jul-Aug 2025) and a long "mod is dead" wave Dec 2025 to Aug 2026 (post-2.31, but same symptoms as pre-2.3 reports). Sub-pattern: "PRESS [None] TO CONTINUE" then crash on Space.
- Horrorble 2025-01-07: "the intro screen says PRESSTO CONTINUE, with no "key" displayed."
- SMmania123 2025-07-20: "It was working fine before 2.3 update, but now its just unusable for me."
- nonamejar404 2026-05-15: "this mod is dead, and all mods useding it are dead. and no one found a solution"
Workarounds posted: harry20199vn (copy vanilla r6/config xmls over the mod's bundled copies, thanked by 3 users); Horrorble (delete xmls, verify, overwrite); deleted130222808 (full uninstall list incl. input_loader.ini and r6/cache/modded); AsUIoc / Neamhaindeus (delete input_loader.ini only); anygoodname 2026-09-03 reproduced the Space crash: ini present but cache xmls and dll deleted. Several "IL" crashes were later traced elsewhere: RadioExt (BULLet369), Enhanced Vehicle System (2001Sniperpilot), Auto Drive mod (HodTogMann), Bloat Begone (cam7128), duplicate .reds (EYYUZ).

**B. cybercmd conflict (11 commenters, 2025-09-07 to 2026-08-10).** TrueTenkaichi 2025-09-07: "Cybercmd is an old dependency that must be fully uninstalled to prevent conflicts and CTDs." JynErsoLives: "This is the solution to almost ALL complaints here." Counter-report HyperRs66 2026-05-31: installing latest cybercmd "seemed to fix my issue." MadySanchez warns against the "rename r6\cache" step.

**C. Version / 2.3 / 2.31 compatibility questions (10, 2025-07-17 to 2026-07-23).** Rawr40k, pablodkl, kedjawen, Cross9293708b13 ask; Vancali 2026-07-26: "It's a shame the author doesn't come back and fix it considering 2.31 is likely the last patch". djkovrik 2025-07-17: "Just wait for red4ext update".

**D. Stale bundled vanilla XML / cache not regenerating (7, 2024-12-27 to 2026-03-15).** aaronlauretani85 quoting djkovrik: "run the game at least twice after doing all steps". thegraydev 2026-03-15: "Red4ext log shows Input Loader version 0.1.1" despite 0.2.3. NuclearPulse: mods overwriting r6\config\inputContexts.xml cause "invalid input".

**E. Controller problems (7, 2025-01-05 to 2026-05-19).** Jax787 xbox triggers unmapped; Berengalathil 2025-07-28 cannot accelerate; Marrionetta (self-solved, Advanced Driving Controls optional file); ImperatorI DualShock vibration; DeusMachina01: NUMLOCK "switches to the left-arrow key on my controller."

**F. Specific bindings broken (7).** Smart Frames softlock (aaronlauretani85, R0bouteGuilliman, adbdul31), twintone purchase (SarenV12, videqualia), Photo Mode Backspace (bc12343 2025-09-08, Hashimei, focusghost), pickup F (DoarLumina).

**G. Hold-action merge bug (2).** LasurDragon 2025-12-18: "Mod seems to always append "hold" actions instead of replacing them" and "it tries to match them by "name" while "action" should be used instead." slimejean (Faster Interface author) hit the same wall.

**H. Install tooling (4).** IActionman: "Missing archive for 2A9BE09DBF36C735" in the Nexus app (manual install fixed); Vortex blank settings; MO2 questions.

**I. Other (1 each).** HangingAL XML parse error at offset 56552; Furego0 VirusTotal IP flag; claychampion mid-session reinit theory (rebutted by djkovrik); MikeyPsyche VRAM drop after removal.

## 3. Feature requests / suggestions
- Author statement or update for 2.31 (4)
- Step-by-step install/troubleshooting guide in description (3)
- Pin a technical explanation (djkovrik's post) or crash guide (2)
- Replace, not append, hold actions (2)
- Documentation for acceptedEvents / event names (1)
- Deadzone and axis inversion for stick bindings (1)

## 4. Sentiment (166 non-author)
Roughly 40% negative, 45% neutral (questions, how-to), 15% positive. Early 2025: mixed, fixes shared and thanked. Jul-Aug 2025: crash spike right after 2.3, then "works for me" confirmations. Dec 2025 to Aug 2026: mostly negative, "stay away" and "dead" posts with no author presence. Aug-Sep 2026: pushback (djkovrik, anygoodname) and fresh positives (LisbethSAO, Gambaruuuu, Valamyr). Absence of the author is itself a repeated complaint.

## 5. Open questions to the author
1. Does 0.2.3 officially support 2.31, and is an update planned?
2. Why does the red4ext log report 0.1.1 for a 0.2.3 install?
3. Is the hold-action match keyed on name instead of action?
4. Why can generated files take two launches to apply?
5. What does the "Error parsing element attribute" log mean and how to find the bad mod xml?
6. VirusTotal network flag: expected?
7. NUMLOCK / keypad keys: supported as global hotkeys?
8. Where are acceptedEvents / event names documented?

## 6. Actionable next steps
1. Replace the 2023 sticky with a 2.31 statement: works, runs only at startup, generates two xmls plus ini, cannot crash mid-game (evidence: djkovrik 2026-08-17, ~10 version questions).
2. Add a hardened uninstall/corruption guard: if input_loader.ini exists but cache xmls or dll are missing, remove the ini or regenerate (anygoodname repro, "PRESS [None]" reports).
3. Stop shipping vanilla xml copies; always derive from r6/config at startup and warn on a mod-overwritten base (harry20199vn fix, NuclearPulse).
4. Fix hold-action merge to key on action (LasurDragon, slimejean; likely fixes Photo Mode Backspace and Faster Interface).
5. Fix the version string exposed to red4ext (thegraydev).
6. Add a troubleshooting section: cybercmd removal, clean uninstall file list, do not delete r6\cache wholesale (TrueTenkaichi, MadySanchez).
7. Investigate first-run regeneration timing (djkovrik "run at least twice").
8. Publish acceptedEvents docs and consider deadzone/inversion attributes (datchleforgeron, 3verstorm1572).
