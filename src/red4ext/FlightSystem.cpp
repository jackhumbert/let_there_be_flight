#include <fmod_errors.h>

#include "FlightSystem.hpp"

#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Addresses-Zoltan.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/Natives/GameInstance.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <iostream>
#include <RED4ext/GameOptions.hpp>

#include "Addresses.hpp"
#include "Flight/Log.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"
#include "LoadResRef.hpp"
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include "Flight/Component.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/UpdateManager.hpp>
#include "Hooks/GetMatrixFromOrientation.hpp"

// These can be found in strings via their variable names
// 1.52 RVA: 0x4782878
// 1.6  RVA: 0x4846C18
// 1.61 RVA: 0x484D2B8
// 1.61hf1 RVA: 0x484F4A8 diff 0x21F0
// RED4ext::RelocPtr<RED4ext::GameOptionBool> PhysXClampHugeImpacts(0x484F4A8);
// 1.52 RVA: 0x47827F8
// 1.6  RVA: 0x4846B98
// 1.61 RVA: 0x484D238
// 1.61hf1 RVA: 0x484F428
// RED4ext::RelocPtr<RED4ext::GameOptionBool> PhysXClampHugeSpeeds(0x484F428);
// 1.52 RVA: 0x47818C8
// 1.6  RVA: 0x4845C68
// 1.61 RVA: 0x484C308
// 1.61hf1 RVA: 0x484E4F8
// RED4ext::RelocPtr<RED4ext::GameOptionBool> AirControlCarRollHelper(0x484E4F8);
// 1.52 RVA: 0x4781A40
// 1.6  RVA: 0x4845DE0
// 1.61 RVA: 0x484C480
// 1.61hf1 RVA: 0x484E670
// RED4ext::RelocPtr<RED4ext::GameOptionFloat> ForceMoveToMaxLinearSpeed(0x484E670);
// 1.52 RVA: 0x4780960
// 1.6  RVA: 0x4844D00
// 1.61 RVA: 0x484B3A0
// 1.61hf1 RVA: 0x484D590
// 1.62hf1 RVA: 0x4891050
// 1.63 RVA: 0x489C060
RED4ext::RelocPtr<RED4ext::GameOptionBool> physicsCCD(0x489C060);
// 1.52 RVA: 0x4781FE8
// 1.6  RVA: 0x4846388
// 1.61 RVA: 0x484CA28
// 1.61hf1 RVA: 0x484EC18
// RED4ext::RelocPtr<RED4ext::GameOptionBool> EnableSmoothWheelContacts(0x484EC18);

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
  auto fs = (FlightSystem *)RED4ext::CGameEngine::Get()->framework->gameInstance->GetSystem(FlightSystem::GetRTTIType());
  return RED4ext::Handle<FlightSystem>(fs);
}

void FlightSystem::RegisterComponent(RED4ext::WeakHandle<FlightComponent> fc) {
  if (fc.instance && fc.refCount && !fc.Expired()) {
    this->flightComponentsMutex.Lock();
    fc.refCount->IncWeakRef();
    this->flightComponents.EmplaceBack(fc);
    this->flightComponentsMutex.Unlock();
  }
  //spdlog::info("[FlightSystem] Component added");
  // __debugbreak();
}

void FlightSystem::UnregisterComponent(RED4ext::WeakHandle<FlightComponent> fc) {
  if (fc.Expired())
    return;
  this->flightComponentsMutex.Lock();
  for (auto i = 0; i < this->flightComponents.size; i++) {
    if (this->flightComponents[i].refCount && !this->flightComponents[i].Expired()) {
      auto efc = this->flightComponents[i].Lock().GetPtr();
      if (efc == fc.Lock().GetPtr()) {
        this->flightComponents.RemoveAt(i);
        //spdlog::info("[FlightSystem] Component removed");
        break;
      }
    }
  }
  this->flightComponentsMutex.Unlock();
}

void PrePhysics(RED4ext::UpdateBucketEnum bucket, RED4ext::FrameInfo& frame, RED4ext::JobQueue& job) {
  // spdlog::info("[FlightSystem] PrePhysics!");
  auto fs = FlightSystem::GetInstance();
  auto wh = fs->soundListener;
  if (!wh.Expired()) {
    RED4ext::Matrix matrix;
    auto h = wh.Lock();
    auto t = h.GetPtr()->worldTransform;
    GetMatrixFromOrientation(&t.Orientation, &matrix);
    matrix.W.X = t.Position.x.Bits * 0.0000076293945;
    matrix.W.Y = t.Position.y.Bits * 0.0000076293945;
    matrix.W.Z = t.Position.z.Bits * 0.0000076293945;
    matrix.W.W = 1.0;
    fs->audio->UpdateListenerMatrix(matrix);
    fs->audio->UpdateVolume();
  }
}

void UpdateComponents(RED4ext::UpdateBucketEnum bucket, RED4ext::FrameInfo& frame, RED4ext::JobQueue& job) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = FlightComponent::GetRTTIType();
  for (auto const &wh : FlightSystem::GetInstance()->flightComponents) {
    if (wh.Expired())
      continue;
    auto fc = wh.Lock().GetPtr();
    if (fc) {
      fc->OnUpdate(frame.deltaTime);
    }
  }
}

// vehicle allocator
// 1.52 RVA: 0x1CA37A0
// 1.61hf1 RVA: 0x1CA4690
//constexpr const uintptr_t VehicleSystemAllocator = 0x1CA4690;

//RED4ext::Memory::IAllocator* FlightSystem::GetAllocator() {
  //RED4ext::RelocFunc<decltype(&FlightSystem::GetAllocator)> call(VehicleSystemAllocator);
  //return call(this);
  //return new RED4ext::Memory::DefaultAllocator();
//}

void FlightSystem::OnRegisterUpdates(RED4ext::UpdateRegistrar *aRegistrar) {
  spdlog::info("[FlightSystem] OnRegisterUpdates!");

  aRegistrar->RegisterUpdate(RED4ext::UpdateBucketMask::Vehicle, RED4ext::UpdateBucketStage::PrePhysicsTick, this,
                               "FlightSystem/PrePhysics", &PrePhysics);
  aRegistrar->RegisterUpdate(RED4ext::UpdateBucketMask::Vehicle, RED4ext::UpdateBucketStage::PhysicsExecuteAsyncQueries, this,
                               "FlightSystem/UpdateComponents", &UpdateComponents);
 }

 void FlightSystem::OnWorldAttached(RED4ext::world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] OnWorldAttached");
  this->audio->ExecuteFunction("OnWorldAttached");

  // VFT Finder
  //auto rtti = RED4ext::CRTTISystem::Get();
  //auto classes = RED4ext::DynArray<RED4ext::CClass *>(new RED4ext::Memory::DefaultAllocator());
  //rtti->GetClasses(nullptr, classes);

  //spdlog::info("Printing all class VFTs");
  //for (const auto &cls : classes) {
  //  if (cls && cls->name != "None" && !cls->flags.isAbstract) {
  //    if (cls->name == "inkInputKeyIconManager")
  //      continue;
  //    auto name = cls->name.ToString();
  //    auto instance = cls->AllocMemory();
  //    cls->ConstructCls(instance);
  //    if (instance) {
  //      auto va = *reinterpret_cast<uintptr_t *>(instance);
  //      auto rva = va - RED4ext::RelocBase::GetImageBase();
  //      if (rva > 0 && rva < 0x3AB847E) {
  //        spdlog::info("#define {}_VFT_RVA 0x{:X}", name, rva);
  //      }
  //    }
  //  }
  //}
  //DebugBreak();

  // Event VFT Finder
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
  //DebugBreak();
}

void FlightSystem::OnBeforeWorldDetach(RED4ext::world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] OnBeforeWorldDetach!");
  this->audio->ExecuteFunction("OnWorldPendingDetach");
}

void FlightSystem::OnWorldDetached(RED4ext::world::RuntimeScene *runtimeScene) {
	spdlog::info("[FlightSystem] OnWorldDetached!");
}

void FlightSystem::OnAfterWorldDetach() {
	spdlog::info("[FlightSystem] OnAfterWorldDetach!");
}

uint32_t FlightSystem::OnBeforeGameSave(const RED4ext::JobGroup& aJobGroup, void* a2) {
	spdlog::info("[FlightSystem] OnBeforeGameSave!");
  return 0;
}

void FlightSystem::OnGameSave(void* aStream) {
	spdlog::info("[FlightSystem] OnGameSave!");
}

void FlightSystem::OnAfterGameSave() {
	spdlog::info("[FlightSystem] OnAfterGameSave!");
}

void FlightSystem::OnGameLoad(const RED4ext::JobGroup& aJobGroup, bool& aSuccess, void* aStream) {
  RED4ext::CNamePool::Add("FlightMalfunctionEffector");
  RED4ext::CNamePool::Add("DisableGravityEffector");
  spdlog::info("[FlightSystem] OnGameLoad!");
  auto r = RED4ext::ResourceReference<RED4ext::ent::MeshComponent>(R"(user\jackhumbert\meshes\engine_corpo.mesh)");
  LoadResRef<RED4ext::ent::MeshComponent>(&r.path, &r.token, false);
  r = RED4ext::ResourceReference<RED4ext::ent::MeshComponent>(R"(user\jackhumbert\meshes\engine_nomad.mesh)");
  LoadResRef<RED4ext::ent::MeshComponent>(&r.path, &r.token, false);

  //EnableSmoothWheelContacts.GetAddr()->value = false;
  //PhysXClampHugeImpacts.GetAddr()->value = false;
  //PhysXClampHugeSpeeds.GetAddr()->value = false;
  //AirControlCarRollHelper.GetAddr()->value = false;
  physicsCCD.GetAddr()->value = true;
  //ForceMoveToMaxLinearSpeed.GetAddr()->value = 100.0;

  // spdlog::info("[FlightSystem] PhysXClampHugeImpacts: {}", PhysXClampHugeImpacts.GetAddr()->value);
  // spdlog::info("[FlightSystem] PhysXClampHugeSpeeds: {}", PhysXClampHugeSpeeds.GetAddr()->value);
  // spdlog::info("[FlightSystem] AirControlCarRollHelper: {}", AirControlCarRollHelper.GetAddr()->value);
  // spdlog::info("[FlightSystem] ForceMoveToMaxLinearSpeed: {}", ForceMoveToMaxLinearSpeed.GetAddr()->value);
  spdlog::info("[FlightSystem] physicsCCD: {}", physicsCCD.GetAddr()->value);
}

bool FlightSystem::OnGameRestored() {
  spdlog::info("[FlightSystem] OnGameRestored!");
  return true;
}

void FlightSystem::OnGamePrepared() {
  spdlog::info("[FlightSystem] OnGamePrepared!");
}

void FlightSystem::OnGamePaused() {
  spdlog::info("[FlightSystem] OnGamePaused!");
  this->audio->Pause();
}

void FlightSystem::OnGameResumed() {
  spdlog::info("[FlightSystem] OnGameResumed!");
  this->audio->Resume();
}

void* FlightSystem::IsSavingLocked(RED4ext::game::SaveLock* aLock, bool a2) {
  spdlog::info("[FlightSystem] IsSavingLocked!");
  return RED4ext::game::IGameSystem::IsSavingLocked(aLock, a2);
}

void FlightSystem::OnStreamingWorldLoaded(RED4ext::world::RuntimeScene* aScene, uint64_t a2, const RED4ext::JobGroup& aJobGroup) {
  spdlog::info("[FlightSystem] OnStreamingWorldLoaded!");
}

void FlightSystem::sub_188() {
  spdlog::info("[FlightSystem] sub_188!");
}

void FlightSystem::sub_190(RED4ext::IGameSystem::HighLow *) {
  spdlog::info("[FlightSystem] sub_190!");
}

void FlightSystem::OnInitialize(const RED4ext::JobHandle& aJob) {
  spdlog::info("[FlightSystem] OnInitialize!");
  this->audio = RED4ext::Handle<FlightAudio>((FlightAudio *)FlightAudio::GetRTTIType()->CreateInstance());
  this->audio->ExecuteFunction("Initialize");
}

void FlightSystem::OnUninitialize() {
  spdlog::info("[FlightSystem] OnUninitialize!");
}

// add FlightSystem to the game systems list to load on start
RED4ext::DynArray<RED4ext::GameSystemData>* __fastcall GetGameSystemsData(RED4ext::DynArray<RED4ext::GameSystemData>* gameSystemsData);

decltype(&GetGameSystemsData) GetGameSystemsData_Original;

RED4ext::DynArray<RED4ext::GameSystemData>* __fastcall GetGameSystemsData(
  RED4ext::DynArray<RED4ext::GameSystemData>* gameSystemsData) {
  GetGameSystemsData_Original(gameSystemsData);
  auto flightSystem = RED4ext::GameSystemData();
  flightSystem.name = "FlightSystem";
  flightSystem.inSingleplayer = true;
  gameSystemsData->EmplaceBack(flightSystem);
  return gameSystemsData;
}

struct FlightSystemModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) override {
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetGameSystemsData_Addr), (void*)&GetGameSystemsData,
                                reinterpret_cast<void **>(&GetGameSystemsData_Original)))
    ;
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) override {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetGameSystemsData_Addr));
  }
};

REGISTER_FLIGHT_MODULE(FlightSystemModule);