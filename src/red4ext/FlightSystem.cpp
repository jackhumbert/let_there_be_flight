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

RED4ext::TTypedClass<FlightSystem> icls("IFlightSystem");
RED4ext::TTypedClass<FlightSystem> cls("FlightSystem");
RED4ext::CClass *classPointer = &cls;

//RED4ext::CClass *IFlightSystem::GetNativeType() { return &icls; }

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

void PrePhysics(RED4ext::Unk2* unk2, float* deltaTime, void* unkStruct) {
  spdlog::info("[FlightSystem] PrePhysics!");
}

 void FlightSystem::RegisterUpdates(RED4ext::UpdateManagerHolder *holder) {
  spdlog::info("[FlightSystem] sub_110/RegisterUpdates!");

  holder->RegisterBucketUpdate(RED4ext::Unk2::OnPreWorldTick, RED4ext::Unk1::PrePhysicsTick, this, "FlightSystem/PrePhysics", &PrePhysics);
 }

 bool FlightSystem::sub_118(RED4ext::world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] sub_118!");
  return true;
}

void FlightSystem::sub_120(RED4ext::world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] sub_120!");
}

void FlightSystem::sub_128(RED4ext::world::RuntimeScene *runtimeScene) {
	spdlog::info("[FlightSystem] sub_128!");
}

void FlightSystem::sub_130() {
	spdlog::info("[FlightSystem] sub_130!");
}

bool FlightSystem::sub_138() {
	spdlog::info("[FlightSystem] sub_138!");
  return 0;
}

void FlightSystem::sub_140() {
	spdlog::info("[FlightSystem] sub_140!");
}

void FlightSystem::sub_148() {
	spdlog::info("[FlightSystem] sub_148!");
}

void FlightSystem::sub_150(void * a1, uint64_t a2, uint64_t a3) { 
  spdlog::info("[FlightSystem] sub_150!");
}

bool FlightSystem::sub_158() {
  spdlog::info("[FlightSystem] sub_158!");
  return true;
}

void FlightSystem::sub_160() {
  spdlog::info("[FlightSystem] sub_160!");
}

void FlightSystem::sub_168() {
  spdlog::info("[FlightSystem] sub_168!");
}

void FlightSystem::sub_170() {
  spdlog::info("[FlightSystem] sub_170!");
}

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

void** FlightSystem::sub_198(void ** unkThing) {
  spdlog::info("[FlightSystem] sub_198!");
  return unkThing;
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