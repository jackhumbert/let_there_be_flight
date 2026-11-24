#include <fmod_errors.h>

#include "FlightSystem.hpp"
#include "Navigation/FlightGuidance.hpp"

#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <RED4ext/Scripting/Natives/GameInstance.hpp>
#include <RED4ext/Scripting/Natives/vehiclePhysicsData.hpp>
#include <fmod.hpp>
#include <chrono>
#include <iostream>
#include <RED4ext/GameOptions.hpp>
#include <RED4ext/ResourceDepot.hpp>
#include <RED4ext/TweakDB.hpp>
#include <RED4ext/Scripting/Natives/gamedataTweakDBRecord.hpp>
#include <shared_mutex>
#include <thread>


#include "Flight/Log.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include "Flight/Component.hpp"
#include "Flight/DeveloperSettings.hpp"
#include "Audio/ListenerDraw.hpp"
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/UpdateManager.hpp>
#include "Hooks/GetMatrixFromOrientation.hpp"

using namespace RED4ext;

namespace {
// How long the loading screen may wait for the thruster meshes. The wait is bounded so that a loader that needs
// this thread, or a very slow disk, costs a warning in the log and never a hang.
constexpr auto kThrusterMeshWait = std::chrono::seconds(15);

// Every mesh a thruster can be given: IFlightConfiguration.CreateMesh's two defaults, the player.vehicle.thrusterMesh
// default, and each vehicle record's thrusterMesh (other mods' records included).
std::vector<ResourcePath> ThrusterMeshPaths() {
  std::vector<ResourcePath> paths{R"(user\jackhumbert\meshes\engine_nomad.mesh)",
                                  R"(user\jackhumbert\meshes\engine_corpo.mesh)"};
  auto tweakDB = TweakDB::Get();
  auto *meshType = CRTTISystem::Get()->GetType("raRef:CResource");
  // Other mods' records are read too, and a broken TweakXL yaml can leave a record without an instance or a flat of
  // another type under the name: both are skipped, never read as a mesh.
  auto addFlat = [&](TweakDBID aFlat) {
    std::shared_lock<SharedSpinLock> flats(tweakDB->mutex00);
    auto *flat = tweakDB->GetFlatValue(aFlat);
    if (!flat || !meshType) {
      return;
    }
    const CStackType value = flat->GetValue();
    if (value.type != meshType || !value.value) {
      return;
    }
    const auto &mesh = *static_cast<ResourceAsyncReference<> *>(value.value);
    if (mesh.path.hash != 0 && std::find(paths.begin(), paths.end(), mesh.path) == paths.end()) {
      paths.push_back(mesh.path);
    }
  };
  addFlat(TweakDBID("player.vehicle.thrusterMesh"));

  auto recordType = CRTTISystem::Get()->GetClass("gamedataVehicle_Record");
  std::vector<TweakDBID> flats;
  {
    std::shared_lock<SharedSpinLock> lock(tweakDB->mutex01);
    if (auto records = recordType ? tweakDB->recordsByType.Get(recordType) : nullptr) {
      for (const auto &record : *records) {
        if (record.instance) {
          flats.emplace_back(reinterpret_cast<game::data::TweakDBRecord *>(record.instance)->recordID, ".thrusterMesh");
        }
      }
    }
  }
  for (const auto &flat : flats) {
    addFlat(flat);
  }
  return paths;
}
} // namespace

// A thruster's mesh component is given its mesh once, when the vehicle is set up, and vehicles are set up while the
// world loads and whenever one spawns. So the meshes are loaded here, behind the loading screen, before the first
// vehicle, and the tokens are kept: a resource is only guaranteed to stay loaded while something holds its token.
void FlightSystem::LoadThrusterMeshes() {
  if (!this->thrusterMeshes.empty()) {
    return;
  }
  const auto start = std::chrono::steady_clock::now();
  auto depot = ResourceDepot::Get();
  auto loader = ResourceLoader::Get();
  const auto paths = ThrusterMeshPaths();
  for (const auto &path : paths) {
    if (!depot->ResourceExists(path)) {
      spdlog::warn("[FlightSystem] thruster mesh {:#x} is in no mounted archive", path.hash);
      continue;
    }
    if (auto token = loader->LoadAsync<CMesh>(path)) {
      this->thrusterMeshes.push_back(token);
    }
  }

  const auto allFinished = [this] {
    return std::all_of(this->thrusterMeshes.begin(), this->thrusterMeshes.end(),
                       [](const auto &aToken) { return aToken->IsFinished(); });
  };
  while (!allFinished() && std::chrono::steady_clock::now() - start < kThrusterMeshWait) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  size_t loaded = 0;
  for (const auto &token : this->thrusterMeshes) {
    if (token->IsLoaded()) {
      loaded++;
    } else {
      spdlog::warn("[FlightSystem] thruster mesh {:#x} {}", token->path.hash,
                   token->IsFailed() ? "failed to load" : "is still loading");
    }
  }
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
  spdlog::info("[FlightSystem] thruster meshes: {} of {} loaded in {} ms", loaded, paths.size(), ms.count());
}

// These can be found in strings via their variable names
// 1.61hf1 RVA: 0x484F4A8
// RelocPtr<GameOptionBool> PhysXClampHugeImpacts(0x484F4A8);
// 1.61hf1 RVA: 0x484F428
// RelocPtr<GameOptionBool> PhysXClampHugeSpeeds(0x484F428);
// 1.61hf1 RVA: 0x484E4F8
// RelocPtr<GameOptionBool> AirControlCarRollHelper(0x484E4F8);
// 1.61hf1 RVA: 0x484E670
// RelocPtr<GameOptionFloat> ForceMoveToMaxLinearSpeed(0x484E670);
// UniversalRelocPtr<GameOptionBool> physicsCCD(3415871802);
// UniversalRelocPtr<GameOptionBool> EnableSmoothWheelContacts(726996866);
// UniversalRelocPtr<GameOptionBool> VehicleTeleportationIfFallsUnderWorld(3118668190);

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
    // in third person and the cinematic camera's shots the player's soundListener stays with V (the game's own
    // listener too): hear from the camera Inputs::Gather set for those
    Matrix camera;
    bool useCamera = FlightAudio::GetCameraListener(camera);
    static auto lastLog = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (useCamera && now - lastLog > std::chrono::seconds(1)) {
      lastLog = now;
      spdlog::debug("[FlightAudio] camera listener at ({:.1f}, {:.1f}, {:.1f}) fwd ({:.2f}, {:.2f}, {:.2f}); component at ({:.1f}, {:.1f}, {:.1f})",
                    camera.W.X, camera.W.Y, camera.W.Z, camera.Y.X, camera.Y.Y, camera.Y.Z, matrix.W.X, matrix.W.Y,
                    matrix.W.Z);
    }
    fs->audio->UpdateListenerMatrix(useCamera ? camera : matrix);
    if (Flight::Developer().drawListener) {
      ListenerDraw::Draw(useCamera ? camera : matrix, useCamera);
    }
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
  // the player's flight route, after the components moved this frame
  FlightGuidance::Update(frame.deltaTime);
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
  
  // UpdateTransformPrePhysics is too early
  // PhysicsExecuteAsyncQueries is ok
  // PhysicsFlushBufferedState is ok, but crash?
  // UpdateTransformPostPhysics is too late
  // PostPhysicsTick is ok, but no animation transforms?
  aRegistrar->RegisterUpdate(UpdateBucketMask::Vehicle, UpdateBucketStage::PhysicsExecuteAsyncQueries, this, 
    "FlightSystem/UpdateComponents", &UpdateComponents);
 }

 void FlightSystem::OnWorldAttached(world::RuntimeScene *runtimeScene) {
  spdlog::info("[FlightSystem] OnWorldAttached");
  ExecuteFunction(this->audio, FlightAudio::GetRTTIType()->GetFunction("OnWorldAttached"), nullptr);
  this->LoadThrusterMeshes();

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
  FlightGuidance::Reset();
}

void FlightSystem::OnAfterWorldDetach() {
	spdlog::info("[FlightSystem] OnAfterWorldDetach!");
  this->thrusterMeshes.clear();
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
  auto func = FlightAudio::GetRTTIType()->GetFunction("Initialize");
  if (func) {
    ExecuteFunction(this->audio, func, nullptr);
  } else {
    MessageBoxW(nullptr, L"The scripts needed for Let There Be Flight could not be loaded - was Redscript compilation successful?", L"Let There Be Flight Redscript Error", MB_SYSTEMMODAL | MB_ICONERROR);
  }
}

void FlightSystem::OnUninitialize() {
  spdlog::info("[FlightSystem] OnUninitialize!");
}

// void game::GameSystemsSetup::LoadConfigFile(void)
REGISTER_FLIGHT_HOOK_HASH(DynArray<GameSystemData> *, 2463305925, GameSystemsSetup_LoadConfigFile, DynArray<GameSystemData> *gameSystemsData) {
  GameSystemsSetup_LoadConfigFile_Original(gameSystemsData);
  auto flightSystem = GameSystemData();
  flightSystem.name = "FlightSystem";
  flightSystem.inSingleplayer = true;
  gameSystemsData->EmplaceBack(flightSystem);
  return gameSystemsData;
}