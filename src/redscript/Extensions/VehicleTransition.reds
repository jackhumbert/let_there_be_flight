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
