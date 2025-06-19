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

#include <ArchiveXL.hpp>
#include <TweakXL.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libloaderapi.h>
#include <string>
#include <winbase.h>

using Query_t = void (*)(RED4ext::PluginInfo *);

enum EDependencyStatus {
  FoundWithCorrectVersion = 0,
  FoundWithIncorrectVersion = 1,
  FoundCouldNotBeQueried = 2,
  FoundNoVersion = 3,
  NotFound = 4
};

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
          spdlog::error(L"{} found, but wrong version: {}", name, std::to_wstring(pluginInfo.version));
          return FoundWithIncorrectVersion;
        }
      } else {
        spdlog::error(L"{} found, but could not be queried for version", name);
        return FoundCouldNotBeQueried;
      }
    } else {
      spdlog::info(L"{} found", name);
      return FoundNoVersion;
    }
  } else {
    spdlog::error(L"{} is not installed/installed properly - aborting", name);
    return NotFound;
  }
}

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

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason,
                                        const RED4ext::Sdk *aSdk) {
  switch (aReason) {
  case RED4ext::EMainReason::Load: {
    pluginHandle = aHandle;

    Utils::CreateLogger();
    spdlog::info("Starting up Let There Be Flight {}", MOD_VERSION_STR);
    auto ptr = GetModuleHandle(nullptr);
    spdlog::info("Base address: {}", fmt::ptr(ptr));
    auto modPtr = aHandle;
    spdlog::info("Mod address: {}", fmt::ptr(modPtr));

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

    auto inputLoader_version = RED4EXT_SEMVER(0, 1, 1);
    auto archiveXL_version = RED4EXT_SEMVER(1, 23, 0);
    auto tweakXL_version = RED4EXT_SEMVER(1, 10, 0);
    auto modSettings_version = RED4EXT_SEMVER(0, 2, 14);

    auto has_inputLoader = HasDependency(L"input_loader", inputLoader_version) == FoundWithCorrectVersion;
    auto has_archiveXL = HasDependency(L"ArchiveXL", archiveXL_version) == FoundWithCorrectVersion;
    auto has_tweakXL = HasDependency(L"TweakXL", tweakXL_version) == FoundWithCorrectVersion;
    // optional, so just test for incorrect version
    auto has_modSettings = HasDependency(L"mod_settings", modSettings_version) != FoundWithIncorrectVersion;

    if (!has_inputLoader || !has_archiveXL || !has_tweakXL || !has_modSettings) {
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
    }

    aSdk->scripts->Add(aHandle, L"packed.reds");
    aSdk->scripts->Add(aHandle, L"module.reds");
    InputLoader::Add(aHandle, L"inputs.xml");
    TweakXL::RegisterTweak(aHandle, MOD_PACKED_TWEAKS_FILENAME);
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
  }
  case RED4ext::EMainReason::Unload: {
    // Free memory, detach hooks.
    // The game's memory is already freed, to not try to do anything with it.
    spdlog::info("Shutting down");
    if (loaded) {
      FlightModuleFactory::GetInstance().Unload(aSdk, aHandle);
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
