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
    this.PlayerStateChange(scriptInterface, 1);
    this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Vehicle, 8);
    this.SendAnimFeature(stateContext, scriptInterface);
    this.SetVehFppCameraParams(stateContext, scriptInterface, false);

    this.PauseStateMachines(stateContext, scriptInterface.executionOwner);
    
    if !VehicleTransition.CanEnterDriverCombat() {
      stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", true, true);
    };    
    // FlightController.GetInstance().Activate();
    let evt = new VehicleFlightActivationEvent();
    // evt.vehicle = scriptInterface.owner as VehicleObject;
    (scriptInterface.owner as VehicleObject).QueueEvent(evt);
  }

  public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightEvents] OnExit");
    this.SetIsInFlight(stateContext, false);
    // (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    // FlightController.GetInstance().Deactivate(false);
    stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", false, true);
    let evt = new VehicleFlightDeactivationEvent();
    evt.silent = false;
    // evt.vehicle = scriptInterface.owner as VehicleObject;
    (scriptInterface.owner as VehicleObject).QueueEvent(evt);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightEvents] OnForcedExit");
    this.SetIsInFlight(stateContext, false);
    // (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    //FlightController.GetInstance().Deactivate(true);
    stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", false, true);
    super.OnForcedExit(stateContext, scriptInterface);
    this.ResumeStateMachines(scriptInterface.executionOwner);
  }

  public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    // let fc = FlightController.GetInstance();
    // fc.OnUpdate(timeDelta, stateContext, scriptInterface);
    // fc.sys.OnUpdate(timeDelta);
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
        // let inputStateTime = scriptInterface.GetActionStateTime(n"Exit");
        let validUnmount = vehicle.CanUnmount(true, scriptInterface.executionOwner);
        stateContext.SetPermanentIntParameter(n"vehUnmountDir", EnumInt(validUnmount.direction), true);
        this.ExitWithTeleport(stateContext, scriptInterface, validUnmount, false, true);
      }
    }
  }
}