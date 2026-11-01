#include "FlightThruster.hpp"

#include "Utils/Utils.hpp"

#include <RED4ext/CName.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/EulerAngles.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Utils.hpp>

#include <mutex>
#include <string>
#include <unordered_set>

IFlightThruster::~IFlightThruster() {
  // meshComponent.~Handle();
}

namespace {
using RED4ext::CBaseFunction;
using RED4ext::CClass;
using RED4ext::CProperty;

Flight::Vec3 ToVec(const RED4ext::Vector4 &aV) { return {aV.X, aV.Y, aV.Z}; }
Flight::Quat ToQuat(const RED4ext::Quaternion &aQ) { return {aQ.i, aQ.j, aQ.k, aQ.r}; }
RED4ext::Vector4 ToVector4(const Flight::Vec3 &aV) { return {aV.x, aV.y, aV.z, 0.0f}; }

// The script fields and the game's natives IFlightThruster.Update uses, resolved once (GetFunction searches the
// parent classes)
struct Natives {
  // IFlightThruster's script fields
  CProperty *motionKind = nullptr, *isFront = nullptr, *isRight = nullptr, *mainThrusterYawFactor = nullptr,
            *maxThrusterAnglePitch = nullptr, *maxThrusterAngleOutside = nullptr, *maxThrusterAngleInside = nullptr,
            *thrusterAngleAllowance = nullptr, *boneLerpAmount = nullptr, *forceThreshold = nullptr,
            *torqueThreshold = nullptr, *animDeviation = nullptr, *initialOrientation = nullptr,
            *deviationName = nullptr, *fxs = nullptr, *force = nullptr, *torque = nullptr;
  // IFlightThrusterFX.instance
  CProperty *fxInstance = nullptr;
  CClass *mainFx = nullptr, *sideFx = nullptr, *regularFx = nullptr, *animationControllerClass = nullptr;
  // the game's own, for each step of Update
  CBaseFunction *makeEulerAngles = nullptr, *toQuat = nullptr, *slerp = nullptr, *multiply = nullptr,
                *getUp = nullptr, *getLocalOrientation = nullptr, *setLocalOrientation = nullptr,
                *setInputFloat = nullptr, *setBlackboardValue = nullptr, *findComponentByName = nullptr,
                *getEntity = nullptr;
  // FindComponentByName is called on the flight component's entity when the component itself does not have it
  bool findOnEntity = false;

  bool Ready() const {
    return motionKind && isFront && isRight && mainThrusterYawFactor && maxThrusterAnglePitch &&
           maxThrusterAngleOutside && maxThrusterAngleInside && thrusterAngleAllowance && boneLerpAmount &&
           forceThreshold && torqueThreshold && animDeviation && initialOrientation && deviationName && fxs &&
           force && torque && fxInstance && mainFx && sideFx && regularFx && animationControllerClass &&
           makeEulerAngles && toQuat && slerp && getUp && getLocalOrientation && setLocalOrientation &&
           setInputFloat && setBlackboardValue && findComponentByName && (!findOnEntity || getEntity);
  }
};

const Natives &Resolve() {
  static const Natives natives = [] {
    Natives n;
    auto rtti = RED4ext::CRTTISystem::Get();
    if (auto cls = rtti->GetClass("IFlightThruster")) {
      n.motionKind = cls->GetProperty("motionKind");
      n.isFront = cls->GetProperty("isFront");
      n.isRight = cls->GetProperty("isRight");
      n.mainThrusterYawFactor = cls->GetProperty("mainThrusterYawFactor");
      n.maxThrusterAnglePitch = cls->GetProperty("maxThrusterAnglePitch");
      n.maxThrusterAngleOutside = cls->GetProperty("maxThrusterAngleOutside");
      n.maxThrusterAngleInside = cls->GetProperty("maxThrusterAngleInside");
      n.thrusterAngleAllowance = cls->GetProperty("thrusterAngleAllowance");
      n.boneLerpAmount = cls->GetProperty("boneLerpAmount");
      n.forceThreshold = cls->GetProperty("forceThreshold");
      n.torqueThreshold = cls->GetProperty("torqueThreshold");
      n.animDeviation = cls->GetProperty("animDeviation");
      n.initialOrientation = cls->GetProperty("initialOrientation");
      n.deviationName = cls->GetProperty("deviationName");
      n.fxs = cls->GetProperty("fxs");
      n.force = cls->GetProperty("force");
      n.torque = cls->GetProperty("torque");
    }
    if (auto cls = rtti->GetClass("IFlightThrusterFX")) {
      n.fxInstance = cls->GetProperty("instance");
    }
    n.mainFx = rtti->GetClass("MainFlightThrusterFX");
    n.sideFx = rtti->GetClass("SideFlightThrusterFX");
    n.regularFx = rtti->GetClass("RegularFlightThrusterFX");
    n.animationControllerClass = rtti->GetClass("entAnimationControllerComponent");
    n.makeEulerAngles = rtti->GetFunction("MakeEulerAngles");
    if (auto cls = rtti->GetClass("EulerAngles")) {
      n.toQuat = cls->GetFunction("ToQuat");
    }
    if (auto cls = rtti->GetClass("Quaternion")) {
      n.slerp = cls->GetFunction("Slerp");
      n.getUp = cls->GetFunction("GetUp");
    }
    // the script's Quaternion * Quaternion; without it UpdateNative multiplies with Flight::Mul
    n.multiply = rtti->GetFunction("OperatorMultiply;QuaternionQuaternion;Quaternion");
    if (auto cls = rtti->GetClass("entIPlacedComponent")) {
      n.getLocalOrientation = cls->GetFunction("GetLocalOrientation");
      n.setLocalOrientation = cls->GetFunction("SetLocalOrientation");
    }
    if (auto cls = rtti->GetClass("entAnimationControllerComponent")) {
      n.setInputFloat = cls->GetFunction("SetInputFloat");
    }
    if (auto cls = rtti->GetClass("gameFxInstance")) {
      n.setBlackboardValue = cls->GetFunction("SetBlackboardValue");
    }
    if (auto cls = rtti->GetClass("FlightComponent")) {
      n.findComponentByName = cls->GetFunction("FindComponentByName");
    }
    if (!n.findComponentByName) {
      if (auto cls = rtti->GetClass("entEntity")) {
        n.findComponentByName = cls->GetFunction("FindComponentByName");
      }
      if (auto cls = rtti->GetClass("entIComponent")) {
        n.getEntity = cls->GetFunction("GetEntity");
      }
      n.findOnEntity = true;
    }
    return n;
  }();
  return natives;
}

// Logs a message the first time it is seen (by its text), so each failure point reports once
void WarnOnceByText(const char *aMessage) {
  static std::mutex mutex;
  static std::unordered_set<std::string> seen;
  {
    std::lock_guard lock(mutex);
    if (!seen.insert(aMessage).second) {
      return;
    }
  }
  spdlog::warn("[IFlightThruster] {}; the script moves the thrusters", aMessage);
}

template <typename T> T &Field(CProperty *aProp, void *aInstance) { return *aProp->GetValuePtr<T>(aInstance); }

// global natives and static functions: no instance
const RED4ext::ScriptInstance kNone = nullptr;

// The game's Quaternion * Quaternion, or the pure code's when the operator was not found
bool Multiply(const Natives &n, RED4ext::Quaternion &aA, RED4ext::Quaternion &aB, RED4ext::Quaternion &aOut) {
  if (n.multiply) {
    return RED4ext::ExecuteFunction(kNone, n.multiply, &aOut, aA, aB);
  }
  auto q = Flight::Mul(ToQuat(aA), ToQuat(aB));
  aOut = {q.i, q.j, q.k, q.r};
  return true;
}
} // namespace

bool IFlightThruster::SetupMotion() {
  motionReady = false;
  const Natives &n = Resolve();
  if (!n.Ready()) {
    LTBF_WARN_ONCE("[IFlightThruster] a script field or a game native for the thruster motion was not found; the "
                   "script moves the thrusters");
    return false;
  }
  auto fc = flightComponent.Lock();
  if (!fc) {
    LTBF_WARN_ONCE("[IFlightThruster] SetupMotion: no flight component; the script moves the thrusters");
    return false;
  }
  // the fallbacks inside the native path, logged once so an A/B knows which natives were really used
  if (!n.multiply) {
    LTBF_WARN_ONCE("[IFlightThruster] the Quaternion * Quaternion native was not found; Flight::Mul multiplies "
                   "instead");
  }
  if (n.findOnEntity) {
    LTBF_WARN_ONCE("[IFlightThruster] FlightComponent has no FindComponentByName; it is called on the component's "
                   "entity instead");
  }

  params.shape = static_cast<Flight::ThrusterShape>(Field<int32_t>(n.motionKind, this));
  params.isFront = Field<bool>(n.isFront, this);
  params.isRight = Field<bool>(n.isRight, this);
  params.maxThrusterAnglePitch = Field<float>(n.maxThrusterAnglePitch, this);
  params.maxThrusterAngleOutside = Field<float>(n.maxThrusterAngleOutside, this);
  params.maxThrusterAngleInside = Field<float>(n.maxThrusterAngleInside, this);
  params.thrusterAngleAllowance = Field<float>(n.thrusterAngleAllowance, this);
  params.mainThrusterYawFactor = Field<float>(n.mainThrusterYawFactor, this);
  params.forceThreshold = Field<float>(n.forceThreshold, this);
  params.torqueThreshold = Field<float>(n.torqueThreshold, this);
  params.boneLerpAmount = Field<float>(n.boneLerpAmount, this);

  auto &effects = Field<RED4ext::DynArray<RED4ext::Handle<RED4ext::IScriptable>>>(n.fxs, this);
  if (effects.size > kMaxFx) {
    LTBF_WARN_ONCE("[IFlightThruster] {} effects on a thruster, at most {} are handled natively; the script moves the "
                   "thrusters",
                   effects.size, kMaxFx);
    return false;
  }
  fxCount = 0;
  for (auto &fx : effects) {
    auto &slot = fxSlots[fxCount++];
    slot.kind = Flight::ThrusterFxKind::Base;
    slot.instance.Reset();
    if (!fx) {
      continue;
    }
    auto type = fx->GetType();
    if (type->IsA(n.mainFx)) {
      slot.kind = Flight::ThrusterFxKind::Main;
    } else if (type->IsA(n.sideFx)) {
      slot.kind = Flight::ThrusterFxKind::Side;
    } else if (type->IsA(n.regularFx)) {
      slot.kind = Flight::ThrusterFxKind::Regular;
    } else {
      LTBF_WARN_ONCE("[IFlightThruster] an effect is not a Main, Side or Regular thruster effect; it is animated as "
                     "a base effect");
    }
    slot.instance = Field<RED4ext::Handle<RED4ext::IScriptable>>(n.fxInstance, fx.GetPtr());
  }

  // the animation controller, found once here instead of every frame
  animationController.Reset();
  RED4ext::CName controllerName = "AnimationController";
  RED4ext::Handle<RED4ext::IScriptable> found;
  bool ok;
  if (n.findOnEntity) {
    RED4ext::Handle<RED4ext::IScriptable> entity;
    ok = RED4ext::ExecuteFunction(fc.GetPtr(), n.getEntity, &entity);
    ok = ok && entity && RED4ext::ExecuteFunction(entity.GetPtr(), n.findComponentByName, &found, controllerName);
  } else {
    ok = RED4ext::ExecuteFunction(fc.GetPtr(), n.findComponentByName, &found, controllerName);
  }
  if (!ok) {
    LTBF_WARN_ONCE("[IFlightThruster] SetupMotion: FindComponentByName failed; the script moves the thrusters");
    return false;
  }
  if (found && found->GetType()->IsA(n.animationControllerClass)) {
    animationController = found;
  } else {
    LTBF_WARN_ONCE("[IFlightThruster] SetupMotion: no AnimationController component; the deviation is not written");
  }

  motionReady = true;
  return true;
}

bool IFlightThruster::UpdateNative(RED4ext::Vector4 aForce, RED4ext::Vector4 aTorque) {
  if (!motionReady) {
    return false;
  }
  const Natives &n = Resolve();
  // A call that fails hands the thruster back to the script from the next frame. Force, torque and scale are already
  // written by then, so that one frame is partly native; the script's Update runs whole from the next one. Each
  // failure point has its own message and is logged once.
  auto fail = [this](const char *aMessage) {
    WarnOnceByText(aMessage);
    motionReady = false;
    return true;
  };

  Flight::Vec3 f = Flight::ThrusterNormalize(ToVec(aForce), params.forceThreshold);
  Flight::Vec3 t = Flight::ThrusterNormalize(ToVec(aTorque), params.torqueThreshold);
  Field<RED4ext::Vector4>(n.force, this) = ToVector4(f);
  Field<RED4ext::Vector4>(n.torque, this) = ToVector4(t);

  auto fc = flightComponent.Lock();
  bool active = fc && fc->active;

  auto mesh = meshComponent.GetPtr();
  if (!mesh) {
    return true;
  }

  auto scale = Flight::ThrusterScale({mesh->visualScale.X, mesh->visualScale.Y, mesh->visualScale.Z}, active);
  mesh->visualScale = {scale.x, scale.y, scale.z};

  Flight::ThrusterAngles angles = Flight::ThrusterEuler(params, f, t, active);
  float pitch = angles.pitch, roll = angles.roll, yaw = angles.yaw;
  RED4ext::EulerAngles euler{};
  RED4ext::Quaternion q{}, target{}, current{}, next{};
  if (!RED4ext::ExecuteFunction(kNone, n.makeEulerAngles, &euler, pitch, roll, yaw) ||
      !RED4ext::ExecuteFunction(kNone, n.toQuat, &q, euler)) {
    return fail("MakeEulerAngles or EulerAngles.ToQuat failed");
  }
  auto &initial = Field<RED4ext::Quaternion>(n.initialOrientation, this);
  if (!Multiply(n, initial, q, target)) {
    return fail("Quaternion * Quaternion failed");
  }
  float lerp = Flight::kThrusterOrientationLerp;
  if (!RED4ext::ExecuteFunction(mesh, n.getLocalOrientation, &current) ||
      !RED4ext::ExecuteFunction(kNone, n.slerp, &next, current, target, lerp) ||
      !RED4ext::ExecuteFunction(mesh, n.setLocalOrientation, nullptr, next)) {
    return fail("the mesh's local orientation could not be read or written");
  }

  // scale the effects to how much work the thruster is doing
  RED4ext::Quaternion local{};
  RED4ext::Vector4 upVector{};
  if (!RED4ext::ExecuteFunction(mesh, n.getLocalOrientation, &local) ||
      !RED4ext::ExecuteFunction(kNone, n.getUp, &upVector, local)) {
    return fail("Quaternion.GetUp failed");
  }
  Flight::Vec3 up = ToVec(upVector);
  float amount = Flight::Dot(up, f);
  RED4ext::CName amountName = "thruster_amount";
  for (uint32_t i = 0; i < fxCount; ++i) {
    float blackboard = 0.0f;
    amount += Flight::ThrusterFxDisplacement(fxSlots[i].kind, up, f, t, params.isFront, params.isRight, blackboard);
    auto instance = fxSlots[i].instance.Lock();
    if (instance &&
        !RED4ext::ExecuteFunction(instance.GetPtr(), n.setBlackboardValue, nullptr, amountName, blackboard)) {
      return fail("SetBlackboardValue failed");
    }
  }

  auto &deviation = Field<float>(n.animDeviation, this);
  deviation = Flight::ThrusterDeviation(params, deviation, amount);
  auto controller = animationController.Lock();
  if (controller) {
    RED4ext::CName deviationName = Field<RED4ext::CName>(n.deviationName, this);
    float value = deviation;
    if (!RED4ext::ExecuteFunction(controller.GetPtr(), n.setInputFloat, nullptr, deviationName, value)) {
      return fail("SetInputFloat failed");
    }
  }
  return true;
}

RED4ext::CString IFlightThruster::MotionState() {
  const Natives &n = Resolve();
  return RED4ext::CString(fmt::format(R"({{"ready":{},"multiply":"{}","findOnEntity":{},"fx":{},"anim":{}}})",
                                      motionReady, n.multiply ? "native" : "fallback", n.findOnEntity, fxCount,
                                      animationController.Lock() ? "true" : "false")
                              .c_str());
}
