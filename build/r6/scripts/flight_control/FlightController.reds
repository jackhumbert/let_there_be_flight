import Codeware.UI.*

enum FlightMode {
  HoverFly = 0,
  Hover = 1,
  Drone = 2,
  Count = 3
} 

// public class FlightSettingsListener extends ConfigVarListener {

//   private let m_ctrl: wref<FlightController>;

//   public final func RegisterController(ctrl: ref<FlightController>) -> Void {
//     this.m_ctrl = ctrl;
//   }

//   public func OnVarModified(groupPath: CName, varName: CName, varType: ConfigVarType, reason: ConfigChangeReason) -> Void {
//     this.m_ctrl.OnVarModified(groupPath, varName, varType, reason);
//   }
// }

// could watch this
// this.m_callbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this, n"OnVehicleStateChanged");

public static func fc() -> wref<FlightController> {
  return FlightController.GetInstance();
}

public static func fcv() -> wref<VehicleObject> {
  return FlightController.GetInstance().GetVehicle();
}

// maybe this should extend ScriptableComponent or GameComponent?
// Singleton instance with player lifetime
public native class FlightController extends IScriptable {
  public let chassis: ref<vehicleChassisComponent>;
  private let gameInstance: GameInstance;
  private let player: ref<PlayerPuppet>;
  private let m_callbackID: ref<CallbackHandle>;
  private let m_vehicleCollisionBBStateID: ref<CallbackHandle>;
  private let stats: ref<FlightStats>;
  private let ui: ref<FlightControllerUI>;
  public final func SetUI(ui: ref<FlightControllerUI>) {
    this.ui = ui;
  }
  public let audio: ref<FlightAudio>;
  public final func GetAudio() -> ref<FlightAudio> {
    return this.audio;
  }
  public final const func GetVehicle() -> wref<VehicleObject> {
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
  private let mode: FlightMode;
  public final const func GetMode() -> FlightMode {
    return this.mode;
  }
  public let showOptions: Bool;
  public let showUI: Bool;
  public let brake: ref<InputPID>;
  public let lift: ref<InputPID>;
  public let liftFactor: Float;
  public let liftFactorDrone: Float;
  public let surge: ref<InputPID>;
  public let surgeFactor: Float;
  public let roll: ref<InputPID>;
  public let pitch: ref<InputPID>;
  public let yaw: ref<InputPID>;
  public let yawFactor: Float;
  public let yawFactorDrone: Float;
  public let yawDirectionalityFactor: Float;
  public let distance: Float;
  public let distanceEase: Float;
  public let normal: Vector4;
  public let normalEase: Float;
  public let lookAheadMax: Float;
  public let lookAheadMin: Float;
  public let lookDown: Vector4;
  public let airResistance: Float;
  public let defaultHoverHeight: Float;
  public let hoverHeight: Float;
  public let minHoverHeight: Float;
  public let maxHoverHeight: Float;
  public let hoverFactor: Float;
  public let hoverGroundPID: ref<PID>;
  public let hoverPID: ref<PID>;
  public let hoverClamp: Float;
  public let pitchGroundPID: ref<DualPID>;
  public let pitchPID: ref<DualPID>;
  public let pitchCorrectionFactor: Float;
  public let pitchFactorDrone: Float;
  public let rollGroundPID: ref<DualPID>;
  public let rollPID: ref<DualPID>;
  public let rollCorrectionFactor: Float;
  public let rollFactorDrone: Float;
  public let yawPID: ref<PID>;
  public let yawD: Float;
  public let yawCorrectionFactor: Float;
  public let brakeFactor: Float;
  public let angularDampFactor: Float;
  public let angularBrakeFactor: Float;
  public let fwtfCorrection: Float;
  public let pitchWithLift: Float;
  public let rollWithYaw: Float;
  public let swayWithYaw: Float;
  public let surgeOffset: Float;
  public let brakeOffset: Float;
  private let hovering: Bool;
  public let referenceZ: Float;
  private let secondCounter: Float;
  public let collisionTimer: Float;
  public let collisionRecoveryDelay: Float;
  public let collisionRecoveryDuration: Float;
  public let isInAnyMenu: Bool;
  public let audioEnabled: Bool;

  public let fl_tire: ref<IPlacedComponent>;
  public let fr_tire: ref<IPlacedComponent>;
  public let bl_tire: ref<IPlacedComponent>;
  public let br_tire: ref<IPlacedComponent>;

  public let isTPP: Bool;  

  private let uiBlackboard: ref<IBlackboard>;
  private let uiSystemBB: ref<UI_SystemDef>;
  // private let trackedMappinId: ref<CallbackHandle>;
  // private let m_currentMappin: wref<IMappin>;
  // private let waypoint: Vector4;

  public let timeDelta: Float;
  public let averageMass: Float;
  
  private let sqs: ref<SpatialQueriesSystem>;
  public let helper: ref<vehicleFlightHelper>;

  // protected let m_settingsListener: ref<FlightSettingsListener>;
  // protected let m_groupPath: CName;

  private func Initialize(player: ref<PlayerPuppet>) {
    this.gameInstance = player.GetGame();
    this.player = player;
    this.mode = FlightMode.HoverFly;
    this.enabled = false;
    this.active = false;
    this.showOptions = false;
    this.showUI = true;
    this.brakeFactor = 1.2;
    this.angularDampFactor = -100.0;
    this.angularBrakeFactor = -10.0;
    this.liftFactor = 8.0;
    this.liftFactorDrone = 30.0;
    this.surgeFactor = 15.0;
    // this.surgeFactor = 50.0;
    this.hoverFactor = 5.0;
    this.hoverClamp = 1.0;

    this.yawFactor = 50.0;
    this.pitchFactorDrone = 0.5;
    this.pitchCorrectionFactor = 3.0;
    this.rollFactorDrone = 5.0;
    this.rollCorrectionFactor = 15.0;
    this.yawCorrectionFactor = 0.05;
    this.yawFactorDrone = 3.0;
    this.yawDirectionalityFactor = 30.0;
    this.brake = InputPID.Create(0.05, 0.5);
    this.lift = InputPID.Create(0.05, 0.2);
    this.surge = InputPID.Create(0.04, 0.2);
    this.roll = InputPID.Create(0.5, 0.5);
    this.pitch = InputPID.Create(0.5, 0.5);
    this.yaw = InputPID.Create(0.02, 0.2);
    this.hoverGroundPID = PID.Create(1.0, 0.01, 0.1);
    this.hoverPID = PID.Create(1.0, 0.01, 0.1);
    this.pitchGroundPID = DualPID.Create(0.8, 0.2, 0.05,  0.8, 0.2, 0.05);
    this.pitchPID = DualPID.Create(1.0, 0.5, 0.5,  1.0, 0.5, 0.5);
    this.rollGroundPID =  DualPID.Create(0.5, 0.2, 0.05,  2.5, 1.5, 0.5);
    this.rollPID =  DualPID.Create(1.0, 0.5, 0.5,  1.0, 0.5, 0.5);
    this.yawPID = PID.Create(1.0, 0.1, 0.0);
    this.yawD = 3.0;
    this.distance = 0.0;
    this.distanceEase = 0.1;
    this.normal = new Vector4(0.0, 0.0, 1.0, 0.0);
    this.normalEase = 0.3;
    this.airResistance = 0.005;
    this.defaultHoverHeight = 3.50;
    this.hoverHeight = this.defaultHoverHeight;
    this.minHoverHeight = 1.0;
    this.maxHoverHeight = 7.0;
    this.lookAheadMax = 10.0;
    // this.lookAheadMin = 1.0;
    this.lookDown = new Vector4(0.0, 0.0, -this.maxHoverHeight - 10.0, 0.0);
    this.fwtfCorrection = 0.0;
    this.pitchWithLift = 0.8;
    // this.rollWithYaw = 0.15;
    // this.swayWithYaw =   0.5;
    // this.surgeOffset = 0.5;
    this.surgeOffset = 0.5;
    // this.brakeOffset = 0.5;
    this.brakeOffset = 0.0;
    this.hovering = true;
    this.referenceZ = 0.0;
    this.secondCounter = 0.0;
    this.collisionRecoveryDelay = 0.8;
    this.collisionRecoveryDuration = 0.8;
    this.collisionTimer = this.collisionRecoveryDelay;

    this.audio = FlightAudio.Create();  
    this.audioEnabled = true;

    this.uiBlackboard = GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_System);
    this.uiSystemBB = GetAllBlackboardDefs().UI_System;
    // this.trackedMappinId = this.uiBlackboard.RegisterListenerVariant(this.uiSystemBB.TrackedMappin, this, n"OnTrackedMappinUpdated");
    // this.uiBlackboard.SignalVariant(this.uiSystemBB.TrackedMappin);

    // this.waypoint = new Vector4(313.6, 208.2, 62.3, 0.0);

    this.sqs = GameInstance.GetSpatialQueriesSystem(this.gameInstance);

    // this.m_groupPath = n"/controls/flight";
    // this.m_settingsListener = new FlightSettingsListener();
    // this.m_settingsListener.RegisterController(this);
    // this.m_settingsListener.Register(this.m_groupPath);
  }

  public const func GetBlackboard() -> ref<IBlackboard> {
    return GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().FlightControllerBB);
  }
  
  public static func CreateInstance(player: ref<PlayerPuppet>) {
    let self: ref<FlightController> = new FlightController();
    self.Initialize(player);  

    // This strong reference will tie the lifetime of the singleton 
    // to the lifetime of the player entity
    player.flightController = self;

    // This weak reference is used as a global variable 
    // to access the mod instance anywhere
    GetAllBlackboardDefs().flightController = self;
    FlightLog.Info("[FlightController] CreateInstance");
  }
  
  public static func GetInstance() -> wref<FlightController> {
    return GetAllBlackboardDefs().flightController;
  }

  // public final func OnVarModified(groupPath: CName, varName: CName, varType: ConfigVarType, reason: ConfigChangeReason) -> Void {
  //   switch varName {
  //     case n"HoverHeight":
  //       let configVar: ref<ConfigVarFloat> = GameInstance.GetSettingsSystem(this.gameInstance).GetVar(this.m_groupPath, n"HoverHeight") as ConfigVarFloat;
  //       this.hoverHeight = configVar.GetValue();
  //       break;
  //     default:
  //   };
  // }

  public func SetupMountedToCallback(psmBB: ref<IBlackboard>) -> Void {
    this.m_callbackID = psmBB.RegisterListenerBool(GetAllBlackboardDefs().PlayerStateMachine.MountedToVehicle, this, n"OnMountedToVehicleChange");
    if psmBB.GetBool(GetAllBlackboardDefs().PlayerStateMachine.MountedToVehicle) {
      this.Enable();
    } 
  }
  
  public cb func OnMountedToVehicleChange(mounted: Bool) -> Bool {
    // FlightLog.Info("[FlightController] OnMountedToVehicleChange");
    if (mounted) {
      this.Enable();
    } else {
      this.Disable();
    }
  }
  
  public func Enable() -> Void {
    this.enabled = true;
    this.active = false;
    this.SetupActions();
    this.audio.Start("windLeft", "wind_TPP");
    this.audio.Start("windRight", "wind_TPP");
    this.stats = FlightStats.Create(this.player);
    this.ui.Setup(this.stats);

    FlightLog.Info("[FlightController] Enable - " + this.GetVehicle().GetDisplayName());

    (this.GetVehicle().FindComponentByName(n"cars_sport_fx") as EffectSpawnerComponent).AddEffect();

    this.helper = this.GetVehicle().AddFlightHelper();
  }

  public func Disable() -> Void {
    if this.active {
      this.Deactivate(true);
    }
    this.enabled = false;
    this.SetupActions();   
    this.stats = null;
    this.audio.Stop("windLeft");
    this.audio.Stop("windRight");

    FlightLog.Info("[FlightController] Disable");
  }

  public func Toggle() -> Bool {
    // if this.active {
    //   this.Deactivate(false);
    // } else {
    //   if (!this.enabled) {
    //     this.Enable();
    //   }
    //   this.Activate();
    // // }
    // this.GetBlackboard().SetBool(GetAllBlackboardDefs().FlightControllerBB.IsActive, this.active, true);
    // this.GetBlackboard().SignalBool(GetAllBlackboardDefs().FlightControllerBB.IsActive);
    return this.active;
  }
  
  private func Activate() -> Void {
    this.active = true;
    this.SetupActions();
    this.hoverGroundPID.Reset();
    this.hoverPID.Reset();
    this.pitchGroundPID.Reset();
    this.pitchPID.Reset();
    this.rollGroundPID.Reset();
    this.rollPID.Reset();
    this.yawPID.Reset();

    this.pitch.Reset();
    this.roll.Reset();
    this.yaw.Reset();
    this.surge.Reset();
    this.lift.Reset();
    this.brake.Reset();

    this.audio.Play("vehicle3_on");

    this.SetupTires();
    this.SetupPositionProviders();
    // this.GetVehicle().TurnOffAirControl();

    // GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"summon_hologram", true);
    GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"test_effect", true);

    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"tire_01_fl_a") as IComponent).Toggle(false);
    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"tire_01_fl_a_shadow") as IComponent).Toggle(false);
    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"wheel_01_fl_a") as IComponent).Toggle(false);

    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"tire_01_fr_a") as IComponent).Toggle(false);
    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"tire_01_fr_a_shadow") as IComponent).Toggle(false);
    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"wheel_01_fr_a") as IComponent).Toggle(false);

    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"tire_01_bl_a") as IComponent).Toggle(false);
    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"tire_01_bl_a_shadow") as IComponent).Toggle(false);
    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"wheel_01_bl_a") as IComponent).Toggle(false);

    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"tire_01_br_a") as IComponent).Toggle(false);
    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"tire_01_br_a_shadow") as IComponent).Toggle(false);
    // (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"wheel_01_br_a") as IComponent).Toggle(false);

    
    this.collisionTimer = this.collisionRecoveryDelay;
    this.hoverHeight = this.defaultHoverHeight;

    this.ui.camera = GetPlayer(this.gameInstance).FindComponentByName(n"vehicleTPPCamera") as vehicleTPPCameraComponent;
    // this.camera.isInAir = false;
    this.ui.camera.drivingDirectionCompensationAngleSmooth = 120.0;
    this.ui.camera.drivingDirectionCompensationSpeedCoef = 0.1;

    // these stop engine noises if they were already playing?
    this.GetVehicle().TurnEngineOn(false);
    // this.GetVehicle().TurnOn(true);

    // AnimationControllerComponent.PushEvent(this.GetVehicle(), n"VehicleNPCDeathData");

    // let evt = new AIEvent();
    // evt.name = n"DriverDead";
    // this.GetVehicle().QueueEvent(evt);

    this.audio.Start("leftFront", "vehicle3_TPP");
    this.audio.Start("rightFront", "vehicle3_TPP");
    this.audio.Start("leftRear", "vehicle3_TPP");
    this.audio.Start("rightRear", "vehicle3_TPP");

    // idk what to do with this
    // let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    // uiSystem.PushGameContext(IntEnum(10));

    this.GetVehicle().GetVehicleComponent().GetVehicleControllerPS().SetLightMode(vehicleELightMode.HighBeams);
    this.GetVehicle().GetVehicleComponent().GetVehicleController().ToggleLights(true);

    this.stats.Reset();
    if (this.showUI) {
      this.ui.Show();
    }
  
    this.ShowSimpleMessage("Flight Control Engaged");
    
    FlightLog.Info("[FlightController] Activate");
    this.GetBlackboard().SetBool(GetAllBlackboardDefs().FlightControllerBB.IsActive, true, true);
    this.GetBlackboard().SignalBool(GetAllBlackboardDefs().FlightControllerBB.IsActive);
  }

  private func Deactivate(silent: Bool) -> Void {
    this.active = false;
    this.SetupActions();

    this.audio.Play("vehicle3_off");
    // (this.GetVehicle().GetPS() as VehicleComponentPS).SetThrusterState(false);

    this.audio.Stop("leftFront");
    this.audio.Stop("rightFront");
    this.audio.Stop("leftRear");
    this.audio.Stop("rightRear");

    // GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"summon_hologram");
    GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"test_effect");

    //let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    //uiSystem.PopGameContext(IntEnum(10));

    // StatusEffectHelper.RemoveStatusEffect(GetPlayer(this.gameInstance), t"GameplayRestriction.NoCameraControl");
    if !silent {
      //this.GetVehicle().TurnOn(true);
      this.GetVehicle().TurnEngineOn(true);
      this.ShowSimpleMessage("Flight Control Disengaged");
      //GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_hacking_access_denied");
    }
    if (this.showUI) {
      this.ui.Hide();
    }

    FlightLog.Info("[FlightController] Deactivate");
    this.GetBlackboard().SetBool(GetAllBlackboardDefs().FlightControllerBB.IsActive, false, true);
    this.GetBlackboard().SignalBool(GetAllBlackboardDefs().FlightControllerBB.IsActive);
  }  

  private func ShowMoreInfo() -> Void {
    // very intrusive - need a prompt/confirmation that they want this popup, eg Detailed Info / About
    let shardUIevent = new NotifyShardRead();
    shardUIevent.title = "Flight Control: Now Available";
    shardUIevent.text = "Your new car is equiped with the state-of-the-art Flight Control!";
    GameInstance.GetUISystem(this.gameInstance).QueueEvent(shardUIevent);
  }

  private func SetupActions() -> Void {
    let player: ref<PlayerPuppet> = GetPlayer(this.gameInstance);
    let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    player.UnregisterInputListener(this);    
    // player.RegisterInputListener(this, n"OpenPauseMenu");
    uiSystem.QueueEvent(FlightController.HideHintFromSource(n"FlightController"));
    if this.enabled {
      // player.RegisterInputListener(this, n"Flight_Toggle");
      if this.active {
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Disable Flight Control", n"Flight_Toggle", n"FlightController"));
        player.RegisterInputListener(this, n"Pitch");
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Pitch", n"Pitch", n"FlightController"));
        player.RegisterInputListener(this, n"Roll");
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Roll", n"Roll", n"FlightController"));
        player.RegisterInputListener(this, n"SurgePos");
        player.RegisterInputListener(this, n"LeanFB");
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Lift", n"LeanFB", n"FlightController"));
        player.RegisterInputListener(this, n"TurnX");
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Yaw", n"TurnX", n"FlightController"));
        player.RegisterInputListener(this, n"SurgeNeg");
        // we may want to look at something else besides this input so ForceBrakesUntilStoppedOrFor will work (not entirely sure it doesn't now)
        // vehicle.GetBlackboard().GetInt(GetAllBlackboardDefs().Vehicle.IsHandbraking)
        player.RegisterInputListener(this, n"Handbrake");
        player.RegisterInputListener(this, n"Choice1_DualState");
        player.RegisterInputListener(this, n"FlightOptions_Up");
        player.RegisterInputListener(this, n"FlightOptions_Down");
        player.RegisterInputListener(this, n"FlightOptions_Left");
        player.RegisterInputListener(this, n"FlightOptions_Right");
        if this.showOptions {
          if Equals(this.mode, FlightMode.HoverFly) {
            uiSystem.QueueEvent(FlightController.ShowHintHelper("Hover Only", n"FlightOptions_Up", n"FlightController"));
          }
          if Equals(this.mode, FlightMode.Hover) {
            uiSystem.QueueEvent(FlightController.ShowHintHelper("Drone", n"FlightOptions_Up", n"FlightController"));
          }
          if Equals(this.mode, FlightMode.Drone) {
            uiSystem.QueueEvent(FlightController.ShowHintHelper("Hover & Fly", n"FlightOptions_Up", n"FlightController"));
          }
          // uiSystem.QueueEvent(FlightController.ShowHintHelper("Raise Hover Height", n"FlightOptions_Up", n"FlightController"));
          // uiSystem.QueueEvent(FlightController.ShowHintHelper("Lower Hover Height", n"FlightOptions_Down", n"FlightController"));
          uiSystem.QueueEvent(FlightController.ShowHintHelper("Toggle UI", n"FlightOptions_Left", n"FlightController"));

        }
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Flight Options", n"Choice1_DualState", n"FlightController"));
      } else {
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Enable Flight Control", n"Flight_Toggle", n"FlightController"));
      }
    }
  }

  // protected cb func OnTrackedMappinUpdated(value: Variant) -> Bool {
  //   this.m_currentMappin = FromVariant<ref<IScriptable>>(value) as IMappin;
  //   if IsDefined(this.m_currentMappin) {
  //     this.waypoint = this.m_currentMappin.GetWorldPosition();
  //   }
  //   // inkCompoundRef.RemoveAllChildren(this.m_TrackedMappinObjectiveContainer);
  //   // inkWidgetRef.SetVisible(this.m_TrackedMappinContainer, IsDefined(this.m_currentMappin));
  //   // if IsDefined(this.m_trackedMappinSpawnRequest) {
  //   //   this.m_trackedMappinSpawnRequest.Cancel();
  //   // };
  //   // if IsDefined(this.m_currentMappin) {
  //   //   this.m_trackedMappinSpawnRequest = this.AsyncSpawnFromLocal(inkWidgetRef.Get(this.m_TrackedMappinObjectiveContainer), n"Objective", this, n"OnTrackedMappinSpawned");
  //   // };
  // }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    let actionType: gameinputActionType = ListenerAction.GetType(action);
    let actionName: CName = ListenerAction.GetName(action);
    let value: Float = ListenerAction.GetValue(action);
    if Equals(actionName, n"OpenPauseMenu") && ListenerAction.IsButtonJustPressed(action) {
      this.audioEnabled = false;
      let engineVolume = 0.0;
      let windVolume = 0.0;
      this.audio.Update("leftFront", Vector4.EmptyVector(), engineVolume);
      this.audio.Update("rightFront", Vector4.EmptyVector(), engineVolume);
      this.audio.Update("leftRear", Vector4.EmptyVector(), engineVolume);
      this.audio.Update("rightRear", Vector4.EmptyVector(), engineVolume);
      this.audio.Update("windLeft", Vector4.EmptyVector(), windVolume);
      this.audio.Update("windRight", Vector4.EmptyVector(), windVolume);
    }
    // FlightLog.Info(ToString(actionType) + ToString(actionName) + ToString(value));
    if Equals(actionName, n"Flight_Toggle") && ListenerAction.IsButtonJustPressed(action) {
        this.Toggle();
        // ListenerActionConsumer.ConsumeSingleAction(consumer);
    }
    if this.active {
      if Equals(actionName, n"Choice1_DualState") {
        if ListenerAction.IsButtonJustPressed(action) {
          FlightLog.Info("Options button pressed");
          this.showOptions = true;
          if (this.showUI) {
            this.ui.ShowInfo();
          }
          this.SetupActions();
        }
        if ListenerAction.IsButtonJustReleased(action) {
          FlightLog.Info("Options button released");
          this.showOptions = false;
          this.SetupActions();
        }
      }
      if this.showOptions {
        // if Equals(actionName, n"FlightOptions_Up") && ListenerAction.IsButtonJustPressed(action) {
        //     this.hoverHeight += 0.1;
        //     GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_menu_onpress");
        //     FlightLog.Info("hoverHeight = " + ToString(this.hoverHeight));
        // }
        // if Equals(actionName, n"FlightOptions_Down") && ListenerAction.IsButtonJustPressed(action) {
        //     this.hoverHeight -= 0.1;
        //     GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_menu_onpress");
        //     FlightLog.Info("hoverHeight = " + ToString(this.hoverHeight));
        // }
        if Equals(actionName, n"FlightOptions_Up") && ListenerAction.IsButtonJustPressed(action) {
            this.mode = IntEnum((EnumInt(this.mode) + 1) % EnumInt(FlightMode.Count));
            if Equals(this.mode, FlightMode.HoverFly) {
              this.ShowSimpleMessage("Flight & Hover Enabled");
            }
            if Equals(this.mode, FlightMode.Hover) {
              this.ShowSimpleMessage("Hover Enabled");
            }
            if Equals(this.mode, FlightMode.Drone) {
              this.ShowSimpleMessage("Drone Enabled");
            }
            GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_menu_onpress");
        }
        if Equals(actionName, n"FlightOptions_Left") && ListenerAction.IsButtonJustPressed(action) {
            this.showUI = !this.showUI;
            if (this.showUI) {
              this.ui.Show();
              this.ShowSimpleMessage("Flight UI Shown");
            } else {
              this.ui.Hide();
              this.ShowSimpleMessage("Flight UI Hidden");
            }
            GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_menu_onpress");
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
          case n"SurgePos":
            this.surge.SetInput(value);
            break;
          case n"TurnX":
            this.yaw.SetInput(value);
            break;
          case n"LeanFB":
            this.lift.SetInput(value);
            break;
          case n"SurgeNeg":
            this.surge.SetInput(-value);
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
      this.surge.SetInput(0.0);
      this.yaw.SetInput(0.0);
      this.pitch.SetInput(0.0);
      this.roll.SetInput(0.0);
      this.brake.SetInput(0.0);
    }
  }

  public func UpdateInputs(timeDelta: Float) -> Void {
    this.yaw.GetValue(timeDelta);
    this.roll.GetValue(timeDelta);
    this.pitch.GetValue(timeDelta);
    this.lift.GetValue(timeDelta);
    this.brake.GetValue(timeDelta);
    this.surge.GetValue(timeDelta);
  }

  public func UpdateAudioParams(timeDelta: Float) -> Void {
    let engineVolume = 0.5;
    let windVolume = 0.5;
    // let engineVolume = (GameInstance.GetSettingsSystem(this.gameInstance).GetVar(n"/audio/volume", n"MasterVolume") as ConfigVarListInt).GetValue();
    // let engineVolume *= (GameInstance.GetSettingsSystem(this.gameInstance).GetVar(n"/audio/volume", n"SfxVolume") as ConfigVarListInt).GetValue();
    if GameInstance.GetTimeSystem(this.gameInstance).IsPausedState() ||
      GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"HubMenu") || 
      GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"WorldMap") ||
      this.isInAnyMenu ||
      !this.audioEnabled {
      engineVolume = 0.0;
      windVolume = 0.0;
      this.audio.Update("leftFront", Vector4.EmptyVector(), engineVolume);
      this.audio.Update("rightFront", Vector4.EmptyVector(), engineVolume);
      this.audio.Update("leftRear", Vector4.EmptyVector(), engineVolume);
      this.audio.Update("rightRear", Vector4.EmptyVector(), engineVolume);
      this.audio.Update("windLeft", Vector4.EmptyVector(), windVolume);
      this.audio.Update("windRight", Vector4.EmptyVector(), windVolume);
      return;
    }

    // might need to handle just the scanning system's dilation, and the pause menu
    if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"radialMenu") {
      engineVolume *= 0.1;
      windVolume *= 0.1;
    }

    this.audio.UpdateSlotProviders();

    let leftFrontPosition = this.audio.GetPosition(n"wheel_front_left") - (this.stats.d_velocity * timeDelta);
    let rightFrontPosition = this.audio.GetPosition(n"wheel_front_right") - (this.stats.d_velocity * timeDelta);
    let leftRearPosition = this.audio.GetPosition(n"wheel_back_left") - (this.stats.d_velocity * timeDelta);
    let rightRearPosition = this.audio.GetPosition(n"wheel_back_right") - (this.stats.d_velocity * timeDelta);

    let windLeftPosition = this.audio.GetPosition(n"window_front_left_a") - (this.stats.d_velocity * timeDelta);
    let windRightPosition = this.audio.GetPosition(n"window_front_right_a") - (this.stats.d_velocity * timeDelta);

    let listener = this.player.FindComponentByName(n"soundListener") as IPlacedComponent;
    this.audio.listenerPosition = Matrix.GetTranslation(listener.GetLocalToWorld());
    this.audio.listenerForward = Matrix.GetDirectionVector(listener.GetLocalToWorld());
    this.audio.listenerUp = Matrix.GetAxisZ(listener.GetLocalToWorld());

    this.audio.speed = this.stats.d_speed;
    this.audio.yawDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_up);
    this.audio.pitchDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_right);


    let ratio = 1.0;
    if this.collisionTimer < this.collisionRecoveryDelay + this.collisionRecoveryDuration {
      ratio = MaxF(0.0, (this.collisionTimer - this.collisionRecoveryDelay) / this.collisionRecoveryDuration);
    }
    
    this.audio.damage = 1.0 - MaxF(GameInstance.GetStatPoolsSystem(this.gameInstance).GetStatPoolValue(Cast<StatsObjectID>(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, false) + ratio, 1.0);

    this.audio.surge = this.surge.GetValue() * ratio;
    if Equals(this.mode, FlightMode.Drone) {
      this.audio.surge *= 0.5;
      this.audio.surge += this.lift.GetValue() * ratio * 0.5;
    }
    this.audio.yaw = this.yaw.GetValue() * ratio;
    this.audio.lift = this.lift.GetValue() * ratio;
    this.audio.brake = this.brake.GetValue();
    this.audio.inside = this.isTPP ? MaxF(0.0, this.audio.inside - timeDelta * 4.0) : MinF(1.0, this.audio.inside + timeDelta * 4.0);
    // engineVolume *= (ratio * 0.5 + 0.5);

    this.audio.Update("leftFront", leftFrontPosition, engineVolume);
    this.audio.Update("rightFront", rightFrontPosition, engineVolume);
    this.audio.Update("leftRear", leftRearPosition, engineVolume);
    this.audio.Update("rightRear", rightRearPosition, engineVolume);
    this.audio.Update("windLeft", windLeftPosition, windVolume);
    this.audio.Update("windRight", windRightPosition, windVolume);
  }

  public func SetupTires() -> Void {
    if this.GetVehicle() == (this.GetVehicle() as CarObject) {
      // this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_left_tire") as IPlacedComponent;
      // this.fr_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_right_tire") as IPlacedComponent;
      // this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_left_tire") as IPlacedComponent;
      // this.br_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_right_tire") as IPlacedComponent;
      this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFL") as IPlacedComponent;
      this.fr_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFR") as IPlacedComponent;
      this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBL") as IPlacedComponent;
      this.br_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBR") as IPlacedComponent;
    } else {
      this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFront") as IPlacedComponent;
      this.fr_tire = this.fl_tire;
      this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBack") as IPlacedComponent;
      this.br_tire = this.bl_tire;
    }
  }

  public func SetupPositionProviders() -> Void {
    this.audio.AddSlotProviders(this.GetVehicle());
  }

  public func FindGround(timeDelta: Float) -> Bool {
    let foundGround = true;


      // let lookAhead = this.stats.d_velocity * timeDelta * this.lookAheadMax;
      // let fl_tire: Vector4 = Matrix.GetTranslation(this.fl_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
      // let fr_tire: Vector4 = Matrix.GetTranslation(this.fr_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
      // let bl_tire: Vector4 = Matrix.GetTranslation(this.bl_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
      // let br_tire: Vector4 = Matrix.GetTranslation(this.br_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
      let fl_tire: Vector4 = Matrix.GetTranslation(this.fl_tire.GetLocalToWorld());
      let fr_tire: Vector4 = Matrix.GetTranslation(this.fr_tire.GetLocalToWorld());
      let bl_tire: Vector4 = Matrix.GetTranslation(this.bl_tire.GetLocalToWorld());
      let br_tire: Vector4 = Matrix.GetTranslation(this.br_tire.GetLocalToWorld());


      let findGround1: TraceResult; 
      let findGround2: TraceResult; 
      let findGround3: TraceResult; 
      let findGround4: TraceResult;

      // all in engine\physics\collision_presets.json
      // VehicleBlocker? RagdollVehicle?
      this.sqs.SyncRaycastByCollisionGroup(fl_tire, fl_tire + this.lookDown, n"VehicleBlocker", findGround1, false, false);
      this.sqs.SyncRaycastByCollisionGroup(fr_tire, fr_tire + this.lookDown, n"VehicleBlocker", findGround2, false, false);
      this.sqs.SyncRaycastByCollisionGroup(bl_tire, bl_tire + this.lookDown, n"VehicleBlocker", findGround3, false, false);
      this.sqs.SyncRaycastByCollisionGroup(br_tire, br_tire + this.lookDown, n"VehicleBlocker", findGround4, false, false);
      
      let groundPoint1: Vector4;
      let groundPoint2: Vector4;
      let groundPoint3: Vector4;
      let groundPoint4: Vector4;

      if TraceResult.IsValid(findGround1) {
        groundPoint1 = Vector4.Vector3To4(findGround1.position) - this.stats.d_velocity * timeDelta;
        if this.showUI {
          this.ui.DrawMark(groundPoint1);
          this.ui.DrawText(groundPoint1, FloatToStringPrec(Vector4.Distance(fl_tire, Cast(findGround1.position)), 2));
        }
      }
      if TraceResult.IsValid(findGround2) {
        groundPoint2 = Vector4.Vector3To4(findGround2.position) - this.stats.d_velocity * timeDelta;
        if this.showUI {
          this.ui.DrawMark(groundPoint2);
          this.ui.DrawText(groundPoint2, FloatToStringPrec(Vector4.Distance(fr_tire, Cast(findGround2.position)), 2));
        }
      }
      if TraceResult.IsValid(findGround3) {
        groundPoint3 = Vector4.Vector3To4(findGround3.position) - this.stats.d_velocity * timeDelta;
        if this.showUI {
          this.ui.DrawMark(groundPoint3);
          this.ui.DrawText(groundPoint3, FloatToStringPrec(Vector4.Distance(bl_tire, Cast(findGround3.position)), 2));
        }
      }
      if TraceResult.IsValid(findGround4) {
        groundPoint4 = Vector4.Vector3To4(findGround4.position) - this.stats.d_velocity * timeDelta;
        if this.showUI {
          this.ui.DrawMark(groundPoint4);
          this.ui.DrawText(groundPoint4, FloatToStringPrec(Vector4.Distance(br_tire, Cast(findGround4.position)), 2));
        }
      }

      if TraceResult.IsValid(findGround1) && TraceResult.IsValid(findGround2) && TraceResult.IsValid(findGround3) && TraceResult.IsValid(findGround4) {
        // let distance = MinF(
        //   MinF(Vector4.Distance(fl_tire, Cast(findGround1.position)),
        //   Vector4.Distance(fr_tire, Cast(findGround2.position))),
        //   MinF(Vector4.Distance(bl_tire, Cast(findGround3.position)),
        //   Vector4.Distance(br_tire, Cast(findGround4.position))));        
        let distance = (Vector4.Distance(fl_tire, Vector4.Vector3To4(findGround1.position)) +
          Vector4.Distance(fr_tire, Vector4.Vector3To4(findGround2.position)) +
          Vector4.Distance(bl_tire, Vector4.Vector3To4(findGround3.position)) +
          Vector4.Distance(br_tire, Vector4.Vector3To4(findGround4.position))) / 4.0;
        // this.distance = distance * (1.0 - this.distanceEase) + this.distance * (this.distanceEase);
        this.distance = distance;
        
        // FromVariant(scriptInterface.GetStateVectorParameter(physicsStateValue.Radius)) maybe?
        let normal = (Vector4.Normalize(Cast(findGround1.normal)) + Vector4.Normalize(Cast(findGround2.normal)) + Vector4.Normalize(Cast(findGround3.normal)) + Vector4.Normalize(Cast(findGround4.normal))) / 4.0;
        // this.normal = Vector4.Interpolate(this.normal, normal, this.normalEase);
        this.normal = Vector4.Normalize(normal);

      } else {
        foundGround = false;
      }   
    return foundGround;
  } 


  public static cb func PhysicsUpdate() {
    let fc = FlightController.GetInstance();
    if (fc.IsActive()) { 
      // fc.OnPhysicsUpdate(1.0/60.0);
          FlightLog.Info("in the physics update!");
    }
  }

  // protected cb func PhysicsUpdate(evt: ref<vehicleFlightPhysicsUpdateEvent>) -> Bool {
    // FlightLog.Info("in the physics update!");
  // }

  public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    // let timeDelta = evt.timeDelta;

    // this.camera.isInAir = false;
    // this.GetVehicle().isOnGround = true;
    this.ui.camera.drivingDirectionCompensationAngleSmooth = 120.0;
    this.ui.camera.drivingDirectionCompensationSpeedCoef = 0.1;


    // let cameraPos = this.ui.camera.GetLocalToWorld() * Vector4.EmptyVector();
    // let localCameraPos = Matrix.GetInverted(this.GetVehicle().chassis.GetLocalToWorld()) * cameraPos;
    // let idealCameraPos = new Vector4(0.0, -6.0, 1.2, 0.0);
    // let yaw = Vector4.GetAngleDegAroundAxis(localCameraPos, idealCameraPos, new Vector4(0.0, 0.0, 1.0, 0.0));
    // let pitch = Vector4.GetAngleDegAroundAxis(localCameraPos, idealCameraPos, new Vector4(1.0, 0.0, 0.0, 0.0));

    // if !IsDefined(this.uiBlackboard) {
    //   this.uiBlackboard = GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_System);
    //   this.uiSystemBB = GetAllBlackboardDefs().UI_System;
    //   this.trackedMappinId = this.uiBlackboard.RegisterListenerVariant(this.uiSystemBB.TrackedMappin, this, n"OnTrackedMappinUpdated");
    //   this.uiBlackboard.SignalVariant(this.uiSystemBB.TrackedMappin);
    // }

    this.isInAnyMenu = this.uiBlackboard.GetBool(this.uiSystemBB.IsInMenu);
    
    if !this.active {
      this.stats.UpdateDynamic(timeDelta);
      this.UpdateAudioParams(timeDelta);
      if (this.showUI) { 
        this.ui.ClearMarks();
      }
      return;
    }
    // might need to handle just the scanning system's dilation, and the pause menu
    if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"radial") {
      // this might happpen?
      timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("radialMenu", "timeDilation");
      //FlightLog.Info("Radial menu dilation"); 
    } else {
      if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive() {
        // i think this is what this is called
        timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("focusModeTimeDilation", "timeDilation");
        //FlightLog.Info("Other time dilation"); 
      }
    }

    // let player: ref<PlayerPuppet> = GetPlayer(this.gameInstance);
    if !IsDefined(this.GetVehicle()) { 
      // if IsDefined(scriptInterface.owner as VehicleObject) {
      //   this.stats = FlightStats.Create(scriptInterface.owner as VehicleObject);
      //   FlightLog.Warn("Vehicle undefined. Redefined to " + this.GetVehicle().GetDisplayName()); 
      // } else {
        FlightLog.Error("Owner not defined"); 
        return;
      // }
    }
    if !this.GetVehicle().IsPlayerMounted() { 
      FlightLog.Error("Vehicle is not player mounted"); 
      return; 
    }


    this.stats.UpdateDynamic(timeDelta);
    if (this.showUI) { 
      this.ui.ClearMarks();
    }

    this.UpdateInputs(timeDelta);

    if Equals(this.mode, FlightMode.Drone) {

      this.ui.camera.yawDelta = 0.0;
      this.ui.camera.pitchDelta = 0.0;

      let dampFactor = -MaxF(this.brake.GetValue() * this.brakeFactor * this.stats.s_brakingFrictionFactor, this.airResistance * this.stats.s_airResistanceFactor);
      let velocityDamp: Vector4 = this.stats.d_velocity * dampFactor;

      let combinedForce: Vector4 = new Vector4(0.0, 0.0, 0.0, 0.0);
      let combinedTorque: Vector4 = new Vector4(0.0, 0.0, 0.0, 0.0);

      // lift
      combinedForce += this.stats.d_up * this.lift.GetValue() * this.liftFactorDrone * timeDelta;
      // surge
      combinedForce += this.stats.d_forward * this.surge.GetValue() * this.surgeFactor * timeDelta;
      // directional brake
      combinedForce += velocityDamp * timeDelta;
      // factor in mass
      combinedForce *= this.stats.s_mass;

      // pitch correction
      combinedTorque.X =  -(this.pitch.GetValue() * this.pitchFactorDrone) * timeDelta;
      // roll correction
      combinedTorque.Y = (this.roll.GetValue() * this.rollFactorDrone) * timeDelta;
      // yaw correction
      combinedTorque.Z = -(this.yaw.GetValue() * this.yawFactorDrone) * timeDelta;
      // rotational brake
      // combinedTorque = combinedTorque + (angularDamp * timeDelta);
      // factor in interia tensor
      combinedTorque *= this.GetVehicle().GetInertiaTensor();

      combinedTorque = this.stats.d_orientation * combinedTorque;
      // this.CreateImpulse(combinedForce);
      // this.CreateMoment(combinedTorque);

      if this.collisionTimer < this.collisionRecoveryDelay + this.collisionRecoveryDuration {
        let collisionDampener = MinF(MaxF(0.0, (this.collisionTimer - this.collisionRecoveryDelay) / this.collisionRecoveryDuration), 1.0);
        combinedTorque *= collisionDampener;
        combinedForce *= collisionDampener;
      }

      this.helper.force = this.helper.force + combinedForce;
      this.helper.torque = this.helper.torque + combinedTorque;

    } else {

      let direction = this.stats.d_direction;
      // if this.stats.d_speed < 1.0 {
      //   direction = Vector4.Normalize(Quaternion.GetForward(this.stats.d_lastOrientation));
      // }

      let hoverCorrection: Float = 0.0;
      let pitchCorrection: Float = 0.0;
      let rollCorrection: Float = 0.0;
      let yawCorrection: Float = 0.0;

      if Equals(this.mode, FlightMode.HoverFly) {
        this.hoverHeight += this.lift.GetValue() * timeDelta * this.liftFactor * (1.0 + this.stats.d_speedRatio * 2.0);
      }
      if this.hovering {
        this.hoverHeight = MaxF(this.minHoverHeight, this.hoverHeight);
      }

      let foundGround = true;
      let findWater: TraceResult;
      this.sqs.SyncRaycastByCollisionGroup(this.stats.d_position, this.stats.d_position - this.lookDown, n"Water", findWater, true, false);
      if TraceResult.IsValid(findWater) {
        // if we're under water, just go up
        hoverCorrection = 1.0;
      } else {
        foundGround = this.FindGround(timeDelta);
      }

      let heightDifference = 0.0;
      let idealNormal = new Vector4(0.0, 0.0, 1.0, 0.0);
      if Equals(this.mode, FlightMode.HoverFly) {
        if ((this.distance > this.maxHoverHeight && this.hovering) || (this.hovering && !foundGround)) {
          this.hovering = false;
          this.referenceZ = this.stats.d_position.Z;
          this.hoverHeight = 0.0;
        }
        if (this.distance <= this.maxHoverHeight && !this.hovering && foundGround) {
          this.hovering = true;
          this.hoverHeight = MaxF(this.distance, this.minHoverHeight);
        }
      // would be cool to fade between these instead of using a boolean
        if this.hovering {
          // close to ground, use as reference
          heightDifference = this.hoverHeight - this.distance;
          // idealNormal = this.normal;
          idealNormal = Vector4.Interpolate(this.normal, idealNormal, (this.distance - this.minHoverHeight) / (this.maxHoverHeight - this.minHoverHeight));
        } else {
          // use absolute Z if too high
          heightDifference = this.referenceZ + this.hoverHeight - this.stats.d_position.Z;
        }
      }
      if Equals(this.mode, FlightMode.Hover) {
        if (foundGround) {
          heightDifference = this.hoverHeight - this.distance;
          idealNormal = this.normal;
        }
      }

      // idealNormal += this.stats.d_right * this.yaw.GetValue() * this.rollWithYaw;

      idealNormal = Vector4.RotateAxis(idealNormal, this.stats.d_forward, this.yaw.GetValue() * this.rollWithYaw);
      idealNormal = Vector4.RotateAxis(idealNormal, this.stats.d_right, this.lift.GetValue() * this.pitchWithLift);
      // idealNormal = Vector4.RotateAxis(idealNormal, this.stats.d_right, this.surge.GetValue() * this.surgeOffset);

      this.pitchPID.SetRatio(this.stats.d_speedRatio * AbsF(Vector4.Dot(this.stats.d_direction, this.stats.d_forward)));
      this.rollPID.SetRatio(this.stats.d_speedRatio * AbsF(Vector4.Dot(this.stats.d_direction, this.stats.d_right)));

      hoverCorrection = this.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, this.hoverClamp) / this.hoverClamp;
      // pitchCorrection = this.pitchPID.GetCorrectionClamped(FlightUtils.IdentCurve(Vector4.Dot(idealNormal, this.stats.d_forward)) + this.lift.GetValue() * this.pitchWithLift, timeDelta, 10.0) + this.pitch.GetValue() / 10.0;
      // rollCorrection = this.rollPID.GetCorrectionClamped(FlightUtils.IdentCurve(Vector4.Dot(idealNormal, this.stats.d_right)), timeDelta, 10.0) + this.yaw.GetValue() * this.rollWithYaw + this.roll.GetValue() / 10.0;
      let pitchDegOff = 90.0 - AbsF(Vector4.GetAngleDegAroundAxis(idealNormal, this.stats.d_forward, this.stats.d_right));
      let rollDegOff = 90.0 - AbsF(Vector4.GetAngleDegAroundAxis(idealNormal, this.stats.d_right, this.stats.d_forward));
      if AbsF(pitchDegOff) < 80.0  {
        // pitchCorrection = this.pitchPID.GetCorrectionClamped(pitchDegOff / 90.0 + this.lift.GetValue() * this.pitchWithLift, timeDelta, 10.0) + this.pitch.GetValue() / 10.0;
        pitchCorrection = this.pitchPID.GetCorrectionClamped(pitchDegOff / 90.0, timeDelta, 10.0) + this.pitch.GetValue() / 10.0;
      }
      if AbsF(rollDegOff) < 80.0 {
        // rollCorrection = this.rollPID.GetCorrectionClamped(rollDegOff / 90.0 + this.yaw.GetValue() * this.rollWithYaw, timeDelta, 10.0) + this.roll.GetValue() / 10.0;
        rollCorrection = this.rollPID.GetCorrectionClamped(rollDegOff / 90.0, timeDelta, 10.0) + this.roll.GetValue() / 10.0;
      }
      // adjust with speed ratio 
      // pitchCorrection = pitchCorrection * (this.pitchCorrectionFactor + 1.0 * this.pitchCorrectionFactor * this.stats.d_speedRatio);
      // rollCorrection = rollCorrection * (this.rollCorrectionFactor + 1.0 * this.rollCorrectionFactor * this.stats.d_speedRatio);
      pitchCorrection *= this.pitchCorrectionFactor;
      rollCorrection *= this.rollCorrectionFactor;
      let changeAngle: Float = Vector4.GetAngleDegAroundAxis(Quaternion.GetForward(this.stats.d_lastOrientation), this.stats.d_forward, this.stats.d_up);
      if AbsF(pitchDegOff) < 30.0 && AbsF(rollDegOff) < 30.0 {
        let directionAngle: Float = Vector4.GetAngleDegAroundAxis(this.stats.d_direction, this.stats.d_forward, this.stats.d_up);
        this.yawPID.integralFloat *= (1.0 - AbsF(this.yaw.GetValue()));
        yawCorrection = this.yawPID.GetCorrectionClamped(directionAngle, timeDelta, 10.0) / 10.0;
      }
      yawCorrection += this.yawD * changeAngle / timeDelta;

      let velocityDamp: Vector4 = this.stats.d_velocity * -MaxF(this.brake.GetValue() * this.brakeFactor * this.stats.s_brakingFrictionFactor, this.airResistance * this.stats.s_airResistanceFactor);
      let angularDamp: Vector4 = this.stats.d_angularVelocity * -MaxF(this.brake.GetValue() * this.angularBrakeFactor * this.stats.s_brakingFrictionFactor, this.angularDampFactor);
      // so we don't get impulsed by the speed limit (100 m/s, i think)
      // if this.stats.d_speed > 90.0 {
      //   velocityDamp *= (1.0 + PowF((this.stats.d_speed - 90.0) / 10.0, 2.0) * 1000.0);
      // }

      // let yawDirectionality: Float = (this.stats.d_speedRatio + AbsF(this.yaw.GetValue()) * this.swayWithYaw) * this.yawDirectionalityFactor;
      let yawDirectionality: Float = this.stats.d_speedRatio * this.yawDirectionalityFactor;
      let liftForce: Float = hoverCorrection * this.hoverFactor * (9.81000042 * 1.4);
      // actual in-game mass (i think)
      // this.averageMass = this.averageMass * 0.99 + (liftForce / 9.8) * 0.01;
      // FlightLog.Info(ToString(this.averageMass) + " vs " + ToString(this.stats.s_mass));
      let surgeForce: Float = this.surge.GetValue() * this.surgeFactor;

      //this.CreateImpulse(this.stats.d_position, this.stats.d_right * Vector4.Dot(this.stats.d_forward - direction, this.stats.d_right) * yawDirectionality / 2.0 * timeDelta);

      let combinedForce: Vector4 = new Vector4(0.0, 0.0, 0.0, 0.0);
      let combinedTorque: Vector4 = new Vector4(0.0, 0.0, 0.0, 0.0); 

      let aeroFactor = Vector4.Dot(this.stats.d_forward, this.stats.d_direction);

      // yawDirectionality - redirect non-directional velocity to vehicle forward
      combinedForce += this.stats.d_forward * AbsF(Vector4.Dot(this.stats.d_forward - direction, this.stats.d_right)) * yawDirectionality * timeDelta * aeroFactor;
      combinedForce += -this.stats.d_direction * AbsF(Vector4.Dot(this.stats.d_forward - direction, this.stats.d_right)) * yawDirectionality * timeDelta * AbsF(aeroFactor);
      // lift
      // combinedForce += new Vector4(0.00, 0.00, liftForce + this.stats.d_speedRatio * liftForce, 0.00) * timeDelta;
      combinedForce += new Vector4(0.00, 0.00, liftForce, 0.00) * timeDelta;
      // surge
      combinedForce += this.stats.d_forward2D * surgeForce * timeDelta;
      // directional brake
      combinedForce += velocityDamp * timeDelta;
      // factor in mass
      combinedForce *= this.stats.s_mass;

      // pitch correction
      combinedTorque.X = -(pitchCorrection + angularDamp.X) * timeDelta;
      // roll correction
      combinedTorque.Y = (rollCorrection - angularDamp.Y) * timeDelta;
      // yaw correction
      combinedTorque.Z = -((yawCorrection * this.yawCorrectionFactor + this.yaw.GetValue() * this.yawFactor) + angularDamp.Z) * timeDelta;
      // rotational brake
      // combinedTorque = combinedTorque + (angularDamp * timeDelta);

      // if this.showOptions {
      //   this.stats.s_centerOfMass.position.X -= combinedTorque.Y * 0.1;
      //   this.stats.s_centerOfMass.position.Y -= combinedTorque.X * 0.1;
      // }

      // factor in interia tensor
      combinedTorque *= this.GetVehicle().GetInertiaTensor();

      combinedTorque = this.stats.d_orientation * combinedTorque;

      if this.collisionTimer < this.collisionRecoveryDelay + this.collisionRecoveryDuration {
        let collisionDampener = MinF(MaxF(0.0, (this.collisionTimer - this.collisionRecoveryDelay) / this.collisionRecoveryDuration), 1.0);
        combinedTorque *= collisionDampener;
        combinedForce *= collisionDampener;
      }

      this.helper.force = this.helper.force + combinedForce;
      this.helper.torque = this.helper.torque + combinedTorque;

      // this.CreateImpulse(combinedForce);
      // this.CreateMoment(combinedTorque);

    }

    this.UpdateAudioParams(timeDelta);
    
    if this.collisionTimer < this.collisionRecoveryDelay + this.collisionRecoveryDuration {
      this.collisionTimer += timeDelta;
    }

    if (this.showUI) {
      this.ui.Update(timeDelta);
      this.ui.DrawMark(this.stats.d_visualPosition);
      this.ui.DrawText(this.stats.d_visualPosition, FloatToStringPrec(1.0 / this.timeDelta, 4));
    }
    this.timeDelta = timeDelta * 0.001 + this.timeDelta * 0.999;
  }

  public func CreateImpulse(direction: Vector4, opt offset: Vector4) -> Void {
    let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
    impulseEvent.radius = 1.0;
    impulseEvent.worldPosition = Vector4.Vector4To3(this.stats.d_position + offset);
    if this.collisionTimer < this.collisionRecoveryDelay + this.collisionRecoveryDuration {
      direction *= MaxF(0.0, (this.collisionTimer - this.collisionRecoveryDelay) / this.collisionRecoveryDuration);
    }
    impulseEvent.worldImpulse = Vector4.Vector4To3(direction);
    this.GetVehicle().QueueEvent(impulseEvent);
  }

  public func CreateMoment(moment: Vector4) ->  Void {
    this.CreateImpulse(this.stats.d_forward * moment.X, this.stats.d_up);
    this.CreateImpulse(this.stats.d_forward * -moment.X, -this.stats.d_up);
    this.CreateImpulse(this.stats.d_up * moment.Y, -this.stats.d_right);
    this.CreateImpulse(this.stats.d_up * -moment.Y, this.stats.d_right);
    this.CreateImpulse(this.stats.d_right * moment.Z, this.stats.d_forward);
    this.CreateImpulse(this.stats.d_right * -moment.Z, -this.stats.d_forward);
  }

  public func ProcessImpact(impact: Float) {
    this.collisionTimer = this.collisionRecoveryDelay - impact;
    this.surge.Reset(this.surge.GetValue() * MaxF(0.0, 1.0 - impact * 5.0));
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
    flightControlStatus.SetFontFamily("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily");
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
    flightControlStatus.SetMargin(100, 1802, 0, 0);

    // Set widget size
    flightControlStatus.SetSize(220.0, 50.0);
    return flightControlStatus;
  }

}

@addMethod(AudioSystem)
public final func PlayFlightSound(sound: CName) -> Void {
  this.Play(sound);
}

@addMethod(AudioSystem)
public final func PlayFlightSoundFrom(sound: CName, object: ref<GameObject>) -> Void {
  let objectID: EntityID = object.GetEntityID();
  if !EntityID.IsDefined(objectID) {
    this.Play(sound, objectID);
  }
}

@addMethod(AudioSystem)
public final func StopFlightSoundFrom(sound: CName, object: ref<GameObject>) -> Void {
  let objectID: EntityID = object.GetEntityID();
  if !EntityID.IsDefined(objectID) {
    this.Stop(sound, objectID);
  }
}

@addField(PlayerPuppet)
public let flightController: ref<FlightController>; // Must be strong reference

@addMethod(PlayerPuppet)
public func GetFlightController() -> ref<FlightController> {
  return this.flightController;
}

@addField(AllBlackboardDefinitions)
public let flightController: wref<FlightController>; // Must be weak reference

// Option 2 -- Get the player instance as soon as it's ready
@wrapMethod(PlayerPuppet)
protected cb func OnGameAttached() -> Bool {
  wrappedMethod();
  if !this.IsReplacer() {
    FlightController.CreateInstance(this);
  }
}

// @wrapMethod(Ground)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return wrappedMethod(stateContext, scriptInterface) || FlightController.GetInstance().IsActive();
// }

// @wrapMethod(Air)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return wrappedMethod(stateContext, scriptInterface) && !FlightController.GetInstance().IsActive();
// }

public class FlightControllerBBDef extends BlackboardDefinition {

  public let IsActive: BlackboardID_Bool;
  public let ShouldShowUI: BlackboardID_Bool;

  public const func AutoCreateInSystem() -> Bool {
    return true;
  }
}

@addField(AllBlackboardDefinitions)
public let FlightControllerBB: ref<FlightControllerBBDef>;

// Hook into corresponding controller
@wrapMethod(hudCarController)
private final func Reset() -> Void {
  wrappedMethod();
  this.OnFlightActiveChanged(false);
}

@addField(hudCarController)
private let m_flightActiveBBConnectionId: ref<CallbackHandle>;

@addField(hudCarController)
private let m_flightControllerStatus: wref<inkText>;

@wrapMethod(hudCarController)
private final func RegisterToVehicle(register: Bool) -> Void {
  wrappedMethod(register);
  let flightControllerBlackboard: wref<IBlackboard>;
  let vehicle: ref<VehicleObject> = this.m_activeVehicle;
  if vehicle == null {
    return;
  };
  flightControllerBlackboard = FlightController.GetInstance().GetBlackboard();
  if IsDefined(flightControllerBlackboard) {
    if register {
      // GetRootWidget() returns root widget of base type inkWidget
      // GetRootCompoundWidget() returns root widget casted to inkCompoundWidget
      if !IsDefined(this.m_flightControllerStatus) {
        this.m_flightControllerStatus = FlightController.HUDStatusSetup(this.GetRootCompoundWidget());
      }
      this.m_flightActiveBBConnectionId = flightControllerBlackboard.RegisterListenerBool(GetAllBlackboardDefs().FlightControllerBB.IsActive, this, n"OnFlightActiveChanged");
    } else {
      flightControllerBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().FlightControllerBB.IsActive, this.m_flightActiveBBConnectionId);
    };
  };
}

@addMethod(hudCarController)
protected cb func OnFlightActiveChanged(active: Bool) -> Bool {
  if !IsDefined(this.m_flightControllerStatus) {
    this.m_flightControllerStatus = FlightController.HUDStatusSetup(this.GetRootCompoundWidget());
  }
  if active {
    this.m_flightControllerStatus.SetText("Flight Control Engaged");
  } else {
    this.m_flightControllerStatus.SetText("Flight Control Available");
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
//   if FlightController.GetInstance().IsActive() {
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
//   if toggle || (Equals(FlightController.GetInstance().GetVehicle(), this.GetVehicle()) && FlightController.GetInstance().GetThrusterState()) {
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
  if evt.isInWater  && !this.GetPS().GetIsSubmerged() {
    if !Equals(FlightController.GetInstance().GetVehicle(), this.GetVehicle()) && FlightController.GetInstance().IsActive() {
      this.BreakAllDamageStageFX(true);
      this.DestroyVehicle();
      this.DestroyRandomWindow();
      this.ApplyVehicleDOT(n"high");
    }
    GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"fire");
  }
  ScriptedPuppet.ReevaluateOxygenConsumption(this.m_mountedPlayer);
  if FlightController.GetInstance().IsActive() {
    let playerPuppet = GameInstance.GetPlayerSystem(this.GetVehicle().GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet;
    let playerStateMachineBlackboard = GameInstance.GetBlackboardSystem(this.GetVehicle().GetGame()).GetLocalInstanced(playerPuppet.GetEntityID(), GetAllBlackboardDefs().PlayerStateMachine);
    playerStateMachineBlackboard.SetInt(GetAllBlackboardDefs().PlayerStateMachine.Swimming, EnumInt(gamePSMSwimming.Surface), true);
  }
}

// @wrapMethod(VehicleObject)
// protected cb func OnLookedAtEvent(evt: ref<LookedAtEvent>) -> Bool {
//   wrappedMethod(evt);
//   if this.IsDestroyed() && this.IsCurrentlyScanned() {
//     let player: ref<PlayerPuppet> = GetPlayer(this.GetGame());
//     let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetGame());
//     if evt.isLookedAt {
//         player.RegisterInputListener(this.m_vehicleComponent, n"Choice1");
//         uiSystem.QueueEvent(FlightController.ShowHintHelper("Repair Vehicle", n"Choice1", n"RepairVehicle"));
//     } else {
//         player.UnregisterInputListener(this.m_vehicleComponent, n"Choice1");
//         uiSystem.QueueEvent(FlightController.HideHintFromSource(n"RepairVehicle"));
//     }
//   }
// } 
// @wrapMethod(VehicleComponent) 
// protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
//   wrappedMethod(action, consumer);
//   let actionName: CName = ListenerAction.GetName(action);
//   let value: Float = ListenerAction.GetValue(action);
//   if Equals(actionName, n"Choice1") && ListenerAction.IsButtonJustReleased(action) {
//     FlightLog.Info("Attempting to repair vehicle");
//     this.RepairVehicle();
//     let player: ref<PlayerPuppet> = GetPlayer(this.GetVehicle().GetGame());
//     let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetVehicle().GetGame());
//     player.UnregisterInputListener(this, n"Choice1");
//     uiSystem.QueueEvent(FlightController.HideHintFromSource(n"RepairVehicle"));
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

// @addField(VehicleObject)
// public let m_colliderComponent: ref<ColliderComponent>;

// @wrapMethod(VehicleObject)
// protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
//   wrappedMethod(ri);
//   EntityRequestComponentsInterface.RequestComponent(ri, n"Collider", n"entColliderComponent", false);
// }

// @wrapMethod(VehicleObject)
// protected cb func OnTakeControl(ri: EntityResolveComponentsInterface) -> Bool {
//   wrappedMethod(ri);
//   this.m_colliderComponent = EntityResolveComponentsInterface.GetComponent(ri, n"Collider") as ColliderComponent;
// }

// @addMethod(VehicleObject)
// public final const func GetColliderComponent() -> ref<ColliderComponent> {
//   return this.m_colliderComponent;
// }