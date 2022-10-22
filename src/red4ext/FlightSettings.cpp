#include "FlightSettings.hpp"
#include "FlightModule.hpp"
#include "FlightLog.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>

#include "Utils.hpp"
#include "stdafx.hpp"

namespace FlightSettings {

RED4ext::TTypedClass<FlightSettings> cls("FlightSettings");

RED4ext::CClass *FlightSettings::GetNativeType() { return &cls; }

RED4ext::Handle<FlightSettings> handle;

FlightSettings *FlightSettings::GetInstance() {
  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightSettings Instance");
    auto instance = reinterpret_cast<FlightSettings *>(cls.AllocInstance());
    handle = RED4ext::Handle<FlightSettings>(instance);
  }
  handle.refCount->IncRef();
  return (FlightSettings *)handle.instance;
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

float GetFloat(RED4ext::CString name) {
  if (floats.allocator) {
    auto fl = floats.Get(RED4ext::CName(name.c_str()));
    if (fl) {
      return *fl;
    }
  }
  spdlog::warn("Could not find Float: {0}", name.c_str());
  return 0.0;
}

void GetFloat(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, float *aOut, int64_t a4) {
  RED4ext::CString name;
  RED4ext::GetParameter(aFrame, &name);

  aFrame->code++;

  if (aOut) {
    *aOut = GetFloat(name);
  }
}

void SetFloat(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, float *aOut, int64_t a4) {
  RED4ext::CString name;
  float value;
  RED4ext::GetParameter(aFrame, &name);
  RED4ext::GetParameter(aFrame, &value);

  aFrame->code++;

  if (aOut) {
    *aOut = GetFloat(name);
  }

  floats.InsertOrAssign(RED4ext::CName(name.c_str()), value);
}

RED4ext::Vector3 GetVector3(RED4ext::CString name) {
  if (vector3s.allocator) {
    auto vector3 = vector3s.Get(RED4ext::CName(name.c_str()));
    if (vector3) {
      return *vector3;
    }
  }
  spdlog::warn("Could not find Vector3: {0}", name.c_str());
  return RED4ext::Vector3(0.0, 0.0, 0.0);
}

void GetVector3(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Vector3 *aOut, int64_t a4) {
  RED4ext::CString name;
  RED4ext::GetParameter(aFrame, &name);

  aFrame->code++;

  if (aOut) {
    *aOut = GetVector3(name);
  }
}

void SetVector3(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Vector3 *aOut, int64_t a4) {
  RED4ext::CString name;
  float x;
  float y;
  float z;
  RED4ext::GetParameter(aFrame, &name);
  RED4ext::GetParameter(aFrame, &x);
  RED4ext::GetParameter(aFrame, &y);
  RED4ext::GetParameter(aFrame, &z);

  aFrame->code++;

  if (aOut) {
    *aOut = GetVector3(name);
  }

  vector3s.InsertOrAssign(RED4ext::CName(name.c_str()), RED4ext::Vector3(x, y, z));
}

void DebugBreak(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Vector3 *aOut, int64_t a4) {
  aFrame->code++;
  __debugbreak();
}

bool Setup(RED4ext::CGameApplication *aApp) {
  auto allocator = new RED4ext::Memory::DefaultAllocator();
  floats = RED4ext::HashMap<RED4ext::CName, float>(allocator);
  vector3s = RED4ext::HashMap<RED4ext::CName, RED4ext::Vector3>(allocator);

  auto fs = FlightSettings::GetInstance();
  if (fs) {
    fs->ExecuteFunction("OnAttach");
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
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    RED4ext::GameState runningState;
    runningState.OnEnter = &Setup;
    runningState.OnUpdate = nullptr;
    runningState.OnExit = nullptr;

    aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &runningState);
  }

  void RegisterTypes() {
    cls.flags = {.isNative = true};
    RED4ext::CRTTISystem::Get()->RegisterType(&cls);
  }

  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto scriptable = rtti->GetClassByScriptName("IScriptable");
    cls.parent = scriptable;

    auto getInstance = RED4ext::CClassStaticFunction::Create(&cls, "GetInstance", "GetInstance", &GetInstanceScripts,
                                                             {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getInstance);

    auto getFloat = RED4ext::CClassStaticFunction::Create(&cls, "GetFloat", "GetFloat", &GetFloat,
                                                          {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getFloat);

    auto setFloat = RED4ext::CClassStaticFunction::Create(&cls, "SetFloat", "SetFloat", &SetFloat,
                                                          {.isNative = true, .isStatic = true});
    cls.RegisterFunction(setFloat);

    auto getVector3 = RED4ext::CClassStaticFunction::Create(&cls, "GetVector3", "GetVector3", &GetVector3,
                                                          {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getVector3);

    auto setVector3 = RED4ext::CClassStaticFunction::Create(&cls, "SetVector3", "SetVector3", &SetVector3,
                                                            {.isNative = true, .isStatic = true});
    cls.RegisterFunction(setVector3);

    auto debugBreak = RED4ext::CClassStaticFunction::Create(&cls, "DebugBreak", "DebugBreak", &DebugBreak,
                                                            {.isNative = true, .isStatic = true});
    cls.RegisterFunction(debugBreak);

  }
};

REGISTER_FLIGHT_MODULE(FlightSettingsModule);

} // namespace FlightSettings