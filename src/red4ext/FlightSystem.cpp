#include <fmod_errors.h>

#include "FlightSystem.hpp"

#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/Natives/GameInstance.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <iostream>
#include <RED4ext/GameOptions.hpp>


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

using namespace RED4ext;

// These can be found in strings via their variable names
// 1.61hf1 RVA: 0x484F4A8
// RelocPtr<GameOptionBool> PhysXClampHugeImpacts(0x484F4A8);
// 1.61hf1 RVA: 0x484F428
// RelocPtr<GameOptionBool> PhysXClampHugeSpeeds(0x484F428);
// 1.61hf1 RVA: 0x484E4F8
// RelocPtr<GameOptionBool> AirControlCarRollHelper(0x484E4F8);
// 1.61hf1 RVA: 0x484E670
// RelocPtr<GameOptionFloat> ForceMoveToMaxLinearSpeed(0x484E670);
UniversalRelocPtr<GameOptionBool> physicsCCD(3415871802);
// UniversalRelocPtr<GameOptionBool> EnableSmoothWheelContacts(726996866);
UniversalRelocPtr<GameOptionBool> VehicleTeleportationIfFallsUnderWorld(3118668190);

//TTypedClass<FlightSystem> icls("IFlightSystem");
//TTypedClass<FlightSystem> cls("FlightSystem");
//CClass *classPointer = &cls;

//CClass *IFlightSystem::GetNativeType() { return &icls; }

//CClass *FlightSystem::GetNativeType() { return classPointer; }

//FlightSystem *FlightSystem::GetInstance() {
//  auto fs = (FlightSystem*)CGameEngine::Get()->framework->gameInstance->GetInstance(FlightSystem::GetRTTIType());
//  return fs;
//}

Handle<FlightSystem> FlightSystem::GetInstance() {
  auto fs = (FlightSystem *)CGameEngine::Get()->framework->gameInstance->GetSystem(FlightSystem::GetRTTIType());
  return Handle<FlightSystem>(fs);
}

void FlightSystem::RegisterComponent(const Handle<FlightComponent> &fc) {
  this->flightComponentsMutex.Lock();
  this->flightComponents[fc->entity->entityID.hash] = fc;
  this->flightComponentsMutex.Unlock();
}

void FlightSystem::UnregisterComponent(const Handle<FlightComponent> &fc) {
  this->flightComponentsMutex.Lock();
  this->flightComponents.erase(fc->entity->entityID.hash);
  this->flightComponentsMutex.Unlock();
}

void PrePhysics(UpdateBucketEnum bucket, FrameInfo& frame, JobQueue& job) {
  // spdlog::info("[FlightSystem] PrePhysics!");
  auto fs = FlightSystem::GetInstance();
  auto handle = fs->soundListener.Lock();
  if (handle) {
    Matrix matrix;
    auto t = handle->worldTransform;
    GetMatrixFromOrientation(&t.Orientation, &matrix);
    matrix.W.X = t.Position.x.Bits * 0.0000076293945;
    matrix.W.Y = t.Position.y.Bits * 0.0000076293945;
    matrix.W.Z = t.Position.z.Bits * 0.0000076293945;
    matrix.W.W = 1.0;
    fs->audio->UpdateListenerMatrix(matrix);
    fs->audio->UpdateVolume();
  }
}

void UpdateComponents(UpdateBucketEnum bucket, FrameInfo& frame, JobQueue& job) {
  auto rtti = CRTTISystem::Get();
  auto fcc = FlightComponent::GetRTTIType();
  auto fs = FlightSystem::GetInstance();
  
  fs->flightComponentsMutex.LockShared();
  for (auto& [_, wh] : fs->flightComponents) {
    if (wh.Expired())
      continue;
    auto handle = wh.Lock();
    if (handle) {
      handle->OnUpdate(frame.deltaTime);
    }
  }
  fs->flightComponentsMutex.UnlockShared();
}

// vehicle allocator
// 1.52 RVA: 0x1CA37A0
// 1.61hf1 RVA: 0x1CA4690
//constexpr const uintptr_t VehicleSystemAllocator = 0x1CA4690;

//Memory::IAllocator* FlightSystem::GetAllocator() {
  //RelocFunc<decltype(&FlightSystem::GetAllocator)> call(VehicleSystemAllocator);
  //return call(this);
  //return new Memory::DefaultAllocator();
//}

void FlightSystem::OnRegisterUpdates(UpdateRegistrar *aRegistrar) {
  spdlog::info("[FlightSystem] OnRegisterUpdates!");

  aRegistrar->RegisterUpdate(UpdateBucketMask::Vehicle, UpdateBucketStage::PrePhysicsTick, this,
    "FlightSystem/PrePhysics", &PrePhysics);
    
  // UpdateTransformPostPhysics is too late
  aRegistrar->RegisterUpdate(UpdateBucketMask::Vehicle, UpdateBucketStage::PhysicsExecuteAsyncQueries, this, 
    "FlightSystem/UpdateComponents", &UpdateComponents);
 }

 void FlightSystem::OnWorldAttached(world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] OnWorldAttached");
  ExecuteFunction(this->audio, FlightAudio::GetRTTIType()->GetFunction("OnWorldAttached"), nullptr);

  // VFT Finder
  //auto rtti = CRTTISystem::Get();
  //auto classes = DynArray<CClass *>(new Memory::DefaultAllocator());
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
  //      auto rva = va - RelocBase::GetImageBase();
  //      if (rva > 0 && rva < 0x3AB847E) {
  //        spdlog::info("#define {}_VFT_RVA 0x{:X}", name, rva);
  //      }
  //    }
  //  }
  //}
  //DebugBreak();

  // Event VFT Finder
  //auto classes = DynArray<CClass *>(new Memory::DefaultAllocator());
  //rtti->GetClasses(nullptr, classes);
  //auto vbc = rtti->GetClass("vehicleBaseObject");
  //for (auto const &cb : vbc->callbacks) {
  //  if (cb.type.name == "function") {
  //    CName typeName = "None";
  //    for (auto const &cls : classes) {
  //      if (cls->callbackTypeId == cb.typeId) {
  //        typeName = cls->name;
  //      }
  //    }
  //    spdlog::info("static constexpr const uintptr_t On_{}_Addr = {}", typeName.ToString(), (uintptr_t)(cb.action.OnEvent) - RelocBase::GetImageBase());
  //  }
  //}
  //DebugBreak();
}

void FlightSystem::OnBeforeWorldDetach(world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] OnBeforeWorldDetach!");
  auto audioCls = FlightAudio::GetRTTIType();
  ExecuteFunction(this->audio, audioCls->GetFunction("OnWorldPendingDetach"), nullptr);
}

void FlightSystem::OnWorldDetached(world::RuntimeScene *runtimeScene) {
	spdlog::info("[FlightSystem] OnWorldDetached!");
}

void FlightSystem::OnAfterWorldDetach() {
	spdlog::info("[FlightSystem] OnAfterWorldDetach!");
}

uint32_t FlightSystem::OnBeforeGameSave(const JobGroup& aJobGroup, void* a2) {
	spdlog::info("[FlightSystem] OnBeforeGameSave!");
  return 0;
}

void FlightSystem::OnGameSave(void* aStream) {
	spdlog::info("[FlightSystem] OnGameSave!");
}

void FlightSystem::OnAfterGameSave() {
	spdlog::info("[FlightSystem] OnAfterGameSave!");
}

void FlightSystem::OnGameLoad(const JobGroup& aJobGroup, bool& aSuccess, void* aStream) {
  CNamePool::Add("FlightMalfunctionEffector");
  CNamePool::Add("DisableGravityEffector");
  spdlog::info("[FlightSystem] OnGameLoad!");
  auto r = ResourceReference<ent::MeshComponent>(R"(user\jackhumbert\meshes\engine_corpo.mesh)");
  LoadResRef<ent::MeshComponent>(&r.path, &r.token, false);
  r = ResourceReference<ent::MeshComponent>(R"(user\jackhumbert\meshes\engine_nomad.mesh)");
  LoadResRef<ent::MeshComponent>(&r.path, &r.token, false);

  this->flightComponents.reserve(256);

  //EnableSmoothWheelContacts.GetAddr()->value = false;
  //PhysXClampHugeImpacts.GetAddr()->value = false;
  //PhysXClampHugeSpeeds.GetAddr()->value = false;
  //AirControlCarRollHelper.GetAddr()->value = false;
  // physicsCCD.GetAddr()->value = true;
  // VehicleTeleportationIfFallsUnderWorld.GetAddr()->value = false;
  //ForceMoveToMaxLinearSpeed.GetAddr()->value = 100.0;

  // spdlog::info("[FlightSystem] PhysXClampHugeImpacts: {}", PhysXClampHugeImpacts.GetAddr()->value);
  // spdlog::info("[FlightSystem] PhysXClampHugeSpeeds: {}", PhysXClampHugeSpeeds.GetAddr()->value);
  // spdlog::info("[FlightSystem] AirControlCarRollHelper: {}", AirControlCarRollHelper.GetAddr()->value);
  // spdlog::info("[FlightSystem] ForceMoveToMaxLinearSpeed: {}", ForceMoveToMaxLinearSpeed.GetAddr()->value);
  // spdlog::info("[FlightSystem] physicsCCD: {}", physicsCCD.GetAddr()->value);
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

void* FlightSystem::IsSavingLocked(game::SaveLock* aLock, bool a2) {
  spdlog::info("[FlightSystem] IsSavingLocked!");
  return game::IGameSystem::IsSavingLocked(aLock, a2);
}

void FlightSystem::OnStreamingWorldLoaded(world::RuntimeScene* aScene, uint64_t a2, const JobGroup& aJobGroup) {
  spdlog::info("[FlightSystem] OnStreamingWorldLoaded!");
}

void FlightSystem::sub_180() {
  spdlog::info("[FlightSystem] sub_180!");
}

void FlightSystem::sub_188() {
  spdlog::info("[FlightSystem] sub_188!");
}

void FlightSystem::sub_190(void *) {
  spdlog::info("[FlightSystem] sub_190!");
}

void FlightSystem::sub_198() {
  spdlog::info("[FlightSystem] sub_198!");
}

void FlightSystem::OnInitialize(const JobHandle& aJob) {
  spdlog::info("[FlightSystem] OnInitialize!");
  this->audio = Handle<FlightAudio>((FlightAudio *)FlightAudio::GetRTTIType()->CreateInstance(true));
  ExecuteFunction(this->audio, FlightAudio::GetRTTIType()->GetFunction("Initialize"), nullptr);
}

void FlightSystem::OnUninitialize() {
  spdlog::info("[FlightSystem] OnUninitialize!");
}

REGISTER_FLIGHT_HOOK_HASH(DynArray<GameSystemData> *, 2463305925, GetGameSystemsDataHook, DynArray<GameSystemData> *gameSystemsData) {
  GetGameSystemsDataHook_Original(gameSystemsData);
  auto flightSystem = GameSystemData();
  flightSystem.name = "FlightSystem";
  flightSystem.inSingleplayer = true;
  gameSystemsData->EmplaceBack(flightSystem);
  return gameSystemsData;
}