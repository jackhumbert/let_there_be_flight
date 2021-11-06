import Codeware.UI.*

enum FlightMode {
  Undef = 0
} 

// public native class FlightStats_Record extends TweakDBRecord {
//   public native let mass: Float;
// }

// public class FlightSettingsListener extends ConfigVarListener {

//   private let m_ctrl: wref<FlightController>;

//   public final func RegisterController(ctrl: ref<FlightController>) -> Void {
//     this.m_ctrl = ctrl;
//   }

//   public func OnVarModified(groupPath: CName, varName: CName, varType: ConfigVarType, reason: ConfigChangeReason) -> Void {
//     this.m_ctrl.OnVarModified(groupPath, varName, varType, reason);
//   }
// }

// this might work better
// module MyMod

// public class MySystem extends ScriptableSystem {
//     private func OnAttach() -> Void {
//         FlightLog.Info("MySystem::OnAttach");
//     }

//     private func OnDetach() -> Void {
//         FlightLog.Info("MySystem::OnDetach");
//     }

//     public func GetData() -> Float {
//         return GetPlayer(this.GetGameInstance()).GetGunshotRange();
//     }
// }

// access
// let container: ref<ScriptableSystemsContainer> = GameInstance.GetScriptableSystemsContainer(this.GetGame());
// let system: ref<MySystem> = container.Get(n"MyMod.MySystem") as MySystem; // Don't forget the namespace if you're using modules

// FlightLog.Info(ToString(system.GetData()));

// maybe this should extend ScriptableComponent or GameComponent?
// Singleton instance with player lifetime
public class FlightController  {
  //public let camera: ref<vehicleTPPCameraComponent>;
  private let gameInstance: GameInstance;
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
  public let brake: ref<PID>;
  public let lift: ref<PID>;
  public let liftFactor: Float;
  public let surge: ref<PID>;
  public let surgeFactor: Float;
  public let roll: ref<PID>;
  public let pitch: ref<PID>;
  public let yaw: ref<PID>;
  public let yawFactor: Float;
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
  public let hoverFactor: Float;
  public let hover: ref<PID>;
  public let pitchPID: ref<PID>;
  public let pitchCorrectionFactor: Float;
  public let rollPID: ref<PID>;
  public let rollCorrectionFactor: Float;
  public let yawPID: ref<PID>;
  public let yawCorrectionFactor: Float;
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

  public let fl_tire: ref<IPlacedComponent>;
  public let fr_tire: ref<IPlacedComponent>;
  public let bl_tire: ref<IPlacedComponent>;
  public let br_tire: ref<IPlacedComponent>;

  // protected let m_settingsListener: ref<FlightSettingsListener>;
  // protected let m_groupPath: CName;

  private func Initialize(player: ref<PlayerPuppet>) {
    this.gameInstance = player.GetGame();
    this.enabled = false;
    this.active = false;
    this.showOptions = false;
    this.brake = PID.Create(0.05, 0.0, 0.0, 0.0);
    this.lift = PID.Create(0.05, 0.0, 0.0, 0.0);
    this.liftFactor = 10.0;
    this.surge = PID.Create(0.04, 0.0, 0.0, 0.0);
    this.surgeFactor = 15.0;
    this.roll = PID.Create(0.5, 0.0, 0.0, 0.0);
    this.pitch = PID.Create(0.5, 0.0, 0.0, 0.0);
    this.yaw = PID.Create(0.01, 0.0, 0.0, 0.0);
    this.yawFactor = 40.0;
    this.yawDirectionalityFactor = 20.0;
    this.distance = 0.0;
    this.distanceEase = 0.1;
    this.normal = new Vector4(0.0, 0.0, 1.0, 0.0);
    this.normalEase = 0.3;
    this.airResistance = 0.01;
    this.defaultHoverHeight = 3.50;
    this.hoverHeight = this.defaultHoverHeight;
    this.maxHoverHeight = 7.0;
    this.hoverFactor = 5.0;
    this.hover = PID.Create(0.1, 0.01, 0.05);
    this.pitchPID = PID.Create(0.5, 0.05, 0.1);
    this.pitchCorrectionFactor = 10.0;
    this.rollPID = PID.Create(0.5, 0.05, 0.1);
    this.rollCorrectionFactor = 10.0;
    this.yawPID = PID.Create(0.5, 0.2, 2.0);
    this.yawCorrectionFactor = 0.1;
    this.brakeFactor = 1.2;
    this.lookAheadMax = 10.0;
    // this.lookAheadMin = 1.0;
    this.lookDown = new Vector4(0.0, 0.0, -this.maxHoverHeight - 10.0, 0.0);
    this.fwtfCorrection = 0.0;
    this.pitchWithLift = -0.3;
    // this.pitchWithLift = 0.0;
    this.rollWithYaw = 0.15;
    this.swayWithYaw =   0.5;
    // this.surgeOffset = 0.5;
    this.surgeOffset = 0.0;
    // this.brakeOffset = 0.5;
    this.brakeOffset = 0.0;
    // this.velocityPointing = 0.5;
    this.velocityPointing = 0.0;
    this.hovering = true;
    this.referenceZ = 0.0;

    this.audio = FlightAudio.Create();  

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
    FlightLog.Info("Flight Control Loaded");
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
  
  public func Enable(vehicle: ref<VehicleObject>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.enabled = true;
    this.active = false;
    this.stats = FlightStats.Create(vehicle);
    // this.GetVehicle().TurnOn(true);
    // this.GetVehicle().TurnEngineOn(true);
    this.SetupActions();
    this.ui.Setup(this.stats);

    // very intrusive - need a prompt/confirmation that they want this popup, eg Detailed Info / About
    // let shardUIevent = new NotifyShardRead();
    // shardUIevent.title = "Flight Control: Now Available";
    // shardUIevent.text = "Your new car is equiped with the state-of-the-art Flight Control!";
    // GameInstance.GetUISystem(this.gameInstance).QueueEvent(shardUIevent);

    FlightLog.Info("Flight Control Enabled for " + this.GetVehicle().GetDisplayName());
  }

  public func Disable(scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if this.active {
      this.Deactivate(true);
    }
    this.enabled = false;
    this.SetupActions();   
    this.stats = null;

    FlightLog.Info("Flight Control Disabled");
  }

  public func Toggle() -> Bool {
    if this.active {
      this.Deactivate(false);
    } else {
      this.Activate();
    }
    this.GetBlackboard().SetBool(GetAllBlackboardDefs().FlightControllerBB.IsActive, this.active, true);
    this.GetBlackboard().SignalBool(GetAllBlackboardDefs().FlightControllerBB.IsActive);
    return this.active;
  }
  
  private func Activate() -> Void {
    this.active = true;
    this.SetupActions();
    this.hover.Reset();
    this.pitchPID.Reset();
    this.rollPID.Reset();
    this.yawPID.Reset();

    this.pitch.Reset();
    this.roll.Reset();
    this.yaw.Reset();
    this.surge.Reset();
    this.lift.Reset();
    this.brake.Reset();

    this.SetupTires();

    //this.camera = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"Collider") as vehicleTPPCameraComponent;

    // (this.GetVehicle().GetPS() as VehicleComponentPS).SetThrusterState(false);
    // (this.GetVehicle().GetPS() as VehicleComponentPS).SetIsDestroyed(true);

    // these stop engine noises too?
    this.GetVehicle().TurnEngineOn(false);
    // this.GetVehicle().TurnOn(true);

    this.audio.Start();

    // this disables engines noises from starting, but also prevents wheels from moving
    // something that only stops engine noises would be preferred, or this could be toggled
    // when close to the ground, to make transitions easier
    this.GetVehicle().GetVehicleComponent().GetVehicleControllerPS().SetState(vehicleEState.Disabled);
    FlightLog.Info(ToString(TweakDBInterface.GetFlightRecord(this.GetVehicle().GetRecordID()).mass));

    this.stats.Reset();
    this.ui.Show();
  
    this.ShowSimpleMessage("Flight Control Engaged");

    // stateContext.SetPermanentCNameParameter(n"VehicleCameraParams", n"", true); 
    // this.driveEvents.UpdateCameraContext(stateContext, scriptInterface);
    // let param: StateResultCName = stateContext.GetPermanentCNameParameter(n"LocomotionCameraParams");
    // if param.valid {
    //     this.driveEvents.UpdateCameraParams(param.value, scriptInterface);
    // };
   // GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_hacking_access_granted");
    // GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"ignition", true);
    // GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(StringToName(this.GetVehicle().GetRecord().Player_audio_resource()));
    // GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"mus_cp_arcade_quadra_START_menu");
    FlightLog.Info("Flight Control Activated");
  }

  private func Deactivate(silent: Bool) -> Void {
    this.active = false;
    this.SetupActions();

    // (this.GetVehicle().GetPS() as VehicleComponentPS).SetThrusterState(false);

    this.GetVehicle().GetVehicleComponent().GetVehicleControllerPS().SetState(vehicleEState.On);

    this.audio.Stop();

    // StatusEffectHelper.RemoveStatusEffect(GetPlayer(this.gameInstance), t"GameplayRestriction.NoCameraControl");
    if !silent {
      this.GetVehicle().TurnOn(true);
      this.GetVehicle().TurnEngineOn(true);
      this.ShowSimpleMessage("Flight Control Disengaged");
      //GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_hacking_access_denied");
    }
    this.ui.Hide();

    FlightLog.Info("Flight Control Deactivated");
  }

  private func SetupActions() -> Bool {
    let player: ref<PlayerPuppet> = GetPlayer(this.gameInstance);
    let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    player.UnregisterInputListener(this);    
    uiSystem.QueueEvent(FlightController.HideHintFromSource(n"FlightController"));
    if this.enabled {
      player.RegisterInputListener(this, n"Flight_Toggle");
      if this.active {
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Disable Flight Control", n"Flight_Toggle", n"FlightController"));
        // player.RegisterInputListener(this, n"Pitch");
        // uiSystem.QueueEvent(FlightController.ShowHintHelper("Pitch", n"Pitch", n"FlightController"));
        // player.RegisterInputListener(this, n"Roll");
        // uiSystem.QueueEvent(FlightController.ShowHintHelper("Roll", n"Roll", n"FlightController"));
        player.RegisterInputListener(this, n"Accelerate");
        player.RegisterInputListener(this, n"LeanFB");
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Lift", n"LeanFB", n"FlightController"));
        player.RegisterInputListener(this, n"TurnX");
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Yaw", n"TurnX", n"FlightController"));
        player.RegisterInputListener(this, n"Decelerate");
        // we may want to look at something else besides this input so ForceBrakesUntilStoppedOrFor will work (not entirely sure it doesn't now)
        // vehicle.GetBlackboard().GetInt(GetAllBlackboardDefs().Vehicle.IsHandbraking) is the value (why int? no enums for it seem to exist)
        player.RegisterInputListener(this, n"Handbrake");
        player.RegisterInputListener(this, n"Choice1_DualState");
        player.RegisterInputListener(this, n"FlightOptions_Up");
        player.RegisterInputListener(this, n"FlightOptions_Down");
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Flight Options", n"Choice1_DualState", n"FlightController"));
      } else {
        uiSystem.QueueEvent(FlightController.ShowHintHelper("Enable Flight Control", n"Flight_Toggle", n"FlightController"));
      }
    }
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    let actionType: gameinputActionType = ListenerAction.GetType(action);
    let actionName: CName = ListenerAction.GetName(action);
    let value: Float = ListenerAction.GetValue(action);
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
          this.ui.ShowInfo();
        }
        if ListenerAction.IsButtonJustReleased(action) {
          FlightLog.Info("Options button released");
          this.showOptions = false;
        }
      }
      if this.showOptions {
        if Equals(actionName, n"FlightOptions_Up") && ListenerAction.IsButtonJustPressed(action) {
            this.hoverHeight += 0.1;
            GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_menu_onpress");
            FlightLog.Info("hoverHeight = " + ToString(this.hoverHeight));
        }
        if Equals(actionName, n"FlightOptions_Down") && ListenerAction.IsButtonJustPressed(action) {
            this.hoverHeight -= 0.1;
            GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_menu_onpress");
            FlightLog.Info("hoverHeight = " + ToString(this.hoverHeight));
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
            this.surge.SetInput(value);
            break;
          case n"TurnX":
            this.yaw.SetInput(value);
            break;
          case n"LeanFB":
            this.lift.SetInput(value);
            break;
          case n"Decelerate":
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

  public func UpdateAudioParams() -> Void {
    this.audio.volume = 1.0;
    // this.audio.volume = (GameInstance.GetSettingsSystem(this.gameInstance).GetVar(n"/audio/volume", n"MasterVolume") as ConfigVarListInt).GetValue();
    // this.audio.volume *= (GameInstance.GetSettingsSystem(this.gameInstance).GetVar(n"/audio/volume", n"SfxVolume") as ConfigVarListInt).GetValue();

    if GameInstance.GetTimeSystem(this.gameInstance).IsPausedState() {
      this.audio.volume = 0.0;
      return;
    }
    // might need to handle just the scanning system's dilation, and the pause menu
    if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"radial") {
      this.audio.volume *= 0.1;
    } else {
      if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive() {
        this.audio.volume *= 0.1;
      }
    }
    this.audio.playerPosition = this.stats.d_position;
    this.audio.playerUp = this.stats.d_up;
    this.audio.playerForward = this.stats.d_forward;

    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(this.gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);

    this.audio.cameraPosition = cameraTransform.position;
    this.audio.cameraUp = Transform.GetUp(cameraTransform);
    this.audio.cameraForward = Transform.GetForward(cameraTransform);

    this.audio.speed = this.stats.d_speed;
    this.audio.yawDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_up);
    this.audio.pitchDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_right);
    
    this.audio.surge = this.surge.GetValue();
    this.audio.yaw = this.yaw.GetValue();
    this.audio.lift = this.lift.GetValue();
    this.audio.brake = this.brake.GetValue();

    this.audio.Update();
  }

  public func SetupTires() -> Void {
    this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_left_tire") as IPlacedComponent;
    this.fr_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_right_tire") as IPlacedComponent;
    this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_left_tire") as IPlacedComponent;
    this.br_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_right_tire") as IPlacedComponent;
  }

  public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    if !this.active {
      return;
    }
    // might need to handle just the scanning system's dilation, and the pause menu
    if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"radial") {
      // this might happpen?
      timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("radialMenu", "timeDilation");
      FlightLog.Info("Radial menu dilation"); 
    } else {
      if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive() {
        // i think this is what this is called
        timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("focusModeTimeDilation", "timeDilation");
        FlightLog.Info("Other time dilation"); 
      }
    }

    let player: ref<PlayerPuppet> = GetPlayer(this.gameInstance);
    if !IsDefined(this.GetVehicle()) { 
      if IsDefined(scriptInterface.owner as VehicleObject) {
        this.stats = FlightStats.Create(scriptInterface.owner as VehicleObject);
        FlightLog.Warn("Vehicle undefined. Redefined to " + this.GetVehicle().GetDisplayName()); 
      } else {
        FlightLog.Error("Owner not defined"); 
        return;
      }
    }
    if !this.GetVehicle().IsPlayerMounted() { 
      FlightLog.Error("Vehicle is not player mounted"); 
      return; 
    }


    this.stats.UpdateDynamic(timeDelta);
    this.ui.ClearMarks();

    let direction = this.stats.d_direction;
    if this.stats.d_speed < 1.0 {
      direction = this.stats.d_forward;
    }

    let hoverCorrection: Float = 0.0;
    let pitchCorrection: Float = 0.0;
    let rollCorrection: Float = 0.0;
    let yawCorrection: Float = 0.0;

    this.UpdateInputs(timeDelta);

    this.hoverHeight += this.lift.GetValue() * timeDelta * this.liftFactor * (1.0 + this.stats.d_speedRatio * 2.0);
    if this.hovering {
      this.hoverHeight = MaxF(1.0, this.hoverHeight);
    }

    let foundGround = true;

    let queryFilter: QueryFilter;
    QueryFilter.AddGroup(queryFilter, n"Water");

    let findWater: TraceResult = scriptInterface.RayCastWithCollisionFilter(this.stats.d_position, this.stats.d_position - this.lookDown, queryFilter);
    if TraceResult.IsValid(findWater) {
      // if we're under water, just go up
      hoverCorrection = 1.0;
    } else {
      QueryFilter.AddGroup(queryFilter, n"Static");
      QueryFilter.AddGroup(queryFilter, n"Terrain");
      // this finds vehicle too - need to figure out how to exclude it
      // QueryFilter.AddGroup(queryFilter, n"PlayerBlocker"); 

      // let lookAhead = this.stats.d_velocity * timeDelta * this.lookAheadMax;
      // let fl_tire: Vector4 = Matrix.GetTranslation(this.fl_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
      // let fr_tire: Vector4 = Matrix.GetTranslation(this.fr_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
      // let bl_tire: Vector4 = Matrix.GetTranslation(this.bl_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
      // let br_tire: Vector4 = Matrix.GetTranslation(this.br_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
      let fl_tire: Vector4 = Matrix.GetTranslation(this.fl_tire.GetLocalToWorld());
      let fr_tire: Vector4 = Matrix.GetTranslation(this.fr_tire.GetLocalToWorld());
      let bl_tire: Vector4 = Matrix.GetTranslation(this.bl_tire.GetLocalToWorld());
      let br_tire: Vector4 = Matrix.GetTranslation(this.br_tire.GetLocalToWorld());

      let findGround1: TraceResult = scriptInterface.RayCastWithCollisionFilter(fl_tire, fl_tire + this.lookDown, queryFilter);
      let findGround2: TraceResult = scriptInterface.RayCastWithCollisionFilter(fr_tire, fr_tire + this.lookDown, queryFilter);
      let findGround3: TraceResult = scriptInterface.RayCastWithCollisionFilter(bl_tire, bl_tire + this.lookDown, queryFilter);
      let findGround4: TraceResult = scriptInterface.RayCastWithCollisionFilter(br_tire, br_tire + this.lookDown, queryFilter);
      if TraceResult.IsValid(findGround1) && TraceResult.IsValid(findGround2) && TraceResult.IsValid(findGround3) && TraceResult.IsValid(findGround4) {
        let distance = MinF(
          MinF(Vector4.Distance(fl_tire, Cast(findGround1.position)),
          Vector4.Distance(fr_tire, Cast(findGround2.position))),
          MinF(Vector4.Distance(bl_tire, Cast(findGround3.position)),
          Vector4.Distance(br_tire, Cast(findGround4.position))));
        // this.distance = distance * (1.0 - this.distanceEase) + this.distance * (this.distanceEase);
        this.distance = distance;

        this.ui.DrawMark(Cast(findGround1.position) - this.stats.d_velocity * timeDelta);
        this.ui.DrawMark(Cast(findGround2.position) - this.stats.d_velocity * timeDelta);
        this.ui.DrawMark(Cast(findGround3.position) - this.stats.d_velocity * timeDelta);
        this.ui.DrawMark(Cast(findGround4.position) - this.stats.d_velocity * timeDelta);

        // let points: array<Vector2>;
        // ArrayPush(points, this.ui.ScreenXY(Cast(findGround1.position), 1920.0, 1080.0));
        // ArrayPush(points, this.ui.ScreenXY(Cast(findGround2.position), 1920.0, 1080.0));
        // ArrayPush(points, this.ui.ScreenXY(Cast(findGround4.position), 1920.0, 1080.0));
        // ArrayPush(points, this.ui.ScreenXY(Cast(findGround3.position), 1920.0, 1080.0));

        // let quad = inkWidgetBuilder.inkShape(n"quad")
        //   .Reparent(this.ui.GetMarksWidget())
        //   //.ShapeName(n"hair_thin")
        //   .Size(1920.0 * 2.0, 1080.0 * 2.0)
        //   .Atlas(r"base\\gameplay\\gui\\widgets\\crosshair\\master_crosshair.inkatlas")
        //   .Part(n"headshot")
        //   .ShapeVariant(inkEShapeVariant.FillAndBorder)
        //   .LineThickness(5.0)
        //   .VertexList(points)
        //   .FillOpacity(0.1)
        //   .Tint(ThemeColors.ElectricBlue())
        //   .BorderColor(ThemeColors.ElectricBlue())
        //   .BorderOpacity(0.5)
        //   .Visible(true)
        //   .BuildShape();

        this.ui.DrawText(Cast(findGround1.position) - this.stats.d_velocity * timeDelta, FloatToStringPrec(Vector4.Distance(fl_tire, Cast(findGround1.position)), 2));
        this.ui.DrawText(Cast(findGround2.position) - this.stats.d_velocity * timeDelta, FloatToStringPrec(Vector4.Distance(fr_tire, Cast(findGround2.position)), 2));
        this.ui.DrawText(Cast(findGround3.position) - this.stats.d_velocity * timeDelta, FloatToStringPrec(Vector4.Distance(bl_tire, Cast(findGround3.position)), 2));
        this.ui.DrawText(Cast(findGround4.position) - this.stats.d_velocity * timeDelta, FloatToStringPrec(Vector4.Distance(br_tire, Cast(findGround4.position)), 2));

        // FromVariant(scriptInterface.GetStateVectorParameter(physicsStateValue.Radius)) maybe?
        let normal = (Vector4.Normalize(Cast(findGround1.normal)) + Vector4.Normalize(Cast(findGround2.normal)) + Vector4.Normalize(Cast(findGround3.normal)) + Vector4.Normalize(Cast(findGround4.normal))) / 4.0;
        // this.normal = Vector4.Interpolate(this.normal, normal, this.normalEase);
        this.normal = normal;

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
    pitchCorrection = this.pitchPID.GetCorrectionClamped(Vector4.Dot(idealNormal, this.stats.d_forward) + this.lift.GetValue() * this.pitchWithLift, timeDelta, 1.0) + this.pitch.GetValue() / 10.0;
    rollCorrection = this.rollPID.GetCorrectionClamped(Vector4.Dot(idealNormal, this.stats.d_right), timeDelta, 1.0) + this.yaw.GetValue() * this.rollWithYaw + this.roll.GetValue() / 10.0;
    // let angle: Float = Vector4.GetAngleDegAroundAxis(Vector4.Interpolate(this.stats.d_forward, direction, this.stats.d_speedRatio * this.velocityPointing), this.stats.d_forward, new Vector4(0.0, 0.0, 1.0, 0.0));
    let angle: Float = Vector4.GetAngleDegAroundAxis(direction, this.stats.d_forward, new Vector4(0.0, 0.0, 1.0, 0.0));

    // decay the integral if we have yaw input - this helps get rid of the windup effect
    this.yawPID.integralFloat *= (1.0 - AbsF(this.yaw.GetValue()));
    this.yawPID.integralFloat *= MinF(1.0, this.stats.d_speedRatio * 4.0);
    let angleTooHigh = MinF(1.0, 8.0 - AbsF(angle) / 180.0 * 8.0);
    yawCorrection = this.yawPID.GetCorrection(angle * angleTooHigh * this.stats.d_speedRatio + this.yaw.GetValue() * this.yawFactor * (1.0 + this.stats.d_speedRatio * 3.0), timeDelta);

    let velocityDamp: Vector4 = MaxF(this.brake.GetValue() * this.brakeFactor, this.airResistance) * this.stats.d_velocity * this.stats.s_mass;
    // so we don't get impulsed by the speed limit (100 m/s, i think)
    if this.stats.d_speed > 90.0 {
      velocityDamp *= (1 + PowF((this.stats.d_speed - 90.0) / 10.0, 2.0) * 1000.0);
    }

    let yawDirectionality: Float = (this.stats.d_speedRatio + AbsF(this.yaw.GetValue()) * this.swayWithYaw) * this.stats.s_mass * this.yawDirectionalityFactor;
    let liftForce: Float = hoverCorrection * this.stats.s_mass * this.hoverFactor * 9.8;
    // actual in-game mass (i think)
    // FlightLog.Info(ToString(hoverCorrection * this.stats.s_mass * this.hoverFactor) + " vs " + this.GetVehicle().GetTotalMass());
    let surgeForce: Float = this.surge.GetValue() * this.stats.s_mass * this.surgeFactor;

    // yawDirectionality
    this.CreateImpulse(this.stats.d_position, this.stats.d_right * Vector4.Dot(this.stats.d_forward - direction, this.stats.d_right) * yawDirectionality * timeDelta);
    this.CreateImpulse(this.stats.d_position, this.stats.d_forward * AbsF(Vector4.Dot(this.stats.d_forward - direction, this.stats.d_right)) * yawDirectionality * timeDelta);
    // lift
    this.CreateImpulse(this.stats.d_position, new Vector4(0.00, 0.00, liftForce, 0.00) * timeDelta);
    // surge
    this.CreateImpulse(this.stats.d_position + this.stats.d_forward * this.surgeOffset, this.stats.d_forward * surgeForce * timeDelta);
    // pitch correction
    this.CreateImpulse(this.stats.d_position - this.stats.d_up,       this.stats.d_forward *  this.stats.s_momentOfInertia.X * -pitchCorrection * this.pitchCorrectionFactor * timeDelta);
    this.CreateImpulse(this.stats.d_position + this.stats.d_up,       this.stats.d_forward *  this.stats.s_momentOfInertia.X * pitchCorrection *  this.pitchCorrectionFactor * timeDelta);
    // roll correction
    this.CreateImpulse(this.stats.d_position - this.stats.d_right,    this.stats.d_up *       this.stats.s_momentOfInertia.Y * rollCorrection *   this.rollCorrectionFactor * timeDelta);
    this.CreateImpulse(this.stats.d_position + this.stats.d_right,    this.stats.d_up *       this.stats.s_momentOfInertia.Y * -rollCorrection *  this.rollCorrectionFactor * timeDelta);
    // yaw correction
    this.CreateImpulse(this.stats.d_position + this.stats.d_forward,  this.stats.d_right *    this.stats.s_momentOfInertia.Z * yawCorrection *    this.yawCorrectionFactor * timeDelta);
    this.CreateImpulse(this.stats.d_position - this.stats.d_forward,  this.stats.d_right *    this.stats.s_momentOfInertia.Z * -yawCorrection *   this.yawCorrectionFactor * timeDelta);
    // brake
    this.CreateImpulse(this.stats.d_position + this.stats.d_forward * this.brake.GetValue() * this.brakeOffset, -velocityDamp * timeDelta);

    this.UpdateAudioParams();

    // (this.GetVehicle().GetPS() as VehicleComponentPS).SetThrusterState(this.surge.GetValue() > 0.99);
    
    this.ui.Update(timeDelta);

  }

  // a generalized method for torque might be nice too
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

@wrapMethod(DriveEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(stateContext, scriptInterface);
  let vehicle: ref<VehicleObject> = scriptInterface.owner as VehicleObject;  
  if vehicle.IsPlayerMounted() {
    FlightController.GetInstance().Enable(vehicle, scriptInterface);
  }
}

@wrapMethod(DriveEvents)
public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightController.GetInstance().Disable(scriptInterface);
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(DriveEvents)
public final func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  FlightController.GetInstance().Disable(scriptInterface);
  wrappedMethod(stateContext, scriptInterface);
}

@wrapMethod(DriveEvents)
public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(timeDelta, stateContext, scriptInterface);
  FlightController.GetInstance().OnUpdate(timeDelta, stateContext, scriptInterface);
}

// @wrapMethod(Ground)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return wrappedMethod(stateContext, scriptInterface) || FlightController.GetInstance().IsActive();
// }

// @wrapMethod(Air)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return wrappedMethod(stateContext, scriptInterface) && !FlightController.GetInstance().IsActive();
// }

// @replaceMethod(VehicleTransition)
// protected final func PauseStateMachines(stateContext: ref<StateContext>, executionOwner: ref<GameObject>) -> Void {
//     let upperBody: ref<PSMStopStateMachine> = new PSMStopStateMachine();
//     let equipmentRightHand: ref<PSMStopStateMachine> = new PSMStopStateMachine();
//     let equipmentLeftHand: ref<PSMStopStateMachine> = new PSMStopStateMachine();
//     let coverAction: ref<PSMStopStateMachine> = new PSMStopStateMachine();
//     let stamina: ref<PSMStopStateMachine> = new PSMStopStateMachine();
//     let aimAssistContext: ref<PSMStopStateMachine> = new PSMStopStateMachine();
//     let cameraContext: ref<PSMStopStateMachine> = new PSMStopStateMachine();
//     if stateContext.IsStateActive(n"UpperBody", n"forceEmptyHands") {
//         upperBody.stateMachineIdentifier.definitionName = n"UpperBody";
//         executionOwner.QueueEvent(upperBody);
//     };
//     equipmentRightHand.stateMachineIdentifier.referenceName = n"RightHand";
//     equipmentRightHand.stateMachineIdentifier.definitionName = n"Equipment";
//     executionOwner.QueueEvent(equipmentRightHand);
//     equipmentLeftHand.stateMachineIdentifier.referenceName = n"LeftHand";
//     equipmentLeftHand.stateMachineIdentifier.definitionName = n"Equipment";
//     executionOwner.QueueEvent(equipmentLeftHand);
//     coverAction.stateMachineIdentifier.definitionName = n"CoverAction";
//     executionOwner.QueueEvent(coverAction);
//     if DefaultTransition.GetBlackboardIntVariable(executionOwner, GetAllBlackboardDefs().PlayerStateMachine.Stamina) == EnumInt(gamePSMStamina.Rested) {
//         stamina.stateMachineIdentifier.definitionName = n"Stamina";
//         executionOwner.QueueEvent(stamina);
//     };
//     aimAssistContext.stateMachineIdentifier.definitionName = n"AimAssistContext";
//     executionOwner.QueueEvent(aimAssistContext);
//     cameraContext.stateMachineIdentifier.definitionName = n"CameraContext";
//     executionOwner.QueueEvent(cameraContext);
// }

// @replaceMethod(VehicleTransition)
// protected final func ResumeStateMachines(executionOwner: ref<GameObject>) -> Void {
//     let upperBody: ref<PSMStartStateMachine> = new PSMStartStateMachine();
//     let equipmentRightHand: ref<PSMStartStateMachine> = new PSMStartStateMachine();
//     let equipmentLeftHand: ref<PSMStartStateMachine> = new PSMStartStateMachine();
//     let coverAction: ref<PSMStartStateMachine> = new PSMStartStateMachine();
//     let stamina: ref<PSMStartStateMachine> = new PSMStartStateMachine();
//     let aimAssistContext: ref<PSMStartStateMachine> = new PSMStartStateMachine();
//     let locomotion: ref<PSMStartStateMachine> = new PSMStartStateMachine();
//     let cameraContext: ref<PSMStartStateMachine> = new PSMStartStateMachine();
//     upperBody.stateMachineIdentifier.definitionName = n"UpperBody";
//     executionOwner.QueueEvent(upperBody);
//     equipmentRightHand.stateMachineIdentifier.referenceName = n"RightHand";
//     equipmentRightHand.stateMachineIdentifier.definitionName = n"Equipment";
//     executionOwner.QueueEvent(equipmentRightHand);
//     equipmentLeftHand.stateMachineIdentifier.referenceName = n"LeftHand";
//     equipmentLeftHand.stateMachineIdentifier.definitionName = n"Equipment";
//     executionOwner.QueueEvent(equipmentLeftHand);
//     coverAction.stateMachineIdentifier.definitionName = n"CoverAction";
//     executionOwner.QueueEvent(coverAction);
//     stamina.stateMachineIdentifier.definitionName = n"Stamina";
//     executionOwner.QueueEvent(stamina);
//     aimAssistContext.stateMachineIdentifier.definitionName = n"AimAssistContext";
//     executionOwner.QueueEvent(aimAssistContext);
//     locomotion.stateMachineIdentifier.definitionName = n"Locomotion";
//     executionOwner.QueueEvent(locomotion);
//     cameraContext.stateMachineIdentifier.definitionName = n"CameraContext";
//     executionOwner.QueueEvent(cameraContext);
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
  if evt.isInWater  {
    this.m_submerged = true;
    if !Equals(FlightController.GetInstance().GetVehicle(), this.GetVehicle()) && FlightController.GetInstance().IsActive() {
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