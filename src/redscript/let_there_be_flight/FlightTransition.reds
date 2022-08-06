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
    switch (scriptInterface.owner as VehicleObject).GetCameraManager().GetActivePerspective() {
      case vehicleCameraPerspective.FPP:
        FlightSystem.GetInstance().cameraIndex = 0;
        break;
      case vehicleCameraPerspective.TPPClose:
        FlightSystem.GetInstance().cameraIndex = 2;
        break;
      case vehicleCameraPerspective.TPPFar:
        FlightSystem.GetInstance().cameraIndex = 3;
    };

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
    this.ExitCustomCamera(scriptInterface);
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
    this.ExitCustomCamera(scriptInterface);
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
    if (!FlightController.GetInstance().showOptions) {
      this.HandleFlightCameraInput(scriptInterface);
    }
    this.HandleFlightExitRequest(stateContext, scriptInterface);
  }

  protected final func HandleFlightCameraInput(scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if scriptInterface.IsActionJustPressed(n"ToggleVehCamera") && !this.IsVehicleCameraChangeBlocked(scriptInterface) {
      this.RequestToggleVehicleFlightCamera(scriptInterface);
    };
    if scriptInterface.IsActionJustTapped(n"VehicleCameraInverse") {
      this.ResetVehicleCamera(scriptInterface);
    };
  }

  protected final func RequestToggleVehicleFlightCamera(scriptInterface: ref<StateGameScriptInterface>) -> Void {
    let camEvent: ref<vehicleRequestCameraPerspectiveEvent>;
    if scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vision) == EnumInt(gamePSMVision.Focus) {
      return;
    };
    camEvent = new vehicleRequestCameraPerspectiveEvent();
    switch (scriptInterface.owner as VehicleObject).GetCameraManager().GetActivePerspective() {
      case vehicleCameraPerspective.FPP:
        if FlightSystem.GetInstance().cameraIndex == 1 {
          camEvent.cameraPerspective = vehicleCameraPerspective.TPPFar;
          FlightSystem.GetInstance().cameraIndex = 2;
        } else {
          this.EnterCustomCamera(scriptInterface);
          FlightSystem.GetInstance().cameraIndex = 1;
        }
        break;
      case vehicleCameraPerspective.TPPClose:
        this.ExitCustomCamera(scriptInterface);
        camEvent.cameraPerspective = vehicleCameraPerspective.FPP;
        FlightSystem.GetInstance().cameraIndex = 3;
        break;
      case vehicleCameraPerspective.TPPFar:
        camEvent.cameraPerspective = vehicleCameraPerspective.TPPClose;
        FlightSystem.GetInstance().cameraIndex = 0;
    };
    scriptInterface.executionOwner.QueueEvent(camEvent);
  }

  public func EnterCustomCamera(scriptInterface: ref<StateGameScriptInterface>) {
    let camera = (scriptInterface.executionOwner as PlayerPuppet).GetFPPCameraComponent();
    if IsDefined(camera) {
      let slotT: WorldTransform;
      let OccupantSlots = (scriptInterface.owner as VehicleObject).GetVehicleComponent().FindComponentByName(n"OccupantSlots") as SlotComponent;
      OccupantSlots.GetSlotTransform(n"seat_front_left", slotT);
      let roof: WorldTransform;
      let vehicle_slots = (scriptInterface.owner as VehicleObject).GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
      vehicle_slots.GetSlotTransform(n"roof_border_front", roof);
      let vwt = Matrix.GetInverted((scriptInterface.owner as VehicleObject).GetLocalToWorld());
      let v = (WorldPosition.ToVector4(WorldTransform.GetWorldPosition(roof)) * vwt) - (WorldPosition.ToVector4(WorldTransform.GetWorldPosition(slotT)) * vwt);
      camera.SetLocalPosition(v + Vector4.Vector3To4(FlightSettings.GetVector3("FPVCameraOffset")));
    }

    // let workspotSystem: ref<WorkspotGameSystem> = scriptInterface.GetWorkspotSystem();
    // workspotSystem.SwitchSeatVehicle(scriptInterface.owner, scriptInterface.executionOwner, n"OccupantSlots", n"CustomFlightCamera");
  }

  public func ExitCustomCamera(scriptInterface: ref<StateGameScriptInterface>) {
    let camera = (scriptInterface.executionOwner as PlayerPuppet).GetFPPCameraComponent();
    if IsDefined(camera) {
      camera.SetLocalPosition(new Vector4(0.0, 0.0, 0.0, 0.0));
    }
    // let workspotSystem: ref<WorkspotGameSystem> = scriptInterface.GetWorkspotSystem();
    // workspotSystem.SwitchSeatVehicle(scriptInterface.owner, scriptInterface.executionOwner, n"OccupantSlots", n"seat_front_left");
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