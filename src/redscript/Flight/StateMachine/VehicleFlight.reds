
public class VehicleFlightTransition extends DefaultTransition {

  protected let m_name: String;

  protected func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.m_name = NameToString(this.GetStateName());
    DefaultTransition.UppercaseFirstChar(this.m_name);
  }

  protected func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
  }

  // Single entry point for "the player asked to toggle flight", used by both the
  // activate and the deactivate decision.
  //
  // With "Gamepad Flight Toggle Hold" at 0, or whenever the last input came from the
  // keyboard, this is the plain tap the mod has always used. Above 0 with a gamepad in
  // hand, the button has to be held for gamepadToggleHold seconds instead, which is
  // what the L3 reporters asked for (L3 is easy to hit by accident while steering).
  //
  // The hold is edge-detected without any stored state: a press that started before we
  // entered the current state always has a longer action-state time than the state time,
  // so the press that turned flight on can never immediately turn it off again while it
  // is still held down.
  protected final const func IsFlightToggleRequested(const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let hold = FlightSettings.GetInstance().gamepadToggleHold;
    if hold <= 0.0 {
      return scriptInterface.IsActionJustTapped(n"Flight_Toggle");
    }
    let player = scriptInterface.executionOwner as PlayerPuppet;
    if !IsDefined(player) || player.PlayerLastUsedKBM() {
      return scriptInterface.IsActionJustTapped(n"Flight_Toggle");
    }
    if scriptInterface.GetActionValue(n"Flight_Toggle") <= 0.0 {
      return false;
    }
    let heldFor = scriptInterface.GetActionStateTime(n"Flight_Toggle");
    return heldFor >= hold && heldFor < this.GetInStateTime();
  }

}

public class VehicleFlightEventsTransition extends VehicleFlightTransition {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightEventsTransition] " + this.m_name);
  }

  protected final func HandleFlightCameraInput(scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if scriptInterface.IsActionJustTapped(n"ToggleVehCamera") && !this.IsVehicleCameraChangeBlocked(scriptInterface) {
      this.RequestToggleVehicleFlightCamera(scriptInterface);
    };
    // vanilla's cinematic camera (a hold on the same key), like during autodrive: only while the
    // Autopilot flies the car
    if scriptInterface.IsActionJustHeld(n"HoldCinematicCamera") && !this.IsVehicleCameraChangeBlocked(scriptInterface) && FlightModeAutopilot.IsEngaged() {
      scriptInterface.executionOwner.QueueEvent(new vehicleCinematicCameraToggleEvent());
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
          camEvent.cameraPerspective = vehicleCameraPerspective.TPPClose;
          this.ExitCustomCamera(scriptInterface);
        } else {
          this.EnterCustomCamera(scriptInterface);
        }
        break;
      case vehicleCameraPerspective.TPPClose:
        camEvent.cameraPerspective = vehicleCameraPerspective.TPPMedium;
        break;
      case vehicleCameraPerspective.TPPMedium:
        camEvent.cameraPerspective = vehicleCameraPerspective.TPPFar;
        break;
      case vehicleCameraPerspective.TPPFar:
        if !scriptInterface.localBlackboard.GetBool(GetAllBlackboardDefs().PlayerStateMachine.IsDriverCombatInTPP) {
          camEvent.cameraPerspective = vehicleCameraPerspective.TPPClose;
        } else {
          camEvent.cameraPerspective = vehicleCameraPerspective.FPP;
        };
        break;
      case vehicleCameraPerspective.DriverCombatClose:
        camEvent.cameraPerspective = vehicleCameraPerspective.TPPClose;
        break;
      case vehicleCameraPerspective.DriverCombatMedium:
        camEvent.cameraPerspective = vehicleCameraPerspective.TPPMedium;
        break;
      case vehicleCameraPerspective.DriverCombatFar:
        camEvent.cameraPerspective = vehicleCameraPerspective.TPPFar;
    };
    scriptInterface.executionOwner.QueueEvent(camEvent);
  }

  public func EnterCustomCamera(scriptInterface: ref<StateGameScriptInterface>) {
    FlightSystem.GetInstance().cameraIndex = 1;
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
    FlightSystem.GetInstance().cameraIndex = 0;
    let camera = (scriptInterface.executionOwner as PlayerPuppet).GetFPPCameraComponent();
    if IsDefined(camera) {
      camera.SetLocalPosition(new Vector4(0.0, 0.0, 0.0, 0.0));
      camera.SetLocalOrientation(EulerAngles.ToQuat(MakeEulerAngles(0.0, 0.0, 0.0)));
    }
    // let workspotSystem: ref<WorkspotGameSystem> = scriptInterface.GetWorkspotSystem();
    // workspotSystem.SwitchSeatVehicle(scriptInterface.owner, scriptInterface.executionOwner, n"OccupantSlots", n"seat_front_left");
  }

}