// public class FlightTransition extends VehicleTransition {

//   protected final func SetIsVehicleFlying(stateContext: ref<StateContext>, value: Bool) -> Void {
//     stateContext.SetPermanentBoolParameter(n"isVehicleFlying", value, true);
//   }
// }

// StateGameScriptInterface

// @addMethod(StateGameScriptInterface)
// public final const func IsVehicleFlying(opt child: ref<GameObject>, opt parent: ref<GameObject>) -> Bool {
//   FlightLog.Info("[StateGameScriptInterface] IsVehicleFlying");
//   return FlightController.GetInstance().IsActive();
// }

// AnimFeature_VehicleData

// @addField(AnimFeature_VehicleData)
// public let isInFlight: Bool;

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

// DriveEvents

// @wrapMethod(DriveEvents)
// protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   wrappedMethod(stateContext, scriptInterface);
//   let vehicle: ref<VehicleObject> = scriptInterface.owner as VehicleObject;  
//   if vehicle.IsPlayerMounted() {
//     FlightController.GetInstance().Enable(vehicle);
//   }
// }

// @wrapMethod(DriveEvents)
// public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   FlightController.GetInstance().Disable();
//   wrappedMethod(stateContext, scriptInterface);
// }

// @wrapMethod(DriveEvents)
// public final func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   FlightController.GetInstance().Disable();
//   wrappedMethod(stateContext, scriptInterface);
// }

// public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   let transition: PuppetVehicleState = this.GetPuppetVehicleSceneTransition(stateContext);
//   if Equals(transition, PuppetVehicleState.CombatSeated) || Equals(transition, PuppetVehicleState.CombatWindowed) {
//     this.SendEquipmentSystemWeaponManipulationRequest(scriptInterface, EquipmentManipulationAction.RequestLastUsedOrFirstAvailableWeapon);
//   };
//   this.SetIsVehicleDriver(stateContext, false);
//   this.SendAnimFeature(stateContext, scriptInterface);
//   this.ResetVehFppCameraParams(stateContext, scriptInterface);
//   this.isCameraTogglePressed = false;
//   stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", false, true);
//   this.ResumeStateMachines(scriptInterface.executionOwner);
// }

// @wrapMethod(DriveEvents)
// public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   wrappedMethod(timeDelta, stateContext, scriptInterface);
//   FlightController.GetInstance().OnUpdate(timeDelta, stateContext, scriptInterface);
// }

// PublicSafeDecisions

// @replaceMethod(PublicSafeDecisions)
// protected cb func OnVehicleChanged(value: Int32) -> Bool {
//   this.m_isInVehicleCombat = value == EnumInt(gamePSMVehicle.Combat) || value == 8;
//   this.m_isInVehTurret = value == EnumInt(gamePSMVehicle.Turret);
//   this.UpdateShouldOnEnterBeEnabled();
// }

// AimingStateDecisions

// @wrapMethod(AimingStateDecisions)
// private final func GetShouldAimValue() -> Bool {
//   return wrappedMethod() || this.m_vehicleState == 8;
// }

// Custom classes

public class FlightDecisions extends VehicleTransition {

  public final const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // FlightLog.Info("[FlightDecisions] EnterCondition");
    // return scriptInterface.IsActionJustPressed(n"Flight_Toggle");
    return true;
  }

  public final const func ToDrive(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // FlightLog.Info("[FlightDecisions] ToDrive");
    return scriptInterface.IsActionJustPressed(n"Flight_Toggle");
  }
}

public class FlightEvents extends VehicleEventsTransition {
  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightEvents] OnEnter");
    super.OnEnter(stateContext, scriptInterface);
    this.SetIsInFlight(stateContext, true);
    this.SetIsVehicleDriver(stateContext, true);
    this.SendAnimFeature(stateContext, scriptInterface);
    this.PauseStateMachines(stateContext, scriptInterface.executionOwner);
    
    this.PlayerStateChange(scriptInterface, 1);
    this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Vehicle, 8);
    if !VehicleTransition.CanEnterDriverCombat() {
      stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", true, true);
    };    
    FlightController.GetInstance().Activate();
    let evt = new VehicleFlightActivationEvent();
    evt.vehicle = scriptInterface.owner as VehicleObject;
    (scriptInterface.owner as VehicleObject).QueueEvent(evt);
  }

  public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightEvents] OnExit");
    this.SetIsInFlight(stateContext, false);
    // (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    FlightController.GetInstance().Deactivate(false);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightEvents] OnForcedExit");
    this.SetIsInFlight(stateContext, false);
    // (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    //FlightController.GetInstance().Deactivate(true);
    super.OnForcedExit(stateContext, scriptInterface);
    this.ResumeStateMachines(scriptInterface.executionOwner);
  }

  public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightController.GetInstance().OnUpdate(timeDelta, stateContext, scriptInterface);
    this.SetIsInVehicle(stateContext, true);
    this.SetSide(stateContext, scriptInterface);
    this.SendAnimFeature(stateContext, scriptInterface);
    this.HandleCameraInput(scriptInterface);
    this.HandleFlightExitRequest(stateContext, scriptInterface);
  }

  public final func HandleFlightExitRequest(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    let isTeleportExiting: StateResultBool = stateContext.GetPermanentBoolParameter(n"teleportExitActive");
    let isScheduledExit: StateResultBool = stateContext.GetPermanentBoolParameter(n"validExitAfterSwitchRequest");
    let isSwitchingSeats: StateResultBool = stateContext.GetPermanentBoolParameter(n"validSwitchSeatExitRequest");
    if isTeleportExiting.value || isScheduledExit.value || isSwitchingSeats.value {
      return;
    };
    if this.IsPlayerAllowedToExitVehicle(scriptInterface) {
      let stateTime = this.GetInStateTime();      
      let exitActionPressCount = scriptInterface.GetActionPressCount(n"Exit");
      let exitPressCountResult = stateContext.GetPermanentIntParameter(n"exitPressCountOnEnter");
      let onDifferentExitPress = !exitPressCountResult.valid || exitPressCountResult.value != Cast<Int32>(exitActionPressCount);
      if onDifferentExitPress && stateTime >= 0.30 && scriptInterface.GetActionValue(n"Exit") > 0.00 && scriptInterface.GetActionStateTime(n"Exit") > 0.30{
        let vehicle = scriptInterface.owner as VehicleObject;
        let inputStateTime = scriptInterface.GetActionStateTime(n"Exit");
        let validUnmount = vehicle.CanUnmount(true, scriptInterface.executionOwner);
        stateContext.SetPermanentIntParameter(n"vehUnmountDir", EnumInt(validUnmount.direction), true);
        this.ExitWithTeleport(stateContext, scriptInterface, validUnmount, false, true);
      }
    }
  }
}