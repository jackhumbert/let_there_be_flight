#include <fmod_errors.h>

#include "FlightSystem.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Addresses.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/Natives/GameInstance.hpp>
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
RED4ext::CClass *classPointer = &cls;

RED4ext::CClass *IFlightSystem::GetNativeType() { return &icls; }

RED4ext::CClass *FlightSystem::GetNativeType() { return &cls; }

RED4ext::Handle<FlightSystem> fsHandle;

FlightSystem *FlightSystem::GetInstance() {
  //if (!fsHandle.instance) {
  //  spdlog::info("[RED4ext] New FlightSystem Instance");
  //  auto instance = reinterpret_cast<FlightSystem *>(cls.AllocInstance());
  //  fsHandle = RED4ext::Handle<FlightSystem>(instance);
  //}

  //return fsHandle.GetPtr();
  
  auto fs = (FlightSystem*)RED4ext::CGameEngine::Get()->framework->gameInstance->GetInstance(classPointer);
  return fs;
}

void GetInstanceScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                        RED4ext::Handle<FlightSystem> *aOut, int64_t a4) {
  aFrame->code++;

  //if (!fsHandle.instance) {
  //  spdlog::info("[RED4ext] New FlightSystem Instance");
  //  auto instance = reinterpret_cast<FlightSystem *>(cls.AllocInstance());
  //  fsHandle = RED4ext::Handle<FlightSystem>(instance);
  //}
  
  auto fs = (FlightSystem *)RED4ext::CGameEngine::Get()->framework->gameInstance->GetInstance(classPointer);

  if (aOut && fs) {
    //fsHandle.refCount->IncRef();
    //*aOut = RED4ext::Handle<FlightSystem>(fsHandle);
    *aOut = RED4ext::Handle<FlightSystem>(fs);
  }
}

void Copy(RED4ext::Update* a1, RED4ext::Update* a2) { 
  *a1 = *a2;
}

void Empty() {

}

void Callback(RED4ext::Update *, RED4ext::Unk0, void *, void *) {
  spdlog::info("[FlightSystem] PrePhysics update!");
}

RED4ext::UpdateStruct<RED4ext::Update *, RED4ext::Unk0, void *, void *> callbackStruct = {
  .Callback = &Callback,
  .StoreDefinition = &Copy,
  .CopyStorage = &Copy,
  .Destruct = &Empty
};

 void FlightSystem::RegisterUpdates(RED4ext::UpdateManagerHolder *holder) {
  // RED4ext::CClass *cls;
  // RED4ext::UpdateCalls *result;
  // RED4ext::UpdateCallStruct ucs1;
  // RED4ext::UpdateCallStruct ucs2;

  // if (flightSystemUpdateRegister >
  //     (NtCurrentTeb()->ThreadLocalStoragePointer +
  //      *(unsigned int *)(0x144B99330 - Addresses::ImageBase)) + 1948) {
  //   reinterpret_cast<void(*)(int32_t*)>(Addresses::Init_thread_header)(&flightSystemUpdateRegister);
  //     if (flightSystemUpdateRegister == -1) {
  //       FlightSystemUpdateCalls.callback = 0;
  //       FlightSystemUpdateCalls.update = &FlightSystemUpdate;
  //       FlightSystemUpdateCalls.copy1 =
  //           reinterpret_cast < void *(*)(void**, void**)>(Addresses::CopyInstance);
  //       FlightSystemUpdateCalls.copy2 =
  //           reinterpret_cast<void *(*)(void **, void **)>(Addresses::CopyInstance);
  //       reinterpret_cast<void(*)(int32_t*)>(Addresses::Init_thread_footer)(&flightSystemUpdateRegister);
  //     }
  // }
  // ucs1.system = this;
  // ucs1.calls = &FlightSystemUpdateCalls;
  // ucs2.calls = &FlightSystemUpdateCalls;
  // FlightSystemUpdateCalls.copy1((void **)&ucs2.system, (void **)&ucs1.system);
  // cls = this->GetType();
  // reinterpret_cast<void (*)(uintptr_t, uint8_t, CClass *, const char *, void *, uint32_t)>(
  //     Addresses::UpdateDefinition_CreateFromParent)(
  //     lookup, 3u, cls, "FlightSystem/Update", &ucs2, 0xA);
  // result = ucs1.calls;
  // if (ucs1.calls && ucs1.calls->callback) {
  //     return ((void *(*)(RED4ext::UpdateCallStruct *))ucs1.calls->callback)(&ucs1);
  // }
  // return result;
  spdlog::info("[FlightSystem] sub_110/RegisterUpdates!");

  //RED4ext::Update update = RED4ext::Update();
  //update.system = this;
  //update.callbackStruct = &callbackStruct;

  //holder->RegisterBucketUpdate(RED4ext::Unk2::OnPreWorldTick, RED4ext::Unk1::PrePhysicsTick, this,
  //  "FlightSystem/PrePhysics", &update);
 }


// OnAttach
bool FlightSystem::sub_118(void *a1) {
  spdlog::info("[FlightSystem] sub_118!");
  return true;
}

// OnDetach
void FlightSystem::sub_120(void * runtimeScene) {
  spdlog::info("[FlightSystem] sub_120!");
}

//void* FlightSystemUpdate(void *, float) { return NULL; }


void FlightSystem::sub_128(void *runtimeScene) {
	spdlog::info("[FlightSystem] sub_128!");
}

void FlightSystem::sub_130() {
	spdlog::info("[FlightSystem] sub_130!");
}

void FlightSystem::sub_138() {
	spdlog::info("[FlightSystem] sub_138!");
}

void FlightSystem::sub_140() {
	spdlog::info("[FlightSystem] sub_140!");
}

void FlightSystem::sub_148() {
	spdlog::info("[FlightSystem] sub_148!");
}

// 150, OnGameLoad
void FlightSystem::sub_150(void * a1, uint64_t a2, uint64_t a3) { 
  spdlog::info("[FlightSystem] sub_150!");
}

// ReturnOne - should probably always return 1
bool FlightSystem::sub_158() {
  spdlog::info("[FlightSystem] sub_158!");
  return true;
}

void FlightSystem::sub_160() {
  spdlog::info("[FlightSystem] sub_160!");
}

// might be called from GameInstance->Systems168o170
void FlightSystem::sub_168() {
  spdlog::info("[FlightSystem] sub_168!");
}

// might be called from GameInstance->Systems168o170
void FlightSystem::sub_170() {
  spdlog::info("[FlightSystem] sub_170!");
}

// something with a CString @ 0x08
void FlightSystem::sub_178(uintptr_t a1, bool a2) {
  spdlog::info("[FlightSystem] sub_178!");
  RED4ext::game::IGameSystem::sub_178(a1, a2);
}

void FlightSystem::sub_180(uint64_t, bool isGameLoaded, uint64_t) {
  spdlog::info("[FlightSystem] sub_180!");
}

void FlightSystem::sub_188() {
  spdlog::info("[FlightSystem] sub_188!");
}

// called from GameInstance->sub_20
void FlightSystem::sub_190(HighLow *) {
  spdlog::info("[FlightSystem] sub_190!");
}

// some systems load tweaks - might be a setup, called from GameInstance->sub_20
void FlightSystem::sub_198(void *) {
  spdlog::info("[FlightSystem] sub_198!");
}

void FlightSystem::sub_1A0() {
  spdlog::info("[FlightSystem] sub_1A0!");
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


constexpr uintptr_t GetGameSystemsDataAddr = 0x2D028A0;

RED4ext::DynArray<RED4ext::GameSystemData> *__fastcall GetGameSystemsData(
    RED4ext::DynArray<RED4ext::GameSystemData> *gameSystemsData);

decltype(&GetGameSystemsData) GetGameSystemsData_Original;

RED4ext::DynArray<RED4ext::GameSystemData>* __fastcall GetGameSystemsData(
  RED4ext::DynArray<RED4ext::GameSystemData>* gameSystemsData) {
  GetGameSystemsData_Original(gameSystemsData);
  auto flightSystem = new RED4ext::GameSystemData();
  flightSystem->name = "FlightSystem";
  flightSystem->inSingleplayer = true;
  gameSystemsData->EmplaceBack(*flightSystem);
  return gameSystemsData;
}

struct FlightSystemModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  //  RED4ext::GameState runningState;
  //  runningState.OnEnter = nullptr;
  //  runningState.OnUpdate = &OnUpdate;
  //  runningState.OnExit = nullptr;

  //  aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &runningState);

  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetGameSystemsDataAddr), &GetGameSystemsData,
                                reinterpret_cast<void **>(&GetGameSystemsData_Original)))
    ;
  }

  void RegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();

    icls.flags = {.isAbstract = true, .isNative = true, .isImportOnly = true};
    icls.parent = rtti->GetClass("gameIGameSystem");
    rtti->RegisterType(&icls);

    cls.flags.isNative = true;
    cls.flags.b20000 = true;
    cls.flags.isAlwaysTransient = true;
    //cls.flags.b1000 = true;
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

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetGameSystemsDataAddr));
  }
};

REGISTER_FLIGHT_MODULE(FlightSystemModule);

} // namespace FlightSystem