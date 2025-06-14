@addMethod(DefaultTransition)
public final static func CanEnterVehicleFlight() -> Bool {
  return TweakDBInterface.GetBool(t"player.vehicle.canEnterVehicleFlight", false);
}

// @addMethod(DefaultTransition)
// protected final const func IsVehicleFlying(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return scriptInterface.IsVehicleFlying();
// }

@addMethod(DefaultTransition)
protected final func SetIsInFlight(stateContext: ref<StateContext>, value: Bool) -> Void {
  // stateContext.SetPermanentBoolParameter(n"isInFlight", value, true);
  FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsActive, value, true);
}
@addMethod(DefaultTransition)
protected final func IsInFlight(stateContext: ref<StateContext>) -> Bool {
  // let state = stateContext.GetPermanentBoolParameter(n"isInFlight");
  // return state.value;
  return FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.IsActive);
}

@addMethod(DefaultTransition)
protected final func SetInMountedVehicleCombat(value: Bool) -> Void {
  FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.InMountedVehicleCombat, value, true);
}

@addMethod(DefaultTransition)
protected final func IsInMountedVehicleCombat() -> Bool {
  return FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.InMountedVehicleCombat);
}


// need to implement some things in order to use this
@addMethod(DefaultTransition)
protected final func IsPlayerAllowedToEnterVehicleFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // if this.IsNoCombatActionsForced(scriptInterface) {
    // return false;
  // };
  let fc = fs().playerComponent;
  let canActivate = IsDefined(fc) && fc.configuration.CanActivate();
  return canActivate; // && StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleFlight");
}

@addMethod(DefaultTransition)
protected final const func IsPlayerAllowedToExitFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleFlightBlockExit") {
    return false;
  };
  return true;
}
