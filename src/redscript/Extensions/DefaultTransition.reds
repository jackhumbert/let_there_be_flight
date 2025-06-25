@addMethod(DefaultTransition)
public final func CanVehicleEnterFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return (scriptInterface.owner as VehicleObject).CanEnterFlight();
}

// @addMethod(DefaultTransition)
// protected final const func IsVehicleFlying(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return scriptInterface.IsVehicleFlying();
// }

@addMethod(DefaultTransition)
protected final func SetIsInFlight(value: Bool) -> Void {
  // stateContext.SetPermanentBoolParameter(n"isInFlight", value, true);
  FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsActive, value, true);
}
@addMethod(DefaultTransition)
protected final func IsInFlight() -> Bool {
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

@addMethod(DefaultTransition)
protected final func IsPlayerAllowedToEnterFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  let fc = fs().playerComponent;
  let canActivate = IsDefined(fc) && fc.configuration.CanActivate();

  let tweakID = (scriptInterface.owner as VehicleObject).GetRecordID();
  TDBID.Append(tweakID, TDBID.Create(".canEnterFlight"));
  let flightEnabled = TweakDBInterface.GetBool(tweakID, true);
  
  let blockFlight = StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleBlockFlight");
  
  return canActivate && flightEnabled && !blockFlight; 
}

@addMethod(DefaultTransition)
protected final const func IsPlayerAllowedToExitFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleBlockFlightExit") {
    return false;
  };
  return true;
}
