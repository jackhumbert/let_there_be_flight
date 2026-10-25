@addMethod(DefaultTransition)
public final func CanVehicleEnterFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  let vehicle = scriptInterface.owner as VehicleObject;
  return IsDefined(vehicle) && vehicle.IsFlightAvailable();
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

// Shared by activation (IsPlayerAllowedToEnterFlight) and the "activate flight" input hint
// (InputContextTransitionEvents.ShowVehicleDriverInputHints) so both agree on when a quest
// vehicle (e.g. Delamain) has flight blocked.
public func IsVehicleFlightBlockedByStatusEffect(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleBlockFlight");
}

// Vanilla autodrive shares L3 with Flight_Toggle: a 0.5 s hold turns autodrive on or off and,
// while it drives, a tap switches between free roam and the destination. Without this gate
// that tap also toggled flight, so flight cannot be activated while autodrive is driving.
public func IsAutodriveActive(game: GameInstance) -> Bool {
  let autodrive = GameInstance.GetScriptableSystemsContainer(game).Get(n"AutoDriveSystem") as AutoDriveSystem;
  return IsDefined(autodrive) && autodrive.GetAutodriveEnabled();
}

@addMethod(DefaultTransition)
protected final func IsPlayerAllowedToEnterFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  let fc = fs().playerComponent;
  let canActivate = IsDefined(fc) && fc.configuration.CanActivate();

  // VehicleObject.IsFlightAvailable() is the single gate: it folds in the per-record
  // flightDisabled / canEnterFlight tweaks and the Flight Availability mode.
  let vehicle = scriptInterface.owner as VehicleObject;
  if !IsDefined(vehicle) || !vehicle.IsFlightAvailable() {
    return false;
  }

  let blockFlight = IsVehicleFlightBlockedByStatusEffect(scriptInterface) || IsAutodriveActive(vehicle.GetGame());

  return canActivate && !blockFlight;
}

@addMethod(DefaultTransition)
protected final const func IsPlayerAllowedToExitFlight(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleBlockFlightExit") {
    return false;
  };
  return true;
}
