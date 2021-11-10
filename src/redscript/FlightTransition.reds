// public class FlightTransition extends VehicleTransition {

//   protected final func SetIsVehicleFlying(stateContext: ref<StateContext>, value: Bool) -> Void {
//     stateContext.SetPermanentBoolParameter(n"isVehicleFlying", value, true);
//   }
// }

// StateGameScriptInterface

@addMethod(StateGameScriptInterface)
public final const func IsVehicleFlying(opt child: ref<GameObject>, opt parent: ref<GameObject>) -> Bool {

}

// VehicleTransition

@addMethod(VehicleTransition)
public final static func CanEnterVehicleFlight() -> Bool {
  return TweakDBInterface.GetBool(t"player.vehicle.canEnterVehicleFlight", false);
}

@addMethod(VehicleTransition)
protected final const func IsVehicleFlying(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return scriptInterface.IsVehicleFlying();
}

@addMethod(VehicleTransition)
protected final func SetIsInFlight(stateContext: ref<StateContext>, value: Bool) -> Void {
  stateContext.SetPermanentBoolParameter(n"isInFlight", value, true);
}

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
  if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
    if Equals(this.GetPuppetVehicleSceneTransition(stateContext), PuppetVehicleState.CombatSeated) {
      return true;
    };
  };
  return false;
}

// Custom classes

public class FlightDecisions extends VehicleTransition {

  public final const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    if this.IsVehicleFlying(scriptInterface) {
      return true;
    };
    return false;
  }

  public final const func ToDrive(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    if stateContext.IsStateActive(n"UpperBody", n"emptyHands") && this.GetInStateTime() >= 0.50 {
      return true;
    };
    if !this.IsPlayerAllowedToEnterCombat(scriptInterface) {
      return true;
    };
    return false;
  }
}

public class FlightEvents extends VehicleEventsTransition {
  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnEnter(stateContext, scriptInterface);
    this.SetIsInFlight(stateContext, true);
    this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Vehicle, 8); // doesn't exist
  }

  public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.SetIsInFlight(stateContext, false);
  }

  public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  
  }
}