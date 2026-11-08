#include "FlightConfiguration.hpp"
#include "Flight/DeveloperSettings.hpp"
#include <RED4ext/Memory/Allocators.hpp>
#include <PhysX3.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderSphere.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/ColliderBox.hpp>
#include <algorithm>
#include <RED4ext/Scripting/Natives/Generated/physics/SimulationFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/QueryFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/FilterData.hpp>
#include <RED4ext/Scripting/Natives/physicsProxyHelper.hpp>
#include <RED4ext/Scripting/Natives/physicsPhysicalSystemProxy.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include "Flight/ChassisShape.hpp"
#include "Flight/FlightStats.hpp"

using namespace RED4ext;

IFlightConfiguration::~IFlightConfiguration() {
  component.~WeakHandle();
  for (auto& thruster : this->thrusters) {
    thruster.~Handle();
  }
}

CClass* IFlightConfiguration::GetConfigurationClass(ent::Entity* entity) {
  auto rtti = CRTTISystem::Get();

  if (!entity) {
    return nullptr;
  }

  bool isCar = entity->IsOfClass(rtti->GetClass("vehicleCarBaseObject"));
  bool isBike = entity->IsOfClass(rtti->GetClass("vehicleBikeBaseObject"));

  uint8_t isSixWheeler = 0;

  auto placedCls = rtti->GetClass("entIPlacedComponent");
  auto hardBindingCls = rtti->GetClass("entHardTransformBinding");

  if (!Utils::LooksLikeDynArray(entity->componentsStorage.components)) {
    LTBF_WARN_ONCE("[FlightConfiguration] componentsStorage.components header looks corrupt (size {} capacity {}); no flight configuration",
                   entity->componentsStorage.components.size, entity->componentsStorage.components.capacity);
    return nullptr;
  }

  for (auto const &handle : entity->componentsStorage.components) {
    auto component = handle.GetPtr();
    if (!component) {
      LTBF_WARN_ONCE("[FlightConfiguration] null component handle in componentsStorage while looking for a configuration class");
      continue;
    }
    auto type = component->GetNativeType();
    bool isPlacedComponent = type && placedCls && type->IsA(placedCls);

    if (isPlacedComponent) {
      auto pth = ((ent::IPlacedComponent *)component)->parentTransform;
      if (pth) {
        // only entHardTransformBinding has slotName; other ITransformBinding subclasses do not
        auto ptType = pth->GetNativeType();
        if (ptType && hardBindingCls && ptType->IsA(hardBindingCls)) {
          auto pt = reinterpret_cast<ent::HardTransformBinding *>(pth.GetPtr());
          if (pt->slotName == "wheel_front_left_b") {
            isSixWheeler |= 1;
          }
          if (pt->slotName == "wheel_back_left_b") {
            isSixWheeler |= 2;
          }
          // the third and fourth rear axles: only the Bratsk U4020 extended has them in vanilla
          if (pt->slotName == "wheel_back_left_c") {
            isSixWheeler |= 4;
          }
        }
      }
    }
  }

  char className[256];
  auto appearance = entity->currentAppearance.ToString();
  sprintf_s(className, "FlightConfiguration_%s", appearance ? appearance : "None");

  auto configurationCls = rtti->GetClassByScriptName(className);
  if (!configurationCls) {
    // every axle gets its thrusters (tools/flightsim/thruster_slots.py checks each vanilla rig against these)
    if (isSixWheeler & 4) {
      configurationCls = rtti->GetClassByScriptName("CustomTwelveWheelCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("TwelveWheelCarFlightConfiguration");
      }
    } else if (isSixWheeler == 3) {
      configurationCls = rtti->GetClassByScriptName("CustomEightWheelCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("EightWheelCarFlightConfiguration");
      }
    } else if (isSixWheeler & 1) {
      configurationCls = rtti->GetClassByScriptName("CustomSixWheelCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("SixWheelCarFlightConfiguration");
      }
    } else if (isSixWheeler & 2) {
      configurationCls = rtti->GetClassByScriptName("CustomSixWheelRearCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("SixWheelRearCarFlightConfiguration");
      }
    } else if (isCar) {
      configurationCls = rtti->GetClassByScriptName("CustomCarFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("CarFlightConfiguration");
      }
    } else if (isBike) {
      configurationCls = rtti->GetClassByScriptName("CustomBikeFlightConfiguration");
      if (!configurationCls) {
        configurationCls = rtti->GetClassByScriptName("BikeFlightConfiguration");
      }
    }
  } else {
    spdlog::info("Found custom flight configuration class: {}", className);
  }
  return configurationCls;
}

void IFlightConfiguration::Setup(vehicle::BaseObject * vehicle) {

  // do not rely on the C++ constructor running for script-derived configuration
  // classes: an unarmed -1 is what keeps RemoveColliders off the chassis shapes
  this->originalShapeCount = -1;

  this->thrusters = DynArray<Handle<IFlightThruster>>(new Memory::DefaultAllocator());
  auto onInit = GetType()->GetFunction("OnSetup");
  if (onInit) {
    auto rtti = CRTTISystem::Get();
    CStackType args[1];
    auto handle = Handle<vehicle::BaseObject>(vehicle);
    args[0] = CStackType(rtti->GetType("handle:vehicleBaseObject"), &handle);
    auto stack = CStack(this, args, 1, nullptr);
    onInit->Execute(&stack);
  }
}

void IFlightConfiguration::AddSlots(ent::SlotComponent *slotComponent) {
  auto rtti = CRTTISystem::Get();

  if (!slotComponent || !Utils::LooksLikeDynArray(slotComponent->slots)) {
    LTBF_WARN_ONCE("[FlightConfiguration] vehicle_slots component missing or its slots header looks corrupt; flight camera slot not added");
    return;
  }

  auto slot = reinterpret_cast<ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance(true));
  slot->boneName = this->flightCameraBone;
  slot->slotName = "CustomFlightCamera";
  slot->relativePosition = this->flightCameraOffset;
  slotComponent->slots.EmplaceBack(*slot);
  slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);

  //for (auto thruster : thrusters) {
  //  auto slot = reinterpret_cast<ent::Slot *>(rtti->GetClass("entSlot")->CreateInstance(true));
  //  slot->boneName = thruster->boneName;
  //  slot->slotName = thruster->slotName;
  //  slot->relativePosition = thruster->relativePosition;
  //  slot->relativeRotation = thruster->relativeRotation;
  //  //slot->relativeRotation = Quaternion(0.0, 0.0, 0.0, 1.0);
  //  slotComponent->slots.EmplaceBack(*slot);
  //  slotComponent->slotIndexLookup.Emplace(slot->slotName, slotComponent->slots.size - 1);
  //}
}

// void IFlightConfiguration::AddMeshes(ent::Entity *entity, ent::VisualControllerComponent *vcc) {
//   for (auto thruster : thrusters) {
//     auto mesh = CreateThrusterEngine(thruster->meshPath, thruster->meshName, thruster->slotName);
//     thruster->meshComponent = Handle<ent::MeshComponent>(mesh);
//     entity->componentsStorage.components.EmplaceBack(thruster->meshComponent);
//     AddToController(vcc, mesh);
//   }
// }

// the chassis body holds shapes we never touch (wheels, wreck pieces), so the
// live list is queried into a buffer big enough for all of them
static constexpr uint32_t shapeQueryBufferSize = 64;

bool IFlightConfiguration::IsTrackedShape(physx::PxShape *shape) const {
  for (int32_t i = 0; i < this->attachedShapeCount; i++) {
    if (this->attachedShapes[i] == shape)
      return true;
  }
  return false;
}

void IFlightConfiguration::TrackShape(physx::PxShape *shape) {
  if (this->attachedShapeCount >= IFlightConfiguration::maxTrackedShapes) {
    LTBF_WARN_ONCE("[FlightConfiguration] more than {} thruster shapes attached; the rest will not be removed",
                   int32_t(IFlightConfiguration::maxTrackedShapes));
    return;
  }
  this->attachedShapes[this->attachedShapeCount] = shape;
  this->attachedShapeCount++;
}

void IFlightConfiguration::AddColliders() {
  auto rtti = CRTTISystem::Get();

  auto filterDataCls = rtti->GetClass("physicsFilterData");

  auto flightComponent = this->component.Lock();

  if (!flightComponent || !flightComponent->entity) {
    LTBF_WARN_ONCE("[FlightConfiguration] AddColliders: flight component or its entity is gone");
    return;
  }

  if (!flightComponent->entity->IsOfClass(rtti->GetClass("vehicleBaseObject")))
    return;

  auto chassis = EntityExt::From(flightComponent->entity)->FindComponent<vehicle::ChassisComponent>();
  auto slot = EntityExt::From(flightComponent->entity)->FindComponent<ent::SlotComponent>("vehicle_slots");

  if (chassis != NULL && slot != NULL) {
    // FlightComponent::Get((vehicle::BaseObject*)flightComponent->entity)->chassis = chassis;
    flightComponent->chassis = chassis;
    physics::ProxyHelper proxyHelper(chassis->proxyID, &chassis->sharedMutex);

    auto key = (physics::PhysicalSystemProxy *) physics::ProxyID::GetProxy(chassis->proxyID);
    if (!key || !Utils::LooksLikeDynArray(key->bodies) || key->bodies.size == 0 || !key->bodies.entries[0]) {
      LTBF_WARN_ONCE("[FlightConfiguration] AddColliders: chassis physics proxy has no body; thruster colliders not added");
      return;
    }
    auto body = (physx::PxRigidDynamic *) key->bodies.entries[0];

    if (this->originalShapeCount == -1) {
      this->originalShapeCount = body->getNbShapes();
      // nothing of ours is attached, so any pointer left over is stale
      this->attachedShapeCount = 0;
    }

    auto filterData = (physics::FilterData*)malloc(sizeof(physics::FilterData));
    physics::FilterData::Init(filterData);
    
    filterData->LoadPreset("Vehicle Chassis");
    Vector3 unk140(1.0, 1.0, 1.0);
    Transform transform;
    int index = 0;

    for (auto const &thruster: this->thrusters) {
      if (thruster && thruster->attached) {
        Handle<physics::ICollider> collider;
        float radius = Flight::Developer().thrusterRadius;
        // the pod as drawn (engine_corpo.mesh's bounds, about the slot): with thrusterOpenBottom a box over its sides and
        // top, open that far up from its bottom, so the road under it is not touched while the skid holds the pod's
        // bottom over it, and walls and kerbs still are; otherwise a sphere of thrusterRadius
        constexpr float kPodHalfX = 0.33f, kPodHalfY = 0.36f, kPodBottom = -0.245f, kPodTop = 0.245f;
        const float open = std::clamp(Flight::Developer().thrusterOpenBottom, 0.0f, kPodTop - kPodBottom - 0.05f);
        float lift = 0.0f, bottom = -radius;
        if (open > 0.0f) {
          auto box = reinterpret_cast<physics::ColliderBox *>(rtti->GetClass("physicsColliderBox")->CreateInstance(true));
          collider = Handle<physics::ICollider>(box);
          box->halfExtents = Vector3(kPodHalfX, kPodHalfY, (kPodTop - kPodBottom - open) * 0.5f);
          lift = (kPodTop + kPodBottom + open) * 0.5f;
          bottom = kPodBottom;
        } else {
          physics::ColliderSphere::createHandleWithRadius(&collider, &radius);
        }

        collider->material = "vehicle_chassis.physmat";

        index = slot->GetSlotIndex(thruster->slotName);
        if (index != -1) {
          slot->GetLocalSlotTransformFromIndex(index, &transform);
          collider->localToBody.position = transform.position - chassis->localTransform.Position.AsVector4();
          collider->localToBody.position.Z += lift;
          // the skid holds the pod's bottom as drawn (or the sphere's) over the ground
          this->colliderBottom = std::min(this->colliderBottom, transform.position.Z + bottom);
          collider->localToBody.orientation = Quaternion(0.0, 0.0, 0.0, 1.0);
            
          auto shape = (physx::PxShape *) collider->CreatePxShape(&unk140, nullptr, 1, nullptr);
          shape->setSimulationFilterData(&filterData->simulationFilter);
          shape->setQueryFilterData(&filterData->queryFilter);

          body->attachShape(*shape);
          this->TrackShape(shape);

          shape->release2();
        }
      }
    }

    free(filterData);

    physx::PxShape *shapes[shapeQueryBufferSize];
    auto nbShapes = body->getShapes(shapes, shapeQueryBufferSize, 0);

    proxyHelper.mutex->Lock();
    // add the indices of our own shapes to the bottom mask
    for (uint32_t i = 0; i < nbShapes; i++) {
      if (this->IsTrackedShape(shapes[i])) {
//      chassis->unk174 |= (1 << i);
        proxyHelper.SetSimulationShape(true, 0, i);
        proxyHelper.SetIsQueryable(true, 0, i);
      }
    }

    proxyHelper.UpdateProxyCache();
    proxyHelper.Unlock();
  }
}

void IFlightConfiguration::MeasureCharacter() {
  auto flightComponent = this->component.Lock();
  if (!flightComponent || !flightComponent->entity)
    return;
  auto stats = FlightStats::Of(flightComponent.GetPtr());
  if (!stats)
    return;
  float thrusters = 0.0f;
  for (auto const &thruster : this->thrusters)
    if (thruster && thruster->attached)
      thrusters += 1.0f;
  stats->s_thrusters = thrusters;

  if (!this->dragAreaMeasured) {
    auto rtti = CRTTISystem::Get();
    auto chassis = EntityExt::From(flightComponent->entity)->FindComponent<vehicle::ChassisComponent>();
    std::vector<Flight::ShapePiece> pieces;
    std::string why;
    if (Flight::ChassisPieces(chassis, pieces, why)) {
      const bool bike = flightComponent->entity->IsOfClass(rtti->GetClass("vehicleBikeBaseObject"));
      const Flight::Vec3 areas = Flight::SilhouetteAreas(pieces);
      const float height = Flight::ShapeHeight(pieces);
      this->chassisBottom = 1e9f;
      Flight::Vec3 low{1e9f, 1e9f, 1e9f}, high{-1e9f, -1e9f, -1e9f};
      for (const auto &piece : pieces)
        for (const Flight::Vec3 &v : piece) {
          this->chassisBottom = std::min(this->chassisBottom, v.z);
          low = {std::min(low.x, v.x), std::min(low.y, v.y), std::min(low.z, v.z)};
          high = {std::max(high.x, v.x), std::max(high.y, v.y), std::max(high.z, v.z)};
        }
      this->chassisSize = Vector3(high.x - low.x, high.y - low.y, high.z - low.z);
      const Flight::Vec3 cda = Flight::DragAreaOf(areas, height, bike);
      this->dragArea = Vector4(cda.x, cda.y, cda.z, 0.0f);
      this->dragAreaMeasured = true;
      spdlog::info("[FlightConfiguration] drag area {:.2f} {:.2f} {:.2f} m2 (silhouette {:.2f} {:.2f} {:.2f} m2, {:.2f} m "
                   "tall, {} pieces{}), {} thrusters",
                   cda.x, cda.y, cda.z, areas.x, areas.y, areas.z, height, pieces.size(), bike ? ", a bike" : "",
                   thrusters);
    } else {
      spdlog::info("[FlightConfiguration] no drag area yet: {}", why);
    }
  }
  stats->s_dragArea = this->dragArea;
}

void IFlightConfiguration::OnActivationCore() {
  this->AddColliders();
  this->MeasureCharacter();
  ExecuteFunction(this, this->nativeType->GetFunction("OnActivation"), nullptr);
}

void IFlightConfiguration::RemoveColliders() {
  if (this->originalShapeCount == -1 || this->attachedShapeCount == 0) {
    this->attachedShapeCount = 0;
    this->originalShapeCount = -1;
    return;
  }

  auto rtti = CRTTISystem::Get();

  auto flightComponent = this->component.Lock();

  if (!flightComponent || !flightComponent->entity) {
    LTBF_WARN_ONCE("[FlightConfiguration] RemoveColliders: flight component or its entity is gone");
    return;
  }

  if (!flightComponent->entity->IsOfClass(rtti->GetClass("vehicleBaseObject")))
    return;

  auto chassis = EntityExt::From(flightComponent->entity)->FindComponent<vehicle::ChassisComponent>();

  if (chassis != NULL) {
    physics::ProxyHelper proxyHelper(chassis->proxyID, &chassis->sharedMutex);

    auto key = (physics::PhysicalSystemProxy *) physics::ProxyID::GetProxy(chassis->proxyID);
    if (!key || !Utils::LooksLikeDynArray(key->bodies) || key->bodies.size == 0 || !key->bodies.entries[0]) {
      LTBF_WARN_ONCE("[FlightConfiguration] RemoveColliders: chassis physics proxy has no body");
      return;
    }
    auto body = (physx::PxRigidDynamic *) key->bodies.entries[0];

    // remove indexes from bottom mask
//    for (int i = this->originalShapeCount; i < nbShapes; i++) {
//      chassis->unk174 &= ~(1 << i);
//      proxyHelper.SetSimulationShape(false, 0, i);
//      proxyHelper.SetIsQueryable(false, 0, i);
//    }
    proxyHelper.mutex->Lock();

    // the shape array is re-queried and matched by pointer: the game compacts
    // it on removal and destruction both removes chassis shapes and adds wreck
    // ones, so anything derived from the index range we attached at is wrong
    physx::PxShape *shapes[shapeQueryBufferSize];
    auto nbShapes = body->getShapes(shapes, shapeQueryBufferSize, 0);
    for (uint32_t i = 0; i < nbShapes; i++) {
      // this might cause a crash when the game exits with:
      // * vehicle flight active?
      // * more than one vehicle flight active?
      if (this->IsTrackedShape(shapes[i])) {
        body->detachShape(*shapes[i], true);
      }
    }

    proxyHelper.UpdateProxyCache();
    proxyHelper.Unlock();
  }

  this->attachedShapeCount = 0;
  this->originalShapeCount = -1;
}

void IFlightConfiguration::OnDeactivationCore() {
  this->RemoveColliders();
  ExecuteFunction(this, this->nativeType->GetFunction("OnDeactivation"), nullptr);
}