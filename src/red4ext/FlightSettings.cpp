#include "FlightSettings.hpp"
#include "Utils/FlightModule.hpp"
#include "Flight/Log.hpp"
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>

#include "Utils/Utils.hpp"
#include "stdafx.hpp"

RED4ext::Handle<FlightSettings> handle;

RED4ext::Handle<FlightSettings> FlightSettings::GetInstance() {
  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightSettings Instance");
    auto instance = reinterpret_cast<FlightSettings *>(RED4ext::CRTTISystem::Get()->GetClass("FlightSettings")->CreateInstance());
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

float FlightSettings::GetFloat(RED4ext::CString name) {
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
  floats.InsertOrAssign(RED4ext::CName(name.c_str()), value);
}

RED4ext::Vector3 FlightSettings::GetVector3(RED4ext::CString name) {
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
  vector3s.InsertOrAssign(RED4ext::CName(name.c_str()), RED4ext::Vector3(x, y, z));
}

bool FlightSettings::GetBool(RED4ext::CString name) {
  if (bools.allocator) {
    auto value = bools.Get(RED4ext::CName(name.c_str()));
    return *value;
  }
  spdlog::warn("Could not find bool: {0}", name.c_str());
  return false;
}

void FlightSettings::SetBool(RED4ext::CString name, bool value) {
  bools.InsertOrAssign(RED4ext::CName(name.c_str()), value);
}

void FlightSettings::DebugBreak() {
  __debugbreak();
}

bool Setup(RED4ext::CGameApplication *aApp) {
  auto allocator = new RED4ext::Memory::DefaultAllocator();
  floats = RED4ext::HashMap<RED4ext::CName, float>(allocator);
  vector3s = RED4ext::HashMap<RED4ext::CName, RED4ext::Vector3>(allocator);
  bools = RED4ext::HashMap<RED4ext::CName, bool>(allocator);

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
    RED4ext::GameState runningState;
    runningState.OnEnter = &Setup;
    runningState.OnUpdate = nullptr;
    runningState.OnExit = nullptr;

    aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &runningState);
  }
};

REGISTER_FLIGHT_MODULE(FlightSettingsModule);