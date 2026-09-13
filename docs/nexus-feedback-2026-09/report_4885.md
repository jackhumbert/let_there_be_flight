# Mod Settings (Nexus 4885) comment analysis

## 1. Numbers
- 503 comments analyzed (thread total 3222). Range 2022-09-21 (one sticky plus a few old reply parents) to 2026-09-10; bulk is 2025-06 to 2026-09.
- By jackhumbert: 3 (2022 sticky; two replies on 2025-06-24 about 0.2.14/0.2.17). Nothing from the author after 2025-06-24.
- Top-level user posts: about 229. Author replied to 2 (Gordo972, Masuryan); about 227 got no author reply.

## 2. Issues (ranked by frequency)

### 2.1 Menu entry missing, blank, or headers only (~150 distinct reporters, 2025-06-11 to 2026-09-10)
Three waves:
- 2.3 wave (2025-07-17 to 07-19, pre-0.2.21). Carenzi 2025-07-17: "all I see when clicking it in the menu, are the mod names and headers for settings. No actual settings for anything." Fixed by 0.2.21 (about 30 thanks on 07-19).
- Redscript 0.5.30 wave (2025-08-14 to 09-02). kaladbolgg 2025-08-18: "Be warned that Redscript lastest update v0.5.30 BREAKS this mod for some reason." Workaround: downgrade to 0.5.28 (TheJaix, zakhrim, filthyjw). jakekentj123 (2026-03-02) says 0.5.31 works.
- 2.31 wave and long tail (2025-09-11 onward). buck0021 2025-09-11: "CP 2077 got update mod setting not working we need an up date plz thanks". Stigir 2025-09-15: "Still works on 2.31. Just make sure you update ArchiveXL and RED4ext." Oct 2025 to Sep 2026 still brings 5 to 15 "not showing" reports per month.

User-found causes: incomplete Vortex redscript install (GrimScythex 2026-02-27: "it only copied the files into the r6 folder and ignored the files (scripts and scc) which need to be copied to engine/config/base and engine/tools respectively"); reinstall redscript last (jaaskk, 396280470); missing ArchiveXL, redscript, RED4ext or Codeware; Input Loader (Alejvip 2026-03-30: "AND INPUT LOADER which is not listed as a dependency on the nexus page but its needed"); Project E3 UI (toosober 2025-09-16, EvilWolf2, GrizzlyOne95, 4FAIV); the 2.3 EULA popup blocked by menu mods (Ravenlash); game still on 2.30; old plugin files not deleted before updating; a redscript-failing third-party mod (Auto Eat and Drink on Combat, Virtual Atelier, Zenitex, NPC gone Wild, Lizzie BD, Dark Future remorse addon, QuickhackLoadouts, Enhanced Vehicle System, In-World Navigation, each named once). Reveurr (2025-08-30) posted a clean-reinstall bisect guide many thanked. Two users show "unresolved reference 'ModSettings'" redscript errors (Neviem6482 2025-09-16, coredump777 2026-09-05).

### 2.2 Vortex / MO2 install problems (~18 reporters, 2025-07-16 to 2026-09-10)
Not version specific. Ieldra 2026-02-18: "@jackhumbert: please disable the mod manager install button." DirtyDeed2 2026-07-16: "if i install only mod settings+dependencies MO2 shows deploy error about packed.reds". MootsMods, MiniDongle, Barinthius, Nexodox, scrawns succeeded only after manual install; Xodolash had the reverse. 2025-07-16: Vortex download stuck pending (enygmainc, Pista420IQ).

### 2.3 Crash or hang on launch/load (~14 reporters, 2025-06-23 to 2026-08-25)
Linux/Proton cluster, post-2.31: fantymingo 2025-10-13: "I can load Audioware or Mod Settings, not both. Loading Both causes crash at load, removing either restores function." Log: "Could not open a thread. The transaction will continue but unexpected behavior might happen. Thread ID: 1236, error code: 87, msg: 'Invalid parameter.'" meik01 2025-10-14: "mine half the time just hangs, infinitely loading Mod Settings's library file" (also with Drive an Aerial Vehicle; suspects a race). napmouse 2026-06-26 indefinite hang; Gutyiort 2026-08-01 has a folder-layout bypass. Windows: F3L1XD0PE 2025-11-06 (save load and new game crash, gone when disabled), 123456LJ 2025-09-13, ref2335 2025-06-19 (HUD Painter crash on 0.2.14, fine on 0.2.8), Arietis013 2026-08-25.

### 2.4 Reset/Default wipes every mod with no confirmation (~11 reporters, 2025-01-16 to 2026-08-06)
Valour549 2025-01-16: "it will irreversibly change the settings for all mods using Mod Settings to their default values." carsonzhang 2026-07-15: "not just one but two buttons that will reset ALL of your mod settings to default without any kind of warning or \"are you sure?\" prompt". fckrg and Bazzbooz hit it via F1. Workaround: back up red4ext\plugins\mod_settings\user.ini.

### 2.5 Settings lost or reset spontaneously (4 reporters, 2025-11-21 to 2026-06-02)
prispimple 2025-11-25: "sometimes i'll load up the game and all the settings on this mod are back to default values". Giever ties it to enabling/disabling mods. chriszhxi 2026-06-02: "if you delete some mod, play game once, then later recover the deleted mod with every single file is the same, it will reset that mod's settings anyway and no way to recover". GrizzlyOne95: sliders all 0 (NemesisVali blames Mod Settings Extension).

### 2.6 Other settings frameworks
yewido6031 2025-12-31 disputes the sticky: "Nope, not compatible.If i install the Native Settings UI, the Mod Settings not appear in the menu, only the Mods." Aharann (2025-08-24) needed "Redscript And Cet Mods Settings" by anygoodname. rickrivera23 needs Mod Settings Extensions installed too.

### 2.7 Single reports
Photo mode flicker after 0.2.18 (Cotzee 2025-07-01), character creator scroll (GunsNRoses1), gallery glitch on 30xx GPUs (Zylenxx); Additional Content tab gone in 0.2.14 (Masuryan, author explained); French sort order (fixed 0.2.17); mouse wheel scrolls both panes (djoole 2026-08-21); Apply hangs 20 s (dictatorkc); lost open-menu keybind unrecoverable (FreckledSpaghett); Flatlined screen entry unopenable (Velgath); Delamain softlock on 2.3 pre-0.2.21; partly alphabetical list and inconsistent Vortex version metadata (soulmindberlin); 0.2.21 headers-only on game 2.2 (PenzRules, morayyy).

## 3. Feature requests
1. Confirmation and/or auto-backup before Reset/Default/F1; per-mod reset (8).
2. List Input Loader (and Codeware) as requirements (3).
3. Fix Vortex/MO2 packaging or remove the mod-manager button (2).
4. Docs/examples for mod authors; negative default values (2).
5. Remember last selection and scroll position (1).
6. Multiple dependencies per setting; hide empty category separators (1).
7. Keep settings for temporarily removed mods (1).
8. Fallback key to reopen menu after keybind loss (1).
9. Isolate mouse wheel per pane (1).

## 4. Sentiment
Non-author comments roughly 20% positive, 35% neutral, 45% negative. June 2025: calm. 2025-07-17 to 07-19: "needs update" flood, then a burst of thanks after 0.2.21 (exigeous: "if you need time TAKE IT - you owe NONE OF US ANYTHING"). Aug to Sep 2025: negative (redscript 0.5.30, 2.31). Oct 2025 to Sep 2026: steadily negative with no author presence; a "dead mod" narrative grows (RandomAmericanGuy 2025-11-20: "@jackhumbert, you may need to fix the mod for 2.31."; LEGIONFIVE 2026-05-05: "the mod has long been defunct."). Countered by loyal users (LisbethSAO 2026-08-22: "Mod Settings latest version from 19 July 2025 (v0.2.21) still working hella preem with game version 2.31/2.31a"). Peer support (Reveurr, 4FAIV, Ravenlash, GrimScythex) carries the thread.

## 5. Open questions to the author
- Is the archive laid out correctly for Vortex/MO2 (packed.reds deploy error)?
- Linux/Proton: thread error 87 at plugin load and intermittent hang with Audioware/DAV.
- Are Input Loader and Codeware hard dependencies?
- Is the global Reset/Default wipe intended?
- Photo mode flicker and character creator scroll after the 0.2.18 gallery fix.
- Redscript 0.5.30+ compatibility; minimum versions to state.
- Native Settings UI compatibility (sticky says yes, users say no).
- Does 0.2.21 support 2.2x?

## 6. Actionable next steps
1. New sticky: v0.2.21 works on 2.31; required RED4ext/ArchiveXL/redscript versions; delete old plugin folder first; Vortex redscript caveat (engine/config/base, engine/tools); where logs and user.ini live; Project E3 UI conflict. Addresses most of 2.1.
2. Re-upload or retag the file for 2.31 to stop "needs update" posts.
3. Confirmation dialog for Reset/Default/F1 plus a user.ini backup before bulk writes; per-mod reset.
4. Fix Vortex/MO2 packaging and add Input Loader/Codeware to requirements or mark them optional.
5. Investigate the Linux thread error 87 and hang using the logs fantymingo and meik01 posted.
6. Keep settings for mods absent from a session; investigate the occasional full reset.
7. Verify Native Settings UI coexistence and Mod Settings Extension interaction; update the sticky.
8. UI fixes: scroll isolation, remember scroll position, hide empty categories, multi-dependency, keybind fallback.
9. Reply to Cotzee and GunsNRoses1 on the gallery regressions.