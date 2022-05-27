#include "FlightSettings.hpp"
#include "FlightModule.hpp"

#include "Utils.hpp"
#include "stdafx.hpp"

namespace FlightSettings {

RED4ext::TTypedClass<FlightSettings> cls("FlightSettings");

RED4ext::CClass *FlightSettings::GetNativeType() { return &cls; }

RED4ext::HashMap<RED4ext::CName, float> floats;

void GetFloat(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, float *aOut, int64_t a4) {
  RED4ext::CName name;
  RED4ext::GetParameter(aFrame, &name);

  aFrame->code++;

  if (aOut) {
    *aOut = *floats.Get(name);
  }
}

void SetFloat(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
  RED4ext::CName name;
  float value;
  RED4ext::GetParameter(aFrame, &name);
  RED4ext::GetParameter(aFrame, &value);

  aFrame->code++;

  floats.InsertOrAssign(name, value);
}

bool Setup(RED4ext::CGameApplication *aApp) {
  auto allocator = new RED4ext::Memory::DefaultAllocator();
  floats = RED4ext::HashMap<RED4ext::CName, float>(allocator);

  floats.Insert("airResistance", 0.001);
  floats.Insert("angularBrakeFactor", 10.0);
  floats.Insert("angularDampFactor", 3.0);
  floats.Insert("brakeFactor", 1.2);
  floats.Insert("brakeOffset", 0.0);
  floats.Insert("collisionRecoveryDelay", 0.8);
  floats.Insert("collisionRecoveryDuration", 0.8);
  floats.Insert("defaultHoverHeight", 3.50);
  floats.Insert("distance", 0.0);
  floats.Insert("distanceEase", 0.1);
  floats.Insert("fwtfCorrection", 0.0);
  floats.Insert("hoverClamp", 10.0);
  floats.Insert("hoverFactor", 20.0);
  floats.Insert("liftFactor", 8.0);
  floats.Insert("liftFactorDrone", 40.0);
  floats.Insert("lookAheadMax", 10.0);
  floats.Insert("lookAheadMin", 1.0);
  floats.Insert("maxHoverHeight", 7.0);
  floats.Insert("minHoverHeight", 1.0);
  floats.Insert("normalEase", 0.3);
  floats.Insert("pitchAeroCorrectionFactor", 0.25);
  floats.Insert("pitchCorrectionFactor", 3.0);
  floats.Insert("pitchDirectionalityFactor", 50.0);
  floats.Insert("pitchFactorDrone", 15.0);
  floats.Insert("pitchWithLift", 0.5);
  floats.Insert("pitchWithSurge", 0.0);
  floats.Insert("referenceZ", 0.0);
  floats.Insert("rollCorrectionFactor", 15.0);
  floats.Insert("rollFactorDrone", 18.0);
  floats.Insert("rollWithYaw", 0.15);
  floats.Insert("secondCounter", 0.0);
  floats.Insert("surgeFactor", 15.0);
  floats.Insert("surgeOffset", 0.5);
  floats.Insert("swayFactor", 5.0);
  floats.Insert("swayWithYaw", 0.5);
  floats.Insert("thrusterFactor", 0.05);
  floats.Insert("yawCorrectionFactor", 0.25);
  floats.Insert("yawD", 3.0);
  floats.Insert("yawDirectionalityFactor", 50.0);
  floats.Insert("yawFactor", 5.0);
  floats.Insert("yawFactorDrone", 5.0);

  return true;
}

struct FlightSettingsModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    RED4ext::GameState runningState;
    runningState.OnEnter = nullptr;
    runningState.OnUpdate = &Setup;
    runningState.OnExit = nullptr;

    aSdk->gameStates->Add(aHandle, RED4ext::EGameStateType::Running, &runningState);
  }

  void RegisterTypes() {
    cls.flags = {.isNative = true};
    RED4ext::CRTTISystem::Get()->RegisterType(&cls);
  }

  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto scriptable = rtti->GetClass("IScriptable");
    cls.parent = scriptable;

    auto getFloat = RED4ext::CClassStaticFunction::Create(&cls, "GetFloat", "GetFloat", &GetFloat,
                                                          {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getFloat);

    auto setFloat = RED4ext::CClassStaticFunction::Create(&cls, "SetFloat", "SetFloat", &SetFloat,
                                                          {.isNative = true, .isStatic = true});
    cls.RegisterFunction(setFloat);

  }
};

REGISTER_FLIGHT_MODULE(FlightSettingsModule);

} // namespace FlightSettings