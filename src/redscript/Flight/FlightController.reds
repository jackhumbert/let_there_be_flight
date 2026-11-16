
// could watch this
// this.m_callbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this, n"OnVehicleStateChanged");

public func fc() -> wref<FlightController> {
  return FlightController.GetInstance();
}

public native class FlightController extends IScriptable {
  // defined on the RED4ext side
  private native let enabled: Bool;
  private native let active: Bool;
  public native let mode: Int32;
  public static native func GetInstance() -> ref<FlightController>;

  // redscript-only
  private let sys: ref<FlightSystem>;
  private let gameInstance: GameInstance;
  private let player: wref<PlayerPuppet>;
  public final func IsEnabled() -> Bool {
    return this.enabled;
  }
  public final func IsActive() -> Bool {
    return this.active;
  }

  public let showOptions: Bool;
  // mode to return to when the autodrive hold switches the Autopilot off
  private let modeBeforeAutopilot: Int32;
  // the autodrive hold just fired; a keyboard Next Mode on the same key (G) must not follow it
  private let autopilotHoldConsumed: Bool;
  // set by the Autopilot after touchdown: the flight state machine then turns flight off as if
  // the toggle had been pressed (VehicleFlightActiveDecisions.ToVehicleFlightDeactivating)
  public let autopilotRequestsFlightOff: Bool;
  public let showUI: Bool;
  public native let linearBrake: ref<InputPID>;
  public native let angularBrake: ref<InputPID>;
  public native let lift: ref<InputPID>;
  public native let surge: ref<InputPID>;
  public native let roll: ref<InputPID>;
  public native let pitch: ref<InputPID>;
  public native let yaw: ref<InputPID>;
  public native let sway: ref<InputPID>;
  // the controls after smoothing and normalisation, filled by StepInputs
  public native let outSurge: Float;
  public native let outLift: Float;
  public native let outSway: Float;
  public native let outYaw: Float;
  public native let outRoll: Float;
  public native let outPitch: Float;
  public native let outLinearBrake: Float;
  public native let outAngularBrake: Float;
  public native func StepInputs() -> Void;

  public let isTPP: Bool;

  private let uiBlackboard: ref<IBlackboard>;

  public let initialized: Bool;
  public let usingKB: Bool;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Flight-Toggle")
  public let vehicleFlight: EInputKey = EInputKey.IK_O;

  // Mod Settings rewrites the mapping tagged overridableUI="vehicleFlightPad" in
  // let_there_be_flight.xml, which is the gamepad half of Flight_Toggle. Clearing this
  // binding is how you turn the L3 flight toggle off.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Flight-Toggle-Pad")
  public let vehicleFlightPad: EInputKey = EInputKey.IK_Pad_LeftThumb;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Yaw-Left")
  public let vehicleFlightYawLeft: EInputKey = EInputKey.IK_A;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Yaw-Right")
  public let vehicleFlightYawRight: EInputKey = EInputKey.IK_D;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Pitch-Forward")
  public let vehicleFlightPitchForward: EInputKey = EInputKey.IK_L;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Pitch-Backward")
  public let vehicleFlightPitchBackward: EInputKey = EInputKey.IK_Period;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Roll-Left")
  public let vehicleFlightRollLeft: EInputKey = EInputKey.IK_Slash;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Roll-Right")
  public let vehicleFlightRollRight: EInputKey = EInputKey.IK_Comma;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Lift-Up")
  public let vehicleFlightLiftUp: EInputKey = EInputKey.IK_Space;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Lift-Down")
  public let vehicleFlightLiftDown: EInputKey = EInputKey.IK_LControl;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Sway-Left")
  public let vehicleFlightSwayLeft: EInputKey = EInputKey.IK_Left;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Sway-Right")
  public let vehicleFlightSwayRight: EInputKey = EInputKey.IK_Right;

  // Both brakes (Flight_LinearBrake and Flight_AngularBrake carry this overridableUI tag, and Mod Settings moves every
  // mapping with it). It replaced vehicleFlightLinearBrake and vehicleFlightAngularBrake.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Brake")
  public let vehicleFlightBrake: EInputKey = EInputKey.IK_C;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Flight-Options")
  public let vehicleFlightOptions: EInputKey = EInputKey.IK_R;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Next-Mode")
  public let vehicleFlightModeSwitchForward: EInputKey = EInputKey.IK_G;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Prev-Mode")
  public let vehicleFlightModeSwitchBackward: EInputKey = EInputKey.IK_J; // as the input xml binds it

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Key-Bindings")
  @runtimeProperty("ModSettings.category.order", "8")
  @runtimeProperty("ModSettings.displayName", "Input-Hint-Toggle-UI")
  public let vehicleFlightUIToggle: EInputKey = EInputKey.IK_Y;

  private func Initialize() {
    FlightLog.Info("[FlightController] Initialize");
    this.sys = FlightSystem.GetInstance();
    this.enabled = false;
    this.active = false;
    this.showOptions = false;
    this.showUI = true;

    this.linearBrake = InputPID.Create(0.5, 0.5);
    this.angularBrake = InputPID.Create(0.5, 0.5);

    this.lift = InputPID.Create(0.1, 1.0);
    this.surge = InputPID.Create(0.1, 1.0);
    this.sway = InputPID.Create(0.1, 1.0);

    this.roll = InputPID.Create(0.5, 1.0);
    this.pitch = InputPID.Create(0.5, 1.0);
    this.yaw = InputPID.Create(0.5, 1.0);

    this.uiBlackboard = GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_System);

    this.initialized = true;

  }

  private func SetPlayer(player: ref<PlayerPuppet>) {
    this.sys.Setup(player);
    FlightHelipadSystem.GetInstance(player.GetGame()).Start(player);
    this.gameInstance = player.GetGame();
    this.player = player;

    this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, FlightSettings.GetFloat("isFlightUIActive") > 0.5);
  }

  public func GetBlackboard() -> ref<IBlackboard> {
    return GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().VehicleFlight);
  }

  public static func CreateInstance(player: ref<PlayerPuppet>) {
    let self = FlightController.GetInstance();
    if !self.initialized {
      self.Initialize();
    }
    self.SetPlayer(player);

    // This strong reference will tie the lifetime of the singleton
    // to the lifetime of the player entity
    // player.flightController = self;

    // This weak reference is used as a global variable
    // to access the mod instance anywhere
    // GetAllBlackboardDefs().flightController = self;
    // FlightLog.Info("[FlightController] CreateInstance Finished");
  }

  public func Enable() -> Void {
    this.enabled = true;
    FlightLog.Info("[FlightController] Enable");
    this.SetupActions();
  }

  public func Disable() -> Void {
    this.enabled = false;
    FlightLog.Info("[FlightController] Disable");
    this.SetupActions();
  }

  private func Activate(silent: Bool) -> Void {
    FlightLog.Info("[FlightController] Activate");
    this.enabled = true;
    this.active = true;
    this.SetupActions();

    this.pitch.Reset();
    this.roll.Reset();
    this.yaw.Reset();
    this.sway.Reset();
    this.surge.Reset();
    this.lift.Reset();
    this.linearBrake.Reset();
    this.angularBrake.Reset();

    this.SetupPositionProviders();

    this.RefreshAutodriveHint();

    // idk what to do with this
    // let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    // uiSystem.PushGameContext(IntEnum(10));

    if !silent {
      this.ShowSimpleMessage(n"Simple-Message-Flight-Control-Engaged");
    }

  }

  private func Deactivate(silent: Bool) -> Void {
    this.active = false;
    this.autopilotRequestsFlightOff = false;
    // don't come back in Autopilot next time flight is switched on
    let autopilot = this.AutopilotModeIndex();
    if autopilot >= 0 && this.mode == autopilot {
      // its own Deactivate takes down vanilla's autodrive panel (and the hints that come with it)
      this.sys.playerComponent.modes[autopilot].Deactivate();
      this.mode = this.modeBeforeAutopilot != autopilot ? this.modeBeforeAutopilot : 0;
      this.GetBlackboard().SetInt(GetAllBlackboardDefs().VehicleFlight.Mode, this.mode);
    }
    this.SetupActions();
    this.RefreshAutodriveHint();

    //let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    //uiSystem.PopGameContext(IntEnum(10));

    if !silent {
      this.ShowSimpleMessage(n"Simple-Message-Flight-Control-Disengaged");
    }

    FlightLog.Info("[FlightController] Deactivate");

  }

  // Vanilla's autodrive hint re-reads its state when AutoDriveAvailable is signalled; in flight it
  // stays lit whatever vanilla says (Extensions/AutoDriveController.reds).
  private func RefreshAutodriveHint() -> Void {
    GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_AutodriveData).SignalBool(GetAllBlackboardDefs().UI_AutodriveData.AutoDriveAvailable);
  }

  private func SetupActions() -> Void {
    this.usingKB = this.player.PlayerLastUsedKBM();
    let evt = new UpdateInputHintMultipleEvent();
    evt.targetHintContainer = n"GameplayInputHelper";

    let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    this.player.UnregisterInputListener(this);
    // this.player.RegisterInputListener(this, n"OpenPauseMenu");
    // uiSystem.QueueEvent(FlightController.HideHintFromSource(n"FlightController"));
    if this.enabled {
      if this.active {
        this.player.RegisterInputListener(this, n"__DEVICE_CHANGED__");
        this.player.RegisterInputListener(this, n"Pitch");
        this.player.RegisterInputListener(this, n"Sway");
        this.player.RegisterInputListener(this, n"Roll");
        this.player.RegisterInputListener(this, n"SurgePos");
        this.player.RegisterInputListener(this, n"Lift");
        this.player.RegisterInputListener(this, n"Yaw");
        this.player.RegisterInputListener(this, n"SurgeNeg");
        this.player.RegisterInputListener(this, n"Flight_LinearBrake");
        this.player.RegisterInputListener(this, n"Flight_AngularBrake");
        this.player.RegisterInputListener(this, n"Flight_Trick");
        this.player.RegisterInputListener(this, n"Flight_Options");
        this.player.RegisterInputListener(this, n"Flight_UIToggle");
        this.player.RegisterInputListener(this, n"Flight_ModeSwitchForward");
        this.player.RegisterInputListener(this, n"Flight_ModeSwitchBackward");
        this.player.RegisterInputListener(this, n"Flight_RightStickToggle");
        this.player.RegisterInputListener(this, n"Flight_HoodDetach");
        this.player.RegisterInputListener(this, n"HoldAutodrive");
      }
    }

    // if this.trick {
    //   evt.AddInputHint(FlightController.CreateInputHint(n"Aileron Roll", n"Yaw"), true);
    // } else {
      // evt.AddInputHint(FlightController.CreateInputHint("nTricks", n"Flight_Trick"), true);
    // }
    // we may want to look at something else besides this input so ForceBrakesUntilStoppedOrFor will work (not entirely sure it doesn't now)
    // vehicle.GetBlackboard().GetInt(GetAllBlackboardDefs().VehicleFlight.IsHandbraking)

    let usesRightStick = this.sys.playerComponent.GetFlightMode().usesRightStickInput;

    // The hint list keeps one hint per action, whatever the source: each action is added once
    // here with its label picked up front. A hide after a show of the same action in one event
    // removes the shown one (Linear Brake and the Autopilot hint never appeared that way), and
    // reordering such pairs crashed the game on flight activation.
    evt.AddInputHint(FlightController.CreateInputHint(this.FlightToggleHintLabel(), n"Flight_Toggle"),      (this.enabled && !this.active) || (this.active && !this.showOptions));
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Yaw", n"Yaw"),                           this.FlightControlsShown() && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Pitch", n"Pitch"),                       this.FlightControlsShown() && !this.showOptions && (usesRightStick || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Roll", n"Roll"),                         this.FlightControlsShown() && !this.showOptions && (usesRightStick || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Lift", n"Lift"),                         this.FlightControlsShown() && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Brake", n"Flight_LinearBrake"),           this.FlightControlsShown() && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Flight-Options", n"Flight_Options"),     this.FlightControlsShown() && !this.showOptions);

    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Sway", n"Sway"),                         this.FlightControlsShown() && this.showOptions && this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint(this.NextModeHintLabel(), n"Flight_ModeSwitchForward"),     this.FlightControlsShown() && (this.showOptions || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Prev-Mode", n"Flight_ModeSwitchBackward"),     this.FlightControlsShown() && this.showOptions && !this.usingKB);
    // evt.AddInputHint(FlightController.CreateInputHint(n"Raise Hover Height", n"FlightOptions_Up"), true);
    // evt.AddInputHint(FlightController.CreateInputHint(n"Lower Hover Height", n"FlightOptions_Down"), true);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Toggle-UI", n"Flight_UIToggle"),         this.FlightControlsShown() && this.showOptions);
    // autodrive while flying: the vanilla autodrive hold toggles the Autopilot mode, and while it
    // flies the camera hold toggles the cinematic camera (vanilla's hint text, LocKey#96518).
    // Engaged, the hints are vanilla autodrive's: no stop hint (the autodrive panel's own key
    // badge is that), only the camera.
    let autopilotEngaged = this.active && this.mode == this.AutopilotModeIndex();
    evt.AddInputHint(FlightController.CreateInputHint(this.AutopilotHintLabel(), n"HoldAutodrive"),         this.active && !autopilotEngaged && !this.showOptions && !this.AutopilotHoldSharesNextMode());
    // the LocKey string as vanilla passes it, resolved by the hint widget (GetLocalizedText here
    // gave "Fast Swim")
    evt.AddInputHint(FlightController.CreateInputHintText("LocKey#96518", n"HoldCinematicCamera"), autopilotEngaged);

    uiSystem.QueueEvent(evt);
  }

  private let trick: Bool;

  public func ProcessImpact(impact: Float) {
  }

  private func SetMode(mode: Int32) -> Void {
    this.mode = mode;
    this.GetBlackboard().SetInt(GetAllBlackboardDefs().VehicleFlight.Mode, this.mode);
    let evt = new VehicleFlightModeChangeEvent();
    evt.mode = this.mode;
    GetMountedVehicle(this.player).QueueEvent(evt);
    let newMode = this.sys.playerComponent.modes[this.mode];
    this.ShowSimpleMessage(StringToName(newMode.GetDescription() + " " + GetLocalizedTextByKey(n"Simple-Message-Suffix-Enabled")));
    GameInstance.GetAudioSystem(this.gameInstance).Play(n"ui_menu_onpress");
    this.SetupActions();
  }

  // Index of the Autopilot mode, -1 if it is not in the mode list.
  public func AutopilotModeIndex() -> Int32 {
    let i = 0;
    for mode in this.sys.playerComponent.modes {
      if IsDefined(mode as FlightModeAutopilot) {
        return i;
      }
      i += 1;
    }
    return -1;
  }

  // Flight control hints: hidden while the Autopilot flies (input still takes over briefly, but
  // that is an escape hatch, not something to advertise).
  private func FlightControlsShown() -> Bool {
    return this.active && this.mode != this.AutopilotModeIndex();
  }

  // The hint list keeps one hint per key: on keyboard the autodrive hold shares G with Next Mode,
  // so the two are one hint while Next Mode is still on G.
  private func AutopilotHoldSharesNextMode() -> Bool {
    return this.usingKB && EnumInt(this.vehicleFlightModeSwitchForward) == EnumInt(EInputKey.IK_G);
  }

  private func FlightToggleHintLabel() -> CName {
    return this.active ? n"Input-Hint-Disable-Flight" : n"Input-Hint-Enable-Flight";
  }

  private func AutopilotHintLabel() -> CName {
    return this.FlightControlsShown() ? n"Input-Hint-Autopilot" : n"Input-Hint-Disable-Autopilot";
  }

  private func NextModeHintLabel() -> CName {
    if !this.showOptions && this.AutopilotHoldSharesNextMode() {
      return n"Input-Hint-Next-Mode-Autopilot";
    }
    return n"Input-Hint-Next-Mode";
  }

  // Next / previous mode, skipping Autopilot: that one is reached through the autodrive hold.
  private func CycleMode(direction: Int32) -> Void {
    let count = ArraySize(this.sys.playerComponent.modes);
    let autopilot = this.AutopilotModeIndex();
    let next = this.mode;
    let steps = 0;
    while steps < count {
      next = (next + direction + count) % count;
      steps += 1;
      if next != autopilot {
        break;
      }
    }
    this.SetMode(next);
  }

  // A tap of the mode switch (G on keyboard, also the autodrive key): with the Autopilot engaged,
  // free roam and the tracked destination switch, as a tap does for vanilla autodrive; otherwise
  // the next flight mode.
  private func TapModeSwitch() -> Void {
    let autopilot = this.AutopilotModeIndex();
    if autopilot >= 0 && this.mode == autopilot {
      let mode = this.sys.playerComponent.GetFlightMode() as FlightModeAutopilot;
      if IsDefined(mode) {
        mode.ToggleFreeRoam();
        return;
      }
    }
    this.CycleMode(1);
  }

  // Back to the mode the Autopilot was engaged from (arrival, or the autodrive hold again).
  public func ExitAutopilot() -> Void {
    let autopilot = this.AutopilotModeIndex();
    if autopilot >= 0 && this.mode == autopilot {
      this.SetMode(this.modeBeforeAutopilot != autopilot ? this.modeBeforeAutopilot : 0);
    }
  }

  // The vanilla autodrive hold (Vehicle_Autodrive: hold G / L3) while flying: fly the planned
  // route to the GPS destination with the Autopilot mode (free roam without one, as vanilla
  // autodrive does), or hand back to the mode before it.
  private func ToggleAutopilot() -> Void {
    let autopilot = this.AutopilotModeIndex();
    if autopilot < 0 {
      return;
    }
    if this.mode == autopilot {
      this.ExitAutopilot();
      return;
    }
    this.modeBeforeAutopilot = this.mode;
    this.SetMode(autopilot);
  }

  protected func ResetInputs() {
      this.lift.SetInput(0.0);
      this.surge.SetInput(0.0);
      this.yaw.SetInput(0.0);
      this.sway.SetInput(0.0);
      this.pitch.SetInput(0.0);
      this.roll.SetInput(0.0);
      this.linearBrake.SetInput(0.0);
      this.angularBrake.SetInput(0.0);
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    if ListenerAction.IsAction(action, n"__DEVICE_CHANGED__") {
      if this.player.PlayerLastUsedKBM() {
        this.usingKB = true;
      } else {
        this.usingKB = false;
      }
      this.SetupActions();
    }
    let actionType: gameinputActionType = ListenerAction.GetType(action);
    let actionName: CName = ListenerAction.GetName(action);
    let value: Float = ListenerAction.GetValue(action);
    if this.active {
      if Equals(actionName, n"Flight_Options") {
        if ListenerAction.IsButtonJustPressed(action) {
          this.showOptions = true;
          this.ResetInputs();
          this.SetupActions();
        }
        if ListenerAction.IsButtonJustReleased(action) {
          this.showOptions = false;
          this.ResetInputs();
          this.SetupActions();
        }
      }

          // let ps = this.sys.playerComponent.FindComponentByName(n"projectileSpawn8722") as ProjectileSpawnComponent;
          // ps.Spawn(Cast<Uint32>(0));

      if Equals(actionName, n"Flight_HoodDetach") && ListenerAction.IsButtonJustPressed(action) {
        GameInstance.GetAudioSystem(this.gameInstance).Play(n"ui_menu_onpress");
        this.sys.playerComponent.GetVehicle().DetachPart(n"Hood");
        this.sys.playerComponent.GetVehicle().DetachPart(n"Trunk");
      }
      if Equals(actionName, n"Flight_RightStickToggle") && ListenerAction.IsButtonJustPressed(action) {
        GameInstance.GetAudioSystem(this.gameInstance).Play(n"ui_menu_onpress");
        this.sys.playerComponent.GetFlightMode().usesRightStickInput = !this.sys.playerComponent.GetFlightMode().usesRightStickInput;
        this.SetupActions();
      }
      if Equals(actionName, n"HoldAutodrive") && Equals(actionType, gameinputActionType.BUTTON_HOLD_COMPLETE) {
        this.ToggleAutopilot();
        this.autopilotHoldConsumed = true;
      }
      if Equals(actionName, n"Flight_ModeSwitchForward") && (this.showOptions || this.player.PlayerLastUsedKBM()) {
        if this.player.PlayerLastUsedKBM() {
          // G is also the autodrive hold on keyboard: switch on release, and not after a hold
          if ListenerAction.IsButtonJustPressed(action) {
            this.autopilotHoldConsumed = false;
          }
          if ListenerAction.IsButtonJustReleased(action) {
            if !this.autopilotHoldConsumed {
              this.TapModeSwitch();
            }
            this.autopilotHoldConsumed = false;
          }
        } else {
          if ListenerAction.IsButtonJustPressed(action) {
            this.TapModeSwitch();
          }
        }
      }
      if Equals(actionName, n"Flight_ModeSwitchBackward") && ListenerAction.IsButtonJustPressed(action) && (this.showOptions || this.player.PlayerLastUsedKBM()) {
        this.CycleMode(-1);
      }
      if this.showOptions && Equals(actionName, n"Flight_UIToggle") && ListenerAction.IsButtonJustPressed(action) {
          this.showUI = !this.showUI;
          if (this.showUI) {
            this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, true, true);
            this.ShowSimpleMessage(n"Simple-Message-Flight-UI-Shown");
          } else {
            this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, false, true);
            this.ShowSimpleMessage(n"Simple-Message-Flight-UI-Hidden");
          }
          GameInstance.GetAudioSystem(this.gameInstance).Play(n"ui_menu_onpress");
      }

      if Equals(actionType, gameinputActionType.AXIS_CHANGE) {
        switch(actionName) {
          case n"Roll":
           if this.sys.playerComponent.GetFlightMode().usesRightStickInput || this.player.PlayerLastUsedKBM() {
            this.roll.SetInput(value);
           } else {
            this.roll.SetInput(0.0);
           }
            break;
          case n"Pitch":
            if this.sys.playerComponent.GetFlightMode().usesRightStickInput || this.player.PlayerLastUsedKBM() {
              this.pitch.SetInput(value);
            } else {
              this.pitch.SetInput(0.0);
            }
            break;
          case n"SurgePos":
            this.surge.SetInput(value);
            break;
          case n"Yaw":
              if this.showOptions {
                this.sway.SetInput(value);
              } else {
                this.yaw.SetInput(value);
              }
            break;
          case n"Sway":
            this.sway.SetInput(value);
            break;
          case n"Lift":
            if this.trick {
              this.lift.SetInput(0.0);
            } else {
              // single choke point for the lift axis, so the invert option covers the
              // gamepad stick and the keyboard up/down buttons alike
              if FlightSettings.GetInstance().invertLift {
                this.lift.SetInput(-value);
              } else {
                this.lift.SetInput(value);
              }
            }
            break;
          case n"SurgeNeg":
            this.surge.SetInput(-value);
            break;
          case n"Flight_LinearBrake":
            this.linearBrake.SetInput(value);
            break;
          case n"Flight_AngularBrake":
            this.angularBrake.SetInput(value);
            break;
          default:
            break;
        }
      }
    } else {
      this.lift.SetInput(0.0);
      this.surge.SetInput(0.0);
      this.yaw.SetInput(0.0);
      this.sway.SetInput(0.0);
      this.pitch.SetInput(0.0);
      this.roll.SetInput(0.0);
      this.linearBrake.SetInput(0.0);
      this.angularBrake.SetInput(0.0);
    }
  }

  // the input smoothing and normalisation in C++ (StepInputs: the out* fields)
  public func UpdateInputs(timeDelta: Float) -> Void {
    this.StepInputs();
  }

  public func SetupPositionProviders() -> Void {
    this.sys.audio.SetWindowSlots(GetMountedVehicle(this.player), FlightAudio.SlotPosition(GetMountedVehicle(this.player), n"window_front_left_a"), FlightAudio.SlotPosition(GetMountedVehicle(this.player), n"window_front_right_a"));
  }

  public final func OnUpdate(timeDelta: Float) -> Void {

    this.UpdateInputs(timeDelta);

  }

  public func ShowSimpleMessage(message: CName) -> Void {
    let msg: SimpleScreenMessage;
    msg.isShown = true;
    msg.duration = 2.00;
    msg.message = GetLocalizedTextByKey(message);
    if StrLen( msg.message) == 0 {
      msg.message = ToString(message);
    };
    msg.isInstant = true;
    GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_Notifications).SetVariant(GetAllBlackboardDefs().UI_Notifications.OnscreenMessage, ToVariant(msg), true);
  }

  // CreateInputHint with ready text (vanilla LocKey# strings).
  public static func CreateInputHintText(text: String, action: CName) -> InputHintData {
    let data = FlightController.CreateInputHint(n"", action);
    data.localizedLabel = text;
    return data;
  }

  public static func CreateInputHint(label: CName, action: CName) -> InputHintData {
    let data: InputHintData;
    data.source = n"FlightController";
    data.action = action;
    data.holdIndicationType = inkInputHintHoldIndicationType.FromInputConfig;
    data.sortingPriority = 0;
    data.enableHoldAnimation = false;
    data.localizedLabel = GetLocalizedTextByKey(label);
    if StrLen( data.localizedLabel) == 0 {
         data.localizedLabel = ToString(label);
    };
    return data;
  }

}

// @addField(PlayerPuppet)
// public let flightController: ref<FlightController>; // Must be strong reference

// @addField(AllBlackboardDefinitions)
// public let flightController: wref<FlightController>; // Must be weak reference

// Option 2 -- Get the player instance as soon as it's ready
@wrapMethod(PlayerPuppet)
protected cb func OnGameAttached() -> Bool {
  wrappedMethod();
  if !this.IsReplacer() {
    FlightController.CreateInstance(this);
  }
}

