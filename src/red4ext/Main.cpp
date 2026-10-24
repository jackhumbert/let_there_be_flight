#include "Audio/FlightAudio.hpp"
#include "Audio/FmodHelper.hpp"
#include "Utils/Utils.hpp"
#include "Utils/FlightModule.hpp"
#include "stdafx.hpp"
#include <InputLoader.hpp>
#include <RED4ext/Api/Sdk.hpp>
#include <RED4ext/InstanceType.hpp>
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Version.hpp>
#include <RedLib.hpp>
#include <iostream>
#include <CyberpunkMod.hpp>

#include <ArchiveXL.hpp>
#include <TweakXL.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#ifdef __APPLE__
#include "MacShims.hpp"
#else
#include <libloaderapi.h>
#include <winbase.h>
#endif

using Query_t = void (*)(RED4ext::PluginInfo *);

enum EDependencyStatus {
  FoundWithCorrectVersion = 0,
  FoundWithIncorrectVersion = 1,
  FoundCouldNotBeQueried = 2,
  FoundNoVersion = 3,
  NotFound = 4
};

#ifdef __APPLE__
// LTBF R41/R42a: an already loaded red4ext/plugins/<name>/<name>.dylib and its Query() (macos/MacShims.cpp). RED4ext-macos loads
// the plugins in folder order and runs each Main right away, so at Main a dependency can be present but not loaded yet.
static const auto kArchiveXLVersion = RED4EXT_SEMVER(1, 27, 0);
static const auto kTweakXLVersion = RED4EXT_SEMVER(1, 11, 0);
static const auto kModSettingsVersion = RED4EXT_SEMVER(0, 2, 21);

enum : int {
  kDepArchiveXL = 1,  // ArchiveXL 1.27+
  kDepTweakXL = 2,    // TweakXL 1.11+
  kDepModSettings = 4 // Mod Settings 0.2.21+ (optional)
};

// aLate = false (Main): a dependency that is present but not loaded yet counts as found.
// aLate = true (ltbf_registerNatives, every plugin is loaded): only a loaded dependency with a good version counts.
// Returns the kDep* bits of the dependencies that count as found. Exported for the offline test (macos/tests/deps_test.cpp).
extern "C" __attribute__((visibility("default"))) int ltbf_mac_check_dependencies(int aLate) {
  const bool allowNotLoaded = aLate == 0;
  auto good = [&](LtbfMac::DepStatus aStatus) {
    return aStatus == LtbfMac::DepStatus::FoundWithCorrectVersion ||
           (allowNotLoaded && aStatus == LtbfMac::DepStatus::PresentNotLoaded);
  };
  int found = 0;
  if (good(LtbfMac::HasDependency("ArchiveXL", kArchiveXLVersion, allowNotLoaded)))
    found |= kDepArchiveXL;
  if (good(LtbfMac::HasDependency("TweakXL", kTweakXLVersion, allowNotLoaded)))
    found |= kDepTweakXL;
  if (good(LtbfMac::HasDependency("mod_settings", kModSettingsVersion, allowNotLoaded)))
    found |= kDepModSettings;
  return found;
}
#else
EDependencyStatus HasDependency(const wchar_t *name, RED4ext::SemVer minVersion) {
  Query_t query;
  RED4ext::PluginInfo pluginInfo;
  auto handle = GetModuleHandle(name);
  if (!handle) {
    SetDllDirectory((Utils::GetRootDir() / "red4ext" / "plugins" / name).c_str());
    handle = LoadLibrary(name);
  }
  if (handle) {
    if (minVersion > RED4EXT_SEMVER(0, 0, 0)) {
      query = reinterpret_cast<Query_t>(GetProcAddress(handle, "Query"));
      if (query) {
        query(&pluginInfo);
        if (pluginInfo.version >= minVersion) {
          spdlog::info(L"{} found with version: {}", name, std::to_wstring(pluginInfo.version));
          return FoundWithCorrectVersion;
        } else {
          spdlog::warn(L"{} found, but wrong version: {}", name, std::to_wstring(pluginInfo.version));
          return FoundWithIncorrectVersion;
        }
      } else {
        spdlog::warn(L"{} found, but could not be queried for version", name);
        return FoundCouldNotBeQueried;
      }
    } else {
      spdlog::info(L"{} found", name);
      return FoundNoVersion;
    }
  } else {
    spdlog::warn(L"{} not found", name);
    return NotFound;
  }
}
#endif

RED4EXT_C_EXPORT void RED4EXT_CALL RegisterTypes() {
  spdlog::info("Registering classes & types");
  FlightModuleFactory::GetInstance().RegisterTypes();
}

RED4EXT_C_EXPORT void RED4EXT_CALL PostRegisterTypes() {
  spdlog::info("Registering functions");

  FlightModuleFactory::GetInstance().PostRegisterTypes();

  RED4ext::CRTTISystem::Get()->RegisterScriptName("entBaseCameraComponent", "BaseCameraComponent");

  // auto rtti = RED4ext::CRTTISystem::Get();

  // auto gamePSMVehicleEnum = rtti->GetEnum("gamePSMVehicle");
  // gamePSMVehicleEnum->hashList.PushBack("Flight");
  // gamePSMVehicleEnum->valueList.PushBack(8);
  // gamePSMVehicleEnum->hashList.PushBack("FlightDriverCombat");
  // gamePSMVehicleEnum->valueList.PushBack(9);
}

bool loaded = false;
RED4ext::PluginHandle pluginHandle;

#ifdef __APPLE__
void FlightSettingsMacEnsureMaps(); // FlightSettings.cpp
void FlightSettingsMacDisable(const char *aReason); // FlightSettings.cpp, R42c
extern "C" bool ltbf_vault_selftest(); // macos/MacVault.cpp, R42c
static std::atomic<bool> s_audioLoaded{false};
static bool LtbfMacAudioLoaded() { return s_audioLoaded.load(); }

// LTBF R41 (macOS): the launcher's NATREG step calls <plugin>_registerNatives after every plugin's Main, at the game's InitScripts
// entry (0x3D8C188), before LoadScripts (the loader's Frida gadget script: TweakXL, ArchiveXL, Codeware, Mod Settings, then this
// one). It does here what the RTTI register callbacks and the game-state callbacks do on Windows. Once only; every phase is logged.
extern "C" __attribute__((visibility("default"))) void ltbf_registerNatives() {
  static std::atomic<bool> s_once{false};
  if (s_once.exchange(true)) {
    LtbfMac::Log("[ltbf] ltbf_registerNatives: already ran");
    return;
  }
  const auto *sdk = LtbfMac::PluginSdk();
  const auto handle = LtbfMac::PluginHandleValue();
  if (!loaded || !sdk) {
    LtbfMac::Log("[ltbf] ltbf_registerNatives: Main(Load) did not finish (see the dependency lines above): nothing registered");
    return;
  }
  if (!RED4ext::CRTTISystem::Get()) {
    LtbfMac::Log("[ltbf] ltbf_registerNatives: no RTTI system yet: nothing registered");
    return;
  }

  // R42c. Pool self-test: allocate and free 64 bytes in each of the 7 fixed vault pools (generated pools resolve at first use). If any fails, FlightSettings becomes a logged
  // no-op, so no engine-memory write happens through a missing pool.
  if (ltbf_vault_selftest()) {
    LtbfMac::Log("[ltbf] ltbf_registerNatives 0/5 pool self-test: all pools ok");
  } else {
    LtbfMac::Log("[ltbf] ltbf_registerNatives 0/5 pool self-test FAILED (see the [vault] lines): FlightSettings is off");
    FlightSettingsMacDisable("pool self-test failed");
  }

  // 1. Types: the module RegisterTypes, the Engine RTTI classes, then RedLib's queue (registrations and descriptions).
  LtbfMac::Log("[ltbf] ltbf_registerNatives 1/5 types: %zu Engine registrars, RedLib %zu registrations and %zu descriptions queued",
               Engine::RTTIRegistrar::Count(), Red::TypeInfoRegistrar::PendingRegisterCount(),
               Red::TypeInfoRegistrar::PendingDescribeCount());
  RegisterTypes();
  const auto registered = Engine::RTTIRegistrar::RunRegisterPhase();
  Red::TypeInfoRegistrar::RunPending();

  // 2. Post-registration: the module PostRegisterTypes, the Engine descriptions (functions, properties, expansions).
  PostRegisterTypes();
  const auto built = Engine::RTTIRegistrar::RunBuildPhase();
  FlightSettingsMacEnsureMaps(); // R42a: FlightSettings' value maps exist before any script can set a value
  LtbfMac::Log("[ltbf] ltbf_registerNatives 2/5 types done: %zu Engine registrations, %zu Engine descriptions, RedLib %zu / %zu left",
               registered, built, Red::TypeInfoRegistrar::PendingRegisterCount(), Red::TypeInfoRegistrar::PendingDescribeCount());

  // 3. Tweaks and archive (TweakXL's and ArchiveXL's natives were registered just before by their own registerNatives). Every plugin
  //    is loaded now: the dependencies are checked again (loaded, Query version), and only the registration whose plugin is missing
  //    or too old is skipped. The natives and types above are registered either way, so LTBF's scripts link.
  const int deps = ltbf_mac_check_dependencies(1);
  const char *tweaks = "SKIPPED (TweakXL 1.11+ is not loaded)";
  if (deps & kDepTweakXL) {
    const bool nativeTweaks = TweakXL::RegisterTweak(handle, MOD_PACKED_NATIVE_TWEAKS_FILENAME);
    const bool yamlTweaks = TweakXL::RegisterTweak(handle, MOD_PACKED_YAML_TWEAKS_FILENAME);
    tweaks = nativeTweaks && yamlTweaks ? "ok" : nativeTweaks ? "yaml FAILED" : yamlTweaks ? "native FAILED" : "FAILED";
#ifdef __APPLE__
    // R42f (diagnostics): LTBF_MAC_TEST_TWEAK=<absolute path to a .yaml> registers one extra tweak file, for test boots only (for
    // example player.vehicle.canEnterFlight: false, which keeps every FlightComponent off). Unset = nothing extra.
    if (const char *testTweak = std::getenv("LTBF_MAC_TEST_TWEAK")) {
      LtbfMac::Log("[ltbf] LTBF_MAC_TEST_TWEAK %s: %s", testTweak,
                   TweakXL::RegisterTweak(std::filesystem::path(testTweak)) ? "registered" : "FAILED");
    }
#endif
  }
  // R42b: by NATREG ArchiveXL's depot is already initialized ("Can't register archive ...: depot is already initialized"), so the
  // packaging installs the archive into archive/Mac/mod/, which ArchiveXL mounts itself at startup. Registering is skipped then.
  const char *archive = "SKIPPED (ArchiveXL 1.27+ is not loaded)";
  std::error_code archiveError;
  const auto macArchive = Utils::GetRootDir() / "archive" / "Mac" / "mod" / "let_there_be_flight.archive";
  if (std::filesystem::is_regular_file(macArchive, archiveError)) {
    archive = "in archive/Mac/mod (ArchiveXL mounts it at startup; not registered here)";
  } else if (deps & kDepArchiveXL) {
    archive = ArchiveXL::RegisterArchive(handle, "let_there_be_flight.archive") ? "ok" : "FAILED";
  }
  LtbfMac::Log("[ltbf] ltbf_registerNatives 3/5 tweaks %s, archive %s, Mod Settings %s", tweaks, archive,
               (deps & kDepModSettings) ? "loaded" : "not loaded (optional: the settings menu stays empty)");

  // 4. FMOD (the Windows build loads it when the Initialization game state exits).
  // With LTBF_MAC_NO_AUDIO the FMOD calls go to the no-op stand-in (macos/fmod-stub), so FlightAudio stays callable and silent.
  s_audioLoaded = FlightAudio::Load(nullptr);
#ifdef LTBF_MAC_NO_AUDIO
  LtbfMac::Log("[ltbf] ltbf_registerNatives 4/5 audio: off in this build (LTBF_MAC_NO_AUDIO, silent stand-in)");
#else
  LtbfMac::Log("[ltbf] ltbf_registerNatives 4/5 audio: FMOD %s", s_audioLoaded.load() ? "loaded" : "FAILED");
#endif

  // 5. The hooks on targets Codeware also hooks (Entity_Initialize, ScriptValidator_Validate), through Codeware's chain.
  FlightModuleFactory::GetInstance().LoadShared(sdk, handle);
  LtbfMac::Log("[ltbf] ltbf_registerNatives 5/5 done");
}

// Diagnostics for an offline smoke test (a plain process that dlopens the dylib): what the dylib queued at load.
extern "C" __attribute__((visibility("default"))) void ltbf_mac_pending(unsigned long *aEngine, unsigned long *aRedRegister,
                                                                          unsigned long *aRedDescribe) {
  if (aEngine)
    *aEngine = Engine::RTTIRegistrar::Count();
  if (aRedRegister)
    *aRedRegister = Red::TypeInfoRegistrar::PendingRegisterCount();
  if (aRedDescribe)
    *aRedDescribe = Red::TypeInfoRegistrar::PendingDescribeCount();
}
#endif

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason,
                                        const RED4ext::Sdk *aSdk) {
  switch (aReason) {
  case RED4ext::EMainReason::Load: {
    pluginHandle = aHandle;

    Utils::CreateLogger();
    spdlog::info("Starting up Let There Be Flight {}", MOD_VERSION_STR);
#ifdef __APPLE__
    auto ptr = reinterpret_cast<void *>(LtbfMac::ImageBase());
#else
    auto ptr = GetModuleHandle(nullptr);
#endif
    spdlog::info("Base address: {}", fmt::ptr(ptr));
    auto modPtr = aHandle;
    spdlog::info("Mod address: {}", fmt::ptr(modPtr));

#ifdef __APPLE__
    // LTBF R41: the cleanup of files older Windows releases installed outside the plugin folder is skipped on macOS (no Mac install
    // ever put them there, and the plugin does not delete files in the game folder).
#else
    auto scriptsFolder = Utils::GetRootDir() / "r6" / "scripts" / "let_there_be_flight";
    if (std::filesystem::exists(scriptsFolder)) {
      spdlog::info("Deleting old scripts folder");
      std::filesystem::remove_all(scriptsFolder);
    }
    auto tweaks = Utils::GetRootDir() / "r6" / "tweaks" / "let_there_be_flight.yaml";
    if (std::filesystem::exists(tweaks)) {
      spdlog::info("Deleting old tweaks");
      std::filesystem::remove_all(tweaks);
    }
    auto archive = Utils::GetRootDir() / "archive" / "pc" / "mod" / "let_there_be_flight.archive";
    if (std::filesystem::exists(archive)) {
      spdlog::info("Deleting old archive");
      std::filesystem::remove_all(archive);
    }
    auto archiveXL = Utils::GetRootDir() / "archive" / "pc" / "mod" / "let_there_be_flight.archive.xl";
    if (std::filesystem::exists(archiveXL)) {
      spdlog::info("Deleting old archive.xl");
      std::filesystem::remove_all(archiveXL);
    }
    auto inputXML = Utils::GetRootDir() / "r6" / "input" / "let_there_be_flight.xml";
    if (std::filesystem::exists(inputXML)) {
      spdlog::info("Deleting old xml");
      std::filesystem::remove_all(inputXML);
    }
#endif

    // Input Loader releases (through v0.2.3) report 0.1.1 from Query(), so the floor must stay there
    auto inputLoader_version = RED4EXT_SEMVER(0, 1, 1);
    auto archiveXL_version = RED4EXT_SEMVER(1, 27, 0);
    auto tweakXL_version = RED4EXT_SEMVER(1, 11, 0);
    auto modSettings_version = RED4EXT_SEMVER(0, 2, 21);

#ifdef __APPLE__
    // LTBF R41: Input Loader is not a plugin on macOS (the launcher runs the Perl input loader), so it is not checked.
    // R42a: a dependency later in the folder order is present but not loaded yet; it counts as found here and is checked again
    // (loaded, Query version) in ltbf_registerNatives.
    (void)inputLoader_version;
    (void)archiveXL_version;
    (void)tweakXL_version;
    (void)modSettings_version;
    const int deps = ltbf_mac_check_dependencies(0);
    auto has_inputLoader = true;
    auto has_archiveXL = (deps & kDepArchiveXL) != 0;
    auto has_tweakXL = (deps & kDepTweakXL) != 0;
    // optional: only a present but too old Mod Settings fails, as on Windows; a missing one is fine
    auto has_modSettings =
        (deps & kDepModSettings) != 0 ||
        LtbfMac::HasDependency("mod_settings", kModSettingsVersion, true) != LtbfMac::DepStatus::FoundWithIncorrectVersion;
#else
    auto has_inputLoader = HasDependency(L"input_loader", inputLoader_version) == FoundWithCorrectVersion;
    auto has_archiveXL = HasDependency(L"ArchiveXL", archiveXL_version) == FoundWithCorrectVersion;
    auto has_tweakXL = HasDependency(L"TweakXL", tweakXL_version) == FoundWithCorrectVersion;
    // optional, so just test for incorrect version
    auto has_modSettings = HasDependency(L"mod_settings", modSettings_version) != FoundWithIncorrectVersion;
#endif

    if (!has_inputLoader || !has_archiveXL || !has_tweakXL || !has_modSettings) {
#ifdef __APPLE__
      spdlog::error("Dependencies not met");
      // LTBF R41: no popups on macOS; the log lines say what is missing.
      // R42a: Main never returns false for a dependency on macOS. The offline script cache already contains LTBF's scripts, so an
      // unloaded LTBF guarantees failed script links and a crash. LTBF stays loaded, registers its natives and types in
      // ltbf_registerNatives, and skips only the tweak or archive registration whose plugin is missing there.
      spdlog::error("missing or too old:{}{}{} - Let There Be Flight stays loaded (its scripts are in the offline cache); the tweaks "
                    "or the archive of a missing plugin are skipped in ltbf_registerNatives",
                    has_archiveXL ? "" : " ArchiveXL 1.27+", has_tweakXL ? "" : " TweakXL 1.11+",
                    has_modSettings ? "" : " Mod Settings 0.2.21+");
      LtbfMac::Log("[ltbf] dependencies not met at Main (see above): continuing");
#else
      spdlog::error("Dependencies not met - game will load without Let There Be Flight");
      auto message = fmt::format(
        L"The following Let There Be Flight requirements were not met:\n\n{}{}{}{}\nPlease ensure the mods above are installed/up-to-date.",
                      has_inputLoader ? L"" : L"* Input Loader v" + std::to_wstring(inputLoader_version) + L"+\n",
                      has_archiveXL ? L"" : L"* ArchiveXL v" + std::to_wstring(archiveXL_version) + L"+\n",
                      has_tweakXL ? L"" : L"* TweakXL v" + std::to_wstring(tweakXL_version) + L"+\n",
                      has_modSettings ? L"" : L"* Mod Settings v" + std::to_wstring(modSettings_version) + L"+\n");
      MessageBoxW(nullptr, message.c_str(), L"Let There Be Flight requirements could not be found",
                  MB_SYSTEMMODAL | MB_ICONERROR);
      return false;
#endif
    }

#ifdef __APPLE__
    // LTBF R41: on macOS the script cache is built offline (cp2077-codeware/tools/build-cache.py) from the plugin's Scripts folder,
    // the key bindings come from r6/input/ltbf-mac.xml (the launcher's input loader), and the game never runs RTTI register callbacks
    // or RED4ext game-state callbacks. So Main only attaches the hooks on targets no other plugin hooks; the types, the tweaks, the
    // archive, FMOD and the two hooks shared with Codeware are set up in ltbf_registerNatives, which the launcher's NATREG step calls
    // after every plugin's Main (at the game's InitScripts entry, before LoadScripts).
    spdlog::info("scripts: packed.reds and module.reds come from red4ext/plugins/let_there_be_flight/Scripts (offline cache build)");
    InputLoader::Add(aHandle, L"inputs.xml");
    LtbfMac::SetPlugin(aSdk, aHandle);
    FlightModuleFactory::GetInstance().Load(aSdk, aHandle);
    spdlog::info("Main(Load) done: waiting for ltbf_registerNatives (types, tweaks, archive, audio, Codeware-shared hooks)");
    loaded = true;
    break;
#else
    aSdk->scripts->Add(aHandle, L"packed.reds");
    aSdk->scripts->Add(aHandle, L"module.reds");
    InputLoader::Add(aHandle, L"inputs.xml");
    TweakXL::RegisterTweak(aHandle, MOD_PACKED_NATIVE_TWEAKS_FILENAME);
    TweakXL::RegisterTweak(aHandle, MOD_PACKED_YAML_TWEAKS_FILENAME);
    ArchiveXL::RegisterArchive(aHandle, "let_there_be_flight.archive");

    RED4ext::RTTIRegistrator::Add(RegisterTypes, PostRegisterTypes);
    Engine::RTTIRegistrar::RegisterPending();
    Red::TypeInfoRegistrar::RegisterDiscovered();

    RED4ext::GameState initState;
    initState.OnEnter = nullptr;
    initState.OnUpdate = nullptr;
    initState.OnExit = &FlightAudio::Load;

    aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Initialization, &initState);

    RED4ext::GameState shutdownState;
    shutdownState.OnEnter = nullptr;
    shutdownState.OnUpdate = &FlightAudio::Unload;
    shutdownState.OnExit = nullptr;

    aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Shutdown, &shutdownState);

    FlightModuleFactory::GetInstance().Load(aSdk, aHandle);

    loaded = true;

    break;
#endif
  }
  case RED4ext::EMainReason::Unload: {
    // Free memory, detach hooks.
    // The game's memory is already freed, to not try to do anything with it.
    spdlog::info("Shutting down");
    if (loaded) {
      FlightModuleFactory::GetInstance().Unload(aSdk, aHandle);
#if defined(__APPLE__)
      // LTBF R41: no RED4ext Shutdown game state on macOS; FMOD is released here.
      if (LtbfMacAudioLoaded())
        FlightAudio::Unload(nullptr);
#endif
    }
    spdlog::shutdown();
    break;
  }
  }

  return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo *aInfo) {
  aInfo->name = L"Let There Be Flight";
  aInfo->author = L"Jack Humbert";
  aInfo->version = RED4EXT_SEMVER(MOD_VERSION_MAJOR, MOD_VERSION_MINOR, MOD_VERSION_PATCH);
  aInfo->runtime = RED4EXT_RUNTIME_INDEPENDENT;
  aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports() { return RED4EXT_API_VERSION_LATEST; }
