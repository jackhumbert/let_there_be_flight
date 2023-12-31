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
protected final func IsPlayerAllowedToEnterVehicleFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // if this.IsNoCombatActionsForced(scriptInterface) {
    // return false;
  // };
  let fc = fs().playerComponent;
  let canActivate = IsDefined(fc) && fc.configuration.CanActivate();
  return canActivate; // && StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleFlight");
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
public final func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
  // if VehicleTransitiorn.CanEnterVehicleFlight() {
    // let fc = scriptInterface.owner.FindComponentByName(n"flightComponent") as FlightComponent;
    // if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) &&
    //     GameInstance.GetQuestsSystem(scriptInterface.GetGame()).GetFact(n"map_blocked") == 0 &&
    //     Equals(this.GetCurrentTier(stateContext), GameplayTier.Tier1_FullGameplay) {
    if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) {
      FlightLog.Info("[DriveDecisions] ToFlight");
      return true;
    };
  };
  return false;
}

// DriverCombatDecisions

@addMethod(DriverCombatDecisions)
public final func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
  // if VehicleTransitiorn.CanEnterVehicleFlight() {
    // let fc = scriptInterface.owner.FindComponentByName(n"flightComponent") as FlightComponent;
    // if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) &&
    //     GameInstance.GetQuestsSystem(scriptInterface.GetGame()).GetFact(n"map_blocked") == 0 &&
    //     Equals(this.GetCurrentTier(stateContext), GameplayTier.Tier1_FullGameplay) {
    if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) {
      FlightLog.Info("[DriverCombatDecisions] ToFlight");
      return true;
    };
  };
  return false;
}

// SceneDecisions

@addMethod(SceneDecisions)
public final func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
    if VehicleTransition.CanEnterVehicleFlight() {
      // if FlightController.GetInstance().IsActive() {
        FlightLog.Info("[SceneDecisions] ToFlight");
        return false;
    };
  };
  return false;
}