# Let There Be Flight (LTBF)

Cyberpunk 2077 mod: flight for all vehicles. RED4ext C++ plugin + redscript + TweakXL tweaks + ArchiveXL archive + FMOD audio banks. Everything ships under `red4ext/plugins/let_there_be_flight/` (scripts, tweaks, archive, inputs are all loaded from the plugin folder, not r6/archive dirs).

## Layout

- `src/red4ext/` - the plugin. `Main.cpp` = entry, dependency version checks, registers scripts/tweaks/archive/inputs with the other plugins. `Hooks/` = one game-function hook per file. `Utils/FlightModule.hpp` = hook registry macros.
- `src/redscript/` - 71 `.reds` files, packed by CMake into `packed.reds` + `module.reds`.
- `src/tweaks/` - `.tweak` (native) + `.yaml` tweaks, packed into one file each.
- `src/wolvenkit/` - Wolvenkit project. The built `packed/archive/pc/mod/let_there_be_flight.archive` is committed; CMake just copies it. Rebuild in Wolvenkit only when the archive content changes.
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

- Game dir is read from the `CYBERPUNK_2077_GAME_DIR` cache var (currently the Steam install). Game version for the `.rc`/badge comes from `Cyberpunk2077.exe` FileVersion via `deps/red4ext.sdk/cmake/GetGameVersion.cmake`; in CI there is no exe so it falls back to the SDK's `RED4EXT_RUNTIME_LATEST` macro. Keep the fork's `Api/v0/Runtime.hpp` `RUNTIME_LATEST` pointing at the current patch or the release name/badge will be wrong.
- Mod version comes from the latest git tag (`ConfigureVersionFromGit`). Untagged local builds get `+<branch>.<n>.<sha>` metadata (CI builds drop it).
- `game_dir_requirements/` is NOT safe to copy wholesale into the game: `FindRED4ext`/`FindArchiveXL`/`FindTweakXL` unpack whatever release zip is cached in `build/downloads/` (fetched once, then never refreshed unless `-DMOD_FORCE_UPDATE_DEPS=ON`), so it can hold years-old RED4ext/ArchiveXL/TweakXL. Do not use it for local testing at all: the in-tree `input_loader.dll` / `mod_settings.dll` report wrong plugin versions (input_loader 0.1.1 from its hardcoded project version, mod_settings the parent's LTBF version), so `Main.cpp`'s dependency check rejects them. Install the real Input Loader / Mod Settings release zips into the game instead.
- `configure_release` regenerates `requirements.md` from `MOD_REQUIREMENTS` (own entries in root `CMakeLists.txt`, plus whatever ArchiveXL/TweakXL/RED4ext Find modules append). It is committed and appended to release notes.
- Redscript is compiled/linted against `deps/mod_settings/redscript` too (`.redscript-ide`).
- Dead config, safe to ignore: `src/red4ext/CMakeLists.txt` and `cmake/FindCodeware.cmake` are never included (the plugin target is built by `configure_red4ext` globbing the dir); the `src/redscript/codeware` submodule in `.gitmodules` is not checked out and not needed. Nothing in the plugin includes Codeware headers.
- `ZOLTAN_CLANG_EXE` is hardcoded in root `CMakeLists.txt` to a local zoltan build; `configure_red4ext_addresses` is commented out, so zoltan does not run in the normal build.

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
7. Update `readme.md` dependency versions, `requirements.md` (regenerated by configure), and `todo.md`.

## Cutting a release

Releases are fully automated by `.github/workflows/release.yaml` on a `v*.*.*` tag push (windows-latest, Ninja, RelWithDebInfo, git-cliff changelog from conventional commits + `requirements.md`, uploads `let_there_be_flight_vX.Y.Z.zip` and `_pdb.zip`, release name `vX.Y.Z for <game version>`). Tags with `-` or `_` are marked prerelease.

```
git tag vX.Y.Z && git push origin main vX.Y.Z
```

- Before tagging: submodule pins committed and pushed to their own repos (CI clones with `submodules: true` over HTTPS; every `.gitmodules` URL must be publicly clonable). `build.yaml` on the push must be green.
- Commit messages feed git-cliff (`cliff.toml`): use `feat:` / `fix:` prefixes for anything that should appear in the changelog. `chore:` and `ci:` are skipped.
- Release-note history: v0.3.17 (2025-08-29) "for 2.30+" - disabled thruster customizations (crystal coat crash), redscript 1.0 update, Limited HUD incompatibility.
- Nexus Mods: manual upload of the same zip today. See "Nexus" below.

## Nexus Mods

Mod page: https://www.nexusmods.com/cyberpunk2077/mods/5208 (mod ID 5208). Uploads have been manual so far.

Nexus now has an official Upload API (REST v3, open beta since 2026-03; personal API key in an `apikey` header, no cookies). Two supported ways to automate:

- GitHub Action `Nexus-Mods/upload-action` (pin a tag, currently `v1.0.0-beta.10`). Inputs: `api_key`, `file_id` (the v3 mod-file chain id from the Files tab "Advanced/API Info"), `filename`, `version`, optional `mod_id` + `changelog`, `update_mod_version`, `archive_existing_version`. Reference workflow: https://github.com/Nexus-Mods/API-Example.
- CLI for local use: `dotnet tool install -g BUTR.NexusUploader` then `unex upload 5208 <zip> -v <ver> --file-id <id>` (v4 uses the official API).

Plan: add a `nexus` job to `release.yaml` after the GitHub release step, gated on a `NEXUS_API_KEY` repo secret, uploading the same `let_there_be_flight_vX.Y.Z.zip` with the git-cliff changelog. Get the key at https://www.nexusmods.com/settings/api-keys. Creating a new mod page is still manual; only new files/versions on the existing page are supported.

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
- 2026-09-13 note: the 2.31 analysis was started in `E:\Cyberpunk_RE\2.31\` by mistake. Once IDA finishes and is closed, move the exe, `.i64`, and `addresses.json` to `...\steamapps\common\Cyberpunk 2077 2.31\` and update `i64_path` in `ida-ports.json`, then remove `E:\Cyberpunk_RE`.
- Older RE assets: `C:\Users\Jack\Documents\cyberpunk\IDA_Cyberpunk_2077` (RTTI dumper, hash lists, `Cyberpunk2077.sig_strings.demangled.txt`), and the RED4ext address hashes used by our hooks are the ones RED4ext resolves from its own address DB, so a missing hash after a patch means RED4ext itself dropped it.

## Conventions

- No em-dashes anywhere (prose, code, commits).
- Don't commit `build/`, `game_dir*/`, `compile_commands.json`, or the `src/wolvenkit/.projectFiles` churn unless the archive actually changed.
- Working tree normally has a dirty `deps/input_loader` (its `requirements.md` is regenerated by configure); ignore it.
