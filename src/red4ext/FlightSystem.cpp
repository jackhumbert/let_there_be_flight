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
#include <RED4ext/GameOptions.hpp>

#include "FlightLog.hpp"
#include "Utils.hpp"
#include "stdafx.hpp"
#include "LoadResRef.hpp"
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include "FlightComponent.hpp"

RED4ext::RelocPtr<RED4ext::GameOptionBool> PhysXClampHugeImpacts(0x4782878);
RED4ext::RelocPtr<RED4ext::GameOptionBool> PhysXClampHugeSpeeds(0x47827F8);
RED4ext::RelocPtr<RED4ext::GameOptionBool> AirControlCarRollHelper(0x47818C8);
RED4ext::RelocPtr<RED4ext::GameOptionFloat> ForceMoveToMaxLinearSpeed(0x4781A40);
RED4ext::RelocPtr<RED4ext::GameOptionBool> physicsCCD(0x4780960);
RED4ext::RelocPtr<RED4ext::GameOptionBool> EnableSmoothWheelContacts(0x4781FE8);

//RED4ext::TTypedClass<FlightSystem> icls("IFlightSystem");
//RED4ext::TTypedClass<FlightSystem> cls("FlightSystem");
//RED4ext::CClass *classPointer = &cls;

//RED4ext::CClass *IFlightSystem::GetNativeType() { return &icls; }

//RED4ext::CClass *FlightSystem::GetNativeType() { return classPointer; }

//FlightSystem *FlightSystem::GetInstance() {
//  auto fs = (FlightSystem*)RED4ext::CGameEngine::Get()->framework->gameInstance->GetInstance(FlightSystem::GetRTTIType());
//  return fs;
//}

RED4ext::Handle<FlightSystem> FlightSystem::GetInstance() {
  auto fs = (FlightSystem *)RED4ext::CGameEngine::Get()->framework->gameInstance->GetInstance(FlightSystem::GetRTTIType());
  return RED4ext::Handle<FlightSystem>(fs);
}

// 1.52 RVA: 0x1C58B0 / 1857712
/// @pattern 4C 8B DC 48 81 EC B8 00 00 00 0F 10 21 41 0F 29 73 E8 0F 28 DC 0F 59 DC C7 44 24 0C 00 00 00 00
RED4ext::Matrix* __fastcall GetMatrixFromOrientation(RED4ext::Quaternion* q, RED4ext::Matrix* m) {
  RED4ext::RelocFunc<decltype(&GetMatrixFromOrientation)> call(0x1C58B0);
  return call(q, m);
}


void FlightSystem::RegisterComponent(RED4ext::WeakHandle<FlightComponent> fc) {
  if (fc.instance && !fc.Expired()) {
    this->flightComponents.EmplaceBack(fc);
  }
  //spdlog::info("[FlightSystem] Component added");
}

void FlightSystem::UnregisterComponent(RED4ext::WeakHandle<FlightComponent> fc) {
  if (fc.Expired())
    return;
  for (auto i = 0; i < this->flightComponents.size; i++) {
    if (!this->flightComponents[i].Expired()) {
      auto efc = this->flightComponents[i].Lock().GetPtr();
      if (efc == fc.Lock().GetPtr()) {
        this->flightComponents.RemoveAt(i);
        //spdlog::info("[FlightSystem] Component removed");
        break;
      }
    }
  }
}

void PrePhysics(RED4ext::Unk2 *unk2, float *deltaTime, void *unkStruct) {
  // spdlog::info("[FlightSystem] PrePhysics!");
  auto fs = FlightSystem::GetInstance();
  auto wh = fs->soundListener;
  if (!wh.Expired()) {
    RED4ext::Matrix matrix;
    auto h = wh.Lock();
    auto t = h.GetPtr()->localTransform;
    GetMatrixFromOrientation(&t.Orientation, &matrix);
    matrix.W.X = t.Position.x.Bits * 0.0000076293945;
    matrix.W.Y = t.Position.y.Bits * 0.0000076293945;
    matrix.W.Z = t.Position.z.Bits * 0.0000076293945;
    matrix.W.W = 1.0;
    fs->audio->UpdateListenerMatrix(matrix);
    fs->audio->UpdateVolume();
  }
}

void UpdateComponents(RED4ext::Unk2 *unk2, float *deltaTime, void *unkStruct) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = FlightComponent::GetRTTIType();
  for (auto const &wh : FlightSystem::GetInstance()->flightComponents) {
    if (wh.Expired())
      continue;
    auto fc = wh.Lock().GetPtr();
    if (fc) {
      auto vehicle = reinterpret_cast<RED4ext::vehicle::BaseObject *>(fc->entity);
      //auto activeProp = fcc->GetProperty("hasUpdate");
      if (vehicle && fc->hasUpdate && vehicle->physicsData) {
        // removes Asleep/0x10
        //vehicle->SetPhysicsState(RED4ext::vehicle::PhysicsState::Asleep, 1u);
        //vehicle->UnsetPhysicsStates();
        // just locks the brakes
        //vehicle->PersistentDataPS->ToggleQuestForceBraking(true);
        // auto onUpdate = fcc->GetFunction("OnUpdate");
        // auto args = RED4ext::CStackType(rtti->GetType("Float"), &deltaTime);
        // auto stack = RED4ext::CStack(fc, &args, 1, nullptr, 0);
        // onUpdate->Execute(&stack);
        fc->ExecuteFunction("OnUpdate", deltaTime);

        vehicle->physicsData->force.X += fc->force.X;
        vehicle->physicsData->force.Y += fc->force.Y;
        vehicle->physicsData->force.Z += fc->force.Z;

        vehicle->physicsData->torque.X += fc->torque.X;
        vehicle->physicsData->torque.Y += fc->torque.Y;
        vehicle->physicsData->torque.Z += fc->torque.Z;

        fc->force.X = 0.0;
        fc->force.Y = 0.0;
        fc->force.Z = 0.0;
        fc->force.W = 0.0;

        fc->torque.X = 0.0;
        fc->torque.Y = 0.0;
        fc->torque.Z = 0.0;
        fc->torque.W = 0.0;
      }
    }
  }
}

void FlightSystem::RegisterUpdates(RED4ext::UpdateManagerHolder *holder) {
  spdlog::info("[FlightSystem] sub_110/RegisterUpdates!");

  holder->RegisterBucketUpdate(RED4ext::Unk2::OnPreWorldTick, RED4ext::Unk1::PrePhysicsTick, this,
                               "FlightSystem/PrePhysics", &PrePhysics);
  holder->RegisterBucketUpdate(RED4ext::Unk2::OnPreWorldTick, RED4ext::Unk1::PhysicsExecuteAsyncQueries, this,
                               "FlightSystem/UpdateComponents", &UpdateComponents);
 }

 bool FlightSystem::WorldAttached(RED4ext::world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] WorldAttached!");
  this->audio->ExecuteFunction("OnWorldAttached");

  //auto rtti = RED4ext::CRTTISystem::Get();

  //// auto types = RED4ext::DynArray<RED4ext::CBaseRTTIType*>(new RED4ext::Memory::DefaultAllocator());
  //// rtti->GetNativeTypes(types);
  //auto classes = RED4ext::DynArray<RED4ext::CClass *>(new RED4ext::Memory::DefaultAllocator());
  //rtti->GetClasses(nullptr, classes);

  //for (const auto &cls : classes) {
  //  if (cls && cls->name != "None" && !cls->flags.isAbstract) {
  //    if (cls->name == "inkInputKeyIconManager")
  //      continue;
  //    auto name = cls->name.ToString();
  //    auto instance = cls->AllocMemory();
  //    cls->ConstructCls(instance);
  //    if (instance) {
  //      auto rva = *reinterpret_cast<uintptr_t *>(instance);
  //      if (rva > RED4ext::RelocBase::GetImageBase()) {
  //        spdlog::info("#define {}_VFT_RVA 0x{:X}", name, rva - RED4ext::RelocBase::GetImageBase());
  //      }
  //    }
  //  }
  //}

  //auto classes = RED4ext::DynArray<RED4ext::CClass *>(new RED4ext::Memory::DefaultAllocator());
  //rtti->GetClasses(nullptr, classes);
  //auto vbc = rtti->GetClass("vehicleBaseObject");
  //for (auto const &cb : vbc->callbacks) {
  //  if (cb.type.name == "function") {
  //    RED4ext::CName typeName = "None";
  //    for (auto const &cls : classes) {
  //      if (cls->callbackTypeId == cb.typeId) {
  //        typeName = cls->name;
  //      }
  //    }
  //    spdlog::info("static constexpr const uintptr_t On_{}_Addr = {}", typeName.ToString(), (uintptr_t)(cb.action.OnEvent) - RED4ext::RelocBase::GetImageBase());
  //  }
  //}


  return true;
}

void FlightSystem::WorldPendingDetach(RED4ext::world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] WorldPendingDetach!");
  this->audio->ExecuteFunction("OnWorldPendingDetach");
}

void FlightSystem::WorldDetached(RED4ext::world::RuntimeScene *runtimeScene) {
	spdlog::info("[FlightSystem] WorldDetached!");
}

void FlightSystem::sub_130() {
	spdlog::info("[FlightSystem] sub_130!");
}

uint32_t FlightSystem::sub_138(uint64_t a1, uint64_t a2) {
	spdlog::info("[FlightSystem] sub_138!");
  return 0;
}

void FlightSystem::sub_140(uint64_t a1) {
	spdlog::info("[FlightSystem] sub_140!");
}

void FlightSystem::sub_148() {
	spdlog::info("[FlightSystem] sub_148!");
}

void FlightSystem::OnGameLoad(void *a1, uint64_t a2, uint64_t a3) { 
  spdlog::info("[FlightSystem] OnGameLoad!");
  auto r = new RED4ext::ResourceReference("user\\jackhumbert\\meshes\\engine_corpo.mesh");
  //r->Fetch();
  //RED4ext::ResourceLoader::Get();
  LoadResRef<bool>(&r->path, &r->token, false);

  //EnableSmoothWheelContacts.GetAddr()->value = false;
  PhysXClampHugeImpacts.GetAddr()->value = false;
  PhysXClampHugeSpeeds.GetAddr()->value = false;
  AirControlCarRollHelper.GetAddr()->value = false;
  physicsCCD.GetAddr()->value = true;
  ForceMoveToMaxLinearSpeed.GetAddr()->value = 100.0;
  //physicsCCD = true;

  spdlog::info("[FlightSystem] PhysXClampHugeImpacts: {}", PhysXClampHugeImpacts.GetAddr()->value);
  spdlog::info("[FlightSystem] PhysXClampHugeSpeeds: {}", PhysXClampHugeSpeeds.GetAddr()->value);
  spdlog::info("[FlightSystem] AirControlCarRollHelper: {}", AirControlCarRollHelper.GetAddr()->value);
  spdlog::info("[FlightSystem] ForceMoveToMaxLinearSpeed: {}", ForceMoveToMaxLinearSpeed.GetAddr()->value);
  spdlog::info("[FlightSystem] physicsCCD: {}", physicsCCD.GetAddr()->value);
}

bool FlightSystem::sub_158() {
  spdlog::info("[FlightSystem] sub_158!");
  return true;
}

void FlightSystem::OnGamePrepared() {
  spdlog::info("[FlightSystem] OnGamePrepared!");
}

void FlightSystem::sub_168() {
  spdlog::info("[FlightSystem] sub_168!");
  this->audio->Pause();
}

void FlightSystem::sub_170() {
  spdlog::info("[FlightSystem] sub_170!");
  this->audio->Resume();
}

void FlightSystem::sub_178(uintptr_t a1, bool a2) {
  spdlog::info("[FlightSystem] sub_178!");
  RED4ext::game::IGameSystem::sub_178(a1, a2);
}

void FlightSystem::OnStreamingWorldLoaded(uint64_t, bool isGameLoaded, uint64_t) {
  spdlog::info("[FlightSystem] OnStreamingWorldLoaded!");
}

void FlightSystem::sub_188() {
  spdlog::info("[FlightSystem] sub_188!");
}

void FlightSystem::sub_190(HighLow *) {
  spdlog::info("[FlightSystem] sub_190!");
}

void FlightSystem::Initialize(void **unkThing) {
  spdlog::info("[FlightSystem] Initialize!");
  this->audio = RED4ext::Handle<FlightAudio>((FlightAudio*)FlightAudio::GetRTTIType()->AllocInstance());
  this->audio->ExecuteFunction("Initialize");
}

void FlightSystem::sub_1A0() {
  spdlog::info("[FlightSystem] sub_1A0!");
}

// add FlightSystem to the game systems list to load on start

constexpr uintptr_t GetGameSystemsDataAddr = 0x2D028A0;
/// @pattern 40 53 48 83 EC 20 48 8B D9 48 8D 4C 24 30 E8 FD F9 48 FD 48 8B D0 48 8B CB E8 12 29 4A FD 48 8D
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
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetGameSystemsDataAddr), &GetGameSystemsData,
                                reinterpret_cast<void **>(&GetGameSystemsData_Original)))
    ;
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetGameSystemsDataAddr));
  }
};

REGISTER_FLIGHT_MODULE(FlightSystemModule);