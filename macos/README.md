# Let There Be Flight for macOS (Apple Silicon)

A port of [jackhumbert/let_there_be_flight](https://github.com/jackhumbert/let_there_be_flight) v0.3.18 to the native macOS build of
Cyberpunk 2077 (2.3.1, Steam, Apple Silicon). Port by codeman101 (skyman3215). MIT, like upstream.

Everything macOS-specific is in this folder and in `#ifdef __APPLE__` blocks in `src/red4ext`. The Windows build is meant to be
unchanged: `WINDOWS_CHECK.md` lists every shared file whose Windows view (`unifdef -U__APPLE__`) differs from v0.3.18 and why each
difference is equivalent. What is not done yet: `NOT_YET.md`. Where every address comes from: `ADDRESS_SURVEY.md`.

## Status

@@STATUS@@

## What you need to run it

- Cyberpunk 2077 2.3.1, the native macOS build (Steam), on Apple Silicon.
- A RED4ext loader for macOS that installs the plugin's hooks and calls `ltbf_registerNatives` after every plugin has loaded (at
  script initialisation). The port was developed on a loader setup that does this from a Frida gadget script, like the macOS
  ports of TweakXL, ArchiveXL and Codeware it depends on. jackmaxwil's public
  [RED4ext-macos](https://github.com/jackmaxwil/RED4ext-macos) registers natives through the game's post-register callback
  instead; support for it is planned (see `NOT_YET.md`).
- The macOS ports of TweakXL, ArchiveXL and [Codeware](https://github.com/codeman101/cp2077-codeware/tree/macos) (Let There Be
  Flight shares two hooks with Codeware through `codeware_mac_hook_chain_attach`).
- Optional: [Mod Settings for macOS](https://github.com/codeman101/mod_settings/tree/macos), for the settings menu.
- risner's Mac Input Loader, for the key bindings (`r6/input/*.xml` merged into `r6/cache/input*.xml`).
- The FMOD Engine 2.04 for macOS (free from fmod.com). FMOD is not in this repository and must never be committed.

## Build

```
cmake -S macos -B macos/build -DCMAKE_BUILD_TYPE=Release
cmake --build macos/build -j8
```

Needs the Xcode command line tools, CMake 3.21+, Homebrew `fmt` and `spdlog`, Python 3, and these checkouts next to this repository
(override with `-DRED4EXT_SDK_DIR=`, `-DCODEWARE_DIR=`, `-DFMOD_DIR=`):

- `../../RED4ext.SDK-macos`: the macOS RED4ext SDK;
- `../../cp2077-codeware`: the macOS Codeware port (its RedLib and its SDK address rows are reused);
- `../fmod-2.04.00`: `core/` and `studio/` (each with `inc` and `lib`), copied from `api/core` and `api/studio` of FMOD's
  `fmodstudioapi20400mac-installer.dmg`. The 2.03 banks of the Windows release load in 2.04.

`-DLTBF_MAC_NO_AUDIO=ON` builds without FMOD: the FMOD calls go to `macos/fmod-stub` (silent stand-ins) and nothing is linked.

After the link, `fix-install-names.sh` points the fmt, spdlog and FMOD dependencies at `@loader_path`, adds `@loader_path` as an rpath
and signs ad hoc; `check-undefined.sh` checks the exports (`Main`, `Query`, `Supports`, `ltbf_registerNatives`) and that no symbol is left
for dynamic lookup. Offline tests: `macos/tests/run.sh`.

## Install layout

`red4ext/plugins/let_there_be_flight/`: `let_there_be_flight.dylib`, `libfmt.12.dylib` and `libspdlog.1.17.dylib` (Homebrew),
`libfmod.dylib` and `libfmodstudio.dylib` (FMOD 2.04), and from the Windows release zip: `base_sounds.bank`,
`base_sounds.strings.bank`, `let_there_be_flight.archive`, `let_there_be_flight.archive.xl`, `let_there_be_flight.tweak`,
`let_there_be_flight.yaml`, `license.md`, `Scripts/packed.reds`, `Scripts/module.reds`.

- The scripts are compiled into the script cache by the loader setup (on macOS they are not added through RED4ext).
- The archive goes to `archive/Mac/mod/`.
- Key bindings: copy the release's `inputs.xml` to `r6/input/` under a name the plugin does not delete at start (for example
  `ltbf-mac.xml`) and **remove the `overridableUI="..."` attributes** from it. A binding with that attribute is bound through a settings
  entry of the same name in `r6/config/settings/platform/mac/options.json`; the Windows Input Loader plugin adds those entries for mods,
  the Mac input loader only merges the XML. With the attribute the flight keys are never bound; without it they work, but they cannot be
  changed in the Controls menu.
- Do not name it `r6/input/let_there_be_flight.xml`, and do not use `r6/scripts/let_there_be_flight/` or
  `r6/tweaks/let_there_be_flight.yaml`: the plugin deletes those at start (Windows behaviour, unchanged).

## How the macOS build differs

- **SDK.** `CMakeLists.txt` generates `build/sdk-overlay`: the macOS SDK's address table plus Codeware's `macos/MacSdkRows.inc` and this
  plugin's `LtbfSdkRows.inc` (an unknown hash resolves to a logged fail-safe stub, `MacShims.cpp`); the fork's `UniRelocFunc` /
  `IHookable`; small fork helpers inserted into a few core SDK headers (`gen_sdk_overlay.py`). `sdk-overlay/` holds only the
  jackhumbert SDK fork headers this plugin includes, each marked where it changed for macOS.
- **Layouts.** The fork's vehicle physics structs do not match the macOS game; `MacLayout.hpp` has the macOS offsets used (physics parent,
  drive helpers, wheels, bike fields, camera fields, air drag). `stdafx.hpp` turns the SDK's `RED4EXT_ASSERT_OFFSET` back on for this
  plugin's own classes (the macOS SDK compiles it to nothing), so every script-visible offset is checked at compile time.
- **ABI.** Results that are not trivially copyable come back through x8 on arm64 (`IsSavingLocked`, the component's persistent state);
  the overrides use the macOS signatures. `libc++`'s `std::unordered_map` is smaller than MSVC's, so `FlightSystem` pads it to keep the
  script offsets.
- **Memory.** `MacVault.cpp` serves every game pool the plugin allocates from; instances are allocated from the allocator the game frees
  them to.
- **Script properties.** The scripts' `@runtimeProperty("offset", ...)` fields include properties the game class already reflects. On
  macOS the ProcessScriptTypes hook does not add a second property with the same name; the script binds to the game's own property (its
  offset is the right one on macOS).
- **Shims** (`MacShims.cpp`): image base, plugin and game folders through `dladdr`, logging, the dependency check, the hook policy (one
  attempt and a log line; Codeware's chain for the two shared targets), base-class calls through the macOS vtables.
- **Entry points.** `Main(Load)` checks the dependencies and attaches the hooks no other plugin uses; `ltbf_registerNatives` registers the
  types, tweaks and archive, loads FMOD and attaches the shared hooks.

## Diagnostics

| variable | effect |
|---|---|
| `LTBF_MAC_DIAG=1` | flight component lifecycle, per-second flight forces and the game's resistance pass while flying |
| `LTBF_MAC_NOHOOKS=all` or `<name,...>` | leave hooks out (bisecting a crash; ProcessScriptTypes and GetGameSystemsData are needed for the scripts to load) |
| `LTBF_MAC_SKIP=thrusters,config,hud,component` | skip parts of the per-vehicle setup |
| `LTBF_MAC_TEST_TWEAK=<file>` | register one extra tweak file at start |

## Credits

Let There Be Flight and the SDK fork it builds on are by jackhumbert (MIT). The macOS RED4ext SDK and loader are by jackmaxwil (MIT).
