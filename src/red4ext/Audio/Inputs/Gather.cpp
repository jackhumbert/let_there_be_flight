#include "Gather.hpp"

#include "Audio/FlightAudio.hpp"
#include "Flight/Component.hpp"
#include "Flight/FlightModeAutopilot.hpp"
#include "Flight/FlightStats.hpp"
#include "FlightController.hpp"
#include "FlightThruster.hpp"
#include "Inputs.hpp"

#include <RED4ext/GameEngine.hpp>
#include <RED4ext/Scripting/Natives/Generated/Matrix.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/StatsObjectID.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/StatPoolType.hpp>
#include <RED4ext/Scripting/Natives/IGameInstance.hpp>
#include <RED4ext/Scripting/Natives/vehicleBaseObject.hpp>

#include <spdlog/spdlog.h>

#include <mutex>
#include <string>
#include <unordered_map>

// defined in Hooks/GetMatrixFromOrientation.hpp, which FlightSystem.cpp includes (the header defines it, so
// only one file can)
RED4ext::Matrix *__fastcall GetMatrixFromOrientation(RED4ext::Quaternion *q, RED4ext::Matrix *m);

using namespace RED4ext;

namespace Gather {

namespace {

// the script classes' fields, found once
struct Props {
  bool ok = true;
  // FlightComponent
  CProperty *isPlayerMounted, *isDestroyed, *collisionTimer, *surge, *lift, *roll, *pitch, *yaw, *smoothForce,
      *mode, *modes;
  // FlightMode (the Autopilot's commands and lift factor are its native fields, FlightModeAutopilot.hpp)
  CProperty *gravityFactor;
  // FlightSystem, FlightController
  CProperty *ctlr, *isTPP;
  // IFlightThruster
  CProperty *isFront, *id, *wheelIndex;
  // FlightAudio
  CProperty *acoustics, *acousticsGain, *acousticsRaySource;
  CClass *autopilot, *drone, *wheeled, *vehicle;
};

CProperty *Need(Props &p, const char *aClass, const char *aName) {
  auto cls = CRTTISystem::Get()->GetClass(aClass);
  auto prop = cls ? cls->GetProperty(aName) : nullptr;
  if (!prop) {
    spdlog::error("[Gather] {}.{} not found: the audio inputs are off", aClass, aName);
    p.ok = false;
  }
  return prop;
}

Props Resolve() {
  Props p;
  auto rtti = CRTTISystem::Get();
  p.isPlayerMounted = Need(p, "FlightComponent", "isPlayerMounted");
  p.isDestroyed = Need(p, "FlightComponent", "isDestroyed");
  p.collisionTimer = Need(p, "FlightComponent", "collisionTimer");
  p.surge = Need(p, "FlightComponent", "surge");
  p.lift = Need(p, "FlightComponent", "lift");
  p.roll = Need(p, "FlightComponent", "roll");
  p.pitch = Need(p, "FlightComponent", "pitch");
  p.yaw = Need(p, "FlightComponent", "yaw");
  p.smoothForce = Need(p, "FlightComponent", "smoothForce");
  p.mode = Need(p, "FlightComponent", "mode");
  p.modes = Need(p, "FlightComponent", "modes");
  p.gravityFactor = Need(p, "FlightMode", "gravityFactor");
  p.ctlr = Need(p, "FlightSystem", "ctlr");
  p.isTPP = Need(p, "FlightController", "isTPP");
  p.isFront = Need(p, "IFlightThruster", "isFront");
  p.id = Need(p, "IFlightThruster", "id");
  p.wheelIndex = Need(p, "IFlightThruster", "wheelIndex");
  p.acoustics = Need(p, "FlightAudio", "acoustics");
  p.acousticsGain = Need(p, "FlightAudio", "acousticsGain");
  p.acousticsRaySource = Need(p, "FlightAudio", "acousticsRaySource");
  p.autopilot = rtti->GetClass("FlightModeAutopilot");
  p.drone = rtti->GetClass("FlightModeDrone");
  p.wheeled = rtti->GetClass("vehicleWheeledBaseObject");
  p.vehicle = rtti->GetClass("vehicleBaseObject");
  p.ok = p.ok && p.autopilot && p.drone && p.wheeled && p.vehicle;
  return p;
}

const Props &GetProps() {
  static const Props props = Resolve();
  return props;
}

// a field of a script object
template <typename T>
const T &Ref(CProperty *aProp, void *aOwner) {
  return *aProp->GetValuePtr<T>(aOwner);
}

IScriptable *Object(CProperty *aProp, void *aOwner) {
  return aOwner ? Ref<Handle<IScriptable>>(aProp, aOwner).GetPtr() : nullptr;
}

Inputs::Vec3 Vec(const Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }

Inputs::Vec3 Vec(CProperty *aProp, void *aOwner) { return Vec(Ref<Vector4>(aProp, aOwner)); }

// an InputPID's input, 0 when there is none
float Input(const Handle<FlightInputPID> &aPid) { return aPid ? aPid->inputFloat : 0.0f; }

// the world matrix of a placed thing: its orientation's axes (x right, y forward, z up) and its position
Matrix WorldMatrix(const WorldTransform &aTransform) {
  Matrix m{};
  Quaternion q = aTransform.Orientation;
  GetMatrixFromOrientation(&q, &m);
  constexpr float kFixedPoint = 1.0f / 131072.0f;
  m.W = Vector4(aTransform.Position.x.Bits * kFixedPoint, aTransform.Position.y.Bits * kFixedPoint,
                aTransform.Position.z.Bits * kFixedPoint, 1.0f);
  return m;
}

Inputs::Axes Axes(const Matrix &aMatrix) {
  return {{aMatrix.X.X, aMatrix.X.Y, aMatrix.X.Z},
          {aMatrix.Y.X, aMatrix.Y.Y, aMatrix.Y.Z},
          {aMatrix.Z.X, aMatrix.Z.Y, aMatrix.Z.Z}};
}

// the car's health, percent (gameStatPoolsSystem.GetStatPoolValue of its entity's Health pool)
float HealthPercent(uint64_t aEntity) {
  StatsObjectID id{};
  id.entityHash = aEntity;
  id.idType = game::StatIDType::EntityID;
  auto pool = game::data::StatPoolType::Health;
  bool percent = true;
  float health = 100.0f;
  Call("gameStatPoolsSystem", "GetStatPoolValue", &health, id, pool, percent);
  return health;
}

// the Transform script struct
struct ScriptTransform {
  Vector4 position;
  Quaternion orientation;
};

// hear from the cinematic camera (the player's soundListener stays with V)
void CameraListener() {
  ScriptTransform transform{};
  bool active = false;
  if (!Call("gameCameraSystem", "GetActiveCameraWorldTransform", &active, transform) || !active) {
    return;
  }
  Vector4 forward, up;
  if (Call("gameCameraSystem", "GetActiveCameraForward", &forward) &&
      Call("gameCameraSystem", "GetActiveCameraUp", &up)) {
    FlightAudio::Get()->SetCameraListener(transform.position, forward, up);
  }
}

// the vehicle's total mass and a wheel's damped spring force, through the game's natives
float TotalMass(const Props &p, vehicle::BaseObject *aVehicle) {
  float mass = 0.0f;
  auto func = p.vehicle->GetFunction("GetTotalMass");
  if (func) {
    ExecuteFunction(aVehicle, func, &mass);
  }
  return mass;
}

float SpringForce(const Props &p, vehicle::BaseObject *aVehicle, int32_t aWheel) {
  float force = 0.0f;
  auto func = p.wheeled->GetFunction("GetDampedSpringForce");
  if (func && aVehicle->GetType()->IsA(p.wheeled)) {
    ExecuteFunction(aVehicle, func, &force, aWheel);
  }
  return force;
}

// one Values per component, by its entity
std::mutex valuesLock;
std::unordered_map<uint64_t, Inputs::Values> values;

} // namespace

IScriptable *System(const char *aClass) {
  auto gameInstance = CGameEngine::Get()->framework->gameInstance;
  auto rtti = CRTTISystem::Get();
  // the game may keep a system under its interface
  for (auto cls = rtti->GetClass(aClass); cls && cls->GetName() != CName("gameIGameSystem") && cls->GetName() != CName("IScriptable");
       cls = cls->parent) {
    if (auto system = gameInstance->GetSystem(cls)) {
      return system;
    }
  }
  return nullptr;
}

void Forget(uint64_t aEntity) {
  std::lock_guard guard(valuesLock);
  values.erase(aEntity);
}

void Component(FlightComponent *c, float deltaTime) {
  const Props &p = GetProps();
  if (!p.ok) {
    return;
  }
  bool photoMode = false;
  Call("gamePhotoModeSystem", "IsPhotoModeActive", &photoMode);
  if (photoMode) {
    return;
  }
  auto vehicle = reinterpret_cast<vehicle::BaseObject *>(c->entity);
  auto audio = FlightAudio::Get();
  auto stats = FlightStats::Of(c);
  auto sys = c->sys.GetPtr();
  auto ctlr = static_cast<FlightController *>(Object(p.ctlr, sys));
  if (!vehicle || !audio || !stats || !ctlr || !c->configuration) {
    return;
  }
  const uint64_t entity = c->entity->entityID.hash;

  Inputs::Frame f;
  f.full = deltaTime > 0.0f;
  f.dt = f.full ? deltaTime : 1.0f / 60.0f;
  const bool mounted = Ref<bool>(p.isPlayerMounted, c);
  const bool cinematic = FlightAudio::Cinematic();
  const bool tpp = Ref<bool>(p.isTPP, ctlr);
  f.outside = tpp || cinematic;
  f.destroyed = Ref<bool>(p.isDestroyed, c);

  // the active mode: the Autopilot has its own commands, the drone its own controls
  const auto &modes = Ref<DynArray<Handle<IScriptable>>>(p.modes, c);
  const int32_t modeIndex = Ref<int32_t>(p.mode, c);
  IScriptable *mode = modeIndex >= 0 && static_cast<uint32_t>(modeIndex) < modes.size ? modes.entries[modeIndex].GetPtr() : nullptr;
  const bool autopilot = mode && mode->GetType()->IsA(p.autopilot);
  f.drone = mode && mode->GetType()->IsA(p.drone);
  f.pilot = mounted ? (autopilot ? Inputs::Pilot::Autopilot : Inputs::Pilot::Player) : Inputs::Pilot::Npc;

  f.speed = stats->d_speed;
  f.forward = Vec(stats->d_forward);
  f.right = Vec(stats->d_right);
  f.up = Vec(stats->d_up);
  f.localUp = Vec(stats->d_localUp);
  f.direction = Vec(stats->d_direction);
  f.angularVelocity = Vec(stats->d_angularVelocity);
  // the force the script handed the thrusters (FlightComponent.smoothForce), the car's frame
  f.force = Vec(p.smoothForce, c);

  if (f.full) {
    auto rtti = CRTTISystem::Get();
    f.collisionTimer = Ref<float>(p.collisionTimer, c);
    f.recoveryDelay = GetFlightSettingsFloat(rtti, "collisionRecoveryDelay", 0.8f);
    f.recoveryDuration = GetFlightSettingsFloat(rtti, "collisionRecoveryDuration", 0.8f);
    f.healthPercent = HealthPercent(entity);
    f.surge = Ref<float>(p.surge, c);
    f.lift = Ref<float>(p.lift, c);
    f.roll = Ref<float>(p.roll, c);
    f.pitch = Ref<float>(p.pitch, c);
    f.yaw = Ref<float>(p.yaw, c);
    f.ctlSurge = Input(ctlr->surge);
    f.ctlYaw = Input(ctlr->yaw);
    f.ctlSway = Input(ctlr->sway);
    f.ctlLift = Input(ctlr->lift);
    f.ctlRoll = Input(ctlr->roll);
    f.ctlPitch = Input(ctlr->pitch);
    f.linearBrake = Input(ctlr->linearBrake);
    f.angularBrake = Input(ctlr->angularBrake);
    if (autopilot) {
      // the native mode (FlightModeAutopilot.hpp), or a redscript subclass of it
      const auto *ap = static_cast<const FlightModeAutopilot *>(mode);
      f.apSurge = ap->ap.commandSurge;
      f.apSway = ap->ap.commandSway;
      f.gravityFactor = Ref<float>(p.gravityFactor, mode);
      f.standardModeLiftFactor = ap->standardModeLiftFactor;
    }
  }

  Inputs::Values v;
  {
    std::lock_guard guard(valuesLock);
    v = values[entity];
  }
  Inputs::Update(f, v);
  {
    std::lock_guard guard(valuesLock);
    values[entity] = v;
  }

  // the player's car: the cinematic camera's listener, the echoes and the wind
  if (mounted) {
    if (cinematic) {
      CameraListener();
    }
    const auto carMatrix = WorldMatrix(vehicle->worldTransform);
    const auto &velocity = stats->d_velocity;
    audio->UpdateAcoustics(carMatrix, Ref<bool>(p.acoustics, audio) ? Ref<float>(p.acousticsGain, audio) : 0.0f, v.inside,
                           Ref<int32_t>(p.acousticsRaySource, audio));
    audio->UpdateWind(carMatrix, velocity, cinematic ? 2 : (tpp ? 0 : 1));
  }

  // the thrusters
  float mass = -1.0f;
  for (auto &handle : c->configuration->thrusters) {
    auto thruster = handle.GetPtr();
    if (!thruster || !thruster->attached || !thruster->meshComponent) {
      continue;
    }
    const bool front = Ref<bool>(p.isFront, thruster);
    const int32_t wheel = Ref<int32_t>(p.wheelIndex, thruster);
    if (mass < 0.0f) {
      mass = TotalMass(p, vehicle);
    }
    Inputs::Values thrusterValues = v;
    thrusterValues.scrape = Inputs::Scrape(SpringForce(p, vehicle, wheel), mass);
    const auto mesh = WorldMatrix(thruster->meshComponent->worldTransform);
    FlightAudio::UpdateThruster(Ref<CString>(p.id, thruster).c_str(), thrusterValues,
                                Inputs::ThrusterVolume(front, f.speed), {mesh.W.X, mesh.W.Y, mesh.W.Z},
                                Inputs::ConeAxes(Axes(mesh)));
  }
}

} // namespace Gather
