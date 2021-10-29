import BaseLib.UI.*

enum FlightControlMode {
  Hover = 0,
  Drone = 1
} 

// this might work better
// module MyMod

// public class MySystem extends ScriptableSystem {
//     private func OnAttach() -> Void {
//         LogChannel(n"DEBUG", "MySystem::OnAttach");
//     }

//     private func OnDetach() -> Void {
//         LogChannel(n"DEBUG", "MySystem::OnDetach");
//     }

//     public func GetData() -> Float {
//         return GetPlayer(this.GetGameInstance()).GetGunshotRange();
//     }
// }

// access
// let container: ref<ScriptableSystemsContainer> = GameInstance.GetScriptableSystemsContainer(this.GetGame());
// let system: ref<MySystem> = container.Get(n"MyMod.MySystem") as MySystem; // Don't forget the namespace if you're using modules

// LogChannel(n"DEBUG", ToString(system.GetData()));

// enum gamedataVehicleModel {
//   Aerondight = 0,
//   Alvarado = 1,
//   Basilisk = 2,
//   Bratsk = 3,
//   Colby = 4,
//   Columbus = 5,
//   Cortes = 6,
//   Emperor = 7,
//   Galena = 8,
//   GalenaNomad = 9,
//   Kusanagi = 10,
//   Mackinaw = 11,
//   Maimai = 12,
//   Octant = 13,
//   Shion = 14,
//   Supron = 15,
//   Thrax = 16,
//   Turbo = 17,
//   Type66 = 18,
//   Zeya = 19,
//   Voight = 20,
//   Count = 21,
//   Invalid = 22,
// }

public class FlightControlAudioStats {
  public let volume: Float;
  public let playerPosition: Vector4;
  public let playerUp: Vector4;
  public let playerForward: Vector4;
  public let cameraPosition: Vector4;
  public let cameraUp: Vector4;
  public let cameraForward: Vector4;
  public let speed: Float;
  public let surge: Float;
  public let yawDiff: Float;
  public let lift: Float;
  public let yaw: Float;
  public let pitchDiff: Float;
  public let brake: Float;
  public static func Create() -> ref<FlightControlAudioStats> {
    let instance = new FlightControlAudioStats();
    instance.volume = 1.0;
    instance.playerPosition = Vector4.EmptyVector();
    instance.playerUp = new Vector4(0.0, 0.0, 1.0, 0.0);
    instance.playerForward =new Vector4(0.0, 1.0, 0.0, 0.0);
    instance.cameraPosition = Vector4.EmptyVector();
    instance.cameraUp = new Vector4(0.0, 0.0, 1.0, 0.0);
    instance.cameraForward = new Vector4(0.0, 1.0, 0.0, 0.0);
    instance.speed = 0.0;
    instance.surge = 0.0;
    instance.yawDiff = 0.0;
    instance.lift = 0.0;
    instance.yaw = 0.0;
    instance.pitchDiff = 0.0;
    instance.brake = 0.0;
    return instance;
  }
}

// enum gamePSMVehicle {
//   Default = 0,
//   Driving = 1,
//   Combat = 2,
//   Passenger = 3,
//   Transition = 4,
//   Turret = 5,
//   DriverCombat = 6,
//   Scene = 7,
//   Flying = 8
// }

// Singleton instance with player lifetime
public class FlightControl {
  private let gameInstance: GameInstance;
  // public let m_flightHUDGameController: ref<FlightHUDGameController>;
  // public let m_flightHUDLogicController: ref<FlightHUDLogicController>;
  private let stats: ref<VehicleStats>;
  private let ui: ref<FlightControlUI>;
  public final func SetUI(ui: ref<FlightControlUI>) {
    this.ui = ui;
  }
  public let audioStats: ref<FlightControlAudioStats>;
  public final const func GetVehicle() -> ref<VehicleObject> {
    if !Equals(this.stats, null) {
      return this.stats.vehicle;
    } else {
      return null;
    }
  }
  private let enabled: Bool;
  public final const func IsEnabled() -> Bool {
    return this.enabled;
  }
  private let active: Bool;
  public final const func IsActive() -> Bool {
    return this.active;
  }
  private let mode: FlightControlMode;
  public final const func GetMode() -> FlightControlMode {
    return this.mode;
  }
  public let showOptions: Bool;
  public let brake: ref<PID>;
  public let lift: ref<PID>;
  public let liftFactor: Float;
  public let surgePos: ref<PID>;
  public let surgeNeg: ref<PID>;
  public let roll: ref<PID>;
  public let pitch: ref<PID>;
  public let yaw: ref<PID>;
  public let yawFactor: Float;
  public let yawCorrectionFactor: Float;
  public let distance: Float;
  public let distanceEase: Float;
  public let normal: Vector4;
  public let normalEase: Float;
  public let lookAheadMax: Float;
  public let lookAheadMin: Float;
  public let lookDown: Vector4;
  public let airResistance: Float;
  public let hoverHeight: Float;
  public let hoverFactor: Float;
  public let hover: ref<PID>;
  public let pitchPID: ref<PID>;
  public let rollPID: ref<PID>;
  public let yawPID: ref<PID>;
  public let brakeFactor: Float;
  public let fwtfCorrection: Float;
  public let pitchWithLift: Float;
  public let rollWithYaw: Float;
  public let swayWithYaw: Float;
  public let surgeOffset: Float;
  public let brakeOffset: Float;
  public let velocityPointing: Float;
  private let hovering: Bool;
  public let referenceZ: Float;
  public let maxHoverHeight: Float;
  public let m_maxSpeedModifier: ref<gameStatModifierData>;

  // defined in the RED4ext part
  // public func StartSnd() -> Void
  // public func StopSnd() -> Void
  // public func SetParams(...) -> Void

  private func Initialize(player: ref<PlayerPuppet>) {
    this.gameInstance = player.GetGame();
    this.enabled = false;
    this.active = false;
    this.showOptions = false;
    this.brake = PID.Create(0.05, 0.0, 0.0, 0.0);
    this.lift = PID.Create(0.05, 0.0, 0.0, 0.0);
    this.liftFactor = 15.0;
    this.surgePos = PID.Create(0.04, 0.0, 0.0, 0.0);
    this.surgeNeg = PID.Create(0.04, 0.0, 0.0, 0.0);
    this.roll = PID.Create(0.5, 0.0, 0.0, 0.0);
    this.pitch = PID.Create(0.5, 0.0, 0.0, 0.0);
    this.yaw = PID.Create(0.02, 0.0, 0.0, 0.0);
    this.yawFactor = 8.0;
    this.yawCorrectionFactor = 10.0;
    this.distance = 0.0;
    this.distanceEase = 0.5;
    this.normal = new Vector4(0.0, 0.0, 1.0, 0.0);
    this.normalEase = 0.01;
    this.airResistance = 2.0;
    this.hoverHeight = 3.50;
    this.maxHoverHeight = 7.0;
    this.hoverFactor = 4.0;
    this.hover = PID.Create(0.1, 0.01, 0.05);
    this.pitchPID = PID.Create(0.5, 0.05, 0.1);
    this.rollPID = PID.Create(0.5, 0.05, 0.1);
    this.yawPID = PID.Create(0.5, 0.5, 0.5);
    this.brakeFactor = 250.0;
    // this.lookAheadMax = 10.0;
    this.lookAheadMax = 0.0;
    this.lookAheadMin = 1.0;
    this.lookDown = new Vector4(0.0, 0.0, -this.maxHoverHeight - 10.0, 0.0);
    this.fwtfCorrection = 0.0;
    this.pitchWithLift = -0.3;
    this.rollWithYaw = 0.15;
    this.swayWithYaw = 0.5;
    this.surgeOffset = 0.5;
    // this.brakeOffset = 0.5;
    this.brakeOffset = 0.0;
    this.velocityPointing = 0.5;
    this.hovering = true;
    this.referenceZ = 0.0;

    this.audioStats = FlightControlAudioStats.Create();  
  }
  
  public static func CreateInstance(player: ref<PlayerPuppet>) {
    let instance: ref<FlightControl> = new FlightControl();
    instance.Initialize(player);  

    // This strong reference will tie the lifetime of the singleton 
    // to the lifetime of the player entity
    player.FlightControlInstance = instance;

    // This weak reference is used as a global variable 
    // to access the mod instance anywhere
    GetAllBlackboardDefs().FlightControlInstance = instance;
    LogChannel(n"DEBUG", "Flight Control Loaded");
  }
  
  public static func GetInstance() -> wref<FlightControl> {
    return GetAllBlackboardDefs().FlightControlInstance;
  }
  
  public func Enable(vehicle: ref<VehicleObject>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.enabled = true;
    this.active = false;
    this.stats = VehicleStats.Create(vehicle);
    this.GetVehicle().TurnOn(true);
    this.GetVehicle().TurnEngineOn(true);
    this.SetupActions();
    this.ui.Setup(this.stats);

    // very intrusive - need a prompt/confirmation that they want this popup, eg Detailed Info / About
    // let shardUIevent = new NotifyShardRead();
    // shardUIevent.title = "Flight Control: Now Available";
    // shardUIevent.text = "Your new car is equiped with the state-of-the-art Flight Control!";
    // GameInstance.GetUISystem(this.gameInstance).QueueEvent(shardUIevent);

    // these don't appear to do anything here - maybe it's only locomotion
    // this.m_maxSpeedModifier = RPGManager.CreateStatModifier(gamedataStatType.MaxSpeed, gameStatModifierType.Multiplier, 20.0);
    // scriptInterface.GetStatsSystem().AddModifier(Cast(scriptInterface.ownerEntityID), this.m_maxSpeedModifier);

    LogChannel(n"DEBUG", "Flight Control Enabled for " + this.GetVehicle().GetDisplayName());
  }

  public func Disable(scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if this.active {
      this.Deactivate(true);
    }
    this.enabled = false;
    this.SetupActions();   
    this.stats = null;

    // if IsDefined(this.m_maxSpeedModifier) {
    //   scriptInterface.GetStatsSystem().RemoveModifier(Cast(scriptInterface.ownerEntityID), this.m_maxSpeedModifier);
    //   this.m_maxSpeedModifier = null;
    // }

    LogChannel(n"DEBUG", "Flight Control Disabled");
  }

  public func Toggle() -> Bool {
    if this.active {
      this.Deactivate(false);
    } else {
      this.Activate();
    }
    this.GetVehicle().GetBlackboard().SetBool(GetAllBlackboardDefs().Vehicle.FlightActive, this.active);
    this.GetVehicle().GetBlackboard().SignalBool(GetAllBlackboardDefs().Vehicle.FlightActive);
    return this.active;
  }
  
  private func Activate() -> Void {
    this.active = true;
    this.SetupActions();
    this.hover.Reset();
    this.pitchPID.Reset();
    this.rollPID.Reset();
    this.yawPID.Reset();
    (this.GetVehicle().GetPS() as VehicleComponentPS).SetThrusterState(false);
    this.GetVehicle().TurnEngineOn(false);
    this.GetVehicle().TurnOn(false);

    this.stats.Reset();
    if IsDefined(this.ui) {
      let fadeInAnim = new inkAnimTransparency();
      fadeInAnim.SetStartTransparency(0.0);
      fadeInAnim.SetEndTransparency(1.0);
      fadeInAnim.SetDuration(0.200);
		  let activationAnim = new inkAnimDef();
		  activationAnim.AddInterpolator(fadeInAnim);
      this.ui.PlayAnimation(activationAnim);
    }
  
      // stateContext.SetPermanentBoolParameter(n"ForceIdleVehicle", true, true);
    // this.GetVehicle().GetBlackboard().SetVariant(GetAllBlackboardDefs().UI_ActiveVehicleData.VehPlayerStateData, ToVariant(3));

    // let hudManager: ref<HUDManager>;
    // let registration: ref<HUDManagerRegistrationRequest>;
    // if this.GetVehicle().IsCrowdVehicle() && !this.GetVehicle().ShouldForceRegisterInHUDManager() {
    //   return;
    // };
    // hudManager = GameInstance.GetScriptableSystemsContainer(this.GetVehicle().GetGame()).Get(n"HUDManager") as HUDManager;
    // if IsDefined(hudManager) {
    //   registration = new HUDManagerRegistrationRequest();
    //   registration.SetProperties(this.GetVehicle(), shouldRegister);
    //   hudManager.QueueRequest(registration);
    // };    
    
    // GetPlayer(this.gameInstance).GetPlayerStateMachineBlackboard().SetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle, EnumInt(gamePSMVehicle.Scene));

    // let psm: ref<PSMStartStateMachine> = new PSMStartStateMachine();
    // psm.stateMachineIdentifier.definitionName = n"Crosshair";
    // this.GetVehicle().QueueEvent(psm);

    // this.HUDGameController.ShowRequest();
    
    // let uiSystemBB = GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UIGameData);
    // let uiSystemBBDef = GetAllBlackboardDefs().UIGameData;
    // uiSystemBB.SetBool(uiSystemBBDef.Popup_IsShown, true);
    // uiSystemBB.SignalBool(uiSystemBBDef.Popup_IsShown);

		// let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
		// let showEvent: ref<ShowFlightHUDEvent> = ShowFlightHUDEvent.Create(this.HUDGameController);

		// uiSystem.QueueEvent(showEvent);

    // let transform: WorldTransform;
    // WorldTransform.SetPosition(transform, this.stats.d_position + new Vector4(0.0, 0.0, 2.0, 0.0));
    // let beam: ref<FxInstance> = GameInstance.GetFxSystem(this.gameInstance).SpawnEffect(this.GetVehicle().GetFxResourceByKey(n"pingNetworkLink"), transform, true);

    // StatusEffectHelper.ApplyStatusEffect(GetPlayer(this.gameInstance), t"GameplayRestriction.NoCameraControl");
    // gameSaveLockReason.PlayerState
    //   private func IsSavingLocked() -> Bool {
    
    this.ShowSimpleMessage("Flight Control Engaged");
    // GameInstance.GetUISystem(this.gameInstance).PushGameContext(UIGameContext.VehicleRace); 


    // stateContext.SetPermanentCNameParameter(n"VehicleCameraParams", n"", true); 
    // this.driveEvents.UpdateCameraContext(stateContext, scriptInterface);
    // let param: StateResultCName = stateContext.GetPermanentCNameParameter(n"LocomotionCameraParams");
    // if param.valid {
    //     this.driveEvents.UpdateCameraParams(param.value, scriptInterface);
    // };
    GameInstance.GetAudioSystem(this.gameInstance).PlayFlightControlSound(n"ui_hacking_access_granted");
    // GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"ignition", true);
    // GameInstance.GetAudioSystem(this.gameInstance).PlayFlightControlSound(StringToName(this.GetVehicle().GetRecord().Player_audio_resource()));
    // GameInstance.GetAudioSystem(this.gameInstance).PlayFlightControlSound(n"mus_cp_arcade_quadra_START_menu");
    LogChannel(n"DEBUG", "Flight Control Activated");
  }

  private func Deactivate(silent: Bool) -> Void {
    this.active = false;
    this.SetupActions();
    // (this.GetVehicle().GetPS() as VehicleComponentPS).SetThrusterState(false);
    this.GetVehicle().TurnOn(true);
    this.GetVehicle().TurnEngineOn(true);
    // StatusEffectHelper.RemoveStatusEffect(GetPlayer(this.gameInstance), t"GameplayRestriction.NoCameraControl");
    if !silent {
      this.ShowSimpleMessage("Flight Control Disengaged");
      GameInstance.GetAudioSystem(this.gameInstance).PlayFlightControlSound(n"ui_hacking_access_denied");
    }
    if IsDefined(this.ui) {
      let fadeOutAnim = new inkAnimTransparency();
      fadeOutAnim.SetStartTransparency(1.0);
      fadeOutAnim.SetEndTransparency(0.0);
      fadeOutAnim.SetDuration(0.200);
		  let deactivationAnim = new inkAnimDef();
		  deactivationAnim.AddInterpolator(fadeOutAnim);
      this.ui.PlayAnimation(deactivationAnim);
    }

    // GetPlayer(this.gameInstance).GetPlayerStateMachineBlackboard().SetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle, EnumInt(gamePSMVehicle.Driving));

    // let psm: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    // psm.stateMachineIdentifier.definitionName = n"Crosshair";
    // this.GetVehicle().QueueEvent(psm);


    // let uiSystemBB = GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UIGameData);
    // let uiSystemBBDef = GetAllBlackboardDefs().UIGameData;
    // uiSystemBB.SetBool(uiSystemBBDef.Popup_IsShown, false);
    // uiSystemBB.SignalBool(uiSystemBBDef.Popup_IsShown);

		// let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
		// let hideEvent: ref<HideFlightHUDEvent> = HideFlightHUDEvent.Create(this.HUDGameController);

		// uiSystem.QueueEvent(hideEvent);

    // GameInstance.GetUISystem(this.gameInstance).PopGameContext(UIGameContext.VehicleRace);
    // let cameraContext: ref<PSMStopStateMachine> = new PSMStopStateMachine();
    // cameraContext.stateMachineIdentifier.definitionName = n"CameraContext";
    // GetPlayer(this.gameInstance).QueueEvent(cameraContext);

    // this.driveEvents.SetVehicleCameraParameters(stateContext, scriptInterface);

    // GameObject.PlaySound(GetPlayer(this.gameInstance), n"drone_disable");
    // GameInstance.GetAudioSystem(this.gameInstance).PlayLootAllSound();
    // GameInstance.GetAudioSystem(this.gameInstance).PlayFlightControlSound(n"mus_cp_arcade_quadra_STOP");
    LogChannel(n"DEBUG", "Flight Control Deactivated");
  }

  private func SetupActions() -> Bool {
    let player: ref<PlayerPuppet> = GetPlayer(this.gameInstance);
    let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    player.UnregisterInputListener(this);    
    uiSystem.QueueEvent(FlightControl.HideHintFromSource(n"FlightControl"));
    if this.enabled {
      player.RegisterInputListener(this, n"Flight_Toggle");
      if this.active {
        uiSystem.QueueEvent(FlightControl.ShowHintHelper("Disable Flight Control", n"Flight_Toggle", n"FlightControl"));
        // player.RegisterInputListener(this, n"Pitch");
        // uiSystem.QueueEvent(FlightControl.ShowHintHelper("Pitch", n"Pitch", n"FlightControl"));
        // player.RegisterInputListener(this, n"Roll");
        // uiSystem.QueueEvent(FlightControl.ShowHintHelper("Roll", n"Roll", n"FlightControl"));
        player.RegisterInputListener(this, n"Accelerate");
        player.RegisterInputListener(this, n"LeanFB");
        uiSystem.QueueEvent(FlightControl.ShowHintHelper("Lift", n"LeanFB", n"FlightControl"));
        player.RegisterInputListener(this, n"TurnX");
        uiSystem.QueueEvent(FlightControl.ShowHintHelper("Yaw", n"TurnX", n"FlightControl"));
        player.RegisterInputListener(this, n"Decelerate");
        // we may want to look at something else besides this input so ForceBrakesUntilStoppedOrFor will work (not entirely sure it doesn't now)
        // vehicle.GetBlackboard().GetInt(GetAllBlackboardDefs().Vehicle.IsHandbraking) is the value (why int? no enums for it seem to exist)
        player.RegisterInputListener(this, n"Handbrake");
        player.RegisterInputListener(this, n"Choice1_DualState");
        player.RegisterInputListener(this, n"FlightOptions_Up");
        player.RegisterInputListener(this, n"FlightOptions_Down");
        uiSystem.QueueEvent(FlightControl.ShowHintHelper("Flight Options", n"Choice1_DualState", n"FlightControl"));
      } else {
        uiSystem.QueueEvent(FlightControl.ShowHintHelper("Enable Flight Control", n"Flight_Toggle", n"FlightControl"));
      }
    }
  }

  // eventually, something like this?
  // protected func SetUIContext() -> Void {
	// 	let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetGame());
	// 	uiSystem.PushGameContext(UIGameContext.ModalPopup);
	// 	uiSystem.RequestNewVisualState(n"inkModalPopupState");
	// }

	// protected func ResetUIContext() -> Void {
	// 	let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetGame());
	// 	uiSystem.PopGameContext(UIGameContext.ModalPopup);
	// 	uiSystem.RestorePreviousVisualState(n"inkModalPopupState");
	// }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    let actionType: gameinputActionType = ListenerAction.GetType(action);
    let actionName: CName = ListenerAction.GetName(action);
    let value: Float = ListenerAction.GetValue(action);
    // LogChannel(n"DEBUG", ToString(actionType) + ToString(actionName) + ToString(value));
    if Equals(actionName, n"Flight_Toggle") && ListenerAction.IsButtonJustPressed(action) {
        this.Toggle();
        // ListenerActionConsumer.ConsumeSingleAction(consumer);
    }
    if this.active {
      if Equals(actionName, n"Choice1_DualState") {
        if ListenerAction.IsButtonJustPressed(action) {
          LogChannel(n"DEBUG", "Options button pressed");
          this.showOptions = true;
        }
        if ListenerAction.IsButtonJustReleased(action) {
          LogChannel(n"DEBUG", "Options button released");
          this.showOptions = false;
        }
      }
      if this.showOptions {
        if Equals(actionName, n"FlightOptions_Up") && ListenerAction.IsButtonJustPressed(action) {
            this.hoverHeight += 0.1;
            GameInstance.GetAudioSystem(this.gameInstance).PlayFlightControlSound(n"ui_menu_onpress");
            LogChannel(n"DEBUG", "hoverHeight = " + ToString(this.hoverHeight));
        }
        if Equals(actionName, n"FlightOptions_Down") && ListenerAction.IsButtonJustPressed(action) {
            this.hoverHeight -= 0.1;
            GameInstance.GetAudioSystem(this.gameInstance).PlayFlightControlSound(n"ui_menu_onpress");
            LogChannel(n"DEBUG", "hoverHeight = " + ToString(this.hoverHeight));
        }
      }
      if Equals(actionType, gameinputActionType.AXIS_CHANGE) {
        switch(actionName) {
          case n"Roll":
            this.roll.SetInput(value);
            break;
          case n"Pitch":
            this.pitch.SetInput(value);
            break;
          case n"Accelerate":
            this.surgePos.SetInput(value);
            break;
          case n"TurnX":
            this.yaw.SetInput(value);
            break;
          case n"LeanFB":
            this.lift.SetInput(value);
            break;
          case n"Decelerate":
            this.surgeNeg.SetInput(value);
            break;
          default:
            return false;
            break;
        }
        // ListenerActionConsumer.ConsumeSingleAction(consumer);
      }
      if Equals(actionName, n"Handbrake") {
        if Equals(actionType, gameinputActionType.BUTTON_PRESSED) {
          this.brake.SetInput(1.0);
        } else {
          this.brake.SetInput(0.0);
        }
      }
    } else {
      this.lift.SetInput(0.0);
      this.surgePos.SetInput(0.0);
      this.surgeNeg.SetInput(0.0);
      this.yaw.SetInput(0.0);
      this.pitch.SetInput(0.0);
      this.roll.SetInput(0.0);
      this.brake.SetInput(0.0);
    }
  }

  public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if !this.active {
      return;
    }
    this.audioStats.volume = 1.0;
    // this.audioStats.volume = (GameInstance.GetSettingsSystem(this.gameInstance).GetVar(n"/audio/volume", n"MasterVolume") as ConfigVarListInt).GetValue();
    // this.audioStats.volume *= (GameInstance.GetSettingsSystem(this.gameInstance).GetVar(n"/audio/volume", n"SfxVolume") as ConfigVarListInt).GetValue();

    if GameInstance.GetTimeSystem(this.gameInstance).IsPausedState() {
      this.audioStats.volume = 0.0;
      return;
    }
    // might need to handle just the scanning system's dilation, and the pause menu
    if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"radial") {
      // this might happpen?
      timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("radialMenu", "timeDilation");
      this.audioStats.volume *= 0.1;
    } else {
      if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive() {
        // i think this is what this is called
        timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("focusModeTimeDilation", "timeDilation");
        this.audioStats.volume *= 0.1;
      }
    }

    let player: ref<PlayerPuppet> = GetPlayer(this.gameInstance);
    if !IsDefined(this.GetVehicle()) { 
      if IsDefined(scriptInterface.owner as VehicleObject) {
        this.stats = VehicleStats.Create(scriptInterface.owner as VehicleObject);
        LogChannel(n"DEBUG", "Vehicle undefined. Redefined to " + this.GetVehicle().GetDisplayName()); 
      } else {
        LogChannel(n"DEBUG", "Owner not defined"); 
      }
    }
    if !this.GetVehicle().IsPlayerMounted() { 
      LogChannel(n"DEBUG", "Vehicle is not player mounted"); 
      return; 
    }

    this.stats.UpdateDynamic(timeDelta);

    let direction = this.stats.d_direction;
    if this.stats.d_speed < 1.0 {
      direction = this.stats.d_forward;
    }

    let hoverCorrection: Float = 0.0;
    let pitchCorrection: Float = 0.0;
    let rollCorrection: Float = 0.0;
    let yawCorrection: Float = 0.0;

    let yawValue = this.yaw.GetValue(timeDelta);
    let rollValue = this.roll.GetValue(timeDelta);
    let pitchValue = this.pitch.GetValue(timeDelta);
    let liftValue = this.lift.GetValue(timeDelta);
    let brakeValue = this.brake.GetValue(timeDelta);
    let surgeValue = this.surgePos.GetValue(timeDelta) - this.surgeNeg.GetValue(timeDelta);

    this.hoverHeight += liftValue * timeDelta * this.liftFactor * (1.0 + this.stats.d_speedRatio * 2.0);
    if this.hovering {
      this.hoverHeight = MaxF(0.0, this.hoverHeight);
    }

    let foundGround = true;

    let queryFilter: QueryFilter;
    QueryFilter.AddGroup(queryFilter, n"Water");

    let findWater: TraceResult = scriptInterface.RayCastWithCollisionFilter(this.stats.d_position, this.stats.d_position - this.lookDown, queryFilter);
    if TraceResult.IsValid(findWater) {
      // if we're under water, just go up
      hoverCorrection = 1.0;
    } else {
      // we could also use these four points (or different ones) to just move each of the corners of the vehicle, instead of doing all this math
      let lookAheadPoint: array<Vector4>;
      lookAheadPoint[0] = (direction + Vector4.Cross(direction, player.GetWorldUp()) * 0.5) * (this.lookAheadMin + this.lookAheadMax * this.stats.d_speedRatio);
      lookAheadPoint[1] = (direction - Vector4.Cross(direction, player.GetWorldUp()) * 0.5) * (this.lookAheadMin + this.lookAheadMax * this.stats.d_speedRatio);
      lookAheadPoint[2] = (-direction + Vector4.Cross(direction, player.GetWorldUp()) * 0.5);
      lookAheadPoint[3] = (-direction - Vector4.Cross(direction, player.GetWorldUp()) * 0.5);

      QueryFilter.AddGroup(queryFilter, n"Static");
      QueryFilter.AddGroup(queryFilter, n"Terrain");
      // this finds vehicle too - need to figure out how to exclude it
      // QueryFilter.AddGroup(queryFilter, n"PlayerBlocker"); 

      let findGround1: TraceResult = scriptInterface.RayCastWithCollisionFilter(this.stats.d_position + lookAheadPoint[0], this.stats.d_position + lookAheadPoint[0] + this.lookDown, queryFilter);
      let findGround2: TraceResult = scriptInterface.RayCastWithCollisionFilter(this.stats.d_position + lookAheadPoint[1], this.stats.d_position + lookAheadPoint[1] + this.lookDown, queryFilter);
      let findGround3: TraceResult = scriptInterface.RayCastWithCollisionFilter(this.stats.d_position + lookAheadPoint[2], this.stats.d_position + lookAheadPoint[2] + this.lookDown, queryFilter);
      let findGround4: TraceResult = scriptInterface.RayCastWithCollisionFilter(this.stats.d_position + lookAheadPoint[3], this.stats.d_position + lookAheadPoint[3] + this.lookDown, queryFilter);
      if TraceResult.IsValid(findGround1) && TraceResult.IsValid(findGround2) && TraceResult.IsValid(findGround3) && TraceResult.IsValid(findGround4) {
        let distance = MaxF(
          MaxF(Vector4.Distance(this.stats.d_position + lookAheadPoint[0], Cast(findGround1.position)),
          Vector4.Distance(this.stats.d_position + lookAheadPoint[1], Cast(findGround2.position))),
          MaxF(Vector4.Distance(this.stats.d_position + lookAheadPoint[2], Cast(findGround3.position)),
          Vector4.Distance(this.stats.d_position + lookAheadPoint[3], Cast(findGround4.position))));
        this.distance = distance * this.distanceEase + this.distance * (1.0 - this.distanceEase);

        // FromVariant(scriptInterface.GetStateVectorParameter(physicsStateValue.Radius)) maybe?
        let normal = (Cast(findGround1.normal) + Cast(findGround2.normal) + Cast(findGround3.normal) + Cast(findGround4.normal)) / 4.0;
        this.normal = Vector4.Interpolate(this.normal, normal, this.normalEase);

      } else {
        foundGround = false;
      }   
    }

    if ((this.distance > this.maxHoverHeight && this.hovering) || (this.hovering && !foundGround)) {
      this.hovering = false;
      this.referenceZ = this.stats.d_position.Z;
      this.hoverHeight = 0.0;
    }
    if (this.distance <= this.maxHoverHeight && !this.hovering && foundGround) {
      this.hovering = true;
      this.hoverHeight = this.distance;
    }

    let heightDifference = 0.0;
    let idealNormal = new Vector4(0.0, 0.0, 1.0, 0.0);
    // would be cool to fade between these instead of using a boolean
    if this.hovering {
      // close to ground, use as reference
      heightDifference = this.hoverHeight - this.distance;
      idealNormal = this.normal;
    } else {
      // use absolute Z if too high
      heightDifference = this.referenceZ + this.hoverHeight - this.stats.d_position.Z;
    }

    hoverCorrection = this.hover.GetCorrectionClamped(heightDifference, timeDelta, 1.0);
    pitchCorrection = this.pitchPID.GetCorrectionClamped(Vector4.Dot(idealNormal, this.stats.d_forward) + liftValue * this.pitchWithLift, timeDelta, 1.0);
    rollCorrection = this.rollPID.GetCorrectionClamped(Vector4.Dot(idealNormal, this.stats.d_right), timeDelta, 1.0) + yawValue * this.rollWithYaw;
    let angle: Float = Vector4.GetAngleDegAroundAxis(Vector4.Interpolate(this.stats.d_forward, direction, this.stats.d_speedRatio * this.velocityPointing), this.stats.d_forward, new Vector4(0.0, 0.0, 1.0, 0.0));

    // decay the integral if we have yaw input - this helps get rid of the windup effect
    this.yawPID.integralFloat *= (1.0 - AbsF(yawValue));
    yawCorrection = this.yawPID.GetCorrection(angle + yawValue * this.yawFactor * (1.0 + this.stats.d_speedRatio * 5.0), timeDelta) * this.yawCorrectionFactor;

    let velocityDamp: Vector4 = MaxF(brakeValue * this.brakeFactor, this.airResistance) * this.stats.d_velocity;
    // so we don't get impulsed by the speed limit (100 m/s, i think)
    if this.stats.d_speed > 90.0 {
      velocityDamp *= (1 + PowF((this.stats.d_speed - 90.0) / 10.0, 2.0) * 1000.0);
    }

    let yawDirectionality: Float = (this.stats.d_speedRatio + AbsF(yawValue) * this.swayWithYaw) * this.stats.s_mass;
    let liftForce: Float = hoverCorrection  * this.stats.s_mass * this.hoverFactor;
    let surgeForce: Float = surgeValue * this.stats.s_mass;

    this.CreateImpulse(this.stats.d_position, new Vector4(0.00, 0.00, liftForce, 0.00) + Vector4.Normalize2D(this.stats.d_right) * Vector4.Dot(this.stats.d_forward - direction, this.stats.d_right) * yawDirectionality);
    this.CreateImpulse(this.stats.d_position + this.stats.d_forward * this.surgeOffset, this.stats.d_forward * surgeForce);
    this.CreateImpulse(this.stats.d_position - this.stats.d_forward * (pitchCorrection + pitchValue / 10.0) * this.stats.s_momentOfInertia.X - this.stats.d_right * (rollCorrection + rollValue / 10.0) * this.stats.s_momentOfInertia.Y, this.stats.d_up * 2000.0);
    this.CreateImpulse(this.stats.d_position + this.stats.d_forward * (pitchCorrection + pitchValue / 10.0) * this.stats.s_momentOfInertia.X + this.stats.d_right * (rollCorrection + rollValue / 10.0) * this.stats.s_momentOfInertia.Y, this.stats.d_up * -2000.0);
    this.CreateImpulse(this.stats.d_position + this.stats.d_forward, this.stats.d_right * this.stats.s_momentOfInertia.Z * (yawCorrection));
    this.CreateImpulse(this.stats.d_position - this.stats.d_forward, this.stats.d_right * this.stats.s_momentOfInertia.Z * -(yawCorrection));
    this.CreateImpulse(this.stats.d_position + this.stats.d_forward * brakeValue * this.brakeOffset, -velocityDamp);

    // (this.GetVehicle().GetPS() as VehicleComponentPS).SetThrusterState(surgeValue > 0.99);

    this.audioStats.playerPosition = this.stats.d_position;
    this.audioStats.playerUp = this.stats.d_up;
    this.audioStats.playerForward = this.stats.d_forward;

    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(this.gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);

    this.audioStats.cameraPosition = cameraTransform.position;
    this.audioStats.cameraUp = Transform.GetUp(cameraTransform);
    this.audioStats.cameraForward = Transform.GetForward(cameraTransform);

    this.audioStats.speed = this.stats.d_speed;
    this.audioStats.yawDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_up);
    this.audioStats.pitchDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_right);
    
    this.audioStats.surge = surgeValue;
    this.audioStats.yaw = yawValue;
    this.audioStats.lift = liftValue;
    this.audioStats.brake = brakeValue;
    
    this.ui.Update(timeDelta);
    // worlduiWidgetComponent

    // always zero from what i can tell :/
    // LogChannel(n"DEBUG", ToString(vehicle.GetBlackboard().GetInt(GetAllBlackboardDefs().Vehicle.IsHandbraking)));

    // i just wanna move the vehicle camera pls
    // GameInstance.GetPlayerSystem(this.gameInstance).SetFreeCameraTransform(Transform.Create(vehicleCOM - this.GetVehicle().d_forward * 10 + this.GetVehicle().d_up * 5));

    // LogChannel(n"DEBUG", "hover: " + ToString(hoverCorrection) + " pitch: " + ToString(pitchCorrection) + " roll: " + ToString(rollCorrection) + " yaw: " + ToString(yawCorrection));
    
    // accurate flying speed on the speedometer for development
    // if we go above 100 units here, it seems to brake us - not sure if the game is checking LV or the BB value
    // it's the linearvelocity level :/

    // LogChannel(n"DEBUG", ToString(GameInstance.GetCameraSystem(this.gameInstance).ProjectPoint(FlightControl.GetInstance().stats.d_position)));

  }

  public func CreateImpulse(position: Vector4, direction: Vector4) -> Void {
    let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
    impulseEvent.radius = 0.5;
    impulseEvent.worldPosition = Vector4.Vector4To3(position);
    impulseEvent.worldImpulse = Vector4.Vector4To3(direction);
    this.GetVehicle().QueueEvent(impulseEvent);
  }

  public func ShowSimpleMessage(message: String) -> Void {
    let msg: SimpleScreenMessage;
    msg.isShown = true;
    msg.duration = 2.00;
    msg.message = message;
    msg.isInstant = true;
    GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_Notifications).SetVariant(GetAllBlackboardDefs().UI_Notifications.OnscreenMessage, ToVariant(msg), true);
  }

  public static func ShowHintHelper(label: String, action: CName, source: CName) -> ref<UpdateInputHintEvent> {
    let evt: ref<UpdateInputHintEvent> = new UpdateInputHintEvent();
    let data: InputHintData;
    data.source = source;
    data.action = action;
    data.localizedLabel = label;

    evt.data = data;
    evt.show = true;
    evt.targetHintContainer = n"GameplayInputHelper";
    return evt;
  }

  public static func HideHintFromSource(source: CName) -> ref<DeleteInputHintBySourceEvent> {
    let evt: ref<DeleteInputHintBySourceEvent> = new DeleteInputHintBySourceEvent();
    evt.source = source;
    evt.targetHintContainer = n"GameplayInputHelper";
    return evt;
  }

  // Method to add new widget requires parent widget
  // inkCompoundWidget is a base class for all widget types that can have children
  // Usually it's inkCanvas or inkFlex containers with absolute positioning
  // And inkHorizontalPanel or inkVerticalPanel containers for auto layouts
  public static func HUDStatusSetup(parent: wref<inkCompoundWidget>) -> ref<inkText> {
    let flightControlStatus: ref<inkText> = new inkText();
    flightControlStatus.SetName(n"flightControlStatus");
    // Add widget instance to the parent
    flightControlStatus.Reparent(parent);

    // Set font
    flightControlStatus.SetFontFamily("base\\gameplay\\gui\\fonts\\orbitron\\orbitron.inkfontfamily");
    flightControlStatus.SetFontStyle(n"Medium");
    flightControlStatus.SetFontSize(24);
    flightControlStatus.SetLetterCase(textLetterCase.UpperCase);

    // Set color
    flightControlStatus.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));

    // Set content
    flightControlStatus.SetText("You shouldn't see this!");
    // flightControlStatus.SetHorizontalAlignment(textHorizontalAlignment.Center);
    // flightControlStatus.SetAnchor(inkEAnchor.TopCenter);


    // Set widget position relative to parent
    // Altough the position is absolute for FHD resoltuion,
    // it will be adapted for the current resoltuion
    flightControlStatus.SetMargin(130, 1822, 0, 0);

    // Set widget size
    flightControlStatus.SetSize(220.0, 50.0);
    return flightControlStatus;
  }

}

// boo
// @wrapMethod(VehicleObject)
// public final native func GetLinearVelocity() -> Vector4 {
//   return wrappedMethod() * 0.10;
// }

// try to reduce internal velocity to combat speed limits
// no luck
// @wrapMethod(DefaultTransition)
// public final static func GetLinearVelocity(const scriptInterface: ref<StateGameScriptInterface>) -> Vector4 {
//   // if FlightControl.GetInstance().IsActive() {
//     return wrappedMethod(scriptInterface) * 0.1;
//   // } else {
//     // return wrappedMethod(scriptInterface);
//   // }
// }

// probably not it
// @wrapMethod(MeleeAttackGenericEvents)
// protected final func ShouldBlockMovementImpulseUpdate(timeDelta: Float, attackData: ref<MeleeAttackData>, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   if !scriptInterface.IsOnGround() && (attackData.forwardImpulse < 0.00 || attackData.forwardImpulse > 0.00) {
//     return true;
//   };
//   if scriptInterface.GetOwnerStateVectorParameterFloat(physicsStateValue.LinearSpeed) >= 10.00 {
//     return true;
//   };
//   return false;
// }

@addMethod(AudioSystem)
public final func PlayFlightControlSound(sound: CName) -> Void {
  this.Play(sound);
}

@addMethod(AudioSystem)
public final func PlayFlightControlSoundFrom(sound: CName, object: ref<GameObject>) -> Void {
  let objectID: EntityID = object.GetEntityID();
  if !EntityID.IsDefined(objectID) {
    this.Play(sound, objectID);
  }
}

@addMethod(AudioSystem)
public final func StopFlightControlSoundFrom(sound: CName, object: ref<GameObject>) -> Void {
  let objectID: EntityID = object.GetEntityID();
  if !EntityID.IsDefined(objectID) {
    this.Stop(sound, objectID);
  }
}

@addField(PlayerPuppet)
public let FlightControlInstance: ref<FlightControl>; // Must be strong reference

@addField(AllBlackboardDefinitions)
public let FlightControlInstance: wref<FlightControl>; // Must be weak reference

// Option 2 -- Get the player instance as soon as it's ready
@wrapMethod(PlayerPuppet)
protected cb func OnGameAttached() -> Bool {
  wrappedMethod();
  if !this.IsReplacer() {
    FlightControl.CreateInstance(this);
  }
}

@wrapMethod(DriveEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(stateContext, scriptInterface);
  let vehicle: ref<VehicleObject> = scriptInterface.owner as VehicleObject;  
  if vehicle.IsPlayerMounted() {
    FlightControl.GetInstance().Enable(vehicle, scriptInterface);
  }
}

@wrapMethod(DriveEvents)
public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightControl.GetInstance().Disable(scriptInterface);
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(DriveEvents)
public final func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightControl.GetInstance().Disable(scriptInterface);
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(DriveEvents)
public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(timeDelta, stateContext, scriptInterface);
  FlightControl.GetInstance().OnUpdate(timeDelta, stateContext, scriptInterface);
}

@addField(VehicleDef)
public let FlightActive: BlackboardID_Bool;

// Hook into corresponding controller
@wrapMethod(hudCarController)
private final func Reset() -> Void {
  wrappedMethod();
  this.OnFlightActiveChanged(false);
}

@addField(hudCarController)
private let m_flightActiveBBConnectionId: ref<CallbackHandle>;

@addField(hudCarController)
private let m_flightControlStatus: wref<inkText>;

@wrapMethod(hudCarController)
private final func RegisterToVehicle(register: Bool) -> Void {
  wrappedMethod(register);
  let vehicleBlackboard: wref<IBlackboard>;
  let vehicle: ref<VehicleObject> = this.m_activeVehicle;
  if vehicle == null {
    return;
  };
  vehicleBlackboard = vehicle.GetBlackboard();
  if IsDefined(vehicleBlackboard) {
    if register {
      // GetRootWidget() returns root widget of base type inkWidget
      // GetRootCompoundWidget() returns root widget casted to inkCompoundWidget
      if !IsDefined(this.m_flightControlStatus) {
        this.m_flightControlStatus = FlightControl.HUDStatusSetup(this.GetRootCompoundWidget());
      }
      this.m_flightActiveBBConnectionId = vehicleBlackboard.RegisterListenerBool(GetAllBlackboardDefs().Vehicle.FlightActive, this, n"OnFlightActiveChanged");
    } else {
      vehicleBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().Vehicle.FlightActive, this.m_flightActiveBBConnectionId);
    };
  };
}

@addMethod(hudCarController)
protected cb func OnFlightActiveChanged(active: Bool) -> Bool {
  if !IsDefined(this.m_flightControlStatus) {
    this.m_flightControlStatus = FlightControl.HUDStatusSetup(this.GetRootCompoundWidget());
  }
  if active {
    this.m_flightControlStatus.SetText("Flight Control Engaged");
  } else {
    this.m_flightControlStatus.SetText("Flight Control Available");
  }
}

// show the real km/s for dev
// @replaceMethod(hudCarController)
// protected cb func OnSpeedValueChanged(speedValue: Float) -> Bool {
//   speedValue = AbsF(speedValue);
//   // let multiplier: Float = GameInstance.GetStatsDataSystem(this.m_activeVehicle.GetGame()).GetValueFromCurve(n"vehicle_ui", speedValue, n"speed_to_multiplier");
//   inkTextRef.SetText(this.m_SpeedValue, IntToString(RoundMath(speedValue)));
// }

// @wrapMethod(VehicleObject)
// public const func IsVehicle() -> Bool {
//   if FlightControl.GetInstance().IsActive() {
//     return false;
//   } else {
//     return wrappedMethod();
//   }
// }

// might be good to replace this
// @wrapMethod(ReactionManagerComponent)
// private final func ShouldStimBeProcessedByCrowd(stimEvent: ref<StimuliEvent>) -> Bool {

// }

// requires vehicle to be off to control? also makes a sound, which is nice
// @replaceMethod(VehicleComponent)
// private final func SetupThrusterFX() -> Void {
//   let toggle: Bool = (this.GetPS() as VehicleComponentPS).GetThrusterState();
//   if toggle || (Equals(FlightControl.GetInstance().GetVehicle(), this.GetVehicle()) && FlightControl.GetInstance().GetThrusterState()) {
//     GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"thrusters", true);
//   } else {
//     GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"thrusters");
//   };
// }

// trying to unstick cars on load
// @wrapMethod(VehicleObject)
// public final func IsOnPavement() -> Bool {
//   return wrappedMethod() || true;
// }

@replaceMethod(VehicleComponent)
protected cb func OnVehicleWaterEvent(evt: ref<VehicleWaterEvent>) -> Bool {
  if evt.isInWater  {
    this.m_submerged = true;
    if !Equals(FlightControl.GetInstance().GetVehicle(), this.GetVehicle()) && FlightControl.GetInstance().IsActive() {
      this.BreakAllDamageStageFX(true);
      this.DestroyVehicle();
      this.DestroyRandomWindow();
      this.ApplyVehicleDOT(n"high");
    }
    GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"fire");
  } else {
    this.m_submerged = false;
  }
  ScriptedPuppet.ReevaluateOxygenConsumption(this.m_mountedPlayer);
}

// @wrapMethod(VehicleObject)
// protected cb func OnLookedAtEvent(evt: ref<LookedAtEvent>) -> Bool {
//   wrappedMethod(evt);
//   if this.IsDestroyed() && this.IsCurrentlyScanned() {
//     let player: ref<PlayerPuppet> = GetPlayer(this.GetGame());
//     let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetGame());
//     if evt.isLookedAt {
//         player.RegisterInputListener(this.m_vehicleComponent, n"Choice1");
//         uiSystem.QueueEvent(FlightControl.ShowHintHelper("Repair Vehicle", n"Choice1", n"RepairVehicle"));
//     } else {
//         player.UnregisterInputListener(this.m_vehicleComponent, n"Choice1");
//         uiSystem.QueueEvent(FlightControl.HideHintFromSource(n"RepairVehicle"));
//     }
//   }
// } 
// @wrapMethod(VehicleComponent) 
// protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
//   wrappedMethod(action, consumer);
//   let actionName: CName = ListenerAction.GetName(action);
//   let value: Float = ListenerAction.GetValue(action);
//   if Equals(actionName, n"Choice1") && ListenerAction.IsButtonJustReleased(action) {
//     LogChannel(n"DEBUG", "Attempting to repair vehicle");
//     this.RepairVehicle();
//     let player: ref<PlayerPuppet> = GetPlayer(this.GetVehicle().GetGame());
//     let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetVehicle().GetGame());
//     player.UnregisterInputListener(this, n"Choice1");
//     uiSystem.QueueEvent(FlightControl.HideHintFromSource(n"RepairVehicle"));
//   }
// }

// @addMethod(VehicleObject)
// public const func IsQuickHackAble() -> Bool {
//   return true;
// }

// @addMethod(VehicleObject)
// public const func IsQuickHacksExposed() -> Bool {
//   return true;
// }

//  public const func HasDirectActionsActive() -> Bool {
//     return false;
//   }


  // protected final func MountFromParent(mountingEvent: ref<MountingEvent>, ownerEntity: ref<Entity>) -> Void {
  //   let instanceData: StateMachineInstanceData;
  //   let initData: ref<VehicleTransitionInitData> = new VehicleTransitionInitData();
  //   let relationship: MountingRelationship = mountingEvent.relationship;
  //   let otherObjectType: gameMountingObjectType = relationship.otherMountableType;
  //   let otherObject: wref<GameObject> = IMountingFacility.RelationshipGetOtherObject(relationship);
  //   switch otherObjectType {
  //     case gameMountingObjectType.Vehicle:
  //       if mountingEvent.request.mountData.mountEventOptions.silentUnmount {
  //         return;
  //       };
  //       initData.instant = mountingEvent.request.mountData.isInstant;
  //       initData.entityID = mountingEvent.request.mountData.mountEventOptions.entityID;
  //       initData.alive = mountingEvent.request.mountData.mountEventOptions.alive;
  //       initData.occupiedByNeutral = mountingEvent.request.mountData.mountEventOptions.occupiedByNeutral;
  //       instanceData.initData = initData;
  //       this.AddStateMachine(n"Vehicle", instanceData, otherObject);