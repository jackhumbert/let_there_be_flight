# Let There Be Flight (LTBF)

Cyberpunk 2077 mod: flight for all vehicles. RED4ext C++ plugin + redscript + TweakXL tweaks + ArchiveXL archive + FMOD audio banks. Everything ships under `red4ext/plugins/let_there_be_flight/` (scripts, tweaks, archive, inputs are all loaded from the plugin folder, not r6/archive dirs).

Sibling repos with the same build/release shape and their own CLAUDE.md: `../mod_settings` (Nexus 4885), `../in_world_navigation` (Nexus 4583), `../input_loader` (Nexus 4575). SDK/cyberpunk_cmake updates are done here first and the commits reused there.

## Layout

- `src/red4ext/` - the plugin. `Main.cpp` = entry, dependency version checks, registers scripts/tweaks/archive/inputs with the other plugins. `Hooks/` = one game-function hook per file. `Utils/FlightModule.hpp` = hook registry macros.
- `src/redscript/` - 71 `.reds` files, packed by CMake into `packed.reds` + `module.reds`.
- `src/tweaks/` - `.tweak` (native) + `.yaml` tweaks, packed into one file each.
- `src/wolvenkit/` - Wolvenkit project. The built `packed/archive/pc/mod/let_there_be_flight.archive` is committed; CMake just copies it. Rebuild it with `python tools/pack_archive.py` (see "Rebuilding the archive").
- `src/fmod_studio/` - FMOD project. Built banks `Build/Desktop/*.bank` are committed; CMake copies them plus `fmod.dll`/`fmodstudio.dll`.
- `src/input_loader/let_there_be_flight.xml` - keybinds, installed as `inputs.xml`.
- `deps/` - git submodules (see below). `deps/fmod`, `deps/PhysX_3.4`, `deps/PxShared` are NOT submodules (gitignored local SDK drops) - do not delete.
- `deps/cyberpunk_cmake/` - our CMake framework (`configure_mod`, `configure_red4ext`, `configure_redscript`, `configure_tweaks`, `configure_release`, `configure_install`). Ships `tools/` (zoltan-clang, redscript-cli, ninja).
- `experiments/`, `examples/` - scratch, not built.
- `game_dir/` - build output in game-folder layout. `game_dir_debug/` - PDBs. Both zipped for release.

## Build

Local toolchain: MSVC 2022 (VS Community, kit in `.vscode/cmake-kits.json`), Ninja, CMake 3.24+. Configure and build:

```
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja
cmake --build build
cmake --install build      # copies game_dir + game_dir_debug into CYBERPUNK_2077_GAME_DIR
cmake --build build --target let_there_be_flight_release   # zips game_dir -> let_there_be_flight_<ver>.zip
```

- C++ compiles need the MSVC environment: from an agent shell a bare `cmake --build build` fails with `Cannot open include file: 'concepts'` (redscript-only rebuilds still succeed). Wrap it in the PowerShell tool: `cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build'`. The same applies to the sibling repos.
- Game dir is read from the `CYBERPUNK_2077_GAME_DIR` cache var (currently the Steam install). Game version for the `.rc`/badge comes from `Cyberpunk2077.exe` FileVersion via `deps/red4ext.sdk/cmake/GetGameVersion.cmake`; in CI there is no exe so it falls back to the SDK's `RED4EXT_RUNTIME_LATEST` macro. Keep the fork's `Api/v0/Runtime.hpp` `RUNTIME_LATEST` pointing at the current patch or the release name/badge will be wrong.
- Mod version comes from the latest git tag (`ConfigureVersionFromGit`). Untagged local builds get `+<branch>.<n>.<sha>` metadata (CI builds drop it).
- `game_dir_requirements/` is NOT safe to copy wholesale into the game: `FindRED4ext`/`FindArchiveXL`/`FindTweakXL` unpack whatever release zip is cached in `build/downloads/` (fetched once, then never refreshed unless `-DMOD_FORCE_UPDATE_DEPS=ON`), so it can hold years-old RED4ext/ArchiveXL/TweakXL. Do not use it for local testing at all: the in-tree `input_loader.dll` / `mod_settings.dll` report wrong plugin versions (input_loader 0.1.1 from its hardcoded project version, mod_settings the parent's LTBF version), so `Main.cpp`'s dependency check rejects them. Install the real Input Loader / Mod Settings release zips into the game instead.
- `configure_release` regenerates `requirements.md` from `MOD_REQUIREMENTS` (own entries in root `CMakeLists.txt`, plus whatever ArchiveXL/TweakXL/RED4ext Find modules append). It is committed and appended to release notes.
- Redscript is compiled/linted against `deps/mod_settings/redscript` too (`.redscript-ide`).
- Dead config, safe to ignore: `src/red4ext/CMakeLists.txt` and `cmake/FindCodeware.cmake` are never included (the plugin target is built by `configure_red4ext` globbing the dir); the `src/redscript/codeware` submodule in `.gitmodules` is not checked out and not needed. Nothing in the plugin includes Codeware headers.
- `ZOLTAN_CLANG_EXE` is hardcoded in root `CMakeLists.txt` to a local zoltan build; `configure_red4ext_addresses` is commented out, so zoltan does not run in the normal build.

## Rebuilding the archive (WolvenKit CLI)

Agents can rebuild the archive without the Wolvenkit GUI:

```
python tools/pack_archive.py           # import changed localization, pack, verify, write packed/.../let_there_be_flight.archive
python tools/pack_archive.py --check   # exit 1 if a localization json has not been packed yet
```

- CLI: WolvenKit.Console 9.0.1 (GitHub release zip), installed at `%LOCALAPPDATA%\Programs\WolvenKit.Console\9.0.1\WolvenKit.CLI.exe`; the script downloads it if missing, `WOLVENKIT_CLI` overrides the path. On the current PC (kevin) it is at `D:\cp2077_work\build\tools\wolvenkit-9.0.1\WolvenKit.CLI.exe` (set `WOLVENKIT_CLI` to it: agent shells there have `%LOCALAPPDATA%` writes redirected), with the game's `oo2ext_7_win64.dll` copied next to it. Do not use the NuGet `wolvenkit.cli` dotnet tool (`cp77tools`, stuck at 1.6.1; 8.x+ packages are not valid tools) or `%LOCALAPPDATA%\Programs\WolvenKit` (GUI, no working CLI).
- Source of truth: the CR2W files in `src/wolvenkit/source/archive/` (45 files, byte-identical to the packed archive as of 2026-09-30). `pack` names its output after the input folder (`archive.archive`); the script renames it.
- Localization is the only thing edited as text: change `src/wolvenkit/source/raw/user/jackhumbert/localization/let_there_be_flight.<lang>.json.json`, then run the script. It deserializes with `cr2w -d` (the `import` command silently exits 3 on these jsons) and decides staleness after a full serialize round trip, because serializing adds default fields (`primaryKey`, `maleVariant`) the raw json omits.
- Every other raw json is a stale export and must not be imported (`flight_hud.inkwidget.json` is from 2022 and predates the packed widget). Meshes, widgets, effects: edit in the Wolvenkit GUI, save into `source/archive/`, then run the script to pack.
- `src/wolvenkit/source` is gitignored but its files are tracked: stage changes with `git add -u <path>` (plain `git add` refuses).
- Other handy commands: `WolvenKit.CLI unbundle <archive> -o <dir>`, `archive <archive> --list`, `cr2w -s <file> -o <dir>` (CR2W to json). `-v` takes `Minimal`/`Diagnostic` etc., not short forms.
- After repacking, `cmake --build build && cmake --install build` to get it into the game.

## Running Lua in the live game (CET bridge)

`tools/cet.py` runs Lua inside the running game through a small CET mod (`tools/cet_bridge/init.lua`, installed as `bin/x64/plugins/cyber_engine_tweaks/mods/claude_bridge/`). CET's sandbox has no sockets, so the transport is `request.txt` / `response.txt` in the mod folder, polled every 0.1 s on the game's main thread.

```
python tools/cet.py install                       # copy the mod into the game (then restart, or CET overlay > Reload all mods)
python tools/cet.py run "Game.GetPlayer()"        # expression -> its value
python tools/cet.py run 'print(x); return y, z'   # chunk; print() output is captured; exit code 1 on a Lua error
python tools/cet.py mcp                           # stdio MCP server with a `cet_lua` tool
```

- MCP: add `"cet": {"type": "stdio", "command": "python", "args": ["tools/cet.py", "mcp"]}` to `.mcp.json` (gitignored, local) and restart Claude Code.
- CET's sandbox has no `load`; the bridge uses `loadstring` (LuaJIT, Lua 5.1: no `table.pack`/`table.unpack`).
- Do not `GameDump()` raw struct returns (e.g. the `Matrix` from `VehicleObject.GetInertiaTensor()`); it crashed CET 1.37.1. Read fields from redscript and log them instead.
- Known CET 1.37.1 crash on quit after any CET mod reload (hot reload or Reload all mods): access violation at `cyber_engine_tweaks+0xac37`. Harmless (only on exit), not LTBF.

## How the plugin finds game functions (what a game patch actually breaks)

- Almost every hook is `REGISTER_FLIGHT_HOOK_HASH(ret, <hash>, Name, args...)` in `src/red4ext/Hooks/*.cpp`. The hash is a RED4ext universal address hash resolved at runtime by `RED4ext::UniversalRelocBase::Resolve`, i.e. by the installed RED4ext's address database for the running game version. `UniversalRelocFunc<...>(hash)` is used the same way for calls.
- So a new game patch needs: (1) a RED4ext release that supports the patch, (2) the SDK with that patch's RTTI/struct layouts, (3) verify each hashed function still exists and its struct offsets (the hand-laid-out `struct Manager`, `vehicleUnk570`, etc.) didn't move. There is no signature scan step to redo unless a hash disappears.
- `Signatures.hpp` and the `/// @hash` / `/// @pattern` comments are documentation and zoltan input only; the build does not consume them.
- The plugin declares `RED4EXT_RUNTIME_INDEPENDENT`, so RED4ext will load it on any game version; a mismatch shows up as a crash or a failed `Attach` loop (`FlightModule.hpp` retries forever - watch `red4ext/logs/let_there_be_flight.log` for "trying again").

## Dependencies (submodules) and who owns them

| Submodule | Source | Notes |
|---|---|---|
| `deps/red4ext.sdk` | jackhumbert/RED4ext.SDK, branch `new-types` | Our fork: ~264 commits ahead of WopsS master (hand-written vehicle/physics/effects/world types, ~400 files). Upstream is 34 commits ahead of us, including patch 2.31 support, the SDK 1.0.0 / API V1 promotion, `CBaseRTTIType` -> `rtti::IType` rename, container rewrites. |
| `deps/red_lib` | jackhumbert/cp2077-red-lib, branch `jack` | Fork of psiberx. Upstream has "Support RED4ext.SDK V1" commits; our branch is 5 ahead / 55 behind. |
| `deps/cyberpunk_cmake` | jackhumbert/cyberpunk_cmake | Ours. |
| `deps/input_loader` | jackhumbert/cyberpunk2077-input-loader | Ours. Built from source in-tree (`add_subdirectory`), shares the parent's SDK target. Pin is 3 behind its main (2.30 xml updates). |
| `deps/mod_settings` | jackhumbert/mod_settings | Ours. Built in-tree for its redscript API. Pin is 8 behind its main (2.3 fixes). |
| `deps/archive_xl`, `deps/tweak_xl` | psiberx | Used for their C++ API headers only (`ArchiveXL.hpp`, `TweakXL.hpp`). |
| `deps/spdlog`, `deps/detours` | upstream | Stable. |

Runtime version floors are in two places and must agree: `Main.cpp` (`HasDependency` semvers, shown in the error popup) and `requirements.md` / `MOD_REQUIREMENTS`.

## Updating for a new game patch (checklist)

1. Confirm targets: game version (`Cyberpunk2077.exe` FileVersion), and the RED4ext / ArchiveXL / TweakXL / Codeware / redscript / Input Loader / Mod Settings releases that support it. Install them into the game dir first.
2. Update `deps/red4ext.sdk`: prefer merging upstream master into `new-types` (keeps the fork's custom types). Expect conflicts in `Api/`, `RTTITypes`, containers, and generated `Natives`. If the V1 API break is too large for one sitting, cherry-pick only the patch-support commit (e.g. `3ccf163b2` for 2.31) as a stopgap and note it in `todo.md`.
3. Update `deps/red_lib` (`jack` branch) to match the SDK API level.
4. Update `deps/input_loader` and `deps/mod_settings` to their latest main (they build in-tree against the same SDK, so they must compile with it).
5. Update `deps/archive_xl` / `deps/tweak_xl` to the tagged release you require; bump the floors in `Main.cpp` and root `CMakeLists.txt`.
6. Build RelWithDebInfo, `cmake --install`, launch the game, check `red4ext/logs/let_there_be_flight.log` for every hook attaching and no "trying again" loops. Test: activate flight in a car, a 6-wheel, a bike; camera; weapons; thruster detach; audio; mod settings UI.
   - Unattended smoke test: `Cyberpunk2077.exe --launcher-skip -skipStartScreen` from `bin/x64`, then poll the LTBF log for `[FlightSystem] OnInitialize` (main menu reached; took ~2 min on 2026-09-13, first launch after a patch can trip the engine's 120 s watchdog and crash once). To see where a stuck main thread is: `cdb -pv -p <pid> -y "srv*;<plugin dir>" -c "~0k 30;q"` (cdb from the Windows 10 SDK debuggers; the plugin `.pdb` is installed next to the dll). Kill any leftover `scc.exe` afterwards or the next redscript install will fail with access denied.
   - To rule the plugin in or out, set `ignored = ["let_there_be_flight"]` in `red4ext/config.ini` and relaunch.
   - In-game testing without a person at the keyboard (set up 2026-10-25), see "Driving the game" below.

## Driving the game (in-game tests with screenshots)

- `r6/scripts/autocontinue.reds` loads the newest save on launch. `ManualSave-178` (2026-10-25) has V in the Mizutani Shion on a street in the city center (-1842, -891, 8); a newer save becomes the autoload.
- `tools/cet.py run "<lua>"` (or `run -f file.lua`) runs Lua in the game through the `claude_bridge` CET mod (see "Running Lua in the live game"). `tools/cet.py wait --timeout 180` blocks until the game has loaded. Lua can call any redscript, including LTBF's (`FlightSystem.GetInstance().playerComponent`).
- `tools/cet/state.lua`: one JSON line with vehicle, position, velocity, flight active and mode. `tools/cet/mount_nearest.lua` with `MATCH = 'kusanagi'` prepended mounts the nearest matching vehicle within 40 m.
- `tools/sendkey.ps1 -Keys o` / `-Keys space -HoldMs 3000`: hardware-scancode key presses to the game window (the game ignores virtual keys). LTBF keys: `O` toggle flight, `Space`/`Left Ctrl` lift, `W`/`S` thrust, `A`/`D` yaw. A load ends on "press Space to continue".
- `tools/screenshot.ps1 -Out x.png`: captures the game window (Borderless Windowed, 3840x2160) downscaled to 1600 wide.
- Clicking menus: `tools/sendkey.ps1 -X <x> -Y <y>` clicks at screenshot coordinates. The game keeps its own menu cursor and only re-reads the OS cursor on mouse input, so the script does SetCursorPos, a 1 px relative nudge, then the click. `tools/sendkey.ps1 -Keys lmb` / `rmb` press mouse buttons (vehicle weapons), `-MoveX/-MoveY` send raw relative moves.
- Finding widgets: `tools/cet/ui.lua` (prepend to a cet.py chunk) has `UI.dump(layer, filter)` (path, class, text of every visible widget on `inkMenuLayer` / `inkHUDLayer`) and `UI.cursor(layer, text)` (ink `SetCursorOverWidget`; works on the pause menu, ignored inside Mod Settings).
- Mod Settings by hand: Esc, `UI.cursor('inkMenuLayer','mod settings')` + `-Keys lmb`, then click the mod's name by coordinates. LTBF's page: two sliders show raw keys (`UI-Settings-Hover-And-Fly-Height-Dampening` / `-Correction`), missing from the localization json (pre-2.5).
- Gamepad: `tools/gamepad.py serve` (run in the background, set `PYTHONUSERBASE=E:\packages\python-user`) plugs in a virtual Xbox 360 pad through the ViGEmBus driver (installed 2026-10-25 via `pip install --user vgamepad`); then `tools/gamepad.py "tap l3 100; wait 500; hold l3 1300; stick l 0.5 0 400"`. The game takes it as a real controller (prompts switch to pad glyphs, `PlayerLastUsedKBM()` false). Start the server before sending; killing it unplugs the pad. On the world map, pad mode pans the map under a fixed reticle: `stick l` to a pin, `tap a` to pin/track it (reliable, unlike mouse right-click there).
- Autodrive needs a tracked destination and a holstered weapon (B in the car draws V's sidearm). `AutoDriveSystem` via `Game.GetScriptableSystemsContainer():Get(CName.new("AutoDriveSystem"))`: `GetAutodriveEnabled()`, `GetFreeRoamEnabled()`, `GetAutodriveAvailable()`.
- Armed vehicles: `Alt` draws the vehicle weapons (first time shows a tutorial, `C` closes it), then `-Keys lmb` fires.
- Summon a player vehicle: `Game.GetVehicleSystem():SpawnPlayerVehicle(gamedataVehicleType.Car, TweakDBID.new("Vehicle.v_standard3_militech_hellhound_player"), false)`. It returns false but the vehicle drives up within ~15 s; summoned vehicles are not in the save. V owns the Kusanagi (bike), Type-66 and Hellhound (armed 6-wheeler).
- Make a new save: `Game.GetInkSystem():GetLayer(CName.new("inkHUDLayer")):GetGameController():GetSystemRequestsHandler():ManualSave("ManualSave-")` (a new slot; never quicksave over Jack's). `LoadLastCheckpoint` from the HUD does nothing; restart the game to reload.
- Gotchas: teleporting a vehicle while flight is active is undone by the flight physics; exit with `F` did not work from the bridge session; kill `scc.exe` before reinstalling.
- First run (2026-10-25, game 2.5): car, bike and Hellhound all toggle flight, climb ~20 m/s, hover, yaw and thrust (~23 m/s).
7. Update `readme.md` dependency versions, `requirements.md` (regenerated by configure), and `todo.md`.

## Cutting a release

Releases are fully automated by `.github/workflows/release.yaml` on a `v*.*.*` tag push (windows-latest, Ninja, RelWithDebInfo, git-cliff changelog from conventional commits + `requirements.md`, uploads `let_there_be_flight_vX.Y.Z.zip` and `_pdb.zip`, release name `vX.Y.Z for <game version>`). Tags with `-` or `_` are marked prerelease.

```
git tag vX.Y.Z && git push origin main vX.Y.Z
```

- Before tagging: submodule pins committed and pushed to their own repos (CI clones with `submodules: true` over HTTPS; every `.gitmodules` URL must be publicly clonable). `build.yaml` on the push must be green.
- Checkout uses `fetch-depth: 0`; with a shallow clone git-cliff sees no history and the release body has no Changes section (every release through v0.3.18 was hand-edited for that reason).
- Commit messages feed git-cliff (`cliff.toml`): use `feat:` / `fix:` prefixes for anything that should appear in the changelog. `chore:` and `ci:` are skipped.
- Release-note history: v0.3.17 (2025-08-29) "for 2.30+" - disabled thruster customizations (crystal coat crash), redscript 1.0 update, Limited HUD incompatibility.
- Nexus Mods: manual upload of the same zip today. See "Nexus" below.

## Nexus Mods

Mod page: https://www.nexusmods.com/cyberpunk2077/mods/5208 (mod ID 5208). Uploads have been manual so far.

Nexus now has an official Upload API (REST v3, open beta since 2026-03; personal API key in an `apikey` header, no cookies). Two supported ways to automate:

- GitHub Action `Nexus-Mods/upload-action` (pin a tag, currently `v1.0.0-beta.10`). Inputs: `api_key`, `file_id` (the v3 mod-file chain id from the Files tab "Advanced/API Info"), `filename`, `version`, optional `mod_id` + `changelog`, `update_mod_version`, `archive_existing_version`. Reference workflow: https://github.com/Nexus-Mods/API-Example.
- CLI for local use: `dotnet tool install -g BUTR.NexusUploader` then `unex upload 5208 <zip> -v <ver> --file-id <id>` (v4 uses the official API).

Implemented in `release.yaml`: after the GitHub release step, `Nexus-Mods/upload-action` uploads `let_there_be_flight_vX.Y.Z.zip` as a new version of file id 2617843 (the main LTBF file; public id, hardcoded) with the git-cliff changelog, and bumps the mod version. The changelog call needs the v3 global mod id, not the site's game-scoped 5208, so a step resolves it via `GET /v3/games/cyberpunk2077/mods/5208` first. The action creates the file version before posting the changelog, so a changelog failure still leaves the file uploaded (happened on v0.3.18). It runs only for non-prerelease tags and only when the repo secret `NEXUSMODS_API_KEY` is set (https://www.nexusmods.com/settings/api-keys). Creating a new mod page is still manual.

Manual API use (key in the `NEXUSMODS_API_KEY` user environment variable; read it with `[Environment]::GetEnvironmentVariable('NEXUSMODS_API_KEY','User')`, send as an `apikey` header, base `https://api.nexusmods.com/v3`; schema in `openapi.yaml` at the repo root, untracked):
- v3 mod id for 5208 is `14315126002776` (`GET /games/cyberpunk2077/mods/5208`).
- `GET /mod-files/2617843/versions` lists uploaded versions; `POST /mods/14315126002776/changelogs` with `{version, changelog}` appends notes (additive, cannot edit); `PUT /mod-files/2617843` with `{name}` renames the persistent file entry.
- 2026-09-13: the 0.3.18 changelog was posted this way after the action's changelog step failed on the wrong mod id.

Reading Nexus comments: the Posts tab and forums are Cloudflare-gated (plain HTTP, reader proxies, and the v2 GraphQL `searchComments` all fail), so `python tools/nexus_comments.py --mod 5208 --since 2025-07-01` drives a real Chrome over the DevTools protocol (needs `pip install websocket-client`): it opens the Posts tab, reads the thread id, and pages the comment endpoint with in-page XHR into a JSON file (author, date, text, replies). Thread ids: LTBF 12009388, Mod Settings 11748838, In-World Navigation 11517568, Input Loader 11989233. Roughly 10 top-level comments per page; LTBF had 4,833 comments total on 2026-09-13.

## Current state (2026-09-13)

Game is 2.31 (2025-09-11, still the latest PC patch). Last release v0.3.17 (2025-08-29) targets 2.30, so it predates 2.31. The Steam install has game 2.31 but RED4ext 1.28.0 / ArchiveXL 1.24 / TweakXL 1.11.0 / Codeware 1.17 from July 2025, so mods do not currently load locally; install the current ones before testing.

| Dependency | We pin / require | Current upstream | Requires |
|---|---|---|---|
| RED4ext | 1.27.0+ | 1.30.0 (2026-03-09) | supports 2.31; API v1 loader |
| RED4ext.SDK | fork `new-types` @ 2025-08-29, API v0, `RUNTIME_LATEST` = 2.30 | 1.0.0 (2026-03-09), master 2026-07-30, API v1, runtime 2.31 | see migration notes |
| ArchiveXL | 1.23.0+ / submodule v1.11.3 | 1.27.3 (2026-09-07) | RED4ext 1.29+, redscript 0.5.31+ |
| TweakXL | 1.10.0+ / submodule v1.7.0 | 1.11.4 (2026-07-28) | RED4ext 1.28+ |
| Codeware | headers only | 1.20.3 (2026-05-03) | RED4ext 1.29+, redscript 0.5.31+ |
| redscript | 0.5.28+ | 0.5.31 stable (2025-08-28); 1.0 still preview only | |
| Input Loader | 0.1.1+ / submodule v0.2.2 | v0.2.3 (2025-07-19) | |
| Mod Settings | 0.2.11+ / submodule v0.2.17 | v0.2.21 (2025-07-19) | |
| red_lib | fork `jack` | psiberx master 2026-08-09 has "Support RED4ext.SDK V1" | |

SDK migration notes (only needed once we move the fork past upstream 2026-03-08): `PluginInfo`/`PluginHandle`/`EMainReason`/`Sdk` move to `RED4ext::v1::`; `RED4EXT_SEMVER` -> `RED4EXT_V1_SEMVER`; `RED4EXT_API_VERSION_LATEST` -> `RED4EXT_API_VERSION_1`; `RED4EXT_SDK_LATEST` -> `RED4EXT_V1_SDK_VERSION_CURRENT`; `CBaseRTTIType` -> `rtti::IType` (4 files here); container rewrites (DynArray in 14 files, HashMap in 6). Report `RED4EXT_API_VERSION_1_COMPAT_0` to stay loadable on RED4ext 1.29.x. `input_loader`, `mod_settings` and `red_lib` all build against the same SDK target in-tree, so they migrate in the same step.

## Plan for the next release (v0.3.18 for 2.31)

Two-stage, ship the small one first:

1. Stopgap (target: v0.3.18). Cherry-pick upstream `3ccf163b2` "Add support for patch 2.31" onto the SDK fork `new-types` (9 files: runtime define, 3 generated natives, `Natives.cpp`). Bump `RED4EXT_V0_RUNTIME_LATEST` to 2.31. Bump `input_loader`/`mod_settings` pins to their current main. Raise floors: RED4ext 1.29.0+, ArchiveXL 1.27.0+, TweakXL 1.11.0+, redscript 0.5.31+ (in `Main.cpp` and root `CMakeLists.txt`). Build, install, play-test on 2.31, verify every hook attaches. Tag, let CI release, upload to Nexus.
2. Full SDK sync (target: v0.4.0). Merge upstream master into `new-types`, do the v1 migration above across LTBF, red_lib `jack`, input_loader, mod_settings, and update the ArchiveXL/TweakXL submodules to current tags. Then retire the stale hand-written types in the fork where upstream now has generated equivalents.

## Reverse engineering a new game binary

- Standard location: a flat folder next to the game install, `C:\Program Files (x86)\Steam\steamapps\common\Cyberpunk 2077 <version>\` (e.g. `... 2.3`, `... 2.21`, `... 1.63hf1`), holding `Cyberpunk2077.exe`, `Cyberpunk2077.exe.i64`, `cyberpunk2077_addresses.json` (copied from `bin/x64`), and generated `Cyberpunk2077.exe.pdb`/`.map`. Every past version since 1.3.1 is there.
- First check, before any IDA work: `python tools/check_hashes.py [--deps]` lists every RED4ext address hash the plugin (and mod_settings/input_loader with `--deps`) uses and looks it up in the shipped `bin/x64/cyberpunk2077_addresses.json`. A hash that is present resolves at runtime; only missing hashes or changed struct layouts need IDA.
- IDA access goes through the ida-router MCP (`D:\Code\iRacing\car-dynamics\tools\ida-router`, wired in this repo's `.mcp.json`). The Cyberpunk entry in `D:\Code\iRacing\car-dynamics\tools\ida-ports.json` is `Cyberpunk2077.exe` on port 8772, alias `binary="cp2077"`; point its `i64_path` at the current version's folder. Tools: `ida_search_names`, `ida_get_pseudocode`, `ida_list_xrefs`, etc., all take `binary=`.
- Fresh analysis of the 60 MB exe in IDA Freeware 8.3 takes hours; launch `ida64.exe <exe>` with `IDA_MCP_PORT=8772` and `IDA_MCP_AUTOSTART=1` set so the MCP plugin comes up with it, press Enter on the "Load a new file" dialog, and wait for the title bar to drop "Loading" and the auto-analysis indicator to go idle. Old IDBs are ~1.1 GB.
- The 2.31 IDB lives in `...\steamapps\common\Cyberpunk 2077 2.31\` (moved there 2026-09-13; `ida-ports.json` points at it). If the `ida` MCP server is not wired into the session (no `mcp__ida__*` tools), call the running backend over HTTP instead: `sys.path.insert(0, r"D:\Code\iRacing\car-dynamics	ools\ida-router"); from backend import Backend; Backend(alias="cp2077", basename="Cyberpunk2077.exe", port=8772).call_tool("ida_get_pseudocode", {"address": "0x14..."})`. Bring IDA up with `D:\Code\iRacing\ida-plugin	ools\ida_session\launch_freeware.ps1 -Map "8772=<i64 path>"` (sets `IDA_MCP_PORT`/`IDA_MCP_AUTOSTART` for you); the port listens about 20 s after launch. Subagents get the same recipe since MCP tools do not propagate to them.
- Older RE assets: `C:\Users\Jack\Documents\cyberpunk\IDA_Cyberpunk_2077` (RTTI dumper, hash lists, `Cyberpunk2077.sig_strings.demangled.txt`), and the RED4ext address hashes used by our hooks are the ones RED4ext resolves from its own address DB, so a missing hash after a patch means RED4ext itself dropped it.

## Conventions

- No em-dashes anywhere (prose, code, commits).
- Don't commit `build/`, `game_dir*/`, `compile_commands.json`, or the `src/wolvenkit/.projectFiles` churn unless the archive actually changed.
- Working tree normally has a dirty `deps/input_loader` (its `requirements.md` is regenerated by configure); ignore it.
