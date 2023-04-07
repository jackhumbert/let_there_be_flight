#include <fmod_errors.h>

#include "FlightSystem.hpp"

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Addresses.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/Natives/GameInstance.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <iostream>
#include <RED4ext/GameOptions.hpp>

#include "Addresses.hpp"
#include "Utils/FlightLog.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"
#include "LoadResRef.hpp"
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include "FlightComponent.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/UpdateManager.hpp>

// These can be found in strings via their variable names
// 1.52 RVA: 0x4782878
// 1.6  RVA: 0x4846C18
// 1.61 RVA: 0x484D2B8
// 1.61hf1 RVA: 0x484F4A8 diff 0x21F0
RED4ext::RelocPtr<RED4ext::GameOptionBool> PhysXClampHugeImpacts(0x484F4A8);
// 1.52 RVA: 0x47827F8
// 1.6  RVA: 0x4846B98
// 1.61 RVA: 0x484D238
// 1.61hf1 RVA: 0x484F428
RED4ext::RelocPtr<RED4ext::GameOptionBool> PhysXClampHugeSpeeds(0x484F428);
// 1.52 RVA: 0x47818C8
// 1.6  RVA: 0x4845C68
// 1.61 RVA: 0x484C308
// 1.61hf1 RVA: 0x484E4F8
RED4ext::RelocPtr<RED4ext::GameOptionBool> AirControlCarRollHelper(0x484E4F8);
// 1.52 RVA: 0x4781A40
// 1.6  RVA: 0x4845DE0
// 1.61 RVA: 0x484C480
// 1.61hf1 RVA: 0x484E670
RED4ext::RelocPtr<RED4ext::GameOptionFloat> ForceMoveToMaxLinearSpeed(0x484E670);
// 1.52 RVA: 0x4780960
// 1.6  RVA: 0x4844D00
// 1.61 RVA: 0x484B3A0
// 1.61hf1 RVA: 0x484D590
RED4ext::RelocPtr<RED4ext::GameOptionBool> physicsCCD(0x484D590);
// 1.52 RVA: 0x4781FE8
// 1.6  RVA: 0x4846388
// 1.61 RVA: 0x484CA28
// 1.61hf1 RVA: 0x484EC18
RED4ext::RelocPtr<RED4ext::GameOptionBool> EnableSmoothWheelContacts(0x484EC18);

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

RED4ext::Matrix* __fastcall GetMatrixFromOrientation(RED4ext::Quaternion* q, RED4ext::Matrix* m) {
  RED4ext::RelocFunc<decltype(&GetMatrixFromOrientation)> call(GetMatrixFromOrientation_Addr);
  return call(q, m);
}

void FlightSystem::RegisterComponent(RED4ext::WeakHandle<FlightComponent> fc) {
  if (fc.instance && !fc.Expired()) {
    this->flightComponentsMutex.Lock();
    this->flightComponents.EmplaceBack(fc);
    this->flightComponentsMutex.Unlock();
  }
  //spdlog::info("[FlightSystem] Component added");
}

void FlightSystem::UnregisterComponent(RED4ext::WeakHandle<FlightComponent> fc) {
  if (fc.Expired())
    return;
  for (auto i = 0; i < this->flightComponents.size; i++) {
    if (this->flightComponents[i].refCount && !this->flightComponents[i].Expired()) {
      auto efc = this->flightComponents[i].Lock().GetPtr();
      if (efc == fc.Lock().GetPtr()) {
        this->flightComponentsMutex.Lock();
        this->flightComponents.RemoveAt(i);
        this->flightComponentsMutex.Unlock();
        //spdlog::info("[FlightSystem] Component removed");
        break;
      }
    }
  }
}

void PrePhysics(RED4ext::Unk2 *unk2, float deltaTime, void *unkStruct) {
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

void UpdateComponents(RED4ext::Unk2 *unk2, float deltaTime, void *unkStruct) {
  auto rtti = RED4ext::CRTTISystem::Get();
  auto fcc = FlightComponent::GetRTTIType();
  for (auto const &wh : FlightSystem::GetInstance()->flightComponents) {
    if (wh.Expired())
      continue;
    auto fc = wh.Lock().GetPtr();
    if (fc) {
      fc->OnUpdate(deltaTime);
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
  RED4ext::CNamePool::Add("FlightMalfunctionEffector");
  RED4ext::CNamePool::Add("DisableGravityEffector");
  spdlog::info("[FlightSystem] OnGameLoad!");
  auto r = RED4ext::ResourceReference<RED4ext::ent::MeshComponent>("user\\jackhumbert\\meshes\\engine_corpo.mesh");
  LoadResRef<RED4ext::ent::MeshComponent>(&r.path, &r.token, false);
  r = RED4ext::ResourceReference<RED4ext::ent::MeshComponent>("user\\jackhumbert\\meshes\\engine_nomad.mesh");
  LoadResRef<RED4ext::ent::MeshComponent>(&r.path, &r.token, false);

  //EnableSmoothWheelContacts.GetAddr()->value = false;
  //PhysXClampHugeImpacts.GetAddr()->value = false;
  //PhysXClampHugeSpeeds.GetAddr()->value = false;
  //AirControlCarRollHelper.GetAddr()->value = false;
  physicsCCD.GetAddr()->value = true;
  //ForceMoveToMaxLinearSpeed.GetAddr()->value = 100.0;

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
  this->audio = RED4ext::Handle<FlightAudio>((FlightAudio *)FlightAudio::GetRTTIType()->CreateInstance());
  this->audio->ExecuteFunction("Initialize");
}

void FlightSystem::sub_1A0() {
  spdlog::info("[FlightSystem] sub_1A0!");
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
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetGameSystemsData_Addr), &GetGameSystemsData,
                                reinterpret_cast<void **>(&GetGameSystemsData_Original)))
    ;
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetGameSystemsData_Addr));
  }
};

REGISTER_FLIGHT_MODULE(FlightSystemModule);