
public class FlightEvents extends VehicleEventsTransition {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightEvents] OnEnter");
    super.OnEnter(stateContext, scriptInterface);
    this.OnEnterFlight(stateContext, scriptInterface);
  }

  public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightEvents] OnExit");
    this.OnExitFlight(stateContext, scriptInterface);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightEvents] OnForcedExit");
    this.ExitCustomCamera(scriptInterface);
    this.SetIsInFlight(stateContext, false);
    // (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    //FlightController.GetInstance().Deactivate(true);
    // stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", false, true);
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
      case vehicleCameraPerspective.DriverCombatClose:
        this.ExitCustomCamera(scriptInterface);
        camEvent.cameraPerspective = vehicleCameraPerspective.FPP;
        FlightSystem.GetInstance().cameraIndex = 3;
        break;
      case vehicleCameraPerspective.TPPFar:
      case vehicleCameraPerspective.DriverCombatFar:
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
      vehicle_slots.GetSlotTransform(n"CustomFlightCamera", roof);
      let vwt = Matrix.GetInverted((scriptInterface.owner as VehicleObject).GetLocalToWorld());
      let v = (WorldPosition.ToVector4(WorldTransform.GetWorldPosition(roof)) * vwt) - (WorldPosition.ToVector4(WorldTransform.GetWorldPosition(slotT)) * vwt);
      camera.SetLocalPosition(v + Vector4.Vector3To4(FlightSettings.GetVector3("FPVCameraOffset")));
      camera.SetLocalOrientation(EulerAngles.ToQuat(MakeEulerAngles(FlightSettings.GetInstance().fpvCameraPitchOffset, 0.0, 0.0)));
    } 

    // let workspotSystem: ref<WorkspotGameSystem> = scriptInterface.GetWorkspotSystem();
    // workspotSystem.SwitchSeatVehicle(scriptInterface.owner, scriptInterface.executionOwner, n"OccupantSlots", n"CustomFlightCamera");
  }

  public func ExitCustomCamera(scriptInterface: ref<StateGameScriptInterface>) {
    let camera = (scriptInterface.executionOwner as PlayerPuppet).GetFPPCameraComponent();
    if IsDefined(camera) {
      camera.SetLocalPosition(new Vector4(0.0, 0.0, 0.0, 0.0));
      camera.SetLocalOrientation(EulerAngles.ToQuat(MakeEulerAngles(0.0, 0.0, 0.0)));
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
    if !this.IsExitVehicleBlocked(scriptInterface) {
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