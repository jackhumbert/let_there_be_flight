#include <fmod_errors.h>

#include "FlightSystem.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/IGameSystem.hpp>
#include <RED4ext/Addresses.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <iostream>

#include "FlightLog.hpp"
#include "Utils.hpp"
#include "stdafx.hpp"

namespace FlightSystem {

struct UpdateCalls {
  void *(*update)(void *, float);
  void *(*copy1)(void **, void **);
  void *(*copy2)(void **, void **);
  RED4ext::game::IGameSystem *(*callback)();
};

struct UpdateCallStruct {
  RED4ext::game::IGameSystem *system;
  void *unk08;
  void *unk10;
  void *unk18;
  UpdateCalls *calls;
};

UpdateCalls FlightSystemUpdateCalls;

int32_t flightSystemUpdateRegister = 0;

RED4ext::TTypedClass<FlightSystem> icls("IFlightSystem");
RED4ext::TTypedClass<FlightSystem> cls("FlightSystem");

//RED4ext::CClass *IFlightSystem::GetNativeType() { return &icls; }

RED4ext::CClass *FlightSystem::GetNativeType() { return &cls; }

RED4ext::Handle<FlightSystem> fsHandle;

FlightSystem *FlightSystem::GetInstance() {
  if (!fsHandle.instance) {
    spdlog::info("[RED4ext] New FlightSystem Instance");
    auto instance = reinterpret_cast<FlightSystem *>(cls.AllocInstance());
    fsHandle = RED4ext::Handle<FlightSystem>(instance);
  }

  return (FlightSystem *)fsHandle.instance;
}

void GetInstanceScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                        RED4ext::Handle<FlightSystem> *aOut, int64_t a4) {
  aFrame->code++;

  if (!fsHandle.instance) {
    spdlog::info("[RED4ext] New FlightSystem Instance");
    auto instance = reinterpret_cast<FlightSystem *>(cls.AllocInstance());
    fsHandle = RED4ext::Handle<FlightSystem>(instance);
  }

  if (aOut) {
    fsHandle.refCount->IncRef();
    *aOut = RED4ext::Handle<FlightSystem>(fsHandle);
  }
}

// OnAttach
bool FlightSystem::sub_118(void *a1) {
  spdlog::info("[FlightSystem] sub_118!");
  return true;
}

// OnDetach
bool FlightSystem::sub_120() {
  spdlog::info("[FlightSystem] sub_120!");
  return true;
}

//void* FlightSystemUpdate(void *, float) { return NULL; }

void* FlightSystem::RegisterUpdate(uintptr_t lookup) {
	//RED4ext::CClass *cls;
	//RED4ext::UpdateCalls *result;
	//RED4ext::UpdateCallStruct ucs1; 
	//RED4ext::UpdateCallStruct ucs2;

	//if (flightSystemUpdateRegister >
	//    (NtCurrentTeb()->ThreadLocalStoragePointer +
	//     *(unsigned int *)(0x144B99330 - Addresses::ImageBase)) + 1948) {
	//  reinterpret_cast<void(*)(int32_t*)>(Addresses::Init_thread_header)(&flightSystemUpdateRegister);
	//    if (flightSystemUpdateRegister == -1) {
	//      FlightSystemUpdateCalls.callback = 0;
	//      FlightSystemUpdateCalls.update = &FlightSystemUpdate;
	//      FlightSystemUpdateCalls.copy1 =
	//          reinterpret_cast < void *(*)(void**, void**)>(Addresses::CopyInstance);
	//      FlightSystemUpdateCalls.copy2 =
	//          reinterpret_cast<void *(*)(void **, void **)>(Addresses::CopyInstance);
	//      reinterpret_cast<void(*)(int32_t*)>(Addresses::Init_thread_footer)(&flightSystemUpdateRegister);
	//    }
	//}
	//ucs1.system = this;
	//ucs1.calls = &FlightSystemUpdateCalls;
	//ucs2.calls = &FlightSystemUpdateCalls;
	//FlightSystemUpdateCalls.copy1((void **)&ucs2.system, (void **)&ucs1.system);
	//cls = this->GetType();
	//reinterpret_cast<void (*)(uintptr_t, uint8_t, CClass *, const char *, void *, uint32_t)>(
	//    Addresses::UpdateDefinition_CreateFromParent)(
	//    lookup, 3u, cls, "FlightSystem/Update", &ucs2, 0xA);
	//result = ucs1.calls; 
	//if (ucs1.calls && ucs1.calls->callback) {
	//    return ((void *(*)(RED4ext::UpdateCallStruct *))ucs1.calls->callback)(&ucs1);
	//}
	//return result;        
	spdlog::info("[FlightSystem] sub_110!");
	return 0;
}

bool OnUpdate(RED4ext::CGameApplication* aApp) {
  //auto fs = FlightSystem::GetInstance();
  //auto rtti = RED4ext::CRTTISystem::Get();
  //auto fc = rtti->GetClass("FlightComponent");
  //auto onUpdate = fc->GetFunction("OnUpdate");
  //for (const auto component : fs->components) {
  //  auto stack = RED4ext::CStack(component, nullptr, 0, nullptr, 0);
  //  onUpdate->Execute(&stack);
  //}
  spdlog::info("[FlightSystem] OnUpdate");
  return true;
}

struct FlightSystemModule : FlightModule {
  //void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  //  RED4ext::GameState runningState;
  //  runningState.OnEnter = nullptr;
  //  runningState.OnUpdate = &OnUpdate;
  //  runningState.OnExit = nullptr;

  //  aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &runningState);
  //}

  void RegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();

    icls.flags = {.isAbstract = true, .isNative = true, .isImportOnly = true};
    icls.parent = rtti->GetClass("gameIGameSystem");
    rtti->RegisterType(&icls);

    cls.flags = {.isNative = true};
    cls.parent = &icls;
    rtti->RegisterType(&cls);
  }

  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto getInstance = RED4ext::CClassStaticFunction::Create(&cls, "GetInstance", "GetInstance", &GetInstanceScripts,
                                                             {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getInstance);

    //FlightSystem::GetInstance();
  }
};

REGISTER_FLIGHT_MODULE(FlightSystemModule);

} // namespace FlightSystem