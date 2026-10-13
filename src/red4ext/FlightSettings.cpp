#include "FlightSettings.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Log.hpp"
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>

#include "Utils/Utils.hpp"
#include "stdafx.hpp"

static RED4ext::Handle<FlightSettings> handle; // LTBF R41: static (Itanium mangling has no type in a variable name, so two globals named handle collide)

RED4ext::Handle<FlightSettings> FlightSettings::GetInstance() {
  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightSettings Instance");
    auto instance = reinterpret_cast<FlightSettings *>(RED4ext::CRTTISystem::Get()->GetClass("FlightSettings")->CreateInstance(true));
    handle = RED4ext::Handle<FlightSettings>(instance);
  }
  handle.refCount->IncRef();
  return handle;
}

void GetInstanceScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                        RED4ext::Handle<FlightSettings> *aOut, int64_t a4) {
  aFrame->code++;

  if (aOut) {
    auto h = RED4ext::Handle<FlightSettings>(FlightSettings::GetInstance());
    h.refCount->IncRef();
    *aOut = h;
  }
}

RED4ext::HashMap<RED4ext::CName, float> floats;
RED4ext::HashMap<RED4ext::CName, RED4ext::Vector3> vector3s;
RED4ext::HashMap<RED4ext::CName, bool> bools;

#ifdef __APPLE__
// LTBF R42c (macOS): set when the pool self-test in ltbf_registerNatives fails. Every Set*/Get* is then a logged no-op, and the
// maps are never created, so nothing allocates through a missing pool.
static bool s_macSettingsDisabled = false;

void FlightSettingsMacDisable(const char *aReason) {
  s_macSettingsDisabled = true;
  LtbfMac::Log("[ltbf] FlightSettings disabled: %s (Set*/Get* are no-ops)", aReason);
}

// True when the call may touch the map. Logs the first refused call per function only.
static bool FlightSettingsMacUsable(uintptr_t aMapAllocator, const char *aWhat, const char *aName, bool &aLogged) {
  if (!s_macSettingsDisabled && aMapAllocator)
    return true;
  if (!aLogged) {
    aLogged = true;
    LtbfMac::Log("[ltbf] FlightSettings::%s(%s) skipped: %s (logged once)", aWhat, aName,
                 s_macSettingsDisabled ? "disabled by the pool self-test" : "maps not set up");
  }
  return false;
}
#endif

float FlightSettings::GetFloat(RED4ext::CString name) {
#ifdef __APPLE__
  static bool s_logged = false;
  if (!FlightSettingsMacUsable(floats.allocator, "GetFloat", name.c_str(), s_logged))
    return 0.0;
#endif
  if (floats.allocator) {
    auto fl = floats.Get(RED4ext::CName(name.c_str()));
    if (fl) {
      return *fl;
    }
  }
  spdlog::warn("Could not find Float: {0}", name.c_str());
  return 0.0;
}

void FlightSettings::SetFloat(RED4ext::CString name, float value) {
#ifdef __APPLE__
  static bool s_logged = false;
  if (!FlightSettingsMacUsable(floats.allocator, "SetFloat", name.c_str(), s_logged))
    return;
#endif
  floats.InsertOrAssign(RED4ext::CName(name.c_str()), value);
}

RED4ext::Vector3 FlightSettings::GetVector3(RED4ext::CString name) {
#ifdef __APPLE__
  static bool s_logged = false;
  if (!FlightSettingsMacUsable(vector3s.allocator, "GetVector3", name.c_str(), s_logged))
    return {0.0, 0.0, 0.0};
#endif
  if (vector3s.allocator) {
    auto vector3 = vector3s.Get(RED4ext::CName(name.c_str()));
    if (vector3) {
      return *vector3;
    }
  }
  spdlog::warn("Could not find Vector3: {0}", name.c_str());
  return {0.0, 0.0, 0.0};
}

void FlightSettings::SetVector3(RED4ext::CString name, float x, float y, float z) {
#ifdef __APPLE__
  static bool s_logged = false;
  if (!FlightSettingsMacUsable(vector3s.allocator, "SetVector3", name.c_str(), s_logged))
    return;
#endif
  vector3s.InsertOrAssign(RED4ext::CName(name.c_str()), RED4ext::Vector3(x, y, z));
}

bool FlightSettings::GetBool(RED4ext::CString name) {
#ifdef __APPLE__
  static bool s_logged = false;
  if (!FlightSettingsMacUsable(bools.allocator, "GetBool", name.c_str(), s_logged))
    return false;
#endif
  if (bools.allocator) {
    auto value = bools.Get(RED4ext::CName(name.c_str()));
#ifdef __APPLE__
    // R42c: an unknown name returns null; Windows dereferences it unchecked.
    if (!value) {
      spdlog::warn("Could not find bool: {0}", name.c_str());
      return false;
    }
#endif
    return *value;
  }
  spdlog::warn("Could not find bool: {0}", name.c_str());
  return false;
}

void FlightSettings::SetBool(RED4ext::CString name, bool value) {
#ifdef __APPLE__
  static bool s_logged = false;
  if (!FlightSettingsMacUsable(bools.allocator, "SetBool", name.c_str(), s_logged))
    return;
#endif
  bools.InsertOrAssign(RED4ext::CName(name.c_str()), value);
}

void FlightSettings::DebugBreak() {
  __debugbreak();
}

#ifdef __APPLE__
// LTBF R42a (macOS): RED4ext-macos runs no game-state callbacks, so the Running-state Setup below never runs on its own. The maps
// are created in ltbf_registerNatives (before any script can call SetFloat), and Setup runs once from FlightSystem::OnInitialize.
void FlightSettingsMacEnsureMaps() {
  if (floats.allocator || s_macSettingsDisabled)
    return;
  auto allocator = new RED4ext::Memory::DefaultAllocator();
  floats = RED4ext::HashMap<RED4ext::CName, float>(allocator);
  vector3s = RED4ext::HashMap<RED4ext::CName, RED4ext::Vector3>(allocator);
  bools = RED4ext::HashMap<RED4ext::CName, bool>(allocator);
}

bool Setup(RED4ext::CGameApplication *aApp);

void FlightSettingsMacSetupOnce() {
  static bool s_done = false;
  if (s_done)
    return;
  s_done = true;
  LtbfMac::Log("[ltbf] FlightSettings setup (macOS: from FlightSystem::OnInitialize, Windows: the Running game state)");
  Setup(nullptr);
}
#endif

bool Setup(RED4ext::CGameApplication *aApp) {
#ifdef __APPLE__
  FlightSettingsMacEnsureMaps();
#else
  auto allocator = new RED4ext::Memory::DefaultAllocator();
  floats = RED4ext::HashMap<RED4ext::CName, float>(allocator);
  vector3s = RED4ext::HashMap<RED4ext::CName, RED4ext::Vector3>(allocator);
  bools = RED4ext::HashMap<RED4ext::CName, bool>(allocator);
#endif

  auto fs = FlightSettings::GetInstance();
  if (fs) {
    RED4ext::ExecuteFunction(fs, fs->nativeType->GetFunction("OnAttach"), nullptr);
  } else {
    spdlog::error("[FlightSettings] Could not initialize");
  }

  //auto rtti = RED4ext::CRTTISystem::Get();
  //auto onUpdate = cls.GetFunction("OnAttach");
  //if (onUpdate) {
  //  auto stack = RED4ext::CStack(FlightSettings::GetInstance(), nullptr, 0, nullptr, 0);
  //  onUpdate->Execute(&stack);
  //}

  //auto mesh = RED4ext::ResourceReference<RED4ext::ent::MeshComponent>(
      //"user\\jackhumbert\\meshes\\engine_corpo.mesh");
  //mesh.Load();

  return true;
}

struct FlightSettingsModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) override {
#ifdef __APPLE__
    // LTBF R42a: no game-state callbacks on macOS; see FlightSettingsMacSetupOnce.
    (void)aSdk;
    (void)aHandle;
    return;
#endif
    RED4ext::GameState runningState;
    runningState.OnEnter = &Setup;
    runningState.OnUpdate = nullptr;
    runningState.OnExit = nullptr;

    aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &runningState);
  }
};

REGISTER_FLIGHT_MODULE(FlightSettingsModule);