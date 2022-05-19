// VehicleTransition

@addMethod(VehicleTransition)
public final static func CanEnterVehicleFlight() -> Bool {
  return TweakDBInterface.GetBool(t"player.vehicle.canEnterVehicleFlight", false);
}

// @addMethod(VehicleTransition)
// protected final const func IsVehicleFlying(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return scriptInterface.IsVehicleFlying();
// }

@addMethod(VehicleTransition)
protected final func SetIsInFlight(stateContext: ref<StateContext>, value: Bool) -> Void {
  stateContext.SetPermanentBoolParameter(n"isInFlight", value, true);
}

// need to implement some things in order to use this
@addMethod(VehicleTransition)
protected final const func IsPlayerAllowedToEnterVehicleFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  if this.IsNoCombatActionsForced(scriptInterface) {
    return false;
  };
  if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleFlight") {
    return true;
  };
  return true;
}

@addMethod(VehicleTransition)
protected final const func IsPlayerAllowedToExitFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleFlightBlockExit") {
    return false;
  };
  return true;
}

// DriveDecisions

@addMethod(DriveDecisions)
public final const func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
  // if VehicleTransitiorn.CanEnterVehicleFlight() {
    if scriptInterface.IsActionJustPressed(n"Flight_Toggle") {
      FlightLog.Info("[DriveDecisions] ToFlight");
      return true;
    };
  // };
  return false;
}

// SceneDecisions

@addMethod(SceneDecisions)
public final const func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
  if VehicleTransition.CanEnterVehicleFlight() {
    // if FlightController.GetInstance().IsActive() {
      FlightLog.Info("[SceneDecisions] ToFlight");
      return false;
    // };
  };
  return false;
}