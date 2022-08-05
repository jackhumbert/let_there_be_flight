// Let There Be Flight
// (C) 2022 Jack Humbert
// https://github.com/jackhumbert/let_there_be_flight
// This file was automatically generated on 2022-08-05 19:13:29.4242714

// FlightAudio.reds

public class FlightAudioUpdate {
  public let speed: Float;
  public let surge: Float;
  public let yawDiff: Float;
  public let lift: Float;
  public let yaw: Float;
  public let pitchDiff: Float;
  public let brake: Float;
  public let inside: Float;
  public let damage: Float;
  public let water: Float;
  public let roll: Float;
  public let pitch: Float;
}

public native class FlightAudio {
  // defined in red4ext part
  public native func Start(emitterName: String, eventName: String) -> Void;
  public native func StartWithPitch(emitterName: String, eventName: String, pitch: Float) -> Void;
  public native func Play(eventName: String) -> Void;
  public native func Stop(emitterName: String) -> Void;
  // public native func Update(emitterName: String, eventLocation: Vector3, eventForward: Vector3, eventUp: Vector3, volume: Float) -> Void;
  public native func Update(emitterName: String, eventLocation: Vector4, volume: Float, update: ref<FlightAudioUpdate>) -> Void;
  public static native func UpdateListener(position: Vector4, forward: Vector4, up: Vector4) -> Void;

  public let parameters: array<String>;

  private let m_positionProviders: ref<inkHashMap>;
  private let m_orientationProviders: ref<inkHashMap>;
  private let m_positions: ref<inkHashMap>;
  private let m_orientations: ref<inkHashMap>;
  private let slots: array<CName>;

  public static func Create() -> ref<FlightAudio> {
    let self = new FlightAudio();

    self.parameters = [
      "speed",
      "surge",
      "yawDiff",
      "lift",
      "yaw",
      "pitchDiff",
      "brake",
      "inside",
      "damage",
      "water",
      "roll",
      "pitch"
    ];

    self.m_positionProviders = new inkHashMap();
    self.m_positions = new inkHashMap();
    self.m_orientationProviders = new inkHashMap();
    self.m_orientations = new inkHashMap();
    self.slots = [
      // n"steering_wheel",
      // n"roofhatch",
      // n"glove_box",
      // n"Base",
      // n"dseat_a",
      // n"dseat_b",
      // n"bumper_front_a",
      // n"bumper_front_b",
      // n"mirror_front_left",
      // n"mirror_front_right",
      n"wheel_front",
      n"wheel_back",
      n"wheel_front_left",
      n"wheel_front_right",
      n"wheel_back_left",
      n"wheel_back_right",
      // n"bumper_back",
      n"window_front_left_a",
      n"window_front_right_a"
    ];
    return self;
  }

  public func AddSlotProvider(entity: ref<Entity>, slot: CName) {
    this.SetPositionProvider(slot, IPositionProvider.CreateSlotPositionProvider(entity, slot));
    // this.SetOrientationProvider(slot, IOrientationProvider.CreateEntityOrientationProvider(null, slot, entity));
  }

  public func AddSlotProviders(entity: ref<Entity>) {
    for slot in this.slots {
      this.SetPositionProvider(slot, IPositionProvider.CreateSlotPositionProvider(entity, slot));
      // this.SetOrientationProvider(slot, IOrientationProvider.CreateEntityOrientationProvider(null, slot, entity));
    }
  }
  
  public func UpdateSlotProvider(slot: CName) {
    this.UpdatePosition(slot);
    // this.UpdateOrientation(slot);
  }

  public func UpdateSlotProviders() {
    for slot in this.slots {
      this.UpdateSlotProvider(slot);
    }
  }

  public func DrawSlotPositions(ui: ref<FlightControllerUI>) {
    for slot in this.slots {
      let position = this.GetPosition(slot);
      ui.DrawMark(position);
      ui.DrawText(position, NameToString(slot));
    }
  }

	public func UpdatePosition(name: CName) -> Void {
		let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));
		if this.m_positionProviders.KeyExist(key) {
      let position = new Vector4Wrapper();
		  (this.m_positionProviders.Get(key) as IPositionProvider).CalculatePosition(position.vector);
      if this.m_positions.KeyExist(key) {
        this.m_positions.Set(key, position);
      } else {
        this.m_positions.Insert(key, position);
      }
    }
  }

	// public func UpdateOrientation(name: CName) -> Void {
	// 	let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));
	// 	if this.m_orientationProviders.KeyExist(key) {
  //     let orientation = new OrientationWrapper();
	// 	  (this.m_orientationProviders.Get(key) as EntityOrientationProvider).CalculateOrientation(orientation.quaternion);
  //     if this.m_orientations.KeyExist(key) {
  //       this.m_orientations.Set(key, orientation);
  //     } else {
  //       this.m_orientations.Insert(key, orientation);
  //     }
  //   }
	// }

	public func GetPosition(name: CName) -> Vector4 {
    let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));
		if this.m_positions.KeyExist(key) {
		  return (this.m_positions.Get(key) as Vector4Wrapper).vector;
    } else {
      return Vector4.EmptyVector();
    }
  }

	public func SetPositionProvider(name: CName, positionProvider: ref<IPositionProvider>) -> Void {
		let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));

		if this.m_positionProviders.KeyExist(key) {
			this.m_positionProviders.Set(key, positionProvider);
		} else {
			this.m_positionProviders.Insert(key, positionProvider);
		}
    this.UpdatePosition(name);
	}

	// public func SetOrientationProvider(name: CName, orientationProvider: ref<IOrientationProvider>) -> Void {
	// 	let key: Uint64 = TDBID.ToNumber(TDBID.Create(NameToString(name)));

	// 	if this.m_orientationProviders.KeyExist(key) {
	// 		this.m_orientationProviders.Set(key, orientationProvider);
	// 	} else {
	// 		this.m_orientationProviders.Insert(key, orientationProvider);
	// 	}
  //   this.UpdateOrientation(name);
	// }
}

public class Vector4Wrapper {
  public let vector: Vector4;
}

public class OrientationWrapper {
  public let quaternion: Quaternion;
}

// FlightComponent.reds

public class FlightComponent extends ScriptableDeviceComponent {
  private let sys: ref<FlightSystem>;
  public let fx: ref<FlightFx>;
  private let helper: ref<vehicleFlightHelper>;
  private let stats: ref<FlightStats>;

  public let m_interaction: ref<InteractionComponent>;
  public let m_healthStatPoolListener: ref<VehicleHealthStatPoolListener>;
  public let m_vehicleBlackboard: wref<IBlackboard>;
  public let m_vehicleTPPCallbackID: ref<CallbackHandle>;

  public let active: Bool;
  public let hasUpdate: Bool;
  public let isPlayerMounted: Bool;

  let hoverGroundPID: ref<PID>;
  // let hoverPID: ref<PID>;
  // let pitchGroundPID: ref<DualPID>;
  let pitchPID: ref<PID>;
  // let rollGroundPID: ref<DualPID>;
  let rollPID: ref<PID>;
  let aeroYawPID: ref<PID>;
  let pitchAeroPID: ref<PID>;

  private let sqs: ref<SpatialQueriesSystem>;

  public let bl_tire: ref<IPlacedComponent>;
  public let br_tire: ref<IPlacedComponent>;
  public let fl_tire: ref<IPlacedComponent>;
  public let fr_tire: ref<IPlacedComponent>;
  public let hood: ref<IPlacedComponent>;
  public let trunk: ref<IPlacedComponent>;
  
  public let collisionTimer: Float;
  
  public let distance: Float;
  public let hoverHeight: Float;

  private let modes: array<ref<FlightMode>>;
  public let mode: Int32;

  private let surge: Float;
  private let lift: Float;
  private let roll: Float;
  private let pitch: Float;
  private let yaw: Float;
  private let sway: Float;
  private let linearBrake: Float;
  private let angularBrake: Float;

  public let force: Vector4;
  public let torque: Vector4;

  // public let ui: wref<worlduiWidgetComponent>;
  // public let ui_info: wref<worlduiWidgetComponent>;

  private let uiBlackboard: wref<IBlackboard>;
  private let menuCallback: ref<CallbackHandle>;
  public let isInMenu: Bool;

  private let uiGameDataBlackboard: wref<IBlackboard>;
  private let popupCallback: ref<CallbackHandle>;
  public let isPopupShown: Bool;
  public let alarmIsPlaying: Bool;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Audio Settings")
  @runtimeProperty("ModSettings.displayName", "Engine Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let engineVolume: Float = 1.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Audio Settings")
  @runtimeProperty("ModSettings.displayName", "Wind Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let windVolume: Float = 0.6;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Audio Settings")
  @runtimeProperty("ModSettings.displayName", "Warning Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let warningVolume: Float = 0.5;

  protected final const func GetVehicle() -> wref<VehicleObject> {
    return this.GetEntity() as VehicleObject;
  }

  private final func OnGameAttach() -> Void {
    //FlightLog.Info("[FlightComponent] OnGameAttach: " + this.GetVehicle().GetDisplayName());
    this.m_interaction = this.FindComponentByName(n"interaction") as InteractionComponent;
    this.m_healthStatPoolListener = new VehicleHealthStatPoolListener();
    this.m_healthStatPoolListener.m_owner = this.GetVehicle();
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestRegisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    this.m_vehicleBlackboard = this.GetVehicle().GetBlackboard();
    // QuickhackModule.RequestRefreshQuickhackMenu(this.GetVehicle().GetGame(), this.GetVehicle().GetEntityID());

    // this.hoverGroundPID = PID.Create(1.0, 0.01, 0.1);
    this.hoverGroundPID = PID.Create(FlightSettings.GetVector3("hoverModePID"));
    // this.hoverPID = PID.Create(1.0, 0.01, 0.1);
    this.pitchPID = PID.Create(FlightSettings.GetVector3("inputPitchPID"));
    this.rollPID =  PID.Create(FlightSettings.GetVector3("inputRollPID"));
    this.aeroYawPID = PID.Create(FlightSettings.GetVector3("aeroYawPID"));
    this.pitchAeroPID = PID.Create(FlightSettings.GetVector3("aeroPitchPID"));

    this.sys = FlightSystem.GetInstance();
    this.sqs = GameInstance.GetSpatialQueriesSystem(this.GetVehicle().GetGame());
    this.fx = FlightFx.Create(this);
    
    // this.helper = this.GetVehicle().AddFlightHelper();
    // this.stats = FlightStats.Create(this.GetVehicle());

    this.collisionTimer = FlightSettings.GetFloat("collisionRecoveryDelay");
    this.distance = 0.0;
    this.hoverHeight = FlightSettings.GetFloat("defaultHoverHeight");
    
    let hoverFlyMode = FlightModeHoverFly.Create(this);
    if hoverFlyMode.enabled {
      ArrayPush(this.modes, hoverFlyMode);
    } else {
      hoverFlyMode.Deinitialize();
    }
    ArrayPush(this.modes, FlightModeHover.Create(this));
    ArrayPush(this.modes, FlightModeAutomatic.Create(this));
    ArrayPush(this.modes, FlightModeFly.Create(this));
    ArrayPush(this.modes, FlightModeDroneAntiGravity.Create(this));
    let droneMode = FlightModeDrone.Create(this);
    if droneMode.enabled {
      ArrayPush(this.modes, droneMode);
    } else {
      droneMode.Deinitialize();
    }

    this.audioUpdate = new FlightAudioUpdate();
    
  }

  private final func OnGameDetach() -> Void {
    //FlightLog.Info("[FlightComponent] OnGameDetach: " + this.GetVehicle().GetDisplayName());
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestUnregisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    this.UnregisterVehicleTPPBBListener();
    this.isDestroyed = true;
    this.hasExploded = true;
    if this.active {
      this.Deactivate(true);
    }
    this.hasUpdate = false;
    if IsDefined(this.uiBlackboard) && IsDefined(this.menuCallback) {
      this.uiBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_System.IsInMenu, this.menuCallback);
    }
    if IsDefined(this.uiGameDataBlackboard) && IsDefined(this.popupCallback) {
      this.uiGameDataBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown, this.popupCallback);
    }
    for mode in this.modes {
      mode.Deinitialize();
    }
  }

  protected cb func OnIsInMenu(inMenu: Bool) -> Bool {
    this.isInMenu = inMenu;
    this.UpdateAudioParams(1.0/60.0);
  }
  protected cb func OnPopupIsShown(isShown: Bool) -> Bool {
    this.isPopupShown = isShown;
    this.UpdateAudioParams(1.0/60.0);
  }
  
  // private final func RegisterInputListener() -> Void {
  //   let playerPuppet: ref<PlayerPuppet> = GameInstance.GetPlayerSystem(this.GetVehicle().GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet;
  //   playerPuppet.RegisterInputListener(this, n"VehicleInsideWheel");
  //   playerPuppet.RegisterInputListener(this, n"VehicleHorn");
  // }

  // private final func UnregisterInputListener() -> Void {
  //   let playerPuppet: ref<PlayerPuppet> = GameInstance.GetPlayerSystem(this.GetVehicle().GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet;
  //   if IsDefined(playerPuppet) {
  //     playerPuppet.UnregisterInputListener(this);
  //   };
  // }

  protected final func SetupVehicleTPPBBListener() -> Void {
    let activeVehicleUIBlackboard: wref<IBlackboard>;
    let bbSys: ref<BlackboardSystem>;
    if !IsDefined(this.m_vehicleTPPCallbackID) {
      bbSys = GameInstance.GetBlackboardSystem(this.GetVehicle().GetGame());
      activeVehicleUIBlackboard = bbSys.Get(GetAllBlackboardDefs().UI_ActiveVehicleData);
      this.m_vehicleTPPCallbackID = activeVehicleUIBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this, n"OnVehicleCameraChange");
    };
  }
  
  protected final func UnregisterVehicleTPPBBListener() -> Void {
    let activeVehicleUIBlackboard: wref<IBlackboard>;
    let bbSys: ref<BlackboardSystem>;
    if IsDefined(this.m_vehicleTPPCallbackID) {
      bbSys = GameInstance.GetBlackboardSystem(this.GetVehicle().GetGame());
      activeVehicleUIBlackboard = bbSys.Get(GetAllBlackboardDefs().UI_ActiveVehicleData);
      activeVehicleUIBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this.m_vehicleTPPCallbackID);
    };
  }

  private func GetPitch() -> Float{
    return ClampF(700.0 / this.stats.s_mass + 0.5, 0.25, 2.0);
  }

  public func GetFlightModeIndex() -> Int32 {
    return this.mode;
  }

  public func GetFlightMode() -> ref<FlightMode> {
    return this.modes[this.mode];
  }

  public func GetNextFlightMode(direction: Int32) -> ref<FlightMode> {
    let mode = this.mode + direction;
    if mode < 0 {
      mode += ArraySize(this.sys.playerComponent.modes);
    } 
    mode = mode % ArraySize(this.sys.playerComponent.modes);
    return this.modes[mode];
  }

  public func GetNextFlightModeDescription() -> String {
    if ArraySize(this.modes) > 0 {
      return this.GetNextFlightMode(1).GetDescription();
    } else {
      return "None";
    }
  }

  // callbacks

  // public let uiControl: ref<FlightControllerUI>;
  
  protected cb func OnMountingEvent(evt: ref<MountingEvent>) -> Bool {
    // this.helper = this.GetVehicle().AddFlightHelper();
    ModSettings.RegisterListenerToClass(this);
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if mountChild.IsPlayer() {
      // this.GetVehicle().TurnOffAirControl();
      this.SetupVehicleTPPBBListener();
      // FlightLog.Info("[FlightComponent] OnMountingEvent: " + this.GetVehicle().GetDisplayName());
      this.sys.audio.Start("windLeft", "wind_TPP");
      this.sys.audio.Start("windRight", "wind_TPP");
      // (this.GetVehicle().FindComponentByName(n"cars_sport_fx") as EffectSpawnerComponent).AddEffect();
      this.sys.playerComponent = this;
      this.isPlayerMounted = true;
      // this.uiControl = FlightControllerUI.Create(this.ui_info.GetGameController(), this.ui_info.GetGameController().GetRootCompoundWidget());
      // this.uiControl.Setup(this.stats);
      // ModSettings.RegisterListenerToClass(this);
    } else {
      // FlightLog.Info("[FlightComponent] OnMountingEvent for other vehicle: " + this.GetVehicle().GetDisplayName());
    }
  }
  
  protected cb func OnVehicleFinishedMountingEvent(evt: ref<VehicleFinishedMountingEvent>) -> Bool {
    // FlightLog.Info("[FlightComponent] OnVehicleFinishedMountingEvent: " + this.GetVehicle().GetDisplayName());
    if this.isPlayerMounted {
      this.sys.ctlr.Enable();
      if this.active {
        this.sys.ctlr.Activate(true);
        // this.sys.audio.Stop("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())));
        //this.sys.audio.Play("vehicle3_on");
        // this.sys.audio.StartWithPitch("playerVehicle", "vehicle3_TPP", this.GetPitch());
      }
    }
    let normal: Vector4;
    this.SetupTires();
    if this.isPlayerMounted && !this.FindGround(normal) || this.distance > FlightSettings.GetInstance().autoActivationHeight {
      this.Activate(true);
    }
  }

  protected cb func OnUnmountingEvent(evt: ref<UnmountingEvent>) -> Bool {
    ModSettings.UnregisterListenerToClass(this);
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if IsDefined(mountChild) && mountChild.IsPlayer() {
      // ModSettings.UnregisterListenerToClass(this);
      this.UnregisterVehicleTPPBBListener();
      this.sys.audio.Stop("windLeft");
      this.sys.audio.Stop("windRight");
      this.sys.playerComponent = null;
      this.isPlayerMounted = false;
      if this.active {
        this.sys.ctlr.Deactivate(true);
        // this.sys.audio.Stop("playerVehicle");
        // this.sys.audio.StartWithPitch("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())), "vehicle3_TPP", this.GetPitch());
      }
      this.sys.ctlr.Disable();
    }
  }

  // protected cb func OnVehicleHasExplodedEvent(evt: ref<VehicleHasExplodedEvent>) -> Bool {
  //   this.sys.audio.Stop("vehicleDestroyed" + this.GetUniqueID());
  //   this.Deactivate(true);
  // }

  protected cb func OnVehicleFlightModeChangeEvent(evt: ref<VehicleFlightModeChangeEvent>) -> Bool {
    this.modes[this.mode].Deactivate();
    this.mode = evt.mode;
    this.modes[this.mode].Activate();
  }

  protected cb func OnDeath(evt: ref<gameDeathEvent>) -> Bool {
    let vehicle: ref<VehicleObject> = this.GetVehicle();
    let gameInstance: GameInstance = vehicle.GetGame();
    let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
    if VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), vehicle) {
      FlightLog.Info("[FlightComponent] OnDeath: " + this.GetVehicle().GetDisplayName());
      if this.active {
        this.isDestroyed = true;
        this.hasExploded = true;
        this.Deactivate(true);
      }
      this.hasUpdate = false;
    }
  }

  protected cb func OnVehicleWaterEvent(evt: ref<VehicleWaterEvent>) -> Bool {
    if evt.isInWater  {
      this.audioUpdate.water = 1.0;
    } else {
      this.audioUpdate.water = 0.0;
    }
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    FlightLog.Info("[FlightComponent] OnAction: " + this.GetVehicle().GetDisplayName());
  }

  protected cb func OnVehicleFlightActivationEvent(evt: ref<VehicleFlightActivationEvent>) -> Bool {
    this.Activate();
  }

  public func Activate(opt silent: Bool) -> Void {
    // this.helper = this.GetVehicle().AddFlightHelper();
    FlightLog.Info("[FlightComponent] OnVehicleFlightActivationEvent: " + this.GetVehicle().GetDisplayName());
    this.GetVehicle().ScheduleAppearanceChange(this.GetVehicle().GetCurrentAppearanceName());
    if !this.active {

      this.uiBlackboard = GameInstance.GetBlackboardSystem(this.sys.ctlr.gameInstance).Get(GetAllBlackboardDefs().UI_System);
      if IsDefined(this.uiBlackboard) {
        if !IsDefined(this.menuCallback) {
          this.menuCallback = this.uiBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_System.IsInMenu, this, n"OnIsInMenu");
          this.isInMenu = this.uiBlackboard.GetBool(GetAllBlackboardDefs().UI_System.IsInMenu);
        }
      }

      this.uiGameDataBlackboard = GameInstance.GetBlackboardSystem(this.sys.ctlr.gameInstance).Get(GetAllBlackboardDefs().UIGameData);
      if IsDefined(this.uiGameDataBlackboard) {
        if !IsDefined(this.popupCallback) {
          this.popupCallback = this.uiGameDataBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown, this, n"OnPopupIsShown");
          this.isPopupShown = this.uiGameDataBlackboard.GetBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown);
        }
      }

      this.stats = FlightStats.Create(this.GetVehicle());
      this.sys.ctlr.ui.Setup(this.stats);

      this.SetupTires();
      this.fx.Start();
      // these stop engine noises if they were already playing?
      this.GetVehicle().TurnEngineOn(false);
      // this.GetVehicle().TurnOn(true);
      this.GetVehicle().GetVehicleComponent().GetVehicleControllerPS().SetLightMode(vehicleELightMode.HighBeams);
      this.GetVehicle().GetVehicleComponent().GetVehicleController().ToggleLights(true);

      this.hoverGroundPID.Reset();
      // this.hoverPID.Reset();
      // this.pitchGroundPID.Reset();
      this.pitchPID.Reset();
      // this.rollGroundPID.Reset();
      this.rollPID.Reset();
      this.aeroYawPID.Reset();
      this.pitchAeroPID.Reset();

      this.modes[this.mode].Activate();

      if this.isPlayerMounted {
        this.mode = this.sys.ctlr.mode;
        this.sys.ctlr.Activate(silent);
        this.sys.audio.Play("vehicle3_on");
        // this.sys.audio.StartWithPitch("playerVehicle", "vehicle3_TPP", this.GetPitch());
        // this.sys.audio.Start("leftFront", "vehicle3_TPP");
        // this.sys.audio.Start("rightFront", "vehicle3_TPP");
        // this.sys.audio.Start("leftRear", "vehicle3_TPP");
        // this.sys.audio.Start("rightRear", "vehicle3_TPP");
      }
      this.sys.audio.StartWithPitch("vehicle" + this.GetUniqueID(), "vehicle3_TPP", this.GetPitch());
      this.active = true;
      this.hasUpdate = true;
    }
  }

  protected cb func OnInteractionUsed(evt: ref<InteractionChoiceEvent>) -> Bool {
    FlightLog.Info("[FlightComponent] OnInteractionUsed: " + ToString(evt.actionType));
  }

  public let trick: ref<FlightTrick>;

  let smoothForce: Vector4;
  let smoothTorque: Vector4;
  let isDestroyed: Bool;
  let hasExploded: Bool;

  protected func OnUpdate(timeDelta: Float) -> Void {
    if this.GetVehicle().IsDestroyed() {
      if !this.isDestroyed {
        this.sys.audio.StartWithPitch("vehicleDestroyed" + this.GetUniqueID(), "vehicle3_destroyed", 1.0);
        this.alarmIsPlaying = true;
        this.isDestroyed = true;
      }
      if this.GetVehicle().GetVehicleComponent().GetPS().GetHasExploded() {
        this.hasExploded = true;
      }
      if this.hasExploded {
        this.Deactivate(true);
        return;
      } 
    }
    if timeDelta <= 0.0 {
      this.stats.UpdateDynamic();
      this.UpdateAudioParams(1.0/60.0);
      return;
    }
    if this.active {
      if this.isPlayerMounted {
        this.sys.ctlr.OnUpdate(timeDelta);
        let fc = this.sys.ctlr;
        this.yaw = fc.yaw.GetValue();
        this.roll = fc.roll.GetValue();
        this.pitch = fc.pitch.GetValue();
        this.lift = fc.lift.GetValue();
        this.linearBrake = fc.linearBrake.GetValue();
        this.angularBrake = fc.angularBrake.GetValue();
        this.surge = fc.surge.GetValue();
        this.sway = fc.sway.GetValue();
      } else {
        let v = this.GetVehicle();
        this.surge = v.acceleration * 0.5 - v.deceleration * 0.1;
        this.yaw = -v.turnX;
        this.linearBrake = v.handbrake * 0.5;
        this.angularBrake = v.handbrake * 0.5;
      }
    } else {
      this.yaw = 0.0;
      this.roll = 0.0;
      this.pitch = 0.0;
      this.lift = 0.0;
      this.linearBrake = 0.0;
      this.angularBrake = 0.0;
      this.surge = 0.0;
      this.sway = 0.0;
    }

    this.stats.UpdateDynamic();

    let force = new Vector4(0.0, 0.0, 0.0, 0.0);
    let torque = new Vector4(0.0, 0.0, 0.0, 0.0);

    let shouldModeUpdate = this.active;
    if IsDefined(this.trick) {
      if this.trick.Update(timeDelta) {
        this.trick = null;
      } else {
        force += this.trick.force;
        torque += this.trick.torque;
        shouldModeUpdate = !this.trick.suspendMode;
      }
    }

    if this.mode < ArraySize(this.modes) {
      if shouldModeUpdate {
        this.modes[this.mode].Update(timeDelta);
        force += this.modes[this.mode].force;
        torque += this.modes[this.mode].torque;
      }
    }

    // this.smoothForce = Vector4.Interpolate(this.smoothForce, force, 0.99);
    // this.smoothTorque = Vector4.Interpolate(this.smoothTorque, torque, 0.99);


    // process user-inputted force/torque in visuals/audio
    this.fx.Update(force, torque);
    this.UpdateAudioParams(timeDelta, force, torque);
    
    if this.isPlayerMounted {
      // this.sys.ctlr.GetBlackboard().SetVector4(GetAllBlackboardDefs().VehicleFlight.Force, force);
      // this.sys.ctlr.GetBlackboard().SetVector4(GetAllBlackboardDefs().VehicleFlight.Torque, torque);
      this.sys.ctlr.GetBlackboard().SetFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, 90.0 - Vector4.GetAngleBetween(this.stats.d_forward, FlightUtils.Up())); //(this.stats.d_forward, this.stats.d_forward2D, Vector4.Cross(this.stats.d_forward2D, FlightUtils.Up())), false);
      this.sys.ctlr.GetBlackboard().SetFloat(GetAllBlackboardDefs().VehicleFlight.Roll, Vector4.GetAngleDegAroundAxis(this.stats.d_localUp, FlightUtils.Up(), FlightUtils.Forward()), false);
      // this.sys.ctlr.GetBlackboard().SignalFloat(GetAllBlackboardDefs().VehicleFlight.Pitch);
      // this.sys.ctlr.GetBlackboard().SetVector4(GetAllBlackboardDefs().VehicleFlight.Position, this.stats.d_position);
      // this.sys.ctlr.GetBlackboard().SignalVector4(GetAllBlackboardDefs().VehicleFlight.Position);
    }
    
    // apply physics helpers
    if this.mode < ArraySize(this.modes) {
      this.modes[this.mode].ApplyPhysics(timeDelta);
      force += this.modes[this.mode].force;
      torque += this.modes[this.mode].torque;
    }

    force *= timeDelta;
    // factor in mass
    force *= this.stats.s_mass;
    // convet to global
    force = this.stats.d_orientation * force;

    torque *= timeDelta;
    // factor in interia tensor - maybe half?
    let it = this.GetVehicle().GetInertiaTensor();
    torque.X *= it.X.X;
    // torque.X *= SqrtF(it.X.X) * 20.0;
    torque.Y *= it.Y.Y;
    // torque.Y *= SqrtF(it.Y.Y) * 20.0;
    torque.Z *= it.Z.Z;
    // torque.Z *= SqrtF(it.Z.Z) * 20.0;
    // convert to global
    torque = this.stats.d_orientation * torque;
    
    if this.collisionTimer < FlightSettings.GetFloat("collisionRecoveryDelay") + FlightSettings.GetFloat("collisionRecoveryDuration") {
      let collisionDampener = MinF(MaxF(0.0, (this.collisionTimer - FlightSettings.GetFloat("collisionRecoveryDelay")) / FlightSettings.GetFloat("collisionRecoveryDuration")), 1.0);
      torque *= collisionDampener;
      force *= collisionDampener;
      this.collisionTimer += timeDelta;
    }

    this.force += force;
    this.torque += torque;
  }

  protected cb func OnVehicleFlightDeactivationEvent(evt: ref<VehicleFlightDeactivationEvent>) -> Bool {
    FlightLog.Info("[FlightComponent] OnVehicleFlightDeactivationEvent: " + this.GetVehicle().GetDisplayName());
    this.Deactivate(evt.silent);
  }

  public func Deactivate(silent: Bool) -> Void{
    this.active = false;
    this.fx.Stop();

    if this.isDestroyed && this.hasExploded && this.alarmIsPlaying {
        this.sys.audio.Stop("vehicleDestroyed" + this.GetUniqueID());
    }

    if !silent {
      this.GetVehicle().TurnEngineOn(true);
    }

    if !FlightSettings.GetInstance().generalApplyFlightPhysicsWhenDeactivated {
      this.hasUpdate = false;
    }

    if this.isPlayerMounted {
      this.sys.ctlr.Deactivate(silent);
      if !silent {
        this.sys.audio.Play("vehicle3_off");
      }
      // this.sys.audio.Stop("playerVehicle");
      // this.sys.audio.Stop("leftFront");
      // this.sys.audio.Stop("rightFront");
      // this.sys.audio.Stop("leftRear");
      // this.sys.audio.Stop("rightRear");
    }
    this.sys.audio.Stop("vehicle" + this.GetUniqueID());
  }

  protected cb func OnGridDestruction(evt: ref<VehicleGridDestructionEvent>) -> Bool {
    let biggestImpact: Float;
    let desiredChange: Float;
    let gridState: Float;
    let i: Int32 = 0;
    let gridID = 0;
    while i < 16 {
      gridState = evt.state[i];
      desiredChange = evt.desiredChange[i];
      if desiredChange > biggestImpact {
        biggestImpact = desiredChange;
        gridID = i;
      };
      i += 1;
    };
      // FlightLog.Info("[FlightComponent] OnGridDestruction: " + FloatToStringPrec(biggestImpact, 2));
    if biggestImpact > 0.00 {
      this.ProcessImpact(biggestImpact);
      if this.isPlayerMounted {
        this.sys.ctlr.ProcessImpact(biggestImpact);
      } else {
        // if biggestImpact > 0.00 {
        //   if !this.active {
        //     this.Activate();
        //   } else {
        //     // this.Deactivate(true);
        //   }
        // }
        // if !this.active {
        //   this.FireVerticalImpulse(gridID);
        // }
        if biggestImpact > 0.20 {
          GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"explosion");
        }
        // let event = new vehicleDriveToPointEvent();
        // event.targetPos = new Vector3(0.0, 0.0, 0.0);
        // vehicle.QueueEvent(event);
      }
    }
  }

  // protected cb func OnHit(evt: ref<gameHitEvent>) -> Bool {
  //   let vehicle: ref<VehicleObject> = this.GetVehicle();
  //   let gameInstance: GameInstance = vehicle.GetGame();
  //   let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
  //   let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), vehicle);
  //   FlightLog.Info("[FlightComponent] OnPhysicalCollision: " + FloatToStringPrec(evt.attackData.vehicleImpactForce, 2));
  //   if isPlayerMounted {
  //       this.sys.ctlr.ProcessImpact(evt.attackData.vehicleImpactForce);
  //   } else {
  //     let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
  //     impulseEvent.radius = 1.0;
  //     impulseEvent.worldPosition = Vector4.Vector4To3(evt.hitPosition);
  //     impulseEvent.worldImpulse = new Vector3(0.0, 0.0, evt.attackData.vehicleImpactForce);
  //     this.GetVehicle().QueueEvent(impulseEvent);
  //   }
  // }

  
  protected cb func OnInteractionActivated(evt: ref<InteractionActivationEvent>) -> Bool {
    let radialRequest: ref<ResolveQuickHackRadialRequest>;
    if !IsDefined(evt.activator as PlayerPuppet) && !IsDefined(evt.activator as Muppet) {
      return false;
    };
    radialRequest = new ResolveQuickHackRadialRequest();
    this.GetVehicle().GetHudManager().QueueRequest(radialRequest);
  }

  protected cb func OnSetExposeQuickHacks(evt: ref<SetExposeQuickHacks>) -> Bool {
    let request: ref<RefreshActorRequest> = new RefreshActorRequest();
    request.ownerID = this.GetVehicle().GetEntityID();
    this.GetVehicle().GetHudManager().QueueRequest(request);
  }
  
  protected cb func OnActionEngineering(evt: ref<ActionEngineering>) -> Bool {
    FlightLog.Info("[FlightComponent] OnActionEngineering");
    // this.FireVerticalImpulse();
  }

  // public func OnQuickHackFlightMalfunction(evt: ref<QuickHackFlightMalfunction>) -> EntityNotificationType {
  //   FlightLog.Info("[FlightComponent] OnQuickHackFlightMalfunction");
  //   // let type: EntityNotificationType = this.OnQuickHackFlightMalfunction(evt);
  //   // if Equals(type, EntityNotificationType.DoNotNotifyEntity) {
  //   //   return type;
  //   // };
  //   if evt.IsStarted() {
  //     // this.ExecutePSAction(this.FireVerticalImpulse());
  //     // this.FireVerticalImpulse();
  //   };
  //   return EntityNotificationType.SendThisEventToEntity;
  // }

  // gridID
  // 0 rear left
  // 1 rear right
  // 2 -
  // 3 -
  // 4 door left
  // 5 door right
  // 6 front left
  // 7 front right

  public func FireVerticalImpulse(gridID: Int32, opt impulse: Float) {
    let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
    impulseEvent.radius = 1.0;
    let offset = new Vector4(0.0, 0.0, 0.0, 0.0);
    if gridID == 0 {
      offset = new Vector4(-0.5, -0.5, 0.0, 0.0);
    }
    if gridID == 1 {
      offset = new Vector4(0.5, -0.5, 0.0, 0.0);
    }
    if gridID == 4 {
      offset = new Vector4(-0.5, 0.0, 0.0, 0.0);
    }
    if gridID == 5 {
      offset = new Vector4(0.5, 0.0, 0.0, 0.0);
    }
    if gridID == 6 {
      offset = new Vector4(-0.5, 0.5, 0.0, 0.0);
    }
    if gridID == 7 {
      offset = new Vector4(0.5, 0.5, 0.0, 0.0);
    }
    if impulse == 0.0 {
      impulse = 1.0;
    }
    // FlightLog.Info("[FlightComponent] FireVerticalImpulse: " + gridID);
    impulseEvent.worldPosition = Vector4.Vector4To3(this.GetVehicle().GetLocalToWorld() * offset);
    impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 10.0 * impulse * this.GetVehicle().GetTotalMass());
    this.GetVehicle().QueueEvent(impulseEvent);
  }

  // protected cb func OnPhysicalCollision(evt: ref<PhysicalCollisionEvent>) -> Bool {
  //   FlightLog.Info("[FlightComponent] OnPhysicalCollision");
  //   let vehicle = this.GetVehicle();
  //   let gameInstance: GameInstance = vehicle.GetGame();
  //   let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
  //   let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), this);
  //   if isPlayerMounted {
  //     // this.sys.ctlr.ProcessImpact(evt.attackData.vehicleImpactForce);
  //   } else {
  //     let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
  //     impulseEvent.radius = 1.0;
  //     impulseEvent.worldPosition = Vector4.Vector4To3(evt.worldPosition);
  //     impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 10000.0);
  //     vehicle.QueueEvent(impulseEvent);
  //   }
  // }

  // protected cb func OnHit(evt: ref<gameHitEvent>) -> Bool {
  //   FlightLog.Info("[FlightComponent] OnHit: " + FloatToStringPrec(evt.attackData.vehicleImpactForce, 2));
  //   let vehicle = this.GetVehicle();
  //   let gameInstance: GameInstance = vehicle.GetGame();
  //   let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
  //   let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), this);
  //   if isPlayerMounted {
  //     this.sys.ctlr.ProcessImpact(evt.attackData.vehicleImpactForce);
  //   } else {
  //     let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
  //     impulseEvent.radius = 1.0;
  //     impulseEvent.worldPosition = Vector4.Vector4To3(evt.hitPosition);
  //     impulseEvent.worldImpulse = new Vector3(0.0, 0.0, evt.attackData.vehicleImpactForce);
  //     vehicle.QueueEvent(impulseEvent);
  //   }
  // }

  // public cb func OnAnyEvent(evt: ref<Event>) {
  //   FlightLog.Info("[FlightComponent] OnAnyEvent: " + ToString(evt.GetClassName()));
  // }

  // hook into sound somehow
  // protected cb func OnVehicleOnPartDetached(evt: ref<VehicleOnPartDetachedEvent>) -> Bool {
  //   let partName: CName = evt.partName;
  //   if Equals(partName, n"Trunk") {
  //     (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.trunk, VehicleDoorState.Detached);
  //   } else {
  //     if Equals(partName, n"Hood") {
  //       (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.hood, VehicleDoorState.Detached);
  //     } else {
  //       if Equals(partName, n"DoorFrontLeft") || Equals(partName, n"DoorFrontLeft_A") || Equals(partName, n"DoorFrontLeft_B") || Equals(partName, n"DoorFrontLeft_C") {
  //         (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.seat_front_left, VehicleDoorState.Detached);
  //       } else {
  //         if Equals(partName, n"DoorFrontRight") || Equals(partName, n"DoorFrontRight_A") || Equals(partName, n"DoorFrontRight_B") || Equals(partName, n"DoorFrontRight_C") {
  //           (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.seat_front_right, VehicleDoorState.Detached);
  //         } else {
  //           if Equals(partName, n"DoorBackLeft") {
  //             (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.seat_back_left, VehicleDoorState.Detached);
  //           } else {
  //             if Equals(partName, n"DoorBackRight") {
  //               (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.seat_back_right, VehicleDoorState.Detached);
  //             };
  //           };
  //         };
  //       };
  //     };
  //   };
  // }
  
  protected final func OnVehicleCameraChange(state: Bool) -> Void {
    this.sys.ctlr.isTPP = state;
  }

  public func ProcessImpact(impact: Float) {
    this.collisionTimer = (FlightSettings.GetFloat("collisionRecoveryDelay") + FlightSettings.GetFloat("collisionRecoveryDuration")) * (1.0 - (impact * this.GetFlightMode().collisionPenalty));
    // this.ui_info.StartGlitching(impact, FlightSettings.GetFloat("collisionRecoveryDuration") + impact);
  }

  public let audioUpdate: ref<FlightAudioUpdate>;

  public func UpdateAudioParams(timeDelta: Float, force: Vector4, torque: Vector4) -> Void {
    let ratio = 1.0;
    if this.collisionTimer < FlightSettings.GetFloat("collisionRecoveryDelay") + FlightSettings.GetFloat("collisionRecoveryDuration") {
      ratio = MaxF(0.0, (this.collisionTimer - FlightSettings.GetFloat("collisionRecoveryDelay")) / FlightSettings.GetFloat("collisionRecoveryDuration"));
    }
    
    let vehicleID = Cast<StatsObjectID>(this.GetVehicle().GetEntityID());
    let vehHealthPercent = GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).GetStatPoolValue(vehicleID, gamedataStatPoolType.Health);
    // this.audioUpdate.damage = 1.0 - MaxF(GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).GetStatPoolValue(Cast<StatsObjectID>(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, false) + ratio, 1.0);
    if this.isDestroyed {
      this.audioUpdate.damage = 0.25;
    } else {
      this.audioUpdate.damage = (1.0 - (vehHealthPercent / 100.0)) * 0.2;
    }
    if this.isPlayerMounted {
      // more responsive-sounding
      this.audioUpdate.surge = this.sys.ctlr.surge.GetInput() * ratio;
      this.audioUpdate.yaw = this.sys.ctlr.yaw.GetInput() * ratio;
      if IsDefined(this.GetFlightMode() as FlightModeDrone) {
        this.audioUpdate.lift = this.sys.ctlr.lift.GetInput() * ratio;
        this.audioUpdate.roll = this.sys.ctlr.roll.GetInput() * ratio;
        this.audioUpdate.pitch = this.sys.ctlr.pitch.GetInput() * ratio;
      } else {
        this.audioUpdate.lift = (Vector4.Dot(Vector4.Normalize(force), this.stats.d_localUp) * 0.1 + this.sys.ctlr.lift.GetInput()) * ratio;
        this.audioUpdate.roll = this.roll * ratio;
        this.audioUpdate.pitch = this.pitch * ratio;
      }
    } else {
      this.audioUpdate.surge = this.surge * ratio;
      this.audioUpdate.lift = (Vector4.Dot(Vector4.Normalize(force), this.stats.d_localUp) * 0.1 + this.lift) * ratio;
      this.audioUpdate.roll = this.roll * ratio;
      this.audioUpdate.pitch = this.pitch * ratio;
      this.audioUpdate.yaw = this.yaw * ratio;
    }
    // if this.mode == 3 {
    //   this.audioUpdate.surge *= 0.5;
    //   this.audioUpdate.surge += this.lift * ratio * 0.5;
    // }
    // this.audioUpdate.lift = this.lift * ratio;
    // this.audioUpdate.brake = this.brake;
    this.audioUpdate.brake = MaxF(this.sys.ctlr.linearBrake.GetInput(), this.sys.ctlr.angularBrake.GetInput());
    // this.audioUpdate.brake = Vector4.Dot(-force, this.stats.d_direction);

    this.UpdateAudioParams(timeDelta);
  }

  public func UpdateAudioParams(timeDelta: Float) -> Void {
    let engineVolume = this.engineVolume;
    let windVolume = this.windVolume;
    let warningVolume = this.warningVolume;
    let master = Cast<Float>((GameInstance.GetSettingsSystem(this.GetVehicle().GetGame()).GetVar(n"/audio/volume", n"MasterVolume") as ConfigVarInt).GetValue()) / 100.0;
    let sfx = Cast<Float>((GameInstance.GetSettingsSystem(this.GetVehicle().GetGame()).GetVar(n"/audio/volume", n"SfxVolume") as ConfigVarInt).GetValue()) / 100.0;
    engineVolume *= (master * sfx);
    windVolume *= (master * sfx);
    warningVolume *= (master * sfx);
    if this.isPopupShown || this.isInMenu || GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsPausedState() ||
      GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsTimeDilationActive(n"HubMenu") || 
      GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsTimeDilationActive(n"WorldMap")
      {
      engineVolume = 0.0;
      windVolume = 0.0;
      warningVolume = 0.0;
      if this.isPlayerMounted {
        // this.sys.audio.Update("playerVehicle", Vector4.EmptyVector(), engineVolume);
        this.sys.audio.Update("windLeft", Vector4.EmptyVector(), windVolume, this.audioUpdate);
        this.sys.audio.Update("windRight", Vector4.EmptyVector(), windVolume, this.audioUpdate);
      }
      if this.active {
        this.sys.audio.Update("vehicle" + this.GetUniqueID(), Vector4.EmptyVector(), engineVolume, this.audioUpdate);
      }
      if this.isDestroyed && !this.GetVehicle().GetVehicleComponent().GetPS().GetHasExploded() && this.alarmIsPlaying {
        this.sys.audio.Update("vehicleDestroyed" + this.GetUniqueID(), Vector4.EmptyVector(), warningVolume, this.audioUpdate);
      }
      // this.sys.audio.Update("leftFront", Vector4.EmptyVector(), engineVolume);
      // this.sys.audio.Update("rightFront", Vector4.EmptyVector(), engineVolume);
      // this.sys.audio.Update("leftRear", Vector4.EmptyVector(), engineVolume);
      // this.sys.audio.Update("rightRear", Vector4.EmptyVector(), engineVolume);
      return;
    }

    // might need to handle just the scanning system's dilation, and the pause menu
    if GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsTimeDilationActive(n"radialMenu") {
      engineVolume *= 0.1;
      windVolume *= 0.1;
      warningVolume *= 0.1;
    }

    this.sys.audio.UpdateSlotProviders();

    // let leftFrontPosition = this.sys.audio.GetPosition(n"wheel_front_left") - (this.stats.d_velocity * timeDelta);
    // let rightFrontPosition = this.sys.audio.GetPosition(n"wheel_front_right") - (this.stats.d_velocity * timeDelta);
    // let leftRearPosition = this.sys.audio.GetPosition(n"wheel_back_left") - (this.stats.d_velocity * timeDelta);
    // let rightRearPosition = this.sys.audio.GetPosition(n"wheel_back_right") - (this.stats.d_velocity * timeDelta);

    let windLeftPosition = this.sys.audio.GetPosition(n"window_front_left_a"); // - (this.stats.d_velocity * timeDelta);
    let windRightPosition = this.sys.audio.GetPosition(n"window_front_right_a"); //- (this.stats.d_velocity * timeDelta);

    // let listenerMatrix = (this.sys.ctlr.player.FindComponentByName(n"soundListener") as IPlacedComponent).GetLocalToWorld();
    let listenerMatrix = this.sys.tppCamera.GetLocalToWorld();
    FlightAudio.UpdateListener(Matrix.GetTranslation(listenerMatrix), Matrix.GetAxisY(listenerMatrix), Matrix.GetAxisZ(listenerMatrix));

    this.audioUpdate.speed = this.stats.d_speed;
    this.audioUpdate.yawDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_up);
    this.audioUpdate.pitchDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_right);

    // engineVolume *= (ratio * 0.5 + 0.5);

    // this.sys.audio.Update("leftFront", leftFrontPosition, engineVolume);
    // this.sys.audio.Update("rightFront", rightFrontPosition, engineVolume);
    // this.sys.audio.Update("leftRear", leftRearPosition, engineVolume);
    // this.sys.audio.Update("rightRear", rightRearPosition, engineVolume);
    if this.isPlayerMounted {
      this.audioUpdate.inside = this.sys.ctlr.isTPP ? MaxF(0.0, this.audioUpdate.inside - timeDelta * 4.0) : MinF(1.0, this.audioUpdate.inside + timeDelta * 4.0);
      this.sys.audio.Update("windLeft", windLeftPosition, windVolume, this.audioUpdate);
      this.sys.audio.Update("windRight", windRightPosition, windVolume, this.audioUpdate);
    } else {
      this.audioUpdate.inside = 0.0;
    }
    if this.active {
      this.sys.audio.Update("vehicle" + this.GetUniqueID(), this.GetVehicle().GetWorldPosition(), engineVolume, this.audioUpdate);
    }
    if this.isDestroyed && !this.GetVehicle().GetVehicleComponent().GetPS().GetHasExploded() && this.alarmIsPlaying {
      this.sys.audio.Update("vehicleDestroyed" + this.GetUniqueID(), this.GetVehicle().GetWorldPosition(), warningVolume, this.audioUpdate);
    }
  }

  private func GetUniqueID() -> String {
    return ToString(EntityID.GetHash(this.GetVehicle().GetEntityID()));
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
      this.hood = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"VehicleHoodEmitter") as IPlacedComponent;
      this.trunk = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"VehicleTrunkEmitter") as IPlacedComponent;
    } else {
      this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFront") as IPlacedComponent;
      this.fr_tire = this.fl_tire;
      this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBack") as IPlacedComponent;
      this.br_tire = this.bl_tire;
    }
  }

  public func FindGround(out normal: Vector4) -> Bool {
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
    let lookDown = new Vector4(0.0, 0.0, -FlightSettings.GetFloat("hoverModeMaxHoverHeight") - 10.0, 0.0);
    this.sqs.SyncRaycastByCollisionGroup(fl_tire, fl_tire + lookDown, n"VehicleBlocker", findGround1, false, false);
    this.sqs.SyncRaycastByCollisionGroup(fr_tire, fr_tire + lookDown, n"VehicleBlocker", findGround2, false, false);
    this.sqs.SyncRaycastByCollisionGroup(bl_tire, bl_tire + lookDown, n"VehicleBlocker", findGround3, false, false);
    this.sqs.SyncRaycastByCollisionGroup(br_tire, br_tire + lookDown, n"VehicleBlocker", findGround4, false, false);
    
    let groundPoint1: Vector4;
    let groundPoint2: Vector4;
    let groundPoint3: Vector4;
    let groundPoint4: Vector4;

    if TraceResult.IsValid(findGround1) {
      groundPoint1 = Vector4.Vector3To4(findGround1.position);// - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint1);
      //   this.ui.DrawText(groundPoint1, FloatToStringPrec(Vector4.Distance(fl_tire, Cast(findGround1.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround2) {
      groundPoint2 = Vector4.Vector3To4(findGround2.position);// - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint2);
      //   this.ui.DrawText(groundPoint2, FloatToStringPrec(Vector4.Distance(fr_tire, Cast(findGround2.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround3) {
      groundPoint3 = Vector4.Vector3To4(findGround3.position);// - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint3);
      //   this.ui.DrawText(groundPoint3, FloatToStringPrec(Vector4.Distance(bl_tire, Cast(findGround3.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround4) {
      groundPoint4 = Vector4.Vector3To4(findGround4.position);// - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint4);
      //   this.ui.DrawText(groundPoint4, FloatToStringPrec(Vector4.Distance(br_tire, Cast(findGround4.position)), 2));
      // }
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
      let n = (Vector4.Normalize(Cast(findGround1.normal)) + Vector4.Normalize(Cast(findGround2.normal)) + Vector4.Normalize(Cast(findGround3.normal)) + Vector4.Normalize(Cast(findGround4.normal))) / 4.0;
      // this.normal = Vector4.Interpolate(this.normal, normal, this.normalEase);
      normal = Vector4.Normalize(n);

      return true;
    } else {
      return false;
    }   
  } 

  protected cb func OnHUDInstruction(evt: ref<HUDInstruction>) -> Bool {
    if evt.quickhackInstruction.ShouldProcess() {
      FlightLog.Info("[FlightComponent] OnHUDInstruction");
      this.GetVehicle().TryOpenQuickhackMenu(evt.quickhackInstruction.ShouldOpen());
    };
  }

  protected let m_attacksSpawned: array<ref<EffectInstance>>;

  public func OnFireWeapon(placeholderQuat: Quaternion, weaponItem:TweakDBID, attachmentSlot: TweakDBID) -> Void {    
    // let weapon = TweakDBInterface.GetWeaponItemRecord(weaponItem);
    let wt: WorldTransform;
    let vehicleSlots = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
    vehicleSlots.GetSlotTransform(StringToName(TweakDBInterface.GetAttachmentSlotRecord(attachmentSlot).EntitySlotName()), wt);
    let quat = WorldTransform.GetOrientation(wt);
    // let start = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(wt));
    // let end = Vector4.Vector3To4(tracePosition);
    // WorldTransform.SetPosition(wt, start)
    WorldTransform.SetOrientation(wt, quat * placeholderQuat);

    let effect = Cast<FxResource>(r"base\\fx\\vehicles\\av\\av_panzer\\weapons\\v_panzer_muzzle_flash.effect");
    // let effect = Cast<FxResource>(r"base\\fx\\weapons\\firearms\\_muzzle_lights\\smart\\w_s_rifles_mq_muzzle_lights_tpp.effect");
    let fxSystem = GameInstance.GetFxSystem(this.GetVehicle().GetGame());
    if IsDefined(fxSystem) {
      fxSystem.SpawnEffect(effect, wt);
    }

    
    // let attack: ref<Attack_GameEffect>;
    // let attackContext: AttackInitContext;
    // let effect: ref<EffectInstance>;
    // let position: Vector4;
    // let slotTransform: WorldTransform;
    // let statMods: array<ref<gameStatModifierData>>;
    // let slotName = StringToName(TweakDBInterface.GetAttachmentSlotRecord(attachmentSlot).EntitySlotName());
    // let validSlotPosition: Bool = vehicleSlots.GetSlotTransform(slotName, slotTransform);
    // if validSlotPosition {
    //   position = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(slotTransform));
    // } else {
    //   position = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.GetVehicle().GetWorldTransform()));
    // };
    // attackContext.source = this.GetVehicle();
    // attackContext.record = weapon.RangedAttacks().DefaultFire().PlayerAttack();
    // attackContext.instigator = this.sys.player;
    // attack = IAttack.Create(attackContext) as Attack_GameEffect;
    // attack.GetStatModList(statMods);
    // effect = attack.PrepareAttack(this.sys.player);
    // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.position, position);
    // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.muzzlePosition, position);
    // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.forward, Quaternion.GetForward(placeholderQuat));
    // EffectData.SetVariant(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.attack, ToVariant(attack));
    // EffectData.SetVariant(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.attackStatModList, ToVariant(statMods));
    // attack.StartAttack();
    
    // effect.AttachToSlot(this.GetVehicle(), slotName, GetAllBlackboardDefs().EffectSharedData.position, GetAllBlackboardDefs().EffectSharedData.forward);
    
    // ArrayPush(this.m_attacksSpawned, effect);

    let broadcaster = this.sys.player.GetStimBroadcasterComponent();
    if IsDefined(broadcaster) {
      broadcaster.TriggerSingleBroadcast(this.sys.player, gamedataStimType.Gunshot, 25.0);
      let data: stimInvestigateData;
      data.illegalAction = true;
      data.attackInstigator = this.sys.player;
      broadcaster.TriggerSingleBroadcast(this.sys.player, gamedataStimType.Gunshot, 100.0, data, true);
    };
    
    // let tp: WorldPosition;
    // WorldPosition.SetVector4(tp, Vector4.Vector3To4(tracePosition));
    // fxi.UpdateTargetPosition(tp);
  }

/*  private final func RegisterToHUDManager(shouldRegister: Bool) -> Void {
    let hudManager: ref<HUDManager>;
    let registration: ref<HUDManagerRegistrationRequest>;
    if this.GetVehicle().IsCrowdVehicle() && !this.GetVehicle().ShouldForceRegisterInHUDManager() {
      return;
    };
    hudManager = GameInstance.GetScriptableSystemsContainer(this.GetVehicle().GetGame()).Get(n"HUDManager") as HUDManager;
    if IsDefined(hudManager) {
      registration = new HUDManagerRegistrationRequest();
      registration.SetProperties(this.GetVehicle(), shouldRegister);
      hudManager.QueueRequest(registration);
    };
  }
*/
}

// FlightContextTransitions.reds

@addMethod(InputContextTransitionEvents)
protected final const func ShowVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  // this.ShowInputHint(scriptInterface, n"Reload", n"VehicleFlight", "LocKey#36198");
  // this.ShowInputHint(scriptInterface, n"WeaponWheel", n"VehicleFlight", "LocKey#36199");
}

@addMethod(InputContextTransitionEvents)
protected final const func RemoveVehicleFlightInputHints(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  // this.RemoveInputHintsBySource(scriptInterface, n"VehicleFlight");
}

@wrapMethod(VehicleDriverContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  // FlightLog.Info("[VehicleDriverContextDecisions] EnterCondition");
  let old = wrappedMethod(stateContext, scriptInterface);
  let currentState: Int32 = scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle);
  return old && currentState != 8;
}

@wrapMethod(VehiclePassengerContextDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  let currentState: Int32 = scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle);
  return currentState != 8;
}

public class VehicleFlightContextEvents extends InputContextTransitionEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextEvents] OnEnter");
    this.ShowVehicleFlightInputHints(stateContext, scriptInterface);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextEvents] OnExit");
    this.RemoveVehicleFlightInputHints(stateContext, scriptInterface);
  }
}

public class VehicleFlightContextDecisions extends InputContextTransitionDecisions {

  private let m_callbackID: ref<CallbackHandle>;

  protected final func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextDecisions] OnAttach");
    let allBlackboardDef: ref<AllBlackboardDefinitions>;
    if IsDefined(scriptInterface.localBlackboard) {
      allBlackboardDef = GetAllBlackboardDefs();
      this.m_callbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this, n"OnVehicleStateChanged");
      this.OnVehicleStateChanged(scriptInterface.localBlackboard.GetInt(allBlackboardDef.PlayerStateMachine.Vehicle));
      FlightController.GetInstance().SetupMountedToCallback(scriptInterface.localBlackboard);
      (scriptInterface.owner as VehicleObject).ToggleFlightComponent(true);
    };
    //this.EnableOnEnterCondition(true);
  }

  protected final func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextDecisions] OnDetach");
    (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    this.m_callbackID = null;
    // FlightController.GetInstance().Disable();
  }

  protected cb func OnVehicleStateChanged(value: Int32) -> Bool {
    FlightLog.Info("[VehicleFlightContextDecisions] OnVehicleStateChanged: " + ToString(IntEnum<gamePSMVehicle>(value)));
    this.EnableOnEnterCondition(value == 8);
  }

  protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // FlightLog.Info("[VehicleFlightContextDecisions] EnterCondition");
    // if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleOnlyForward") {
    //   return false;
    // };
    // if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"NoDriving") {
    //   return false;
    // };
    return true;
  }
}

// FlightController.reds

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

// public static func fcv() -> wref<VehicleObject> {
//   return FlightController.GetInstance().GetVehicle();
// }

public native class FlightController extends IScriptable {
  // defined on the RED4ext side
  private native let enabled: Bool;
  private native let active: Bool;
  public native let mode: Int32;
  public static native func GetInstance() -> ref<FlightController>;
  
  // redscript-only
  private let sys: ref<FlightSystem>;
  private let gameInstance: GameInstance;
  private let player: ref<PlayerPuppet>;
  private let m_callbackID: ref<CallbackHandle>;
  private let m_vehicleCollisionBBStateID: ref<CallbackHandle>;
  private let ui: ref<FlightControllerUI>;
  public final func SetUI(ui: ref<FlightControllerUI>) {
    this.ui = ui;
  }
  public final const func IsEnabled() -> Bool {
    return this.enabled;
  }
  public final const func IsActive() -> Bool {
    return this.active;
  }

  public let showOptions: Bool;
  public let showUI: Bool;
  public let linearBrake: ref<InputPID>;
  public let angularBrake: ref<InputPID>;
  public let lift: ref<InputPID>;
  public let surge: ref<InputPID>;
  public let roll: ref<InputPID>;
  public let pitch: ref<InputPID>;
  public let yaw: ref<InputPID>;
  public let sway: ref<InputPID>;

  private let secondCounter: Float;

  public let isInAnyMenu: Bool;
  public let audioEnabled: Bool;

  public let isTPP: Bool;  

  private let uiBlackboard: ref<IBlackboard>;
  private let uiSystemBB: ref<UI_SystemDef>;

  public let initialized: Bool;
  public let usingKB: Bool;

  // public let effectInstance: ref<EffectInstance>;

  // protected let m_settingsListener: ref<FlightSettingsListener>;
  // protected let m_groupPath: CName;

  private func Initialize() {
    FlightLog.Info("[FlightController] Initialize");
    this.sys = FlightSystem.GetInstance();
    // this.sys = GameInstance.GetScriptableSystemsContainer(player.GetGame()).Get(n"FlightSystem") as FlightSystem;
    this.enabled = false;
    this.active = false;
    this.showOptions = false;
    this.showUI = true;

    this.linearBrake = InputPID.Create(0.5, 0.5);
    this.angularBrake = InputPID.Create(0.5, 0.5);
    this.lift = InputPID.Create(0.05, 0.2);
    this.surge = InputPID.Create(0.2, 0.2);
    this.roll = InputPID.Create(0.25, 1.0);
    this.pitch = InputPID.Create(0.25, 1.0);
    this.yaw = InputPID.Create(0.1, 0.2);
    this.sway = InputPID.Create(0.2, 0.2);
    
    this.secondCounter = 0.0;

    this.audioEnabled = true;

    this.uiBlackboard = GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_System);
    this.uiSystemBB = GetAllBlackboardDefs().UI_System;

    this.initialized = true;

    // this.trackedMappinId = this.uiBlackboard.RegisterListenerVariant(this.uiSystemBB.TrackedMappin, this, n"OnTrackedMappinUpdated");
    // this.uiBlackboard.SignalVariant(this.uiSystemBB.TrackedMappin);

    // this.waypoint = new Vector4(313.6, 208.2, 62.3, 0.0);


    // this.m_groupPath = n"/controls/flight";
    // this.m_settingsListener = new FlightSettingsListener();
    // this.m_settingsListener.RegisterController(this);
    // this.m_settingsListener.Register(this.m_groupPath);
  }

  private func SetPlayer(player: ref<PlayerPuppet>) {
    this.sys.Setup(player);
    this.gameInstance = player.GetGame();
    this.player = player;

    this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, FlightSettings.GetFloat("isFlightUIActive") > 0.5);
  }

  public const func GetBlackboard() -> ref<IBlackboard> {
    return GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().VehicleFlight);
  }
  
  public static func CreateInstance(player: ref<PlayerPuppet>) {
    // FlightLog.Info("[FlightController] CreateInstance Started");
    // let self: ref<FlightController> = new FlightController();
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
  
  // public static func GetInstance() -> wref<FlightController> {
  //   return GetAllBlackboardDefs().flightController;
  // }

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
    // if (mounted) {
    //   this.Enable();
    // } else {
    //   this.Disable();
    // }
  }
  
  public func Enable() -> Void {
    this.enabled = true;
    this.SetupActions();
    FlightLog.Info("[FlightController] Enable");
  }

  public func Disable() -> Void {
    this.enabled = false;
    this.SetupActions();   
    FlightLog.Info("[FlightController] Disable");
  }
  
  private func Activate(silent: Bool) -> Void {
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

    // let data: InputHintGroupData;
    // data.localizedTitle = "Flight Control";
    // data.localizedDescription = "The controls used in Let There Be Flight";
    // data.sortingPriority = 0;
    // let evt: ref<AddInputGroupEvent> = new AddInputGroupEvent();
    // evt.data = data;
    // evt.groupId = n"FlightController";
    // evt.targetHintContainer = n"GameplayInputHelper";
    // GameInstance.GetUISystem(this.gameInstance).QueueEvent(evt);

    // let wheel = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"wheel_01_br_a") as MeshComponent;
    // if IsDefined(wheel) {
    //   wheel.UpdateHardTransformBinding(n"vehicle_slots", n"wheel_back_left");
    //   this.GetVehicle().AddComponent(wheel);
    // }
    
    this.SetupPositionProviders();

    // this.sys.tppCamera = GetPlayer(this.gameInstance).FindComponentByName(n"vehicleTPPCamera") as vehicleTPPCameraComponent;

    // idk what to do with this
    // let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    // uiSystem.PushGameContext(IntEnum(10));

    if (this.showUI) {
      this.ui.Show();
      // FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, true);
    }
  
    if !silent {
      this.ShowSimpleMessage("Flight Control Engaged");
    }
    
    FlightLog.Info("[FlightController] Activate");
    this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsActive, true, true);
    // this.GetBlackboard().SignalBool(GetAllBlackboardDefs().VehicleFlight.IsActive);
  }

  private func Deactivate(silent: Bool) -> Void {
    this.active = false;
    this.SetupActions();

    //let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    //uiSystem.PopGameContext(IntEnum(10));

    if !silent {
      this.ShowSimpleMessage("Flight Control Disengaged");
    }
    if (this.showUI) {
      this.ui.Hide();
    }
    // FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, false);

    FlightLog.Info("[FlightController] Deactivate");
    this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsActive, false, true);
    // this.GetBlackboard().SignalBool(GetAllBlackboardDefs().VehicleFlight.IsActive);

    
    // let evt: ref<DeleteInputGroupEvent> = new DeleteInputGroupEvent();
    // evt.groupId = n"FlightControl";
    // evt.targetHintContainer = n"GameplayInputHelper";
    // GameInstance.GetUISystem(this.gameInstance).QueueEvent(evt);
  }  

  private func ShowMoreInfo() -> Void {
    // very intrusive - need a prompt/confirmation that they want this popup, eg Detailed Info / About
    let shardUIevent = new NotifyShardRead();
    shardUIevent.title = "Flight Control: Now Available";
    shardUIevent.text = "Your new car is equiped with the state-of-the-art Flight Control!";
    GameInstance.GetUISystem(this.gameInstance).QueueEvent(shardUIevent);
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
      // this.player.RegisterInputListener(this, n"Flight_Toggle");
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
      }
    }

    // if this.trick {
    //   evt.AddInputHint(FlightController.CreateInputHint("Aileron Roll", n"Yaw"), true);
    // } else {
      // evt.AddInputHint(FlightController.CreateInputHint("Tricks", n"Flight_Trick"), true);
    // }
    // we may want to look at something else besides this input so ForceBrakesUntilStoppedOrFor will work (not entirely sure it doesn't now)
    // vehicle.GetBlackboard().GetInt(GetAllBlackboardDefs().VehicleFlight.IsHandbraking)

    let usesRightStick = this.sys.playerComponent.GetFlightMode().usesRightStickInput;

    evt.AddInputHint(FlightController.CreateInputHint("Enable Flight", n"Flight_Toggle"),       this.enabled && !this.active);

    evt.AddInputHint(FlightController.CreateInputHint("Disable Flight", n"Flight_Toggle"),      this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint("Yaw", n"Yaw"),                           this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint("Pitch", n"Pitch"),                       this.active && !this.showOptions && (usesRightStick || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint("Roll", n"Roll"),                         this.active && !this.showOptions && (usesRightStick || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint("Lift", n"Lift"),                         this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint("Linear Brake", n"Flight_LinearBrake"),   this.active && !this.showOptions && this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint("Angular Brake", n"Flight_AngularBrake"), this.active && !this.showOptions && this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint("Brake", n"Flight_LinearBrake"),          this.active && !this.showOptions && !this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint("Flight Options", n"Flight_Options"),     this.active && !this.showOptions);

    evt.AddInputHint(FlightController.CreateInputHint("Sway", n"Sway"),                         this.active && this.showOptions && this.usingKB);
    // let desc: String;
    // desc = this.sys.playerComponent.GetNextFlightModeDescription();
    evt.AddInputHint(FlightController.CreateInputHint("Next Mode", n"Flight_ModeSwitchForward"),     this.active && (this.showOptions || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint("Prev Mode", n"Flight_ModeSwitchBackward"),     this.active && this.showOptions && !this.usingKB);
    // evt.AddInputHint(FlightController.CreateInputHint("Raise Hover Height", n"FlightOptions_Up"), true);
    // evt.AddInputHint(FlightController.CreateInputHint("Lower Hover Height", n"FlightOptions_Down"), true);
    evt.AddInputHint(FlightController.CreateInputHint("Toggle UI", n"Flight_UIToggle"),         this.active && this.showOptions);
    // evt.AddInputHint(FlightController.CreateInputHint("Fire", n"ShootPrimary"),                 this.active && !this.showOptions);

    uiSystem.QueueEvent(evt);
  }

  private let trick: Bool;

  public func ProcessImpact(impact: Float) {
    // this.surge.Reset(this.surge.GetValue() * MaxF(0.0, 1.0 - impact * 5.0));
  }

  private func CycleMode(direction: Int32) -> Void {
    let newMode = this.sys.playerComponent.GetNextFlightMode(direction);
    this.mode = this.mode + direction;
    if this.mode < 0 {
      this.mode += ArraySize(this.sys.playerComponent.modes);
    } 
    this.mode = this.mode % ArraySize(this.sys.playerComponent.modes);
    this.GetBlackboard().SetInt(GetAllBlackboardDefs().VehicleFlight.Mode, this.mode);
    let evt = new VehicleFlightModeChangeEvent();
    evt.mode = this.mode;
    GetMountedVehicle(this.player).QueueEvent(evt);
    this.ShowSimpleMessage(newMode.GetDescription() + " Enabled");
    GameInstance.GetAudioSystem(this.gameInstance).Play(n"ui_menu_onpress");
    this.SetupActions();
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
    // FlightLog.Info(ToString(actionType) + ToString(actionName) + ToString(value));
    // if Equals(actionName, n"Flight_Toggle") && ListenerAction.IsButtonJustPressed(action) {
        // this.Toggle();
        // ListenerActionConsumer.ConsumeSingleAction(consumer);
    // }
    if this.active {
      // if Equals(actionName, n"Flight_ModeSwitchForward") && ListenerAction.IsButtonJustPressed(action) {
      //   this.CycleMode(1);
      // }
      if Equals(actionName, n"Flight_Options") {
        if ListenerAction.IsButtonJustPressed(action) {
          // FlightLog.Info("Options button pressed");
          this.showOptions = true;
          // GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"summon_hologram", true);
          if (this.showUI) {
            this.ui.ShowInfo();
          }
          this.SetupActions();
        }
        if ListenerAction.IsButtonJustReleased(action) {
          // FlightLog.Info("Options button released");
          this.showOptions = false; 
          // GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"summon_hologram");
          this.SetupActions();
        }
      }

      // if Equals(actionName, n"Flight_Trick") {
      //   if ListenerAction.IsButtonJustPressed(action) {

          // let ps = this.sys.playerComponent.FindComponentByName(n"projectileSpawn8722") as ProjectileSpawnComponent;
          // ps.Spawn(Cast<Uint32>(0));

          // let attack: ref<Attack_GameEffect>;
          // let attackContext: AttackInitContext;
          // let effect: ref<EffectInstance>;
          // let statMods: array<ref<gameStatModifierData>>;    
          // let position: Vector4;
          // let forward: Vector4;
          // attackContext.source = this.sys.playerComponent.GetVehicle();
          // attackContext.record = TweakDBInterface.GetAttackRecord(t"Attacks.ExplodingPanzerBulletProjectile");
          // attackContext.instigator = attackContext.source;
          // attack = IAttack.Create(attackContext) as Attack_GameEffect;
          // attack.GetStatModList(statMods);
          // effect = attack.PrepareAttack(this.sys.playerComponent.GetVehicle());
          // GameInstance.GetTargetingSystem(this.gameInstance).GetDefaultCrosshairData(this.sys.player, position, forward);
          // position = this.sys.playerComponent.stats.d_position + new Vector4(0.0, 0.0, 1.0, 0.0);
          // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.position, position);
          // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.muzzlePosition, position);
          // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.forward, this.sys.playerComponent.stats.d_forward);
          // // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.forward, forward);
          // EffectData.SetVariant(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.attack, ToVariant(attack));
          // EffectData.SetVariant(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.attackStatModList, ToVariant(statMods));
          // attack.StartAttack();
          
          // let wt: WorldTransform;
          // WorldTransform.SetPosition(wt, position);
          // WorldTransform.SetOrientation(wt, this.sys.playerComponent.stats.d_orientation);

          // let effect = Cast<FxResource>(r"base\\fx\\vehicles\\av\\av_panzer\\weapons\\v_panzer_muzzle_flash.effect");
          // GameInstance.GetFxSystem(this.sys.playerComponent.GetVehicle().GetGame()).SpawnEffect(effect, wt);
      //   }
      // }

          // // FlightLog.Info("Options button pressed");
          // this.trick = true;
          // let direction = 0.0;
          // if AbsF(this.sway.GetInput()) > 0.9 {
          //   direction = this.sway.GetInput();
          // }
          // if AbsF(this.yaw.GetInput()) > 0.9 {
          //   direction = this.yaw.GetInput();
          // }
          // if this.sys.playerComponent.trick == null  && AbsF(direction) > 0.9 {
          //   this.sys.playerComponent.trick = FlightTrickAileronRoll.Create(this.sys.playerComponent, Cast<Float>(RoundF(direction)));
          // }
          // this.SetupActions();
        // }
        // if ListenerAction.IsButtonJustReleased(action) {
        //   FlightLog.Info("Options button released");
        //   this.trick = false; 
        //   this.SetupActions();
        // }
      // }
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
      if Equals(actionName, n"Flight_ModeSwitchForward") && ListenerAction.IsButtonJustPressed(action) && (this.showOptions || this.player.PlayerLastUsedKBM()) {
        this.CycleMode(1);
        this.SetupActions();
      }
      if Equals(actionName, n"Flight_ModeSwitchBackward") && ListenerAction.IsButtonJustPressed(action) && (this.showOptions || this.player.PlayerLastUsedKBM()) {
        this.CycleMode(-1);
        this.SetupActions();
      }
      if this.showOptions && Equals(actionName, n"Flight_UIToggle") && ListenerAction.IsButtonJustPressed(action) {
          this.showUI = !this.showUI;
          if (this.showUI) {
            this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, true, true);
            this.ShowSimpleMessage("Flight UI Shown");
          } else {
            this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, false, true);
            this.ShowSimpleMessage("Flight UI Hidden");
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
            // if this.trick {
            //   if this.sys.playerComponent.trick == null  && AbsF(value) > 0.9 {
            //     this.sys.playerComponent.trick = FlightTrickAileronRoll.Create(this.sys.playerComponent, Cast<Float>(RoundF(value)));
            //   }
            //   this.yaw.SetInput(0.0);
            //   this.sway.SetInput(0.0);
            // } else {
              if this.showOptions {
                this.sway.SetInput(value);
                // this.yaw.SetInput(0.0);
              } else {
                // this.sway.SetInput(0.0);
                this.yaw.SetInput(value);
              }
            // }
            break;
          case n"Sway":
            this.sway.SetInput(value);
            break;
          case n"Lift":
            if this.trick {
              this.lift.SetInput(0.0);
            } else {
              this.lift.SetInput(value);
            }
            break;
          case n"SurgeNeg":
            this.surge.SetInput(-value);
            break;
          default:
            // return false;
            break;
        }
      }
        // ListenerActionConsumer.ConsumeSingleAction(consumer);
      if Equals(actionName, n"Flight_LinearBrake") {
        if Equals(actionType, gameinputActionType.BUTTON_PRESSED) {
          this.linearBrake.SetInput(1.0);
        } else {
          this.linearBrake.SetInput(0.0);
        }
      }
      if Equals(actionName, n"Flight_AngularBrake") {
        if Equals(actionType, gameinputActionType.BUTTON_PRESSED) {
          this.angularBrake.SetInput(1.0);
        } else {
          this.angularBrake.SetInput(0.0);
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

  public func UpdateInputs(timeDelta: Float) -> Void {
    this.yaw.GetValue(timeDelta);
    this.sway.GetValue(timeDelta);
    this.roll.GetValue(timeDelta);
    this.pitch.GetValue(timeDelta);
    this.lift.GetValue(timeDelta);
    this.linearBrake.GetValue(timeDelta);
    this.angularBrake.GetValue(timeDelta);
    this.surge.GetValue(timeDelta);
  }


  public func SetupPositionProviders() -> Void {
    this.sys.audio.AddSlotProviders(GetMountedVehicle(this.player));
  }

  // protected cb func PhysicsUpdate(evt: ref<vehicleFlightPhysicsUpdateEvent>) -> Bool {
    // FlightLog.Info("in the physics update!");
  // }

  // public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  public final func OnUpdate(timeDelta: Float) -> Void {

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

    // this.isInAnyMenu = this.uiBlackboard.GetBool(this.uiSystemBB.IsInMenu);
    
    // if !this.active {
    //   if (this.showUI) { 
    //     this.ui.ClearMarks();
    //   }
    //   return;
    // }
    // might need to handle just the scanning system's dilation, and the pause menu
    // if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"radial") {
    //   // this might happpen?
    //   timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("radialMenu", "timeDilation");
    //   //FlightLog.Info("Radial menu dilation"); 
    // } else {
    //   if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive() {
    //     // i think this is what this is called
    //     timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("focusModeTimeDilation", "timeDilation");
    //     //FlightLog.Info("Other time dilation"); 
    //   }
    // }

    // let player: ref<PlayerPuppet> = GetPlayer(this.gameInstance);
    // if !IsDefined(this.GetVehicle()) { 
    //   // if IsDefined(scriptInterface.owner as VehicleObject) {
    //   //   this.stats = FlightStats.Create(scriptInterface.owner as VehicleObject);
    //   //   FlightLog.Warn("Vehicle undefined. Redefined to " + this.GetVehicle().GetDisplayName()); 
    //   // } else {
    //     FlightLog.Error("Owner not defined"); 
    //     return;
    //   // }
    // }
    // if !this.GetVehicle().IsPlayerMounted() { 
    //   FlightLog.Error("Vehicle is not player mounted"); 
    //   return; 
    // }

    // if (this.showUI) { 
    //   // this.navPath.Update();
    //   this.ui.ClearMarks();
    // }

    this.UpdateInputs(timeDelta);

    // if (this.showUI) {
      // this.ui.Update(timeDelta);
      // this.ui.DrawMark(this.stats.d_visualPosition);
      // this.ui.DrawText(this.stats.d_visualPosition, FloatToStringPrec(1.0 / this.timeDelta, 4));
    // }

    // this.timeDelta = timeDelta * 0.001 + this.timeDelta * 0.999;
  }

  public func ShowSimpleMessage(message: String) -> Void {
    let msg: SimpleScreenMessage;
    msg.isShown = true;
    msg.duration = 2.00;
    msg.message = message;
    msg.isInstant = true;
    GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_Notifications).SetVariant(GetAllBlackboardDefs().UI_Notifications.OnscreenMessage, ToVariant(msg), true);
  }

  public static func CreateInputHint(label: String, action: CName) -> InputHintData {
    let data: InputHintData;
    data.source = n"FlightController";
    data.action = action;
    data.localizedLabel = label;
    // data.groupId = n"FlightController";
    return data;
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

// @addMethod(AudioSystem)
// public final func PlayFlightSound(sound: CName) -> Void {
//   this.Play(sound);
// }

// @addField(PlayerPuppet)
// public let flightController: ref<FlightController>; // Must be strong reference

// @addMethod(PlayerPuppet)
// public func GetFlightController() -> ref<FlightController> {
//   return this.flightController;
// }

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

// @wrapMethod(Ground)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return wrappedMethod(stateContext, scriptInterface) || FlightController.GetInstance().IsActive();
// }

// @wrapMethod(Air)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return wrappedMethod(stateContext, scriptInterface) && !FlightController.GetInstance().IsActive();
// }

// might be good to replace this
// @wrapMethod(ReactionManagerComponent)
// private final func ShouldStimBeProcessedByCrowd(stimEvent: ref<StimuliEvent>) -> Bool {

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


// FlightControllerUI.reds

// handle this somewhere
// this.m_tppBBConnectionId = this.m_activeVehicleUIBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this, n"OnCameraModeChanged");

// public static func OperatorAssignAdd(out a: array<ref<Elem>>, b: ref<Text>) -> array<ref<Elem>> {
//   ArrayPush(a, b as Elem);
//   return a;
// }

public class FlightControllerUI extends inkCanvas {
  public let controller: ref<inkGameController>;
  public let stats: ref<FlightStats>;
  private let m_rootAnim: ref<inkAnimProxy>;
  private let m_markerRadius: Float;
  private let m_lastPitchValue: Float;

  private let m_arrow_forward: ref<inkImage>;
  private let m_arrow_left: ref<inkImage>;
  private let m_arrow_right: ref<inkImage>;
  private let m_arrow_left_rear: ref<inkImage>;
  private let m_arrow_right_rear: ref<inkImage>;
  private let m_info: ref<inkCanvas>;
  private let m_roll_markers: array<ref<inkRectangle>>;
  private let m_pitch: ref<inkCanvas>;
  private let m_marks: ref<inkCanvas>;
  let updateFrequency: Float;

  public static func Create(controller: ref<inkGameController>, parent: ref<inkCompoundWidget>) -> ref<FlightControllerUI> {
    let instance = new FlightControllerUI();
    instance.controller = controller;
    instance.Reparent(parent);
    instance.SetName(n"flightControllerUI");
    FlightController.GetInstance().SetUI(instance);
    instance.updateFrequency = 0.01;
    return instance;
  }
  public final const func GetVehicle() -> wref<VehicleObject> {
    if !Equals(this.stats, null) {
      return this.stats.vehicle;
    } else {
      return null;
    }
  }
  public func Setup(stats: ref<FlightStats>) -> Void {

    // let box_bg_image = Image.New(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas", n"frame_top_bg");
    // let box_text = Text.New("Howdy");    
    // // let box_text2 = Text.New("Hi there");

    // let box = Box.New([box_text as Elem]);
    // // // box.children += box_text2;
    // box.BackgroundImage(box_bg_image);
    // // RenderElem(box, new Vector2(500.0, 500.0)).Reparent(this);

    this.stats = stats;
    this.SetOpacity(0.0);
    this.RemoveAllChildren();
		this.SetAnchor(inkEAnchor.TopLeft);
    
    // let w: ref<inkWidget> = this.controller.SpawnFromExternal(this, r"base\\gameplay\\gui\\widgets\\turret_hud\\turret_hud.inkwidget", n"Root");
    // w.Reparent(this);
    // return;

    // this.m_arrow_forward = inkWidgetBuilder.inkImage(n"arrow_forward")
		//   .Reparent(this)
		//   .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		//   .Part(n"arrow_right")
		//   .Size(24.0, 24.0)
    //   .Anchor(1.0, 0.5)
		//   .Opacity(1.0)
		//   .Tint(FlightUtils.ElectricBlue())
    //   .BuildImage();

    // // wheel markers

    // this.m_arrow_left = inkWidgetBuilder.inkImage(n"arrow_left")
		//   .Reparent(this)
		//   .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		//   .Part(n"arrow_right")
		//   .Size(24.0, 24.0)
    //   .Anchor(1.0, 0.5)
		//   .Opacity(1.0)
		//   .Tint(FlightUtils.ElectricBlue())
    //   .BuildImage();

    // this.m_arrow_right = inkWidgetBuilder.inkImage(n"arrow_right")
		//   .Reparent(this)
		//   .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		//   .Part(n"arrow_right")
		//   .Size(24.0, 24.0)
    //   .Anchor(1.0, 0.5)
		//   .Opacity(1.0)
		//   .Tint(FlightUtils.ElectricBlue())
    //   .BuildImage();

    // this.m_arrow_left_rear = inkWidgetBuilder.inkImage(n"arrow_left_rear")
		//   .Reparent(this)
		//   .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		//   .Part(n"arrow_right")
		//   .Size(24.0, 24.0)
    //   .Anchor(1.0, 0.5)
		//   .Opacity(1.0)
		//   .Tint(FlightUtils.ElectricBlue())
    //   .BuildImage();

    // this.m_arrow_right_rear = inkWidgetBuilder.inkImage(n"arrow_right_rear")
		//   .Reparent(this)
		//   .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		//   .Part(n"arrow_right")
		//   .Size(24.0, 24.0)
    //   .Anchor(1.0, 0.5)
		//   .Opacity(1.0)
		//   .Tint(FlightUtils.ElectricBlue())
    //   .BuildImage();

    // roll markers

    let rulers = inkWidgetBuilder.inkCanvas(n"rulers")
      .Reparent(this)
	    .Anchor(inkEAnchor.Centered)
      .Opacity(0.5)
      .Margin(0.0, 0.0, 0.0, 0.0)
      .Translation(0.0, 230.0)
      .BuildCanvas();
  
    let marks = [-30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30];
    this.m_markerRadius = 820.0;
    let points: array<Vector2>;
    for mark in marks {
      ArrayPush(this.m_roll_markers, inkWidgetBuilder.inkRectangle(StringToName("roll_marker_" + mark))
        .Tint(FlightUtils.ElectricBlue())
        .Opacity(1.0)
        .Size(3.0, (mark == 0) ? 50.0 : 30.0)
        .Anchor(0.5, 0.0)
        .Translation(CosF(Deg2Rad(Cast<Float>(mark) - 90.0)) * this.m_markerRadius, SinF(Deg2Rad(Cast<Float>(mark) - 90.0)) * this.m_markerRadius)
        .Rotation(Cast<Float>(mark) + 180.0)
        .Reparent(rulers)
        .BuildRectangle());
    }
    let offset = 20.0;
    let i = -40;
    while i <= 40 {
      ArrayPush(points, new Vector2(CosF(Deg2Rad(Cast<Float>(i)  - 90.0)) * (this.m_markerRadius + offset), SinF(Deg2Rad(Cast<Float>(i) - 90.0)) * (this.m_markerRadius + offset)));
      i += 1;
    }

    let arc = inkWidgetBuilder.inkShape(n"arc")
      .Reparent(rulers)
      .Size(1024.0, 1024.0)
      .UseNineSlice(true)
      .ShapeVariant(inkEShapeVariant.FillAndBorder)
      .LineThickness(3.0)
      .FillOpacity(0.0)
      .Tint(FlightUtils.ElectricBlue())
      .BorderColor(FlightUtils.ElectricBlue())
      .BorderOpacity(1.0)
      .EndCapStyle(inkEEndCapStyle.SQUARE)
      .Visible(true)
      .BuildShape();
    arc.SetVertexList(points);
    arc.SetRenderTransformPivot(0.0, 0.0);
  

    inkWidgetBuilder.inkText(n"roll_text")
      .Reparent(rulers)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .Tint(FlightUtils.ElectricBlue())
      .Text("0.00")
		  .Opacity(1.0)
      .Anchor(0.5, 0.0)
      .BuildText();

    inkWidgetBuilder.inkRectangle(n"roll_marker_top")
      .Tint(FlightUtils.ElectricBlue())
      .Opacity(1.0)
      .Size(3.0, 70.0)
      .Anchor(0.5, 0.0)
      .Reparent(rulers)
      .BuildRectangle();

    this.SetupPitchDisplay();

    // info block

    this.m_info = inkWidgetBuilder.inkCanvas(n"info")
      .Size(500.0, 200.0)
      .Reparent(this)
	    .Anchor(inkEAnchor.CenterLeft)
      .Margin(0.0, 0.0, 0.0, 0.0)
      .BuildCanvas();
    // info.SetEffectEnabled(inkEffectType.Glitch, n"Glitch_0", true);
    // info.SetEffectParamValue(inkEffectType.Glitch, n"Glitch_0", n"intensity", 1.0);
		// altitude.SetAnchorPoint(new Vector2(0.5, 0.5));

    let top = new inkHorizontalPanel();
		top.SetName(n"top");
		top.SetSize(500.0, 18.0);
    top.Reparent(this.m_info);
		// top.SetHAlign(inkEHorizontalAlign.Left);
		// top.SetVAlign(inkEVerticalAlign.Top);
    top.SetMargin(0.0, 0.0, 0.0, 0.0);

    let manufacturer = this.stats.vehicle.GetRecord().Manufacturer().EnumName();
    let iconRecord = TweakDBInterface.GetUIIconRecord(TDBID.Create("UIIcon." + manufacturer));
    inkWidgetBuilder.inkImage(n"manufacturer")
      .Reparent(top)
      .Part(iconRecord.AtlasPartName())
      .Atlas(iconRecord.AtlasResourcePath())
		  .Size(174.0, 18.0)
      .Margin(20.0, 0.0, 0.0, 0.0)
		  .Opacity(1.0)
		  .Tint(FlightUtils.Bittersweet())
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .BuildImage();

    inkWidgetBuilder.inkImage(n"fluff")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"fluff_01")
      .Tint(FlightUtils.Bittersweet())
      .Opacity(1.0)
      .Margin(10.0, 0.0, 0.0, 0.0)
		  .Size(174.0, 18.0)
      .Anchor(inkEAnchor.LeftFillVerticaly)
      .Reparent(top)
      .BuildImage();

    inkWidgetBuilder.inkImage(n"fluff2")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"p20_sq_element")
      .Tint(FlightUtils.Bittersweet())
      .Opacity(1.0)
      .Margin(10.0, 0.0, 0.0, 0.0)
		  .Size(18.0, 18.0)
      .Anchor(inkEAnchor.LeftFillVerticaly)
      .Reparent(top)
      .BuildImage();

    let panel = inkWidgetBuilder.inkFlex(n"panel")
      .Margin(0.0, 24.0, 0.0, 0.0)
      .Padding(10.0, 10.0, 10.0, 10.0)
		  .FitToContent(true)
      .Reparent(this.m_info)
		  .Anchor(inkEAnchor.Fill)
      .BuildFlex();
    
   inkWidgetBuilder.inkImage(n"background")
      .Reparent(panel)
      .Anchor(inkEAnchor.Fill)
      .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
      .Part(n"frame_top_bg")
      .NineSliceScale(true)
      .Margin(0.0, 0.0, 0.0, 0.0)
      .Tint(FlightUtils.PureBlack())
      .Opacity(0.1)
      .BuildImage();
    // .BindProperty(n"tintColor", n"Briefings.BackgroundColour");

    inkWidgetBuilder.inkImage(n"frame")
      .Reparent(panel)
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .Atlas(r"base\\gameplay\\gui\\widgets\\crosshair\\smart_rifle\\arghtlas_smargun.inkatlas")
		  .Part(n"smartFrameTopLeft")
		  .NineSliceScale(true)
      .Margin(new inkMargin(0.0, 0.0, 0.0, 0.0))
		  .Tint(FlightUtils.ElectricBlue())
		  .Opacity(1.0)
      .BuildImage();
    //  .BindProperty(n"tintColor", n"Briefings.BackgroundColour");
    
    inkWidgetBuilder.inkText(n"position")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
      .Text("You shouldn't see this!")
      .Margin(20.0, 15.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkText(n"velocity")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
      .Text("You shouldn't see this!")
      .Margin(20.0, 40.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    // marks canvas

    this.m_marks = inkWidgetBuilder.inkCanvas(n"marks")
      .Reparent(this)
      .Anchor(inkEAnchor.Centered)
      .BuildCanvas();
  }

  private func UpdateRollSplay(factor: Float) -> Void {
    let indexes = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
    let marks = [-30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30];
    // let points: array<Vector2>;
    for i in indexes {
      let mark_scale = factor * Cast<Float>(marks[i]);
      this.m_roll_markers[i].SetTranslation(CosF(Deg2Rad(mark_scale - 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark_scale - 90.0)) * this.m_markerRadius);
      this.m_roll_markers[i].SetRotation(mark_scale + 180.0);
      // ArrayPush(points, new Vector2(CosF(Deg2Rad(mark_scale - 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark_scale - 90.0)) * this.m_markerRadius));
    }

    // (this.GetWidget(n"rulers/arc") as inkShape).ChangeShape(n"Rectangle");
    // (this.GetWidget(n"rulers/arc") as inkShape).SetVertexList(points);
    // (this.GetWidget(n"rulers/arc") as inkShape).SetVisible(true);
    (this.GetWidget(n"rulers/arc") as inkShape).SetScale(new Vector2(1.0 + (factor - 1.0) * 0.05, 1.0 + (factor - 1.0) * 0.05));
  }

  private func UpdatePitchDisplayHeight(factor: Float) -> Void {
    this.m_pitch.SetSize(60.0, 520.0 + (factor - 1.0) * 1000.0);
  }

  private func SetupPitchDisplay() -> Void {

      let mark_scale = 20.0;
      let height = 520.0;
      let width = 60.0;

      this.m_pitch = inkWidgetBuilder.inkCanvas(n"this.m_pitch")
        .Size(width, height)
        .Reparent(this)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.Centered)
        .Margin(0.0, 0.0, 0.0, 0.0)
        .Translation(-920.0, 230.0)
        .Opacity(0.5)
        .BuildCanvas();
      // this.m_pitch.SetChildOrder(inkEChildOrder.Backward);

      inkWidgetBuilder.inkImage(n"arrow")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_right_bg")
        .Size(20.0, 20.0)
        .Anchor(1.0, 0.5)
        .Anchor(inkEAnchor.CenterLeft)
        .Margin(-15.0, 0.0, 0.0, 0.0)
        .Opacity(1.0)
        .Tint(FlightUtils.ElectricBlue())
        .BuildImage();
        
      inkWidgetBuilder.inkText(n"fluff_text")
        .Reparent(this.m_pitch)
        .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
        .FontSize(12)
        .Anchor(0.0, 1.0)
        .Anchor(inkEAnchor.TopLeft)
        .Tint(FlightUtils.Bittersweet())
        .Text("89V_PITCH")
        .HAlign(inkEHorizontalAlign.Left)
        .Margin(-14.0, -10.0, 0.0, 0.0)
        // .Overflow(textOverflowPolicy.AdjustToSize) pArrayType was nullptr.
        .BuildText();

      inkWidgetBuilder.inkImage(n"border")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_cell_fg")
        .Size(width + 24.0, height)
        .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.CenterFillVerticaly)
        .Translation(-2.5, 0.0)
        .Opacity(1.0)
        .Tint(FlightUtils.ElectricBlue())
        .BuildImage();

      inkWidgetBuilder.inkImage(n"fill")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_cell_bg")
        .Size(width + 24.0, height)
        .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.CenterFillVerticaly)
        .Translation(-2.5, 0.0)
        .Opacity(0.1)
        .Tint(FlightUtils.PureBlack())
        .BuildImage();

      inkWidgetBuilder.inkMask(n"mask")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\quests\\q000\\aerondight_rayfield\\assets\\logo_and_mask.inkatlas")
        .Part(n"Maska_textura")
        // .Size(width, height)
        // .Opacity(1.0)     
        .FitToContent(true)
        // .MaskTransparency(1.0)
        // .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        // .InvertMask(true)
        .Anchor(inkEAnchor.Centered)
        // .MaskSource(inkMaskDataSource.TextureAtlas)
        .BuildMask();
      // mask.SetDynamicTextureMask(n"fill");
      // mask.SetEffectEnabled(inkEffectType.Mask, n"Mask_0", true);
      // mask.SetEffectParamValue(inkEffectType.Mask, n"Mask_0", n"opacity", 1.0);

      // FlightLog.Info("Does arrow_cell_bg exist on the mask atlas? " + ToString(mask.IsTexturePartExist(n"arrow_cell_bg")));

      let markers = inkWidgetBuilder.inkCanvas(n"markers")
        .Size(width, 180.0 * mark_scale)
        .Reparent(this.m_pitch)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.Centered)
        .Margin(0.0, 0.0, 0.0, 0.0)
        .BuildCanvas();
      markers.CreateEffect(n"inkBoxBlurEffect", n"BoxBlur_0");
      markers.SetEffectEnabled(inkEffectType.BoxBlur, n"BoxBlur_0", true);
      markers.SetEffectParamValue(inkEffectType.BoxBlur, n"BoxBlur_0", n"intensity", 0.0);
      markers.SetBlurDimension(n"BoxBlur_0", inkEBlurDimension.Vertical);
      // markers.SetEffectEnabled(inkEffectType.Mask, n"Mask_0", true);
		  // markers.SetRenderTransformPivot(new Vector2(0.0, 0.0));

      let midbar_size = 16.0;
      let marks: array<Float> = [-100.0, -90.0, -80.0, -70.0, -60.0, -50.0, -40.0, -30.0, -20.0, -10.0, 0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0];
      let marks_inc: array<Float> = [-4.0, -3.0, -2.0, -1.0, 1.0, 2.0, 3.0, 4.0, 5.0];

      inkWidgetBuilder.inkRectangle(n"m1_00000")
        .Tint(FlightUtils.ElectricBlue())
        // .Opacity(mark == 0.0 ? 1.0 : 0.5)
        .Opacity(1.0)
        .Reparent(markers)
        .Size(width, 19.0)
        .Anchor(0.0, 0.0)
        .Translation(0.0, 90.0 * mark_scale - 20.0)
        .BuildRectangle();

      inkWidgetBuilder.inkRectangle(n"m1_00001")
        .Tint(FlightUtils.ElectricBlue())
        // .Opacity(mark == 0.0 ? 1.0 : 0.5)
        .Opacity(1.0)
        .Reparent(markers)
        .Size(width, 19.0)
        .Anchor(0.0, 1.0)
        .Translation(0.0, 90.0 * mark_scale + 20.0)
        .BuildRectangle();

      // let text = inkWidgetBuilder.inkText(n"text")
      //   .Reparent(markers)
      //   .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily", n"Heavy")
      //   .FontSize(16)
      //   .Anchor(0.5, 0.5)
      //   .Tint(FlightUtils.PureBlack())
      //   .Text("LEVEL")
      //   .Opacity(1.0)
      //   .HAlign(inkEHorizontalAlign.Center)
      //   .VAlign(inkEVerticalAlign.Center)
      //   .Margin(0.0, 0.0, 0.0, 0.0)
      //   .Translation(width / 2.0, 90.0 * mark_scale)
      //   // .Overflow(textOverflowPolicy.AdjustToSize)
      //   .BuildText();

      for mark in marks {
        if mark != 0.0 {
          inkWidgetBuilder.inkText(n"text")
            .Reparent(markers)
            .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
            .FontSize(20)
            .Anchor(0.5, 0.5)
            .Tint(FlightUtils.ElectricBlue())
            .Text(FloatToStringPrec(AbsF(mark), 0))
            .HAlign(inkEHorizontalAlign.Center)
            .Margin(0.0, 0.0, 0.0, 0.0)
            .Translation(width / 2.0, (mark + 90.0) * mark_scale)
            // .Overflow(textOverflowPolicy.AdjustToSize)
            .BuildText();

          inkWidgetBuilder.inkRectangle(StringToName("m1_" + FloatToString(mark)))
            .Tint(FlightUtils.ElectricBlue())
            // .Opacity(mark == 0.0 ? 1.0 : 0.5)
            .Size(midbar_size, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(width - midbar_size, (mark + 90.0) * mark_scale)
            .Reparent(markers)
            .BuildRectangle();

          inkWidgetBuilder.inkRectangle(StringToName("m2_" + FloatToString(mark)))
            .Tint(FlightUtils.ElectricBlue())
            // .Opacity(mark == 0.0 ? 1.0 : 0.5)
            .Size(midbar_size, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(0.0, (mark + 90.0) * mark_scale)
            .Reparent(markers)
            .BuildRectangle();
        }
        for mark_inc in marks_inc {
          inkWidgetBuilder.inkRectangle(StringToName("m_" + FloatToString(mark + mark_inc)))
            .Tint(FlightUtils.ElectricBlue())
            .Opacity(mark_inc == 5.0 ? 0.25 : 0.05)
            .Size(width, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(0.0, ((mark + 90.0) + mark_inc) * mark_scale)
            .Reparent(markers)
            .BuildRectangle();
        }
      } 
  }

  public func ClearMarks() -> Void {
    this.m_marks.RemoveAllChildren();
  }

  public func GetMarksWidget() -> ref<inkCanvas> {
    return this.m_marks;
  }

  public func DrawMark(position: Vector4) -> Void {
    inkWidgetBuilder.inkImage(StringToName("marker_" + ToString(RandF())))
      .Reparent(this.m_marks)
      .Atlas(r"base\\gameplay\\gui\\widgets\\crosshair\\master_crosshair.inkatlas")
      .Part(n"lockon-b")
      .Tint(FlightUtils.ElectricBlue())
      .Opacity(0.5)
      .Size(10.0, 10.0)
      .Anchor(0.5, 0.5)
      .Translation(this.ScreenXY(position))
      .BuildImage();
  }

  public func DrawText(position: Vector4, text: String) -> Void {
    inkWidgetBuilder.inkText(StringToName("text_" + ToString(RandF())))
      .Reparent(this.m_marks)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .Tint(FlightUtils.ElectricBlue())
      .Opacity(0.2)
      .Anchor(0.0, 0.5)
      .Text(text)
      .Translation(this.ScreenXY(position, 15.0, 0.0))
      .BuildText();
  }

  public func Show() -> Void {
    
    let startValue = this.GetOpacity();
    let duration = 1.0 - startValue;
    if IsDefined(this.m_rootAnim) && this.m_rootAnim.IsPlaying() {
      this.m_rootAnim.Stop();
    };
    this.m_rootAnim = this.PlayAnimation(InkAnimHelper.GetDef_Transparency(startValue, 1.0, duration * 2.0, 0.0, inkanimInterpolationType.Quadratic, inkanimInterpolationMode.EasyInOut));

    let animSelect = new inkAnimDef();
    let animEffectInterp = new inkAnimEffect();
    animEffectInterp.SetStartDelay(0.00);
    animEffectInterp.SetEffectType(inkEffectType.Glitch);
    animEffectInterp.SetEffectName(n"Glitch_0");
    animEffectInterp.SetParamName(n"intensity");
    animEffectInterp.SetStartValue(1.0);
    animEffectInterp.SetEndValue(0.0);
    animEffectInterp.SetDuration(duration * 2.0);
    animSelect.AddInterpolator(animEffectInterp);
    let info_anim = this.m_info.PlayAnimation(animSelect);
    info_anim.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnInfoAnimationCompleted");
  }

  public func Hide() -> Void {    
    let startValue = this.GetOpacity();
    let duration = startValue;
    if IsDefined(this.m_rootAnim) && this.m_rootAnim.IsPlaying() {
      this.m_rootAnim.Stop();
    };
    this.m_rootAnim = this.PlayAnimation(InkAnimHelper.GetDef_Transparency(startValue, 0.0, duration * 0.5, 0.0, inkanimInterpolationType.Quadratic, inkanimInterpolationMode.EasyInOut));
    this.m_rootAnim.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnHideAnimationCompleted");
  } 
   
  protected cb func OnHideAnimationCompleted(anim: ref<inkAnimProxy>) -> Bool {
    if (FlightController.GetInstance().IsActive()) {
      this.HideInfo();
    } else {
      // FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, false, true);
      // FlightController.GetInstance().GetBlackboard().SignalBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive);
    }
  }

  protected cb func OnInfoAnimationCompleted(anim: ref<inkAnimProxy>) -> Bool {
    this.HideInfo();
  }

  public func HideInfo() -> Void {
    this.m_info.PlayAnimation(InkAnimHelper.GetDef_Transparency(1.0, 0.0, 2.0, 5.0, inkanimInterpolationType.Quadratic, inkanimInterpolationMode.EasyInOut));
  }

  public func ShowInfo() -> Void {
    let info_anim = this.m_info.PlayAnimation(InkAnimHelper.GetDef_Transparency(0.0, 1.0, 1.0, 0.0, inkanimInterpolationType.Quadratic, inkanimInterpolationMode.EasyInOut));
    info_anim.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnInfoAnimationCompleted");
  }

  public func Update(timeDelta: Float) -> Void {
    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(FlightController.GetInstance().gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);
    // (this.GetWidget(n"fps") as inkText).SetText(FloatToStringPrec(1.0 / timeDelta));
    (this.GetWidget(n"info/panel/position") as inkText).SetText("Location: < " + FloatToStringPrec(this.stats.d_position.X, 1)
      + ", " + FloatToStringPrec(this.stats.d_position.Y, 1) + ", " + FloatToStringPrec(this.stats.d_position.Z, 1) + " >");
    (this.GetWidget(n"info/panel/velocity") as inkText).SetText("Velocity: < " + FloatToStringPrec(this.stats.d_velocity.X, 1)
      + ", " + FloatToStringPrec(this.stats.d_velocity.Y, 1) + ", " + FloatToStringPrec(this.stats.d_velocity.Z, 1) + " >");
      // + "Velocity: <" + FloatToStringPrec(this.stats.d_velocity.X, 1)
      // + ", " + FloatToStringPrec(this.stats.d_velocity.Y, 1) + ", " + FloatToStringPrec(this.stats.d_velocity.Z, 1) + " >");
    // this.m_info.SetTranslation(this.ScreenXY(this.stats.d_visualPosition + Transform.GetRight(cameraTransform) * 2.5));

    // let fl_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_left_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    // let fr_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_right_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
   
    // let bl_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_left_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    // let br_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_right_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;

    // let screen_position: Vector4 = this.stats.d_position - this.stats.d_velocity * timeDelta;
    // this.m_arrow_forward.SetTranslation(this.ScreenXY(screen_position + this.stats.d_forward * 3.0));
    // this.m_arrow_forward.SetRotation(this.ScreenAngle(screen_position, screen_position + this.stats.d_forward));
    // this.m_arrow_forward.SetOpacity(this.OpacityForPosition(screen_position + this.stats.d_forward * 3.0));

    // this.m_arrow_left.SetTranslation(this.ScreenXY(fl_position - this.stats.d_right * 0.5));
    // this.m_arrow_left.SetRotation(this.ScreenAngle(fl_position, fl_position + this.stats.d_right * 0.5));
    // this.m_arrow_left.SetOpacity(this.OpacityForPosition(fl_position - this.stats.d_right * 0.5));

    // this.m_arrow_right.SetTranslation(this.ScreenXY(fr_position + this.stats.d_right * 0.5));
    // this.m_arrow_right.SetRotation(this.ScreenAngle(fr_position, fr_position - this.stats.d_right * 0.5));
    // this.m_arrow_right.SetOpacity(this.OpacityForPosition(fr_position + this.stats.d_right * 0.5));

    // this.m_arrow_left_rear.SetTranslation(this.ScreenXY(bl_position - this.stats.d_right * 0.5));
    // this.m_arrow_left_rear.SetRotation(this.ScreenAngle(bl_position, bl_position + this.stats.d_right * 0.5));
    // this.m_arrow_left_rear.SetOpacity(this.OpacityForPosition(bl_position - this.stats.d_right * 0.5));

    // this.m_arrow_right_rear.SetTranslation(this.ScreenXY(br_position + this.stats.d_right * 0.5));
    // this.m_arrow_right_rear.SetRotation(this.ScreenAngle(br_position, br_position - this.stats.d_right * 0.5));
    // this.m_arrow_right_rear.SetOpacity(this.OpacityForPosition(br_position + this.stats.d_right * 0.5));
 
 
    // this.GetWidget(n"ruler_left").SetTranslation(this.ScreenXY(this.stats.d_position - Transform.GetUp(cameraTransform) * 2.0));
    // this.GetWidget(n"ruler_left").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position - Transform.GetUp(cameraTransform) * 2.0));
 
    // this.GetWidget(n"ruler_right").SetTranslation(this.ScreenXY(this.stats.d_position + Transform.GetUp(cameraTransform) * 2.0));
    // this.GetWidget(n"ruler_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + Transform.GetUp(cameraTransform) * 2.0));

    let pitch_mark = Vector4.GetAngleBetween(this.stats.d_forward, new Vector4(0.0, 0.0, 1.0, 0.0));
    this.GetWidget(n"pitch/markers").SetTranslation(0.0, (pitch_mark - 90.0) * 20.0);
    this.GetWidget(n"pitch/markers").SetEffectParamValue(inkEffectType.BoxBlur, n"BoxBlur_0", n"intensity", AbsF(pitch_mark - this.m_lastPitchValue) / timeDelta * 0.0001);
    this.m_lastPitchValue = pitch_mark;

    // let marker_vector = Vector4.RotateAxis(Transform.GetRight(cameraTransform), Transform.GetForward(cameraTransform), Deg2Rad(Vector4.GetAngleBetween(this.stats.d_right, new Vector4(0.0, 0.0, 1.0, 0.0))));
    // this.GetWidget(n"rulers").SetTranslation(this.ScreenXY(this.stats.d_position - this.stats.d_velocity * timeDelta));

    let splay = 1.0 + 0.2 * MaxF(0.0, FlightController.GetInstance().surge.GetValue()) - 0.2 * FlightController.GetInstance().linearBrake.GetValue() + ((RandF() * 0.02 - 0.01) * this.stats.d_speedRatio);
    let mark = Vector4.GetAngleBetween(this.stats.d_right, new Vector4(0.0, 0.0, 1.0, 0.0)) + 90.0;
    let mark_effected = mark * splay;
    this.UpdateRollSplay(splay);
    this.UpdatePitchDisplayHeight(splay);

    this.GetWidget(n"rulers/roll_marker_top").SetTranslation(CosF(Deg2Rad(mark_effected + 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark_effected + 90.0)) * this.m_markerRadius);
    this.GetWidget(n"rulers/roll_marker_top").SetRotation(mark_effected);

    (this.GetWidget(n"rulers/roll_text") as inkText).SetText(((mark - 180.0) > 0.0 ? "+" : "") + FloatToStringPrec(mark - 180.0, 2) + "°");
    this.GetWidget(n"rulers/roll_text").SetRotation(mark_effected - 180.0);    
    this.GetWidget(n"rulers/roll_text").SetTranslation(CosF(Deg2Rad(mark_effected + 90.0)) * (this.m_markerRadius + 30.0), SinF(Deg2Rad(mark_effected + 90.0)) * (this.m_markerRadius + 30.0));

 
    // this.GetWidget(n"roll_marker_right").SetTranslation(this.ScreenXY(this.stats.d_position + marker_vector * 2.0));
    // this.GetWidget(n"roll_marker_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + marker_vector));
 
    // this.GetWidget(n"scaler_left").SetTranslation(this.ScreenXY(this.stats.d_position - Transform.GetRight(cameraTransform) * 4.0));
    // this.GetWidget(n"scaler_left").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position - Transform.GetRight(cameraTransform)));
 
    // this.GetWidget(n"scaler_right").SetTranslation(this.ScreenXY(this.stats.d_position + Transform.GetRight(cameraTransform) * 4.0));
    // this.GetWidget(n"scaler_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + Transform.GetRight(cameraTransform)));
 
  }

  // public func DrawSphere(position: Vector4, radius: Float) -> Void {
  //   let segments = 16;

  //   let point: Vector4;
  //   for (segment in segments) {
  //   }
  //   let normalLine = inkWidgetBuilder.inkShape(n"normalLine")
  //     .Reparent(this.ui.GetMarksWidget())
  //     .Size(1024.0, 1024.0)
  //     .UseNineSlice(true)
  //     .ShapeVariant(inkEShapeVariant.FillAndBorder)
  //     .LineThickness(3.0)
  //     .FillOpacity(0.0)
  //     .Tint(FlightUtils.ElectricBlue())
  //     .BorderColor(FlightUtils.ElectricBlue())
  //     .BorderOpacity(0.1)
  //     .Visible(true)
  //     .BuildShape();
  //   normalLine.SetVertexList([this.ui.ScreenXY(this.stats.d_visualPosition), this.ui.ScreenXY(this.stats.d_visualPosition + this.stats.d_up)]);
  //   this.ui.DrawMark(this.stats.d_visualPosition + this.stats.d_up);
  // }

  public func OpacityForPosition(position: Vector4) -> Float {
    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(FlightController.GetInstance().gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);
    let cameraForward = Transform.GetForward(cameraTransform);
    return 0.1 + MaxF(0.0, MinF(0.9, (Vector4.Dot(cameraTransform.position - position, cameraForward) - Vector4.Dot(cameraTransform.position - this.stats.d_position, cameraForward)) / 4.0 * 0.9));
  }

  public func ScreenXY(position: Vector4) -> Vector2 {
    return this.ScreenXY(position, 0.0, 0.0);
  }

  public func ScreenXY(position: Vector4, offsetX: Float, offsetY: Float) -> Vector2 {
    if IsDefined(this.controller) {
      let translation = this.controller.ProjectWorldToScreen(position);
      translation.X = translation.X * 1920.0 + offsetX;
      translation.Y = translation.Y * -1080.0 + offsetY;
      return translation;
    } else {
      return new Vector2(0.0, 0.0);
    }
  }

  public func ScreenAngle(a: Vector4, b: Vector4) -> Float {
    if IsDefined(this.controller) {
      let point_a = this.controller.ProjectWorldToScreen(a);
      let point_b = this.controller.ProjectWorldToScreen(b);
      return Rad2Deg(AtanF(point_a.Y - point_b.Y, point_b.X - point_a.X));
    } else {
      return 0.0;
    }
  }

}

// FlightDevice.reds

public class FlightDevice extends GameObject {
  public let flightController: ref<FlightController>;

  protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
    super.OnRequestComponents(ri);
    EntityRequestComponentsInterface.RequestComponent(ri, n"updateComponent", n"UpdateComponent", false);
    FlightLog.Info("[FlightDevice] OnRequestComponents");
  }

  protected cb func OnTakeControl(ri: EntityResolveComponentsInterface) -> Bool {
    super.OnTakeControl(ri);
    FlightLog.Info("[FlightDevice] OnTakeControl");
  }

  protected cb func OnGameAttached() -> Bool {
    super.OnGameAttached();
    FlightLog.Info("[FlightDevice] OnGameAttached");
  }

  protected cb func OnDetach() -> Bool {
    super.OnDetach();
    FlightLog.Info("[FlightDevice] OnDetach");
  }
}

// FlightEvents.reds

public abstract class vehicleFlightEvent extends Event {
  // public native let vehicle: ref<VehicleObject>;
}

// public native class vehicleFlightPhysicsUpdateEvent extends vehicleFlightEvent {
//   public native let timeDelta: Float;
// }

public class VehicleFlightActivationEvent extends vehicleFlightEvent {
}

public class VehicleFlightDeactivationEvent extends vehicleFlightEvent {
  let silent: Bool;
}

public class VehicleFlightUIActivationEvent extends vehicleFlightEvent {
  public let m_activate: Bool;
}


public class VehicleFlightModeChangeEvent extends vehicleFlightEvent {
  let mode: Int32;
}

// FlightFx.reds

public class FlightFx {
  private let sys: ref<FlightSystem>;
  public let component: ref<FlightComponent>;

  public let f_fx: ref<FxInstance>;
  public let b_fx: ref<FxInstance>;
  public let fl_fx: ref<FxInstance>;
  public let flb_fx: ref<FxInstance>;
  public let fr_fx: ref<FxInstance>;
  public let frb_fx: ref<FxInstance>;
  public let bl_fx: ref<FxInstance>;
  public let br_fx: ref<FxInstance>;

  public let bl_retroFx: ref<FxInstance>;
  public let br_retroFx: ref<FxInstance>;
  public let fl_retroFx: ref<FxInstance>;
  public let fr_retroFx: ref<FxInstance>;
  public let laserFx: ref<FxInstance>;
  public let laserPointFx: ref<FxInstance>;

  public let resource: FxResource;
  public let retroResource: FxResource;
  public let laser: FxResource;
  public let laserPoint: FxResource;

  let f_fx_wt: WorldTransform;
  let b_fx_wt: WorldTransform;
  let bl_fx_wt: WorldTransform;
  let br_fx_wt: WorldTransform;
  let fl_fx_wt: WorldTransform;
  let fr_fx_wt: WorldTransform;
  let flb_fx_wt: WorldTransform;
  let frb_fx_wt: WorldTransform;

  public let fl_thruster: ref<MeshComponent>;
  public let fr_thruster: ref<MeshComponent>;
  public let bl_thruster: ref<MeshComponent>;
  public let br_thruster: ref<MeshComponent>;

  public let chassis: ref<IComponent>;

  public let fc: array<ref<IComponent>>;
  public let bc: array<ref<IComponent>>;
  public let blc: array<ref<IComponent>>;
  public let brc: array<ref<IComponent>>;
  public let flc: array<ref<IComponent>>;
  public let flbc: array<ref<IComponent>>;
  public let frc: array<ref<IComponent>>;
  public let frbc: array<ref<IComponent>>;
  public let ui: ref<IPlacedComponent>;
  public let ui_info: ref<IPlacedComponent>;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightFx> {
    return new FlightFx().Initialize(component);
  }

  public func Initialize(component: ref<FlightComponent>) -> ref<FlightFx> {
    // FlightLog.Info("[FlightFx] Initialize");
    this.sys = component.sys;
    this.component = component;
    this.resource = Cast<FxResource>(r"user\\jackhumbert\\effects\\ion_thruster.effect");
    this.retroResource = Cast<FxResource>(r"user\\jackhumbert\\effects\\retro_thruster.effect");
    this.laser = Cast<FxResource>(r"user\\jackhumbert\\effects\\aim.effect");
    this.laserPoint = Cast<FxResource>(r"user\\jackhumbert\\effects\\sparks.effect");

    let vehicleComponent = this.component.GetVehicle().GetVehicleComponent();

    this.fl_thruster = vehicleComponent.FindComponentByName(n"ThrusterFL") as MeshComponent;
    this.fl_thruster.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.fl_thruster.Toggle(false);
    this.fl_thruster.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 180.0, 180.0)));

    this.fr_thruster = vehicleComponent.FindComponentByName(n"ThrusterFR") as MeshComponent;
    this.fr_thruster.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.fr_thruster.Toggle(false);
    this.fr_thruster.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -180.0)));

    this.bl_thruster = vehicleComponent.FindComponentByName(n"ThrusterBL") as MeshComponent;
    this.bl_thruster.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.bl_thruster.Toggle(false);
    this.bl_thruster.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 180.0, 180.0)));

    this.br_thruster = vehicleComponent.FindComponentByName(n"ThrusterBR") as MeshComponent;
    this.br_thruster.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.br_thruster.Toggle(false);
    this.br_thruster.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -180.0)));

    return this;
  }

  public func HasFWheel() -> Bool {
    return ArraySize(this.fc) > 0;
  }
  
  public func HasBWheel() -> Bool {
    return ArraySize(this.bc) > 0;
  }

  public func HasFLWheel() -> Bool {
    return ArraySize(this.flc) > 0;
  }
  
  public func HasFLBWheel() -> Bool {
    return ArraySize(this.flbc) > 0;
  }
  
  public func HasFRWheel() -> Bool {
    return ArraySize(this.frc) > 0;
  }
  
  public func HasFRBWheel() -> Bool {
    return ArraySize(this.frbc) > 0;
  }
  
  public func HasBLWheel() -> Bool {
    return ArraySize(this.blc) > 0;
  }
  
  public func HasBRWheel() -> Bool {
    return ArraySize(this.brc) > 0;
  }

  public func Start() {
    this.fc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front");
    if ArraySize(this.fc) == 0 {
      this.fc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_rot_set");
    }
    this.bc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_back");
    if ArraySize(this.bc) == 0 {
      this.bc = this.component.GetVehicle().GetComponentsUsingSlot(n"axle_back_wheel");
    }
    this.blc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_back_left");
    this.brc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_back_right");
    this.flc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_left");
    this.flbc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_left_b");
    this.frc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_right");
    this.frbc = this.component.GetVehicle().GetComponentsUsingSlot(n"wheel_front_right_b");

    
    let vehicleComponent = this.component.GetVehicle().GetVehicleComponent();
    this.chassis = vehicleComponent.FindComponentByName(n"chassis") as IComponent;
    if !IsDefined(this.chassis) {
      this.chassis = vehicleComponent.FindComponentByName(n"chasis") as IComponent;
    }

    this.ui = vehicleComponent.FindComponentByName(n"flight_screen") as IPlacedComponent;
    this.ui_info = vehicleComponent.FindComponentByName(n"flight_screen_info") as IPlacedComponent;

    this.HideWheelComponents();

    let wt = new WorldTransform();


    let effectTransform: WorldTransform;
    WorldTransform.SetPosition(effectTransform, this.component.stats.d_position);
    
    // if !IsDefined(this.laserFx) {
    //   this.laserFx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.laser, effectTransform);
    //   WorldTransform.SetPosition(wt, new Vector4(0.0, -0.5, 0.6, 0.0));
    //   this.laserFx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"FunGun", wt);
    // }
    // if !IsDefined(this.laserPointFx) {
    //   this.laserPointFx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.laserPoint, wt);
    // }

    // WorldTransform.SetPosition(wt, new Vector4(0.0, -10.0, 0.5, 0.0));
    // laserFx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.TargetPosition, n"FunGun", wt);
    // laser.SetBlackboardValue(n"alpha", 1.0);
    WorldTransform.SetPosition(wt, new Vector4(0.0, 0.0, -0.25, 0.0));
    let chassisOffset = (vehicleComponent.FindComponentByName(n"Chassis") as vehicleChassisComponent).GetLocalPosition();
    let vehicleSlots = this.component.GetVehicle().GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
    let vwt = Matrix.GetInverted(this.component.GetVehicle().GetLocalToWorld());
    if this.HasFWheel() {
      this.f_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.f_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_front", this.f_fx_wt);
      this.component.fl_tire.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.f_fx_wt)) * vwt);
      this.f_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterFront", wt);
    }
    if this.HasBWheel() {
      this.b_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.b_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_back", this.b_fx_wt);
      this.component.bl_tire.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.b_fx_wt)) * vwt);
      this.b_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"WheelAudioEmitterBack", wt);
    }
    if this.HasBLWheel() {
      this.bl_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.bl_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      this.bl_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"ThrusterBL", wt);
      this.bl_thruster.Toggle(true);

      let wt_retro: WorldTransform;
      // let p = this.component.bl_tire.GetLocalPosition();
      // p.Z = 0.0;
      // WorldTransform.SetPosition(wt_retro, p);
      WorldTransform.SetOrientation(wt_retro, EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -90.0)));
      this.bl_retroFx =  GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.retroResource, effectTransform);
      // this.bl_retroFx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", wt_retro);
      this.bl_retroFx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"ThrusterBL", wt_retro);
    }
    if this.HasBRWheel() {
      this.br_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.br_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      this.br_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"ThrusterBR", wt);
      this.br_thruster.Toggle(true);

      let wt_retro: WorldTransform;
      // let p = this.component.br_tire.GetLocalPosition();
      // p.Z = 0.0;
      // WorldTransform.SetPosition(wt_retro, p);
      WorldTransform.SetOrientation(wt_retro, EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -90.0)));
      this.br_retroFx =  GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.retroResource, effectTransform);
      // this.br_retroFx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", wt_retro);
      this.br_retroFx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"ThrusterBR", wt_retro);
    }
    if this.HasFLWheel() {
      this.fl_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.fl_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      this.fl_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"ThrusterFL", wt);
      this.fl_thruster.Toggle(true);

      let wt_retro: WorldTransform;
      // let p = this.component.fl_tire.GetLocalPosition();
      // p.Z = 0.0;
      // WorldTransform.SetPosition(wt_retro, p);
      WorldTransform.SetOrientation(wt_retro, EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -90.0)));
      this.fl_retroFx =  GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.retroResource, effectTransform);
      // this.fl_retroFx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", wt_retro);
      this.fl_retroFx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"ThrusterFL", wt_retro);
    }
    if this.HasFRWheel() {
      this.fr_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.fr_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      this.fr_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"ThrusterFR", wt);
      this.fr_thruster.Toggle(true);

      let wt_retro: WorldTransform;
      // let p = this.component.fr_tire.GetLocalPosition();
      // p.Z = 0.0;
      // WorldTransform.SetPosition(wt_retro, p);
      WorldTransform.SetOrientation(wt_retro, EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -90.0)));
      this.fr_retroFx =  GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.retroResource, effectTransform);
      // this.fr_retroFx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", wt_retro);
      this.fr_retroFx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"ThrusterFR", wt_retro);
    }
    if this.HasFLBWheel() {
      this.flb_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.flb_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_front_left_b", this.flb_fx_wt);
      this.component.hood.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.flb_fx_wt)) * vwt);
      this.flb_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"VehicleHoodEmitter", wt);
    }
    if this.HasFRBWheel() {
      this.frb_fx = GameInstance.GetFxSystem(this.component.GetVehicle().GetGame()).SpawnEffect(this.resource, effectTransform);
      this.frb_fx.SetBlackboardValue(n"thruster_amount", 0.0);
      vehicleSlots.GetSlotTransform(n"wheel_front_right_b", this.frb_fx_wt);
      this.component.trunk.SetLocalPosition(-chassisOffset + WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.frb_fx_wt)) * vwt);
      this.frb_fx.AttachToComponent(this.component.GetVehicle(), entAttachmentTarget.Transform, n"VehicleTrunkEmitter", wt);
    }
  }

  public func Stop() {
    if IsDefined(this.f_fx) {
      this.f_fx.BreakLoop();
    }
    if IsDefined(this.b_fx) {
      this.b_fx.BreakLoop();
    }
    if IsDefined(this.bl_fx) {
      this.bl_fx.BreakLoop();
      // this.bl_thruster.Toggle(false);
      // this.bl_thruster.visualScale = new Vector3(0.0, 0.0, 0.0);
      // this.bl_thruster.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 180.0, 180.0)));
    }
    if IsDefined(this.br_fx) {
      this.br_fx.BreakLoop();
      // this.br_thruster.Toggle(false);
      // this.br_thruster.visualScale = new Vector3(0.0, 0.0, 0.0);
      // this.br_thruster.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -180.0)));
    }
    if IsDefined(this.fl_fx) {
      this.fl_fx.BreakLoop();
      this.component.GetVehicle().DetachPart(n"ThrusterFL");
      // this.fl_thruster.Toggle(false);
      // this.fl_thruster.visualScale = new Vector3(0.0, 0.0, 0.0);
      // this.fl_thruster.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 180.0, 180.0)));
    }
    if IsDefined(this.fr_fx) {
      this.fr_fx.BreakLoop();
      // this.fr_thruster.Toggle(false);
      // this.fr_thruster.visualScale = new Vector3(0.0, 0.0, 0.0);
      // this.fr_thruster.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -180.0)));
    }
    if IsDefined(this.flb_fx) {
      this.flb_fx.BreakLoop();
    }
    if IsDefined(this.frb_fx) {
      this.frb_fx.BreakLoop();
    }

    if IsDefined(this.bl_retroFx) {
      this.bl_retroFx.BreakLoop();
    }
    if IsDefined(this.br_retroFx) {
      this.br_retroFx.BreakLoop();
    }
    if IsDefined(this.fl_retroFx) {
      this.fl_retroFx.BreakLoop();
    }
    if IsDefined(this.fr_retroFx) {
      this.fr_retroFx.BreakLoop();
    }

    this.ShowWheelComponents();
  }

  public func Update(force: Vector4, torque: Vector4) {
    // would be nice to do this periodically, or when the vehicle comes back into the frustum
    if this.component.active {
      this.HideWheelComponents();

      // kinda glitchy/slow
      let forward = Quaternion.GetForward(this.component.stats.d_orientation);
      forward.Z = 0.0;
      let y = Quaternion.BuildFromDirectionVector(forward, FlightUtils.Up());
      let cq = Quaternion.Conjugate(this.component.stats.d_orientation);
      // Quaternion.SetZRot(cq, 0.0);
      this.ui_info.SetLocalOrientation(cq * y);

      // let wp: WorldPosition;
      // let q = this.component.GetVehicle().GetWeaponPlaceholderOrientation(0);
      // let slotT: WorldTransform;
      // let vehicleSlots = this.component.GetVehicle().GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
      // vehicleSlots.GetSlotTransform(n"PanzerCannon", slotT);
      // // this.laserPointFx.UpdateTransform(slotT);
      // let v = WorldTransform.GetOrientation(slotT) * (q * new Vector4(0.0, -100.0, 0.0, 0.0));
      // let p = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(slotT));
      // let findTarget: TraceResult;
      // this.component.sqs.SyncRaycastByCollisionGroup(p, p - v, n"Shooting", findTarget, false, false);
      // let pointWt: WorldTransform;
      // if TraceResult.IsValid(findTarget) {
      //   let position = Vector4.Vector3To4(findTarget.position);
      //   WorldPosition.SetVector4(wp, position);
      //   WorldTransform.SetPosition(pointWt, position);
      //   WorldTransform.SetOrientation(pointWt, Quaternion.BuildFromDirectionVector(position - p) * new Quaternion(0.0, -0.707, 0.707, 0.0));
      // } else {
      //   WorldPosition.SetVector4(wp, p - v);
      //   WorldTransform.SetPosition(pointWt, p - v);
      //   WorldTransform.SetOrientation(pointWt, Quaternion.BuildFromDirectionVector(v - p) * new Quaternion(0.0, -0.707, 0.707, 0.0));
      // }
      // this.laserFx.UpdateTargetPosition(wp);
      // this.laserPointFx.UpdateTargetPosition(wp);
      // this.laserPointFx.UpdateTransform(pointWt);

      let thrusterAmount = Vector4.Dot(new Vector4(0.0, 0.0, 1.0, 0.0), force);
      // let thrusterAmount = ClampF(this.surge.GetValue(), 0.0, 1.0) * 1.0;
      if this.HasFWheel() {
        this.f_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount + torque.X + torque.Y + AbsF(force.Y)) * FlightSettings.GetFloat("thrusterFactor"));
        if thrusterAmount > 0.0 {
          this.component.fl_tire.SetLocalOrientation(Quaternion.Slerp(this.component.fl_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
            ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Right()), -45.0, 45.0),
            0.0,
            torque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Forward()), -45.0, 45.0)
          )), 0.1));
        }
      }
      if this.HasBWheel() {
        this.b_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount + torque.X - torque.Y + AbsF(force.Y)) * FlightSettings.GetFloat("thrusterFactor"));
        if thrusterAmount > 0.0 {
          this.component.bl_tire.SetLocalOrientation(Quaternion.Slerp(this.component.bl_tire.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
            ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Right()), -45.0, 45.0), 
            0.0,
            torque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Forward()), -45.0, 45.0)
          )), 0.1));
        }
      }
      if this.HasBLWheel() {
        this.bl_thruster.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.bl_thruster.visualScale), new Vector4(1.0, 1.0, 1.0, 1.0), 0.1));
        let amount = (thrusterAmount + torque.X + torque.Y + AbsF(force.Y)) * FlightSettings.GetFloat("thrusterFactor");
        this.bl_fx.SetBlackboardValue(n"thruster_amount", amount);
        AnimationControllerComponent.SetInputFloat(this.component.GetVehicle(), n"veh_rad_w_b_l", 1.0 - amount * 0.1);
        if thrusterAmount > 0.0 {
          this.bl_thruster.SetLocalOrientation(Quaternion.Slerp(this.bl_thruster.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
            -ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Right()), -45.0, 45.0),
            180.0,
            -torque.Z * 0.5 - ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Forward()), -15.0, 15.0)
          )), 0.1));
        }
        this.bl_retroFx.SetBlackboardValue(n"thruster_amount", (Vector4.Dot(new Vector4(1.0, 0.0, 0.0, 0.0), force) + torque.Z) * 0.1);
      }
      if this.HasBRWheel() {
        this.br_thruster.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.br_thruster.visualScale), new Vector4(1.0, 1.0, 1.0, 1.0), 0.1));
        let amount = (thrusterAmount + torque.X - torque.Y + AbsF(force.Y)) * FlightSettings.GetFloat("thrusterFactor");
        this.br_fx.SetBlackboardValue(n"thruster_amount", amount);
        AnimationControllerComponent.SetInputFloat(this.component.GetVehicle(), n"veh_rad_w_b_r", 1.0 - amount * 0.1);
        if thrusterAmount > 0.0 {
          this.br_thruster.SetLocalOrientation(Quaternion.Slerp(this.br_thruster.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
            ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Right()), -45.0, 45.0), 
            0.0,
            torque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Forward()), -15.0, 15.0)
          )), 0.1));
        }
        this.br_retroFx.SetBlackboardValue(n"thruster_amount", (Vector4.Dot(new Vector4(-1.0, 0.0, 0.0, 0.0), force) - torque.Z) * 0.1);
      }
      if this.HasFLWheel() {
        this.fl_thruster.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.fl_thruster.visualScale), new Vector4(1.0, 1.0, 1.0, 1.0), 0.1));
        let amount = (thrusterAmount - torque.X + torque.Y + AbsF(force.Y)) * FlightSettings.GetFloat("thrusterFactor");
        this.fl_fx.SetBlackboardValue(n"thruster_amount", amount);
        AnimationControllerComponent.SetInputFloat(this.component.GetVehicle(), n"veh_rad_w_f_l", 1.0 - amount * 0.1);
        if thrusterAmount > 0.0 {
          this.fl_thruster.SetLocalOrientation(Quaternion.Slerp(this.fl_thruster.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
            -ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Right()), -45.0, 45.0), 
            180.0,
            torque.Z * 0.5 - ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Forward()), -15.0, 15.0)
            // 0.0
          )), 0.1));
        }
        this.fl_retroFx.SetBlackboardValue(n"thruster_amount", (Vector4.Dot(new Vector4(1.0, 0.0, 0.0, 0.0), force) - torque.Z) * 0.1);
      }
      if this.HasFRWheel() {
        this.fr_thruster.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.fr_thruster.visualScale), new Vector4(1.0, 1.0, 1.0, 1.0), 0.1));
        let amount = (thrusterAmount - torque.X - torque.Y + AbsF(force.Y)) * FlightSettings.GetFloat("thrusterFactor");
        this.fr_fx.SetBlackboardValue(n"thruster_amount", amount);
        AnimationControllerComponent.SetInputFloat(this.component.GetVehicle(), n"veh_rad_w_f_r", 1.0 - amount * 0.1);
        if thrusterAmount > 0.0 {
          this.fr_thruster.SetLocalOrientation(Quaternion.Slerp(this.fr_thruster.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
            ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Right()), -45.0, 45.0), 
            0.0,
            -torque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Forward()), -15.0, 15.0)
          )), 0.1));
        }
        this.fr_retroFx.SetBlackboardValue(n"thruster_amount", (Vector4.Dot(new Vector4(-1.0, 0.0, 0.0, 0.0), force) + torque.Z) * 0.1);
      }
      if this.HasFLBWheel() {
        let amount = (thrusterAmount - torque.X + torque.Y + AbsF(force.Y)) * FlightSettings.GetFloat("thrusterFactor");
        this.flb_fx.SetBlackboardValue(n"thruster_amount", amount);
        AnimationControllerComponent.SetInputFloat(this.component.GetVehicle(), n"veh_rad_w_f_l", 1.0 - amount * 0.1);
        if thrusterAmount > 0.0 {
          this.component.hood.SetLocalOrientation(Quaternion.Slerp(this.component.hood.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
            ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Right()), -45.0, 45.0), 
            0.0,
            -torque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Forward()), -45.0, 45.0)
          )), 0.1));
        }
      }
      if this.HasFRBWheel() {
        this.frb_fx.SetBlackboardValue(n"thruster_amount", (thrusterAmount - torque.X - torque.Y + AbsF(force.Y)) * FlightSettings.GetFloat("thrusterFactor"));
        if thrusterAmount > 0.0 {
          this.component.trunk.SetLocalOrientation(Quaternion.Slerp(this.component.trunk.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(
            ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Right()), -45.0, 45.0), 
            0.0,
            -torque.Z * 0.5 + ClampF(Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), force, FlightUtils.Forward()), -45.0, 45.0)
          )), 0.1));
        }
      }
    } else {
      if this.HasBLWheel() {
        this.bl_thruster.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.bl_thruster.visualScale), new Vector4(0.0, 0.0, 0.0, 0.0), 0.1));
        this.bl_fx.SetBlackboardValue(n"thruster_amount", 0.0);
        this.bl_thruster.SetLocalOrientation(Quaternion.Slerp(this.bl_thruster.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(0.0, 180.0, 180.0)), 0.1));
      }
      if this.HasBRWheel() {
        this.br_thruster.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.br_thruster.visualScale), new Vector4(0.0, 0.0, 0.0, 0.0), 0.1));
        this.br_fx.SetBlackboardValue(n"thruster_amount", 0.0);
        this.br_thruster.SetLocalOrientation(Quaternion.Slerp(this.br_thruster.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -180.0)), 0.1));
      }
      if this.HasFLWheel() {
        this.fl_thruster.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.fl_thruster.visualScale), new Vector4(0.0, 0.0, 0.0, 0.0), 0.1));
        this.fl_fx.SetBlackboardValue(n"thruster_amount", 0.0);
        this.fl_thruster.SetLocalOrientation(Quaternion.Slerp(this.fl_thruster.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(0.0, 180.0, 180.0)), 0.1));
      }
      if this.HasFRWheel() {
        this.fr_thruster.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.fr_thruster.visualScale), new Vector4(0.0, 0.0, 0.0, 0.0), 0.1));
        this.fr_fx.SetBlackboardValue(n"thruster_amount", 0.0);
        this.fr_thruster.SetLocalOrientation(Quaternion.Slerp(this.fr_thruster.GetLocalOrientation(), EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -180.0)), 0.1));
      }
    }
  }

  public func HideWheelComponents() {
    // hide wheels, tires & brakes (chassis)

    // if this.chassis.IsEnabled() {
    //   this.chassis.Toggle(false);
    // }

    for c in this.fc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.blc, c);
    }
    for c in this.bc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.blc, c);
    }
    for c in this.blc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.blc, c);
    }
    for c in this.brc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.brc, c);
    }
    for c in this.flc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.flc, c);
    }
    for c in this.frc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.frc, c);
    }
    for c in this.flbc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.frc, c);
    }
    for c in this.frbc {
      if c.IsEnabled() {
        c.Toggle(false);
      }
      // ArrayRemove(this.frc, c);
    }
  }

  public func ShowWheelComponents() {
    // this.chassis.Toggle(true);

    for c in this.fc {
        c.Toggle(true);
    }
    for c in this.bc {
        c.Toggle(true);
    }
    for c in this.blc {
        c.Toggle(true);
    }
    for c in this.brc {
        c.Toggle(true);
    }
    for c in this.flc {
        c.Toggle(true);
    }
    for c in this.frc {
        c.Toggle(true);
    }
    for c in this.flbc {
        c.Toggle(true);
    }
    for c in this.frbc {
        c.Toggle(true);
    }
  }
}

// FlightLog.reds

public native class FlightLog {
  // defined in red4ext part
  public static native func Info(value: String) -> Void;
  public static native func Warn(value: String) -> Void;
  public static native func Error(value: String) -> Void;
  public static native func Probe(image: ref<inkImage>, atlasResourcePath: ResRef) -> Void;
}

// FlightModeAutomatic.reds

public class FlightModeAutomatic extends FlightModeStandard {
  protected let hovering: Float;
  protected let referenceZ: Float;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeAutomatic> {
    let self = new FlightModeAutomatic();
    self.Initialize(component);
    self.hovering = 1.0;
    return self;
  }

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.collisionPenalty = 1.0;
  }

  public func Activate() -> Void {
    let normal: Vector4;
    this.referenceZ = this.component.stats.d_position.Z;
    this.component.FindGround(normal);
    this.component.hoverHeight = MaxF(this.component.distance, FlightSettings.GetFloat("hoverModeMinHoverHeight"));
  }
  
  public func GetDescription() -> String = "Automatic";

  public func Update(timeDelta: Float) -> Void {
    let lastHovering = this.hovering;
    let normal: Vector4;
    let foundGround = this.component.FindGround(normal);
    if foundGround {
      this.hovering = ClampF(1.0 - (this.component.distance - FlightSettings.GetFloat("hoverModeMinHoverHeight")) / (FlightSettings.GetFloat("hoverModeMaxHoverHeight") - FlightSettings.GetFloat("hoverModeMinHoverHeight")), 0.0, 1.0);
    } else {
      this.hovering = 0.0;
    }

    if lastHovering == 0.0 && this.hovering > 0.0 {
      this.component.hoverHeight = MaxF(this.component.distance + this.component.lift * timeDelta * FlightSettings.GetFloat("hoverModeLiftFactor"), FlightSettings.GetFloat("hoverModeMinHoverHeight"));
    } else {
      this.component.hoverHeight = MaxF(this.component.hoverHeight + this.component.lift * timeDelta * FlightSettings.GetFloat("hoverModeLiftFactor"), FlightSettings.GetFloat("hoverModeMinHoverHeight"));
    }

    let heightDifference = this.component.hoverHeight - this.component.distance;
    let idealNormal = Vector4.Interpolate(FlightUtils.Up(), normal, this.hovering);

    let hoverCorrection = this.component.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, FlightSettings.GetFloat("hoverClamp"));// / FlightSettings.GetFloat("hoverClamp");
    let liftFactor = LerpF(this.hovering, this.component.lift - this.component.stats.d_velocity.Z * 0.1, hoverCorrection);

    this.UpdateWithNormalLift(timeDelta, idealNormal, liftFactor * FlightSettings.GetFloat("hoverFactor") + (9.81000042) * this.gravityFactor);

    let aeroFactor = Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction);
    let yawDirectionality: Float = this.component.stats.d_speedRatio * FlightSettings.GetFloat("automaticModeYawDirectionality");

    let directionFactor = AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right));

    this.force += FlightUtils.Forward() * directionFactor * yawDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * directionFactor * yawDirectionality * AbsF(aeroFactor);

    if AbsF(this.component.surge) < 1.0 {    
      let velocityDamp: Vector4 = (1.0 - AbsF(this.component.surge)) * FlightSettings.GetFloat("automaticModeAutoBrakingFactor") * this.component.stats.d_localDirection2D * (this.component.stats.d_speed2D / 100.0);
      this.force -= velocityDamp;
    }
  }
}

// FlightModeDrone.reds

public class FlightModeDrone extends FlightMode {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Mode Settings")
  @runtimeProperty("ModSettings.displayName", "Drone Mode Enabled")
  public let enabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Drone Mode")
  @runtimeProperty("ModSettings.displayName", "Drone Mode Name")
  public let droneModeName: CName = n"Drone Mode";

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Drone Mode")
  @runtimeProperty("ModSettings.displayName", "Lift Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeLiftFactor: Float = 40.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Drone Mode")
  @runtimeProperty("ModSettings.displayName", "Pitch Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100")
  public let droneModePitchFactor: Float = 5.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Drone Mode")
  @runtimeProperty("ModSettings.displayName", "Roll Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100")
  public let droneModeRollFactor: Float = 12.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Drone Mode")
  @runtimeProperty("ModSettings.displayName", "Surge Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeSurgeFactor: Float = 15.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Drone Mode")
  @runtimeProperty("ModSettings.displayName", "Yaw Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100")
  public let droneModeYawFactor: Float = 5.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Drone Mode")
  @runtimeProperty("ModSettings.displayName", "Sway Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeSwayFactor: Float = 15.0;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeDrone> {
    let self = new FlightModeDrone();
    self.Initialize(component);
    return self;
  }

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.usesRightStickInput = true;
    ModSettings.RegisterListenerToClass(this);
  }

  public func Deinitialize() -> Void {
    ModSettings.UnregisterListenerToClass(this);
  }

  public func Activate() -> Void {
    // let camera = this.component.sys.player.GetFPPCameraComponent();
    // if IsDefined(camera) {
    //   let slotT: WorldTransform;
    //   let vehicleSlots = this.component.GetVehicle().GetVehicleComponent().FindComponentByName(n"OccupantSlots") as SlotComponent;
    //   vehicleSlots.GetSlotTransform(n"seat_front_left", slotT);
    //   let vwt = Matrix.GetInverted(this.component.GetVehicle().GetLocalToWorld());
    //   let v = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(slotT)) * vwt;
    //   camera.SetLocalPosition(new Vector4(0.0, FlightSettings.GetFloat("FPVCameraOffsetY"), FlightSettings.GetFloat("FPVCameraOffsetZ"), 0.0) - v);
    //   // camera.Activate(1.0);
    // }
  }

  public func Deactivate() -> Void {
    // let camera = this.component.sys.player.GetFPPCameraComponent();
    // if IsDefined(camera) {
    //   camera.SetLocalPosition(new Vector4(0.0, 0.0, 0.0, 0.0));
    // }
  }

  public func GetDescription() -> String = "Drone";

  public func Update(timeDelta: Float) -> Void {
      let velocityDamp: Vector4 = this.component.stats.d_localVelocity * this.component.linearBrake * FlightSettings.GetInstance().brakeFactorLinear * this.component.stats.s_brakingFrictionFactor;   
      let angularDamp: Vector4 = this.component.stats.d_angularVelocity * this.component.angularBrake * FlightSettings.GetInstance().brakeFactorAngular * this.component.stats.s_brakingFrictionFactor;

      this.force = new Vector4(0.0, 0.0, 0.0, 0.0);
      // lift
      this.force += FlightUtils.Up() * this.component.lift * this.droneModeLiftFactor;
      // surge
      this.force += FlightUtils.Forward() * this.component.surge * this.droneModeSurgeFactor;
      // sway
      this.force += FlightUtils.Right() * this.component.sway * this.droneModeSwayFactor;
      // directional brake
      this.force -= velocityDamp;

      this.torque = new Vector4(0.0, 0.0, 0.0, 0.0);
      // pitch correction
      this.torque.X = -(this.component.pitch * this.droneModePitchFactor + angularDamp.X);
      // roll correction
      this.torque.Y = (this.component.roll * this.droneModeRollFactor - angularDamp.Y);
      // yaw correction
      this.torque.Z = -(this.component.yaw * this.droneModeYawFactor + angularDamp.Z);
  }
}

// FlightModeDroneAntiGravity.reds

public class FlightModeDroneAntiGravity extends FlightModeDrone {
  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeDroneAntiGravity> {
    let self = new FlightModeDroneAntiGravity();
    self.Initialize(component);
    return self;
  }

  public func GetDescription() -> String = "Anti-Gravity Drone";

  public func Update(timeDelta: Float) -> Void {
    super.Update(timeDelta);
    this.force += this.component.stats.d_localUp *  (9.81000042) * this.gravityFactor;
  }
}

// FlightModeFly.reds

public class FlightModeFly extends FlightModeStandard {
  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeFly> {
    let self = new FlightModeFly();
    self.Initialize(component);
    return self;
  }

  public func GetDescription() -> String = "Fly";

  public func Update(timeDelta: Float) -> Void {
    let idealNormal = FlightUtils.Up();  
    let liftForce: Float = FlightSettings.GetFloat("flyModeLiftFactor") * this.component.lift + (9.81000042) * this.gravityFactor;
    this.UpdateWithNormalLift(timeDelta, idealNormal, liftForce);
  }
}

// FlightModeHover.reds

public class FlightModeHover extends FlightModeStandard {
  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeHover> {
    let self = new FlightModeHover();
    self.Initialize(component);
    return self;
  }

  public func GetDescription() -> String = "Hover";

  public func Activate() -> Void {
    let normal: Vector4;
    this.component.FindGround(normal);
    this.component.hoverHeight = MaxF(this.component.distance, FlightSettings.GetFloat("hoverModeMinHoverHeight"));
  }
  
  public func Update(timeDelta: Float) -> Void {
    this.component.hoverHeight = MaxF(FlightSettings.GetFloat("hoverModeMinHoverHeight"), this.component.hoverHeight);

    // let findWater: TraceResult;
    let heightDifference = 0.0;
    let normal: Vector4;
    let idealNormal = FlightUtils.Up();

    // this.component.sqs.SyncRaycastByCollisionGroup(this.component.stats.d_position, this.component.stats.d_position - FlightSettings.GetFloat("lookDown"), n"Water", findWater, true, false);
    // if !TraceResult.IsValid(findWater) {
      if (this.component.FindGround(normal)) {
          heightDifference = this.component.hoverHeight - this.component.distance;
          idealNormal = normal;
      }
    // }

    this.UpdateWithNormalDistance(timeDelta, idealNormal, heightDifference);
  }
}

// FlightModeHoverFly.reds

public class FlightModeHoverFly extends FlightModeStandard {
  protected let hovering: Float;
  protected let referenceZ: Float;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Mode Settings")
  @runtimeProperty("ModSettings.displayName", "Hover & Fly Enabled")
  public let enabled: Bool = true;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeHoverFly> {
    let self = new FlightModeHoverFly();
    self.Initialize(component);
    self.hovering = 1.0;
    return self;
  }

  public func Activate() -> Void {
    let normal: Vector4;
    this.referenceZ = this.component.stats.d_position.Z;
    this.component.FindGround(normal);
    this.component.hoverHeight = MaxF(this.component.distance, FlightSettings.GetFloat("hoverModeMinHoverHeight"));
  }
  
  public func GetDescription() -> String = "Hover & Fly";

  public func Update(timeDelta: Float) -> Void {
    let lastHovering = this.hovering;
    let normal: Vector4;
    let foundGround = this.component.FindGround(normal);
    if foundGround {
      this.hovering = ClampF(1.0 - (this.component.distance - FlightSettings.GetFloat("hoverModeMinHoverHeight")) / (FlightSettings.GetFloat("hoverModeMaxHoverHeight") - FlightSettings.GetFloat("hoverModeMinHoverHeight")), 0.0, 1.0);
    } else {
      this.hovering = 0.0;
    }

    if lastHovering == 0.0 && this.hovering > 0.0 {
      this.component.hoverHeight = MaxF(this.component.distance + this.component.lift * timeDelta * FlightSettings.GetFloat("hoverModeLiftFactor"), FlightSettings.GetFloat("hoverModeMinHoverHeight"));
    } else {
      this.component.hoverHeight = MaxF(this.component.hoverHeight + this.component.lift * timeDelta * FlightSettings.GetFloat("hoverModeLiftFactor"), FlightSettings.GetFloat("hoverModeMinHoverHeight"));
    }

    let heightDifference = this.component.hoverHeight - this.component.distance;
    let idealNormal = Vector4.Interpolate(FlightUtils.Up(), normal, this.hovering);

    let hoverCorrection = this.component.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, FlightSettings.GetFloat("hoverClamp"));// / FlightSettings.GetFloat("hoverClamp");
    let liftFactor = LerpF(this.hovering, this.component.lift - this.component.stats.d_velocity.Z * 0.1, hoverCorrection);

    this.UpdateWithNormalLift(timeDelta, idealNormal, liftFactor * FlightSettings.GetFloat("hoverFactor") + (9.81000042) * this.gravityFactor);
  }
}

// FlightMode_.reds

public abstract class FlightMode {
  protected let sys: ref<FlightSystem>;
  protected let component: ref<FlightComponent>;

  public let force: Vector4;
  public let torque: Vector4;

  public static let gravityFactor: Float;

  public let usesRightStickInput: Bool;
  public let collisionPenalty: Float;

  // public let enabled: Bool;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    this.component = component;
    this.sys = component.sys;
    // this.gravityFactor = 2.885;
    this.gravityFactor = 1.0;
  }

  public func Deinitialize() -> Void;

  public func Activate() -> Void;
  public func Deactivate() -> Void;
  public func GetDescription() -> String;

  public func Update(timeDelta: Float) -> Void;

  public func ApplyPhysics(timeDelta: Float) -> Void {
    
    let velocityDamp: Vector4 = this.component.stats.d_speed * this.component.stats.d_localVelocity * FlightSettings.GetInstance().generalDampFactorLinear * this.component.stats.s_airResistanceFactor;
    let angularDamp: Vector4 = this.component.stats.d_angularVelocity * FlightSettings.GetInstance().generalDampFactorAngular;

    let direction = this.component.stats.d_direction;
    if Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_forward) < 0.0 {
      direction = -this.component.stats.d_direction;
    }
    let yawDirectionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_up);
    let pitchDirectionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_right);

    let aeroDynamicYaw = this.component.aeroYawPID.GetCorrectionClamped(yawDirectionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;
    let aeroDynamicPitch = this.component.pitchAeroPID.GetCorrectionClamped(pitchDirectionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;

    let yawDirectionality: Float = this.component.stats.d_speedRatio * FlightSettings.GetInstance().generalYawDirectionalityFactor;
    let pitchDirectionality: Float = this.component.stats.d_speedRatio * FlightSettings.GetInstance().generalPitchDirectionalityFactor;
    let aeroFactor = Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction);
    // yawDirectionality - redirect non-directional velocity to vehicle forward

    this.force = -velocityDamp;
    
    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * AbsF(aeroFactor);

    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_up)) * pitchDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_up)) * pitchDirectionality * AbsF(aeroFactor);

    this.torque = -angularDamp;
    this.torque.Z -= aeroDynamicYaw * FlightSettings.GetInstance().generalYawAeroFactor;
    this.torque.X -= aeroDynamicPitch * FlightSettings.GetInstance().generalPitchAeroFactor;
  }
}

// FlightMode_Standard.reds

public abstract class FlightModeStandard extends FlightMode {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Standard (Non-Drone) Mode")
  @runtimeProperty("ModSettings.displayName", "Surge Factor")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "200.0")
  public let standardModeSurgeFactor: Float = 15.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Standard (Non-Drone) Mode")
  @runtimeProperty("ModSettings.displayName", "Yaw Factor")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "50.0")
  public let standardModeYawFactor: Float = 5.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Standard (Non-Drone) Mode")
  @runtimeProperty("ModSettings.displayName", "Sway Factor")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "50.0")
  public let standardModeSwayFactor: Float = 5.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Standard (Non-Drone) Mode")
  @runtimeProperty("ModSettings.displayName", "Pitch Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "20.0")
  public let standardModePitchFactor: Float = 3.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Standard (Non-Drone) Mode")
  @runtimeProperty("ModSettings.displayName", "Pitch Input Angle")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "90.0")
  public let standardModePitchInputAngle: Float = 45.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Standard (Non-Drone) Mode")
  @runtimeProperty("ModSettings.displayName", "Roll Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "20.0")
  public let standardModeRollFactor: Float = 15.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Standard (Non-Drone) Mode")
  @runtimeProperty("ModSettings.displayName", "Roll Input Angle")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "90.0")
  public let standardModeRollInputAngle: Float = 45.0;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.collisionPenalty = 0.5;
    ModSettings.RegisterListenerToClass(this);
  }

  public func Deinitialize() -> Void {
    ModSettings.UnregisterListenerToClass(this);
  }

  protected func UpdateWithNormalDistance(timeDelta: Float, normal: Vector4, heightDifference: Float) -> Void {
    let hoverCorrection = this.component.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, FlightSettings.GetFloat("hoverClamp"));// / FlightSettings.GetFloat("hoverClamp");
    let liftForce: Float = hoverCorrection * FlightSettings.GetFloat("hoverFactor") + (9.81000042) * this.gravityFactor;
    this.UpdateWithNormalLift(timeDelta, normal, liftForce);
  }

  protected func UpdateWithNormalLift(timeDelta: Float, normal: Vector4, liftForce: Float) -> Void {
    let pitchCorrection: Float = 0.0;
    let rollCorrection: Float = 0.0;

    normal = Vector4.RotateAxis(normal, this.component.stats.d_forward, this.component.yaw * FlightSettings.GetFloat("rollWithYaw"));
    normal = Vector4.RotateAxis(normal, this.component.stats.d_right, this.component.lift * FlightSettings.GetFloat("pitchWithLift") * Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction));
    normal = Vector4.RotateAxis(normal, this.component.stats.d_right, this.component.surge * FlightSettings.GetFloat("pitchWithSurge"));
    // normal = Vector4.RotateAxis(normal, this.component.stats.d_right, this.component.surge * this.component.stats.s_forwardWeightTransferFactor);
    

    // this.component.pitchPID.SetRatio(this.component.stats.d_speedRatio * AbsF(Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_forward)));
    // this.component.rollPID.SetRatio(this.component.stats.d_speedRatio * AbsF(Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_right)));

    // pitchCorrection = this.component.pitchPID.GetCorrectionClamped(FlightUtils.IdentCurve(Vector4.Dot(normal, FlightUtils.Forward())) + this.lift.GetValue() * this.pitchWithLift, timeDelta, 10.0) + this.pitch.GetValue() / 10.0;
    // rollCorrection = this.component.rollPID.GetCorrectionClamped(FlightUtils.IdentCurve(Vector4.Dot(normal, FlightUtils.Right())), timeDelta, 10.0) + this.yaw.GetValue() * this.rollWithYaw + this.roll.GetValue() / 10.0;
    let pitchDegOff = 90.0 - AbsF(Vector4.GetAngleDegAroundAxis(normal, this.component.stats.d_forward, this.component.stats.d_right));
    pitchDegOff += this.component.pitch * this.standardModePitchInputAngle;
    let rollDegOff = 90.0 - AbsF(Vector4.GetAngleDegAroundAxis(normal, this.component.stats.d_right, this.component.stats.d_forward));
    rollDegOff += this.component.roll * this.standardModeRollInputAngle;
    if AbsF(pitchDegOff) < 120.0  {
      // pitchCorrection = this.component.pitchPID.GetCorrectionClamped(pitchDegOff / 90.0 + this.lift.GetValue() * this.pitchWithLift, timeDelta, 10.0) + this.pitch.GetValue() / 10.0;
      pitchCorrection = this.component.pitchPID.GetCorrectionClamped(pitchDegOff / 90.0, timeDelta, 10.0);// + this.component.pitch / 10.0;
    }
    if AbsF(rollDegOff) < 120.0 {
      // rollCorrection = this.component.rollPID.GetCorrectionClamped(rollDegOff / 90.0 + this.yaw.GetValue() * this.rollWithYaw, timeDelta, 10.0) + this.roll.GetValue() / 10.0;
      rollCorrection = this.component.rollPID.GetCorrectionClamped(rollDegOff / 90.0, timeDelta, 10.0);// + this.component.roll / 10.0;
    }
    // adjust with speed ratio 
    // pitchCorrection = pitchCorrection * (this.pitchCorrectionFactor + 1.0 * this.pitchCorrectionFactor * this.component.stats.d_speedRatio);
    // rollCorrection = rollCorrection * (this.rollCorrectionFactor + 1.0 * this.rollCorrectionFactor * this.component.stats.d_speedRatio);
    pitchCorrection *= this.standardModePitchFactor;
    rollCorrection *= this.standardModeRollFactor;
    // let changeAngle: Float = Vector4.GetAngleDegAroundAxis(Quaternion.GetForward(this.component.stats.d_lastOrientation), this.component.stats.d_forward, this.component.stats.d_up);
    // if AbsF(pitchDegOff) < 30.0 && AbsF(rollDegOff) < 30.0 {

    // }
    // yawCorrection += FlightSettings.GetFloat("yawD") * changeAngle / timeDelta;

    let velocityDamp: Vector4 = this.component.linearBrake * FlightSettings.GetInstance().brakeFactorLinear * this.component.stats.s_brakingFrictionFactor * this.component.stats.d_localVelocity;
    let angularDamp: Vector4 = this.component.stats.d_angularVelocity * this.component.angularBrake * FlightSettings.GetInstance().brakeFactorAngular * this.component.stats.s_brakingFrictionFactor;

    // let yawDirectionality: Float = (this.component.stats.d_speedRatio + AbsF(this.yaw.GetValue()) * this.swayWithYaw) * this.yawDirectionalityFactor;
    // actual in-game mass (i think)
    // this.averageMass = this.averageMass * 0.99 + (liftForce / 9.8) * 0.01;
    // FlightLog.Info(ToString(this.averageMass) + " vs " + ToString(this.component.stats.s_mass));
    let surgeForce: Float = this.component.surge * this.standardModeSurgeFactor;

    //this.CreateImpulse(this.component.stats.d_position, FlightUtils.Right() * Vector4.Dot(FlightUtils.Forward() - direction, FlightUtils.Right()) * yawDirectionality / 2.0);


    this.force = new Vector4(0.0, 0.0, 0.0, 0.0);
    this.torque = new Vector4(0.0, 0.0, 0.0, 0.0);

    // lift
    // force += new Vector4(0.00, 0.00, liftForce + this.component.stats.d_speedRatio * liftForce, 0.00);
    this.force += liftForce * this.component.stats.d_localUp;
    // this.force += liftForce * FlightUtils.Up();
    // surge
    this.force += FlightUtils.Forward() * surgeForce;
    // sway
    this.force += FlightUtils.Right() * this.component.sway * this.standardModeSwayFactor;
    // directional brake
    this.force -= velocityDamp;

    // pitch correction
    this.torque.X = -(pitchCorrection + angularDamp.X);
    // roll correction
    this.torque.Y = (rollCorrection - angularDamp.Y);
    // yaw correction
    this.torque.Z = -(this.component.yaw * this.standardModeYawFactor + angularDamp.Z);
    // rotational brake
    // torque = torque + (angularDamp);

    // if this.showOptions {
    //   this.component.stats.s_centerOfMass.position.X -= torque.Y * 0.1;
    //   this.component.stats.s_centerOfMass.position.Y -= torque.X * 0.1;
    // }
  }
}

// FlightSettings.reds

public static func FlightSettings() -> ref<FlightSettings> {
  return FlightSettings.GetInstance();
}

public native class FlightSettings extends IScriptable {
  public native static func GetInstance() -> ref<FlightSettings>;
  public native static func GetFloat(name: String) -> Float;
  public native static func SetFloat(name: String, value: Float) -> Float;
  public native static func GetVector3(name: String) -> Vector3;
  public native static func SetVector3(name: String, x: Float, y: Float, z: Float) -> Vector3;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "General Flight Settings")
  @runtimeProperty("ModSettings.displayName", "Auto Activation Height")
  @runtimeProperty("ModSettings.description", "In-game units for detecting when flight should automatically be activated on spawn")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.max", "10.0")
  public let autoActivationHeight: Float = 3.0;

  // Flight Control Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Control Settings")
  @runtimeProperty("ModSettings.displayName", "Linear Brake Factor")
  @runtimeProperty("ModSettings.description", "How much the linear brake button slows the vehicle's velocity")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "10.0")
  public let brakeFactorLinear: Float = 1.2;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Control Settings")
  @runtimeProperty("ModSettings.displayName", "Angular Brake Factor")
  @runtimeProperty("ModSettings.description", "How much the angular brake button slows the vehicle's rotation")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "50.0")
  public let brakeFactorAngular: Float = 10.0;
  
  // Flight Physics Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Apply Flight Physics When Deactivated")
  @runtimeProperty("ModSettings.description", "Useful for continuing to control the vehicle mid-air when deactivating")
  public let generalApplyFlightPhysicsWhenDeactivated: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Linear Damp Factor")
  @runtimeProperty("ModSettings.description", "How much resistance any linear movement is given")
  @runtimeProperty("ModSettings.step", "0.0001")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "0.01")
  public let generalDampFactorLinear: Float = 0.001;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Angular Damp Factor")
  @runtimeProperty("ModSettings.description", "How much resistance any angular movement is given")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "10.0")
  public let generalDampFactorAngular: Float = 3.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Pitch Aero Factor")
  @runtimeProperty("ModSettings.description", "How much the vehicle is rotated (pitch) towards its velocity")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalPitchAeroFactor: Float = 0.25;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Yaw Aero Factor")
  @runtimeProperty("ModSettings.description", "How much the vehicle is rotated (yaw) towards its velocity")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalYawAeroFactor: Float = 0.1;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Pitch Directionality Factor")
  @runtimeProperty("ModSettings.description", "How much the vehicle's pitch affects its velocity")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let generalPitchDirectionalityFactor: Float = 80.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Yaw Directionality Factor")
  @runtimeProperty("ModSettings.description", "How much the vehicle's yaw affects its velocity")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let generalYawDirectionalityFactor: Float = 50.0;

  // Flight Camera Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Camera Settings")
  @runtimeProperty("ModSettings.displayName", "Driving Direction Compensation Angle Smoothing")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "180.0")
  public let drivingDirectionCompensationAngleSmooth: Float = 120.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Camera Settings")
  @runtimeProperty("ModSettings.displayName", "Driving Direction Compensation Speed Coef")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let drivingDirectionCompensationSpeedCoef: Float = 0.1;

  // public cb func OnModSettingsUpdate(variable: CName, value: Variant) {
  //   switch (variable) {
  //     case n"autoActivationHeight":
  //       this.autoActivationHeight = FromVariant<Float>(value);
  //       break;
  //   }
  // }

  private func OnAttach() -> Void {
    FlightLog.Info("[FlightSettings] OnAttach");
    ModSettings.RegisterListenerToClass(this);
    
    FlightSettings.SetVector3("inputPitchPID", 1.0, 0.5, 0.5);
    FlightSettings.SetVector3("inputRollPID", 1.0, 0.5, 0.5);
    
    FlightSettings.SetVector3("aeroYawPID", 1.0, 0.01, 1.0);
    FlightSettings.SetVector3("aeroPitchPID", 1.0, 0.01, 1.0);

    FlightSettings.SetVector3("hoverModePID", 1.0, 0.005, 0.5);

    FlightSettings.SetFloat("automaticModeAutoBrakingFactor", 200.0);
    FlightSettings.SetFloat("automaticModeYawDirectionality", 300.0);

    FlightSettings.SetFloat("brakeOffset", 0.0);
    FlightSettings.SetFloat("collisionRecoveryDelay", 0.8);
    FlightSettings.SetFloat("collisionRecoveryDuration", 0.8);
    FlightSettings.SetFloat("defaultHoverHeight", 3.50);
    FlightSettings.SetFloat("distance", 0.0);
    FlightSettings.SetFloat("distanceEase", 0.1);

    FlightSettings.SetFloat("flyModeLiftFactor", 20.0);

    FlightSettings.SetVector3("FPVCameraOffset", 0.0, 0.0, -0.5);

    FlightSettings.SetFloat("fwtfCorrection", 0.0);
    FlightSettings.SetFloat("hoverClamp", 10.0);
    FlightSettings.SetFloat("hoverFactor", 40.0);
    FlightSettings.SetFloat("hoverModeLiftFactor", 8.0);
    FlightSettings.SetFloat("isFlightUIActive", 1.0);
    FlightSettings.SetFloat("liftFactor", 8.0);
    FlightSettings.SetFloat("lockFPPCameraForDrone", 1.0);
    FlightSettings.SetFloat("lookAheadMax", 10.0);
    FlightSettings.SetFloat("lookAheadMin", 1.0);
    FlightSettings.SetFloat("hoverModeMaxHoverHeight", 7.0);
    FlightSettings.SetFloat("hoverModeMinHoverHeight", 1.0);
    FlightSettings.SetFloat("normalEase", 0.3);
    FlightSettings.SetFloat("referenceZ", 0.0);
    FlightSettings.SetFloat("secondCounter", 0.0);
    
    FlightSettings.SetFloat("surgeOffset", 0.5);

    FlightSettings.SetFloat("swayWithYaw", 0.5);
    FlightSettings.SetFloat("rollWithYaw", 0.15);
    FlightSettings.SetFloat("pitchWithLift", 0.0);
    FlightSettings.SetFloat("pitchWithSurge", 0.0);

    FlightSettings.SetFloat("thrusterFactor", 0.05);
    FlightSettings.SetFloat("yawD", 3.0);
  }
}

// FlightStats.reds

public class FlightStats {
  public let vehicle: wref<VehicleObject>;
  public let s_record: wref<Vehicle_Record>;
  // public let s_fc_record: wref<FlightControl_Record>;
  public let s_driveModelData: wref<VehicleDriveModelData_Record>;
  public let s_engineData: wref<VehicleEngineData_Record>;
  public let s_wheelDimensions: wref<VehicleWheelDimensionsSetup_Record>;
  public let s_wheelDriving: wref<VehicleWheelDrivingSetup_Record>;
  public let s_mass: Float;
  public let s_centerOfMass: Vector3;
  public let s_momentOfInertia: Vector4;
  public let s_forwardWeightTransferFactor: Float;
  public let s_brakingFrictionFactor: Float;
  public let s_airResistanceFactor: Float;

  public let d_position: Vector4;
  public let d_visualPosition: Vector4;
  public let d_orientation: Quaternion;
  public let d_lastOrientation: Quaternion;
  public let d_forward: Vector4;
  public let d_right: Vector4;
  public let d_up: Vector4;
  public let d_localUp: Vector4;
  public let d_forward2D: Vector4;
  public let d_orientationChange: Quaternion;
  public let d_angularVelocity: Vector4;
  public let d_velocity: Vector4;
  public let d_localVelocity: Vector4;
  public let d_velocity2D: Vector4;
  public let d_speed: Float;
  public let d_speedRatio: Float;
  public let d_speedRatioSquared: Float;
  public let d_speed2D: Float;
  public let d_direction: Vector4;
  public let d_localDirection: Vector4;
  public let d_direction2D: Vector4;
  public let d_localDirection2D: Vector4;

  private let reset: Bool;
  private let ipp: ref<IPositionProvider>;
  private let iop: ref<IOrientationProvider>;

  public static func Create(vehicle: wref<VehicleObject>) -> ref<FlightStats> {
    let self: ref<FlightStats> = new FlightStats();
    self.vehicle = vehicle;
    self.vehicle.chassis = self.vehicle.FindComponentByName(n"Chassis") as vehicleChassisComponent;
    self.reset = false;
    self.UpdateStatic();
    // self.d_position = self.vehicle.GetWorldPosition() + self.s_centerOfMass;
    self.ipp = IPositionProvider.CreateEntityPositionProvider(self.vehicle, self.s_centerOfMass);
    // self.ipp = IPositionProvider.CreatePlacedComponentPositionProvider(self.vehicle.chassis);//, Vector4.Vector4To3(-self.vehicle.chassis.GetInitialPosition()));
    self.ipp.CalculatePosition(self.d_position);
    // CreateMoveComponentVelocityProvider(this.player);
    // self.d_position = self.vehicle.chassis.GetLocalToWorld(); * self.s_centerOfMass;
    // self.d_position = self.vehicle.chassis.GetLocalToWorld() * new Vector4(0.0, 0.0, 0.0, 0.0);
    // self.d_position += self.s_centerOfMass;
    // self.d_orientation = Matrix.ToQuat(self.vehicle.chassis.GetLocalToWorld());
    self.d_orientation = self.vehicle.GetWorldOrientation();
    // self.d_orientation = self.vehicle.chassis.GetWorldOrientation();
    // self.d_position = Vector4.EmptyVector();
    return self;
  }

  public func Reset() -> Void {
    this.reset = true;
  }

  public final func UpdateStatic() -> Void {
    this.s_record = this.vehicle.GetRecord();
    // this.s_fcRecord = FlightStatsData.GetInstance(this.vehicle.GetGame()).Get(this.vehicle.GetRecordID());
    // this.s_fc_record = TweakDBInterface.GetFlightRecord(this.vehicle.GetRecordID());
    this.s_driveModelData = this.s_record.VehDriveModelData();  
    this.s_brakingFrictionFactor = this.s_driveModelData.BrakingFrictionFactor();
    this.s_airResistanceFactor = this.s_driveModelData.AirResistanceFactor();
    // RollingResistanceFactor() -> Float;
    // HandbrakeBrakingTorque() -> Float;
    this.s_wheelDimensions = this.s_record.VehWheelDimensionsSetup();
    // .FrontPreset() & .BackPreset();
      // .TireRadius() -> Float;
      // .RimRadius() -> Float;
      // .TireWidth() -> Float;
      // .WheelOffset() -> Float;
    this.s_wheelDriving = this.s_driveModelData.WheelSetup();
    // .FrontPreset() & .BackPreset();
      // .MaxBrakingTorque() -> Float;
      // .Mass() -> Float;
    // this.s_mass = this.s_fcRecord.mass;
    // this.s_mass = this.s_fc_record.mass;
    // if this.s_mass == 0.0 {
      this.s_mass = this.vehicle.GetTotalMass(); // might be dynamic? pulled from otherPhysicsData->totalMass
      // this.s_playerMass = 60.0;
    // }
    // small sample size (2) for this value under limited circumstances
    // this.s_mass *= 1.396;
    // need to just dynamically calculate this instead of messing around with random values
    // this.s_centerOfMass = Cast(this.s_driveModelData.Center_of_mass_offset());
    this.s_centerOfMass = this.vehicle.GetCenterOfMass();

    // let weightedVector = (this.vehicle.chassis.GetLocalToWorld() * this.s_centerOfMass.position) * this.s_mass;

    // let cbl = this.vehicle.FindComponentByName(n"ColliderBL") as ColliderComponent;
    // if IsDefined(cbl) {
    //   this.s_mass += cbl.mass;
    //   weightedVector += (cbl.GetLocalToWorld() * cbl.comOffset.position) * cbl.mass;
    // }
    // let cbr = this.vehicle.FindComponentByName(n"ColliderBR") as ColliderComponent;
    // if IsDefined(cbr) {
    //   this.s_mass += cbr.mass;
    //   weightedVector += (cbr.GetLocalToWorld() * cbr.comOffset.position) * cbr.mass;
    // }
    // let cfl = this.vehicle.FindComponentByName(n"ColliderFL") as ColliderComponent;
    // if IsDefined(cfl) {
    //   this.s_mass += cfl.mass;
    //   weightedVector += (cfl.GetLocalToWorld() * cfl.comOffset.position) * cfl.mass;
    // }
    // let cfr = this.vehicle.FindComponentByName(n"ColliderFR") as ColliderComponent;
    // if IsDefined(cfr) {
    //   this.s_mass += cfr.mass;
    //   weightedVector += (cfr.GetLocalToWorld() * cfr.comOffset.position) * cfr.mass;
    // }
    // let cc = this.vehicle.FindComponentByName(n"ColliderC") as ColliderComponent;
    // if IsDefined(cc) {
    //   this.s_mass += cc.mass;
    //   weightedVector += (cc.GetLocalToWorld() * cc.comOffset.position) * cc.mass;
    // }
    // weightedVector /= this.s_mass;

    // this.s_centerOfMass.position = Matrix.GetInverted(this.vehicle.chassis.GetLocalToWorld()) * weightedVector;


    // let playerMass = 80.0;
    // this.s_centerOfMass = playerMass * (Matrix.GetInverted(this.vehicle.chassis.GetLocalToWorld()) * this.player.GetWorldPosition()) / (playerMass + this.s_mass);
    // this.s_centerOfMass += this.s_fcRecord.comOffset_position;
    // this.s_momentOfInertia = Cast(this.s_driveModelData.MomentOfInertia());
    // this.s_momentOfInertia = Cast(this.s_fcRecord.inertia);
    // this isn't defined for all vehicles, so throw some numbers in for now
    // would be nice to compute from meshes outside this and import those values in
    // if Vector4.IsXYZFloatZero(this.s_momentOfInertia) {
      // LogChannel(n"DEBUG", "[FlightStats] no MOI, setting to bad defaults");
    let it = this.vehicle.GetInertiaTensor();
    this.s_momentOfInertia.X = it.X.X;
    this.s_momentOfInertia.Y = it.Y.Y;
    this.s_momentOfInertia.Z = it.Z.Z; 
    // this torques the vehicile in some way upon acceleration - the details aren't currently known
    // it could also be tied to Vehicle.RPMValue - we could use vehicle.GetBlackboard().GetFloat(GetAllBlackboardDefs().Vehicle.RPMValue)
    this.s_forwardWeightTransferFactor = this.s_driveModelData.ForwardWeightTransferFactor();
  }
  
  public final func UpdateDynamic() -> Void {
    this.d_lastOrientation = this.d_orientation;
    let orientation = this.vehicle.GetWorldOrientation();
    // let orientation = Matrix.ToQuat(this.vehicle.chassis.GetLocalToWorld());
    this.d_orientation = orientation;
    this.d_orientationChange = Quaternion.MulInverse(this.d_orientation, this.d_lastOrientation);
    this.d_angularVelocity = Quaternion.Conjugate(this.d_orientation) * Vector4.Vector3To4(this.vehicle.GetAngularVelocity());
    Quaternion.GetAxes(this.d_orientation, this.d_forward, this.d_right, this.d_up);
    this.d_forward2D = Vector4.Normalize2D(this.d_forward);
    // this.d_forward = Vector4.Normalize(Quaternion.GetForward(this.d_orientation));
    // this.d_right = Vector4.Normalize(Quaternion.GetRight(this.d_orientation));
    // this.d_up = Vector4.Normalize(Quaternion.GetUp(this.d_orientation));
    // this.d_forward = Quaternion.Transform(this.s_centerOfMass.orientation, this.d_forward);
    // this.d_right = Quaternion.Transform(this.s_centerOfMass.orientation, this.d_right);
    // this.d_up = Quaternion.Transform(this.s_centerOfMass.orientation, this.d_up);
    
    // let playerMass = 80.0;

    // this.s_centerOfMass.position = this.s_playerMass * (Matrix.GetInverted(this.vehicle.GetLocalToWorld()) * this.player.GetWorldPosition()) / (this.s_playerMass + this.s_mass);
    // this.s_centerOfMass.position = this.s_playerMass * (Matrix.GetInverted(this.vehicle.chassis.GetLocalToWorld()) * this.player.GetWorldPosition()) / (this.s_playerMass + this.s_mass);
    // this.s_centerOfMass /= this.s_centerOfMass.W;
    
    // GameInstance.GetSpatialQueriesSystem(FlightController.GetInstance().gameInstance).GetGeometryDescriptionSystem();
    let position: Vector4;
    // if IsDefined(this.vehicle.chassis) {
      // position = this.vehicle.GetWorldPosition() * this.s_mass;
      // position += this.player.GetWorldPosition() * this.s_playerMass;
      // position /= (this.s_mass + this.s_playerMass);
      // position /= position.W;
      // position = this.vehicle.chassis.GetLocalToWorld() * -this.vehicle.chassis.GetComOffset();
      // position = this.vehicle.chassis.GetLocalToWorld() * this.s_centerOfMass;
      // position = this.vehicle.GetLocalToWorld() * this.s_centerOfMass;
    // } else {
      // position = this.vehicle.GetWorldPosition() + this.s_centerOfMass;
    // }
    this.ipp.CalculatePosition(position);
    // position += Quaternion.Transform(this.d_orientation, Vector4.Vector3To4(this.s_centerOfMass));

    // if Vector4.Length(position - this.d_position) / timeDelta <= this.d_speed * 1.1 {
    // if this.reset {
    //   this.reset = false;
    //   this.d_position = position;
    // } else {
      // try to smooth out the position some
      // this.d_position = 0.99999 * this.d_position + 0.00001 * position;
      // this.d_position = Vector4.Interpolate(this.d_position, position, 0.4);
      // this.d_position = position;
    // }

    this.d_velocity = this.vehicle.GetLinearVelocity();
    this.d_localVelocity = Quaternion.Conjugate(this.d_orientation) * this.d_velocity;
    this.d_localUp = Quaternion.Conjugate(this.d_orientation) * FlightUtils.Up();
    // this.d_localUp = this.d_orientation * this.d_up;

    this.d_speed = Vector4.Length(this.d_velocity);
    this.d_speedRatio = this.d_speed / 100.0;
    this.d_speedRatioSquared = this.d_speedRatio * this.d_speedRatio;
    this.d_speed2D = Vector4.Length2D(this.d_velocity);
    this.d_direction = Vector4.Normalize(this.d_velocity);
    this.d_localDirection = Vector4.Normalize(this.d_localVelocity);
    this.d_direction2D = Vector4.Normalize2D(this.d_velocity);
    this.d_localDirection2D = Vector4.Normalize2D(this.d_localVelocity);
    this.d_velocity2D = this.d_direction2D * this.d_speed2D;

    // let minS = 0.3;
    // let maxS = 0.7;
    // let factor = Vector4.Distance(position, this.d_position) / timeDelta / this.d_speed;
    // this.d_position = this.d_position * (minS + (maxS - minS) * factor) + position * (1.0 - minS - (maxS - minS) * factor);
    this.d_visualPosition = this.d_position;
    this.d_position =  position;
    // this.d_visualPosition = this.d_position - this.d_velocity * timeDelta;
  }
}

// FlightSystem.reds

public native abstract importonly class IFlightSystem extends IGameSystem {
}

public func fs() -> ref<FlightSystem> = FlightSystem.GetInstance();

public native class FlightSystem extends IFlightSystem {
  public static native func GetInstance() -> ref<FlightSystem>;

  public let gameInstance: GameInstance;
  public let player: wref<PlayerPuppet>;
  public let ctlr: ref<FlightController>;
  public let stats: ref<FlightStats>;
  public let audio: ref<FlightAudio>;
  public let fx: ref<FlightFx>;
  public let tppCamera: wref<vehicleTPPCameraComponent>;
  public let playerComponent: wref<FlightComponent>;

  public func Setup(player: ref<PlayerPuppet>) -> Void {
    // FlightLog.Info("[FlightSystem] FlightSettings Created");
    this.player = player;
    this.gameInstance = player.GetGame();
    if !IsDefined(this.audio) {
      this.audio = FlightAudio.Create();
      FlightLog.Info("[FlightSystem] FlightAudio Created");
    }
    this.ctlr = FlightController.GetInstance();
    this.tppCamera = player.FindComponentByName(n"vehicleTPPCamera") as vehicleTPPCameraComponent;
  }

//   public static func Get(gameInstance: GameInstance) -> ref<FlightSystem> {
//     return GameInstance.GetScriptableSystemsContainer(gameInstance).Get(n"FlightSystem") as FlightSystem;
//   }

  protected final func OnFlightSystemAttach() -> Void {
    FlightLog.Info("[FlightSystem] OnFlightSystemAttach");
  }

  public final func OnPlayerAttach(request: ref<PlayerAttachRequest>) -> Void {
    FlightLog.Info("[FlightSystem] OnPlayerAttach");
  }

//   private func OnDetach() -> Void {
//     FlightLog.Info("[FlightSystem] OnDetach");
//   }

//   private func OnRestored(saveVersion: Int32, gameVersion: Int32) -> Void {
//     FlightLog.Info("[FlightSystem] OnRestored");
//   }

//   // private func IsSavingLocked() -> Bool {
//   //   return FlightController.GetInstance().IsActive();
//   // }
}



// FlightTransition.reds

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
  let flightCamera: Int32;

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
        this.flightCamera = 0;
        break;
      case vehicleCameraPerspective.TPPClose:
        this.flightCamera = 2;
        break;
      case vehicleCameraPerspective.TPPFar:
        this.flightCamera = 3;
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
        if this.flightCamera == 1 {
          camEvent.cameraPerspective = vehicleCameraPerspective.TPPFar;
          this.flightCamera = 2;
        } else {
          this.EnterCustomCamera(scriptInterface);
          this.flightCamera = 1;
        }
        break;
      case vehicleCameraPerspective.TPPClose:
        this.ExitCustomCamera(scriptInterface);
        camEvent.cameraPerspective = vehicleCameraPerspective.FPP;
        this.flightCamera = 3;
        break;
      case vehicleCameraPerspective.TPPFar:
        camEvent.cameraPerspective = vehicleCameraPerspective.TPPClose;
        this.flightCamera = 0;
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

// FlightTricks.reds

public abstract class FlightTrick
{
  public let suspendMode: Bool;
  public let force: Vector4;
  public let torque: Vector4;
  private let time: Float;
  private let component: ref<FlightComponent>;
  public func Initialize(component: ref<FlightComponent>, suspend: Bool) -> Void {
    this.component = component;
    this.suspendMode = suspend;
  }
  // returns true if finished
  public func Update(timeDelta: Float) -> Bool {
    this.time += timeDelta;
    return false;
  }
}

public class FlightTrickAileronRoll extends FlightTrick {
  private let direction: Float;
  public static func Create(component: ref<FlightComponent>, direction: Float) -> ref<FlightTrickAileronRoll> {
    let self = new FlightTrickAileronRoll();
    self.Initialize(component, true);
    self.direction = direction;
    return self;
  }
  public func Update(timeDelta: Float) -> Bool {
    if this.time < 0.3 {
      this.force = this.component.stats.d_localUp * 9.81000042;
      if this.time < 0.3 {
        this.torque.Y = 75.0 * this.direction; 
      }
      return super.Update(timeDelta);
    } else {
      return true;
    }
  }
}

// FlightUtils.reds

public class FlightUtils {
    public static func SqrtCurve(input: Float) -> Float {
        if input != 0.0 {
            return SqrtF(AbsF(input)) * AbsF(input) / input;
        } else {
            return 0.0;
        }
    }
    public static func IdentCurve(input: Float) -> Float {
        return input;
    }

    public static func Right() -> Vector4 = new Vector4(1.0, 0.0, 0.0, 0.0);
    public static func Left() -> Vector4 = new Vector4(-1.0, 0.0, 0.0, 0.0);
    public static func Forward() -> Vector4 = new Vector4(0.0, 1.0, 0.0, 0.0);
    public static func Backward() -> Vector4 = new Vector4(0.0, -1.0, 0.0, 0.0);
    public static func Up() -> Vector4 = new Vector4(0.0, 0.0, 1.0, 0.0);
    public static func Down() -> Vector4 = new Vector4(0.0, 0.0, -1.0, 0.0);
    
	public static func ElectricBlue() -> HDRColor = new HDRColor(0.368627, 0.964706, 1.0, 1.0)
	public static func Bittersweet() -> HDRColor = new HDRColor(1.1761, 0.3809, 0.3476, 1.0)
	public static func Dandelion() -> HDRColor = new HDRColor(1.1192, 0.8441, 0.2565, 1.0)
	public static func LightGreen() -> HDRColor = new HDRColor(0.113725, 0.929412, 0.513726, 1.0)
	public static func BlackPearl() -> HDRColor = new HDRColor(0.054902, 0.054902, 0.090196, 1.0)

	public static func RedOxide() -> HDRColor = new HDRColor(0.411765, 0.086275, 0.090196, 1.0)
	public static func Bordeaux() -> HDRColor = new HDRColor(0.262745, 0.086275, 0.094118, 1.0)

	public static func PureBlack() -> HDRColor = new HDRColor(0.0, 0.0, 0.0, 1.0)
	public static func PureWhite() -> HDRColor = new HDRColor(1.0, 1.0, 1.0, 1.0)
}

// hudFlightController.reds


public class FlightUIVehicleHealthStatPoolListener extends CustomValueStatPoolsListener {

  public let m_owner: wref<hudFlightController>;
  public let m_vehicle: wref<VehicleObject>;

  public func OnStatPoolValueChanged(oldValue: Float, newValue: Float, percToPoints: Float) -> Void {
    if IsDefined(this.m_owner) {
      this.m_owner.ReactToHPChange(newValue);
    };
  }
}

public class hudFlightController extends inkHUDGameController {

  private let m_Date: inkTextRef;
  private let m_Timer: inkTextRef;
  private let m_CameraID: inkTextRef;
  private let healthStatus: inkTextRef;
  private let m_MessageText: inkTextRef;
  private let m_pitchFluff: inkTextRef;
  private let m_yawFluff: inkTextRef;
  private let m_leftPart: inkWidgetRef;
  private let m_rightPart: inkWidgetRef;
  // @default(hudFlightController, -838.0f)
  private let offsetLeft: Float;
  // @default(hudFlightController, 1495.0f)
  private let offsetRight: Float;
  private let currentTime: GameTime;
  private let m_currentHealth: Int32;
  private let m_previousHealth: Int32;
  private let m_maximumHealth: Int32;
  private let m_playerObject: wref<GameObject>;
  private let m_playerPuppet: wref<GameObject>;
  private let m_gameInstance: GameInstance;
  private let m_animationProxy: ref<inkAnimProxy>;

  private let m_bbPlayerStats: wref<IBlackboard>;
  private let m_scannerBlackboard: wref<IBlackboard>;
  private let m_vehicleBlackboard: wref<IBlackboard>;
  private let m_vehicleFlightBlackboard: wref<IBlackboard>;
  private let m_psmBlackboard: wref<IBlackboard>;

  private let m_uiScannerVisibleCallbackID: ref<CallbackHandle>;
  private let m_bbPlayerEventId: ref<CallbackHandle>;
  private let m_PSM_BBID: ref<CallbackHandle>;
  private let m_playerStateBBConnectionId: ref<CallbackHandle>;
  private let m_vehicleBBUIActivId: ref<CallbackHandle>;
  private let m_vehicleBBActivId: ref<CallbackHandle>;
  private let m_vehicleBBModeId: ref<CallbackHandle>;
  private let m_vehicleRollID: ref<CallbackHandle>;
  private let m_vehiclePitchID: ref<CallbackHandle>;
  private let m_tppBBConnectionId: ref<CallbackHandle>;

  public let m_healthStatPoolListener: ref<FlightUIVehicleHealthStatPoolListener>;
  private let m_hp_mask: inkWidgetRef;
  private let m_hp_condition_text: inkTextRef;
  private let m_currentZoom: Float;

  protected cb func OnInitialize() -> Bool {
    FlightLog.Info("[hudFlightController] OnInitialize");
    let delayInitialize: ref<DelayedHUDInitializeEvent>;
    // inkTextRef.SetText(this.m_Date, "XX-XX-XXXX");
    delayInitialize = new DelayedHUDInitializeEvent();
    GameInstance.GetDelaySystem(this.GetPlayerControlledObject().GetGame()).DelayEvent(this.GetPlayerControlledObject(), delayInitialize, 0.10);
    // this.GetPlayerControlledObject().RegisterInputListener(this);
    this.offsetLeft = -838.0;
    this.offsetRight = 1495.0;
    this.GetRootWidget().SetVisible(false);
    // this.PlayLibraryAnimation(n"outro");

    let vehicle = FlightSystem.GetInstance().playerComponent.GetVehicle();
    this.m_healthStatPoolListener = new FlightUIVehicleHealthStatPoolListener();
    this.m_healthStatPoolListener.m_owner = this;
    this.m_healthStatPoolListener.m_vehicle = vehicle;
    let stats = GameInstance.GetStatPoolsSystem(vehicle.GetGame());
    if IsDefined(stats) {  
      stats.RequestRegisteringListener(Cast<StatsObjectID>(vehicle.GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    }
  }

  protected cb func OnUninitialize() -> Bool {
    FlightLog.Info("[hudFlightController] OnUninitialize");
    // TakeOverControlSystem.CreateInputHint(this.GetPlayerControlledObject().GetGame(), false);
    // SecurityTurret.CreateInputHint(this.GetPlayerControlledObject().GetGame(), false);
    if IsDefined(this.m_healthStatPoolListener) {
      GameInstance.GetStatPoolsSystem(this.m_gameInstance).RequestUnregisteringListener(Cast(this.m_healthStatPoolListener.m_vehicle.GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    }
  }

  private func UpdateTime() -> Void {
    this.currentTime = GameInstance.GetTimeSystem(this.m_gameInstance).GetGameTime();
    inkTextRef.SetText(this.m_Timer, ToString(GameTime.Hours(this.currentTime)) + ":" + ToString(GameTime.Minutes(this.currentTime)) + ":" + ToString(GameTime.Seconds(this.currentTime)));
  }

  private final func IsUIactive() -> Bool {
    if IsDefined(this.m_vehicleFlightBlackboard) && this.m_vehicleFlightBlackboard.GetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive) {
      return true;
    };
    return false;
  }

  private final func IsActive() -> Bool {
    if IsDefined(this.m_vehicleFlightBlackboard) && this.m_vehicleFlightBlackboard.GetBool(GetAllBlackboardDefs().VehicleFlight.IsActive) {
      return true;
    };
    return false;
  }

  protected cb func OnActivateUI(activate: Bool) -> Bool {
    this.ActivateUI(activate);
  }

  protected cb func OnActivate(activate: Bool) -> Bool {
    if this.IsUIactive() {
      this.ActivateUI(activate);
    }
  }

  protected cb func OnModeChange(mode: Int32) -> Bool {
    inkTextRef.SetText(this.m_CameraID, FlightSystem.GetInstance().playerComponent.GetFlightMode().GetDescription());
  }

  protected cb func OnCameraModeChanged(tpp: Bool) -> Bool {
    let hp_gauge = this.GetRootCompoundWidget().GetWidget(n"hp_gauge");
    if IsDefined(hp_gauge) {
      if tpp {
        hp_gauge.SetMargin(new inkMargin(1555.0, -120.0, 0.0, 0.0));
      } else {
        hp_gauge.SetMargin(new inkMargin(1555.0, -120.0 - 100.0, 0.0, 0.0));
      }
    }
  }

  protected cb func OnVehicleRollChanged(roll: Float) -> Bool {
    // if FlightSystem.GetInstance().playerComponent.GetFlightMode().usesRightStickInput && !FlightSystem.GetInstance().ctlr.isTPP {
    if !FlightSystem.GetInstance().ctlr.isTPP { // FPP
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/rulers").SetRotation(0);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/rulers/h").SetRotation(-roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/rotation").SetRotation(-roll);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetVisible(false);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetRotation(0);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch_mask").SetVisible(true);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetRotation(0);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetScale(new Vector2(1.0, 1.0));
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/crosshair").SetRotation(0);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/crosshair/reticle").SetRotation(-roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/RADIUS").SetRotation(-roll);
    } else { // TPP
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/rulers").SetRotation(roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/rulers/h").SetRotation(-roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/rotation").SetRotation(0);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetVisible(true);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetRotation(roll);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch_mask").SetVisible(false);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch_mask").SetRotation(roll);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetScale(new Vector2(1.0, 1.0));
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/crosshair").SetRotation(roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/crosshair/reticle").SetRotation(0);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/RADIUS").SetRotation(roll);
    }
  }

  protected cb func OnVehiclePitchChanged(pitch: Float) -> Bool {
    this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/rotation/translation").SetTranslation(new Vector2(0.0, pitch/90.0 * 900.0));
  }

  private let m_introAnimationProxy: ref<inkAnimProxy>;
  private let m_outroAnimationProxy: ref<inkAnimProxy>;

  private func ActivateUI(activate: Bool) -> Void {
    FlightLog.Info("[hudFlightController] ActivateUI");
    if activate {
      let vehicle = FlightSystem.GetInstance().playerComponent.GetVehicle();
      if IsDefined(vehicle) {
        inkTextRef.SetText(this.m_hp_condition_text, vehicle.GetDisplayName());
        let stats = GameInstance.GetStatPoolsSystem(this.m_gameInstance);
        this.ReactToHPChange(stats.GetStatPoolValue(Cast<StatsObjectID>(vehicle.GetEntityID()), gamedataStatPoolType.Health, true));  
      }
      inkTextRef.SetText(this.m_CameraID, FlightSystem.GetInstance().playerComponent.GetFlightMode().GetDescription());


      this.GetRootWidget().SetVisible(true);
      let options: inkAnimOptions;
      options.executionDelay = 0.50;
      if IsDefined(this.m_outroAnimationProxy) && this.m_outroAnimationProxy.IsPlaying() {
        this.m_outroAnimationProxy.Stop();
      }
      this.m_introAnimationProxy = this.PlayLibraryAnimation(n"intro", options);
      // this.PlayAnim(n"intro", n"OnIntroComplete");
      // optionIntro.executionDelay = 0.25;
      // this.PlayLibraryAnimation(n"Malfunction_off", optionIntro);
      // this.PlayAnim(n"Malfunction_timed", n"OnMalfunction");
      // this.UpdateJohnnyThemeOverride(true);
    } else {
      // this.GetRootWidget().SetVisible(false);
      // this.PlayLibraryAnimation(n"outro");
      // this.PlayLibraryAnimation(n"Malfunction");
      let options: inkAnimOptions;
      if IsDefined(this.m_introAnimationProxy) && this.m_introAnimationProxy.IsPlaying() {
        this.m_introAnimationProxy.Stop();
      }
      this.m_outroAnimationProxy = this.PlayLibraryAnimation(n"outro", options);

      // this.PlayAnim(n"outro", n"OnOutroComplete");
      // this.UpdateJohnnyThemeOverride(false);
    }
  }

  protected cb func OnPlayerAttach(playerPuppet: ref<GameObject>) -> Bool {
    FlightLog.Info("[hudFlightController] OnPlayerAttach");
    this.m_playerObject = playerPuppet;
    this.m_playerPuppet = playerPuppet;
    this.m_gameInstance = playerPuppet.GetGame();
    // this.UpdateTime();
    this.m_vehicleBlackboard = FlightSystem.GetInstance().playerComponent.GetVehicle().GetBlackboard();
    this.m_vehicleFlightBlackboard = FlightController.GetInstance().GetBlackboard();
    this.m_scannerBlackboard = GameInstance.GetBlackboardSystem(this.m_gameInstance).Get(GetAllBlackboardDefs().UI_Scanner);
    this.RegisterBB();
    this.ActivateUI(this.IsUIactive() && this.IsActive());
  }

  protected func RegisterBB() {
    // this.m_bbPlayerStats = this.GetBlackboardSystem().Get(GetAllBlackboardDefs().UI_PlayerBioMonitor);
    // this.m_bbPlayerEventId = this.m_bbPlayerStats.RegisterListenerVariant(GetAllBlackboardDefs().UI_PlayerBioMonitor.PlayerStatsInfo, this, n"OnStatsChanged");    
    // this.m_psmBlackboard = this.GetPSMBlackboard(this.m_playerPuppet);
    // if IsDefined(this.m_psmBlackboard) {
    //   this.m_PSM_BBID = this.m_psmBlackboard.RegisterDelayedListenerFloat(GetAllBlackboardDefs().PlayerStateMachine.ZoomLevel, this, n"OnZoomChange");
    // };

    if IsDefined(this.m_vehicleFlightBlackboard) {
      if !IsDefined(this.m_vehicleBBUIActivId) {
        this.m_vehicleBBUIActivId = this.m_vehicleFlightBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this, n"OnActivateUI");
      }
      if !IsDefined(this.m_vehicleBBActivId) {
        this.m_vehicleBBActivId = this.m_vehicleFlightBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this, n"OnActivate");
      };
      if !IsDefined(this.m_vehicleBBModeId) {
        this.m_vehicleBBModeId = this.m_vehicleFlightBlackboard.RegisterListenerInt(GetAllBlackboardDefs().VehicleFlight.Mode, this, n"OnModeChange");
      };
      if !IsDefined(this.m_vehicleRollID) {
        this.m_vehicleRollID = this.m_vehicleFlightBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Roll, this, n"OnVehicleRollChanged");
      };
      if !IsDefined(this.m_vehiclePitchID) {
        this.m_vehiclePitchID = this.m_vehicleFlightBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this, n"OnVehiclePitchChanged");
      };
    };
    if IsDefined(this.m_vehicleBlackboard) {
      this.m_tppBBConnectionId = this.m_vehicleBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this, n"OnCameraModeChanged");
    }
    if IsDefined(this.m_scannerBlackboard) && !IsDefined(this.m_uiScannerVisibleCallbackID) {
      this.m_uiScannerVisibleCallbackID = this.m_scannerBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_Scanner.UIVisible, this, n"OnScannerUIVisibleChanged");
    };
  }

  protected func UnregisterBB() {
    // if IsDefined(this.m_bbPlayerStats) {
      // this.m_bbPlayerStats.UnregisterListenerVariant(GetAllBlackboardDefs().UI_PlayerBioMonitor.PlayerStatsInfo, this.m_bbPlayerEventId);
    // };
    // if IsDefined(this.m_psmBlackboard) && IsDefined(this.m_PSM_BBID) {
    //   this.m_psmBlackboard.UnregisterDelayedListener(GetAllBlackboardDefs().PlayerStateMachine.ZoomLevel, this.m_PSM_BBID);
    // };
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if IsDefined(this.m_vehicleBBUIActivId) {
        this.m_vehicleFlightBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this.m_vehicleBBUIActivId);
      }
      if IsDefined(this.m_vehicleBBActivId) {
        this.m_vehicleFlightBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this.m_vehicleBBActivId);
      };
      if IsDefined(this.m_vehicleBBModeId) {
        this.m_vehicleFlightBlackboard.UnregisterListenerInt(GetAllBlackboardDefs().VehicleFlight.Mode, this.m_vehicleBBModeId);
      };
      if IsDefined(this.m_vehicleRollID) {
        this.m_vehicleFlightBlackboard.UnregisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Roll, this.m_vehicleRollID);
      };
      if IsDefined(this.m_vehiclePitchID) {
        this.m_vehicleFlightBlackboard.UnregisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this.m_vehiclePitchID);
      };
    }
    if IsDefined(this.m_vehicleBlackboard) {
      if IsDefined(this.m_tppBBConnectionId) {
        this.m_vehicleBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this.m_tppBBConnectionId);
      }
    }
    if IsDefined(this.m_scannerBlackboard) {
      if IsDefined(this.m_uiScannerVisibleCallbackID) {
        this.m_scannerBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_Scanner.UIVisible, this.m_uiScannerVisibleCallbackID);
      }
    }
  }

  protected cb func OnPlayerDetach(playerPuppet: ref<GameObject>) -> Bool {
    FlightLog.Info("[hudFlightController] OnPlayerDetach");
    this.UnregisterBB();
  }

  // protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
  //   let yaw: Float = ClampF(this.m_playerPuppet.GetWorldYaw(), -300.00, 300.00);
  //   inkTextRef.SetText(this.m_yawFluff, ToString(yaw));
  //   inkTextRef.SetText(this.m_pitchFluff, ToString(yaw * 1.50));
  //   inkWidgetRef.SetMargin(this.m_leftPart, new inkMargin(yaw, this.offsetLeft, 0.00, 0.00));
  //   inkWidgetRef.SetMargin(this.m_rightPart, new inkMargin(this.offsetRight, yaw, 0.00, 0.00));
  //   this.UpdateTime();
  // }
  
  protected cb func OnZoomChange(evt: Float) -> Bool {
    // if evt > this.m_currentZoom {
    //     this.PlayLibraryAnimation(n"zoomUp");
    // } else {
    //     this.PlayLibraryAnimation(n"zoomDown");
    // }
    // this.m_currentZoom = evt;
  }

  protected cb func OnIntroComplete(anim: ref<inkAnimProxy>) -> Bool {
    // GameInstance.GetAudioSystem(this.GetPlayerControlledObject().GetGame()).Play(n"ui_main_menu_cc_loading");
  }
  protected cb func OnOutroComplete(anim: ref<inkAnimProxy>) -> Bool {
      // this.GetRootWidget().SetVisible(false);
  }

  protected cb func OnStatsChanged(value: Variant) -> Bool {
    // let incomingData: PlayerBioMonitor = FromVariant<PlayerBioMonitor>(value);
    // this.m_previousHealth = this.m_currentHealth;
    // this.m_maximumHealth = incomingData.maximumHealth;
       
   
    // this.m_currentHealth = CeilF(GameInstance.GetStatPoolsSystem(this.m_playerObject.GetGame()).GetStatPoolValue(Cast<StatsObjectID>(GetPlayer(this.m_playerObject.GetGame()).GetEntityID()), gamedataStatPoolType.Health, false));
    // this.m_currentHealth = Clamp(this.m_currentHealth, 0, this.m_maximumHealth);
  }

  protected cb func OnScannerUIVisibleChanged(visible: Bool) -> Bool {
    if this.IsUIactive() && this.IsActive() {
      this.ActivateUI(!visible);
    }
  }

  public func ReactToHPChange(value: Float) -> Void {
    inkTextRef.SetText(this.healthStatus, IntToString(RoundF(value)) + "/100");
    inkWidgetRef.SetMargin(this.m_hp_mask, new inkMargin(-1720.0 - ((100.0 - value) * 9.0), 826.66638183, 0, 0));
  }

  protected cb func OnDelayedHUDInitializeEvent(evt: ref<DelayedHUDInitializeEvent>) -> Bool {
    // TakeOverControlSystem.CreateInputHint(this.GetPlayerControlledObject().GetGame(), true);
    // SecurityTurret.CreateInputHint(this.GetPlayerControlledObject().GetGame(), true);
  }

  public final func PlayAnim(animName: CName, opt callBack: CName, opt animOptions: inkAnimOptions) -> Void {
    if IsDefined(this.m_animationProxy) && this.m_animationProxy.IsPlaying() {
      this.m_animationProxy.Stop(true);
    };
    this.m_animationProxy = this.PlayLibraryAnimation(animName, animOptions);
    if NotEquals(callBack, n"") {
      this.m_animationProxy.RegisterToCallback(inkanimEventType.OnFinish, this, callBack);
    };
  }

  private final func UpdateJohnnyThemeOverride(value: Bool) -> Void {
    let uiSystem: ref<UISystem>;
    let controlledPuppet: wref<gamePuppetBase> = GetPlayer(this.m_gameInstance);
    if IsDefined(controlledPuppet) && controlledPuppet.IsJohnnyReplacer() {
      uiSystem = GameInstance.GetUISystem(this.m_gameInstance);
      if IsDefined(uiSystem) {
        if value {
          uiSystem.SetGlobalThemeOverride(n"Johnny");
        } else {
          uiSystem.ClearGlobalThemeOverride();
        };
      };
    };
  }
}


// inkWidgetBuilder.reds

enum inkWidgetBuilderType {
  inkCanvas = 0,
  inkFlex = 1,
  inkImage = 2,
  inkText = 3,
  inkRectangle = 4,
  inkCircle = 5,
  inkShape = 6,
  inkScrollArea = 7,
  inkMask = 8
}

public class inkWidgetBuilder {
  private let widget: ref<inkWidget>;
  private let type: inkWidgetBuilderType;

  // inkWidget generics

  public func Visible(value: Bool) -> ref<inkWidgetBuilder> {
    this.widget.SetVisible(value);
    return this;
  }

  public func Reparent(parent: ref<inkCompoundWidget>) -> ref<inkWidgetBuilder> {
    this.widget.Reparent(parent);
    return this;
  }

  public func Tint(tint: HDRColor) -> ref<inkWidgetBuilder> {
    this.widget.SetTintColor(tint);
    return this;
  }

  public func Tint(binding: CName) -> ref<inkWidgetBuilder> {
    this.widget.BindProperty(n"tintColor", binding);
    return this;
  }

  public func Opacity(opacity: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetOpacity(opacity);
    return this;
  }

  public func Anchor(anchor: inkEAnchor) -> ref<inkWidgetBuilder> {
    this.widget.SetAnchor(anchor);
    return this;
  }

  public func Anchor(anchorPoint: Vector2) -> ref<inkWidgetBuilder> {
    this.widget.SetAnchorPoint(anchorPoint);
    return this;
  }

  public func Anchor(x: Float, y: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetAnchorPoint(x, y);
    return this;
  }

  public func Size(x: Float, y: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetSize(x, y);
    return this;
  }

  public func Margin(margin: inkMargin) -> ref<inkWidgetBuilder> {
    this.widget.SetMargin(margin);
    return this;
  }

  public func Margin(a: Float, b: Float, c: Float, d: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetMargin(a, b, c, d);
    return this;
  }

  public func Padding(padding: inkMargin) -> ref<inkWidgetBuilder> {
    this.widget.SetPadding(padding);
    return this;
  }

  public func Padding(a: Float, b: Float, c: Float, d: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetPadding(a, b, c, d);
    return this;
  }

  public func FitToContent(fit: Bool) -> ref<inkWidgetBuilder> {
    this.widget.SetFitToContent(fit);
    return this;
  }

  public func Scale(x: Float, y: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetScale(new Vector2(x, y));
    return this;
  }

  public func Scale(scale: Vector2) -> ref<inkWidgetBuilder> {
    this.widget.SetScale(scale);
    return this;
  }

  public func Translation(Translation: Vector2) -> ref<inkWidgetBuilder> {
    this.widget.SetTranslation(Translation);
    return this;
  }

  public func Translation(x: Float, y: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetTranslation(x, y);
    return this;
  }

  public func Rotation(Rotation: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetRotation(Rotation);
    return this;
  }

  // inkCanvas methods

  public static func inkCanvas(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkCanvas() as inkWidget;
    self.type = inkWidgetBuilderType.inkCanvas;
    self.widget.SetName(name);
    return self;
  }

  public func BuildCanvas() -> ref<inkCanvas> {
    return this.widget as inkCanvas;
  }

  // inkFlex methods

  public static func inkFlex(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkFlex() as inkWidget;
    self.type = inkWidgetBuilderType.inkFlex;
    self.widget.SetName(name);
    return self;
  }

  public func BuildFlex() -> ref<inkFlex> {
    return this.widget as inkFlex;
  }

  // inkImage methods

  public static func inkImage(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkImage() as inkWidget;
    self.type = inkWidgetBuilderType.inkImage;
    self.widget.SetName(name);
    return self;
  }

  public func BuildImage() -> ref<inkImage> {
    return this.widget as inkImage;
  }

  public func Atlas(atlas: ResRef) -> ref<inkWidgetBuilder> {
    if Equals(this.widget, this.widget as inkImage) {
      (this.widget as inkImage).SetAtlasResource(atlas);
    }
    if Equals(this.widget, this.widget as inkMask) {
      (this.widget as inkMask).SetAtlasResource(atlas);
    }
    return this;
  }

  public func Part(texture: CName) -> ref<inkWidgetBuilder> {
    if Equals(this.widget, this.widget as inkImage) {
      (this.widget as inkImage).SetTexturePart(texture);
    }
    if Equals(this.widget, this.widget as inkMask) {
      (this.widget as inkMask).SetTexturePart(texture);
    }
    return this;
  }

  public func NineSliceScale(value: Bool) -> ref<inkWidgetBuilder> {
    if Equals(this.widget, this.widget as inkImage) {
    (this.widget as inkImage).SetNineSliceScale(value);
      }
    if Equals(this.widget, this.widget as inkMask) {
      (this.widget as inkMask).SetNineSliceScale(value);
    }
    return this;
  }

  // inkText methods

  public static func inkText(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkText() as inkWidget;
    self.type = inkWidgetBuilderType.inkText;
    self.widget.SetName(name);
    return self;
  }

  public func BuildText() -> ref<inkText> {
    return this.widget as inkText;
  }

  public func Font(font: String, opt fontStyle: CName) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetFontFamily(font, fontStyle);
    return this;
  }

  public func FontStyle(font: CName) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetFontStyle(font);
    return this;
  }

  public func FontSize(size: Int32) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetFontSize(size);
    return this;
  }

  public func Text(text: String) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetText(text);
    return this;
  }

  public func LetterCase(value: textLetterCase) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetLetterCase(value);
    return this;
  }

  public func Overflow(value: textOverflowPolicy) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetOverflowPolicy(value);
    return this;
  }

  public func HAlign(contentHAlign: inkEHorizontalAlign) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetContentHAlign(contentHAlign);
    return this;
  }

  public func VAlign(contentVAlign: inkEVerticalAlign) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetContentVAlign(contentVAlign);
    return this;
  }

  // inkRectangle
  
  public static func inkRectangle(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkRectangle() as inkWidget;
    self.type = inkWidgetBuilderType.inkRectangle;
    self.widget.SetName(name);
    return self;
  }

  public func BuildRectangle() -> ref<inkRectangle> {
    return this.widget as inkRectangle;
  }

  // inkCircle
  
  public static func inkCircle(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkCircle() as inkWidget;
    self.type = inkWidgetBuilderType.inkCircle;
    self.widget.SetName(name);
    return self;
  }

  public func BuildCircle() -> ref<inkCircle> {
    return this.widget as inkCircle;
  }

  // inkShape
  
  public static func inkShape(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkShape() as inkWidget;
    self.type = inkWidgetBuilderType.inkShape;
    self.widget.SetName(name);
    return self;
  }

  public func BuildShape() -> ref<inkShape> {
    return this.widget as inkShape;
  }
  
  public func ShapeVariant(shapeVariant: inkEShapeVariant) -> ref<inkWidgetBuilder> {
    (this.widget as inkShape).SetShapeVariant(shapeVariant);
    return this;
  }

  public func BorderOpacity(borderOpacity: Float) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetBorderOpacity(borderOpacity);
    return this;
  }

  public func BorderColor(borderColor: HDRColor) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetBorderColor(borderColor);
    return this;
  }

  public func FillOpacity(fillOpacity: Float) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetFillOpacity(fillOpacity);
    return this;
  }

  public func VertexList(vertexList: array<Vector2>) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetVertexList(vertexList);
    return this;
  }

  public func JointStyle(jointStyle: inkEJointStyle) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetJointStyle(jointStyle);
    return this;
  }

  public func EndCapStyle(endCapStyle: inkEEndCapStyle) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetEndCapStyle(endCapStyle);
    return this;
  }

  public func LineThickness(lineThickness: Float) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetLineThickness(lineThickness);
    return this;
  }
  public func ShapeName(shapeName: CName) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetShapeName(shapeName);
    return this;
  }
  public func UseNineSlice(value: Bool) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetUseNineSlice(value);
    return this;
  }
  // public func ShapeResource(shapeResource: ResRef) -> ref<inkWidgetBuilder>  {
  //   (this.widget as inkShape).SetShapeResource(shapeResource);
  //   return this;
  // }
  public func ChangeShape(shapeName: CName) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).ChangeShape(shapeName);
    return this;
  }

  // inkScrollArea

  public static func inkScrollArea(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkScrollArea() as inkWidget;
    self.type = inkWidgetBuilderType.inkScrollArea;
    self.widget.SetName(name);
    return self;
  }

  public func BuildScrollArea() -> ref<inkScrollArea> {
    return this.widget as inkScrollArea;
  }

  public func Mask(value: Bool) -> ref<inkWidgetBuilder> {
    (this.widget as inkScrollArea).SetUseInternalMask(value);
    return this;
  }

  // inkMask

  public static func inkMask(name: CName) -> ref<inkWidgetBuilder> {
    let self = new inkWidgetBuilder();
    self.widget = new inkMask() as inkWidget;
    self.type = inkWidgetBuilderType.inkMask;
    self.widget.SetName(name);
    return self;
  }

  public func BuildMask() -> ref<inkMask> {
    return this.widget as inkMask;
  }

  public func MaskSource(value: inkMaskDataSource) -> ref<inkWidgetBuilder> {
    (this.widget as inkMask).SetDataSource(value); 
    return this;
  }

  public func MaskTransparency(value: Float) -> ref<inkWidgetBuilder> {
    (this.widget as inkMask).SetMaskTransparency(value); 
    return this;
  }

  public func InvertMask(value: Bool) -> ref<inkWidgetBuilder> {
    (this.widget as inkMask).SetInvertMask(value); 
    return this;
  }




}

// OperatorHelpers.reds

// Matrix

public static func OperatorMultiply(m: Matrix, v: Vector4) -> Vector4 {
  return Vector4.Transform(m, v);
}
public static func OperatorMultiply(v: Vector4, m: Matrix) -> Vector4 {
  return m * v;
}

public static func OperatorAssignMultiply(out v: Vector4, m: Matrix) -> Vector4 {
  v = m * v;
  return v;
}

// Quaternion

public static func OperatorXor(q: Quaternion, n: Int32) -> Quaternion {
  let out = q;
  while (n > 0) {
    Quaternion.SetInverse(out);
    n -= 1;
  }
  return out;
}

// EulerAngles

public static func Cast(a: EulerAngles) -> Vector4 {
  let v = Vector4.EmptyVector();
  v.X = a.Pitch;
  v.Y = a.Roll;
  v.Z = a.Yaw;
  return v;
}

public static func Cast(a: EulerAngles) -> WorldTransform {
  let wt = new WorldTransform();
  WorldTransform.SetOrientation(wt, EulerAngles.ToQuat(a));
  return wt;
}

public static func OperatorMultiply(e: EulerAngles, f: Float) -> EulerAngles {
  let out = e;
  e.Roll *= f;
  e.Yaw *= f;
  e.Pitch += f;
  return out;
}

public static func OperatorAssignMultiply(out e: EulerAngles, f: Float) -> EulerAngles {
  e = e * f;
  return e;
}

public static func OperatorDivide(e: EulerAngles, f: Float) -> EulerAngles {
  let out = e;
  e.Roll /= f;
  e.Yaw /= f;
  e.Pitch /= f;
  return out;
}

public static func OperatorAdd(e: EulerAngles, v: Vector4) -> Vector4 {
  let out = v;
  v.X += e.Pitch;
  v.Y += e.Roll;
  v.Z += e.Yaw;
  return out;
}

public static func OperatorAdd(v: Vector4, e: EulerAngles) -> Vector4 {
  return e + v;
}

public static func OperatorAssignAdd(out v: Vector4, e: EulerAngles) -> Vector4 {
  v = e + v;
  return v;
}

// Vector3

public static func OperatorAdd(a: Vector4, b: Vector3) -> Vector4 {
  return a + Vector4.Vector3To4(b);
}

// FxResource

public static native func Cast(a: ResRef) -> FxResource;


public static func OperatorEqual(a: Vector3, b: Vector3) -> Bool {
  return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
}

public static func OperatorEqual(a: Vector4, b: Vector4) -> Bool {
  return a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
}

public static func OperatorNotEqual(a: Vector3, b: Vector3) -> Bool {
  return !OperatorEqual(a, b);
}

public static func OperatorNotEqual(a: Vector4, b: Vector4) -> Bool {
  return !OperatorEqual(a, b);
}

// PID.reds

public class PID {
  private let valueFloat: Float;
  // private let valueVector: Vector4;
  private let inputFloat: Float;
  // private let inputVector: Vector4;
  private let P: Float;
  private let I: Float;
  private let D: Float;
  public let integralFloat: Float;
  // private let integralVector: Vector4;
  private let lastErrorFloat: Float;
  // private let lastErrorVector: Vector4;
  public static func Create(P: Float, I: Float, D: Float) -> ref<PID> {
    let instance: ref<PID> = new PID();
    instance.P = P;
    instance.I = I;
    instance.D = D;
    instance.Reset();
    return instance;
  }
  public static func Create(v: Vector3) -> ref<PID> {
    return PID.Create(v.X, v.Y, v.Z);
  }
  public static func Create(P: Float, I: Float, D: Float, initialValue: Float) -> ref<PID> {
    let instance: ref<PID> = PID.Create(P, I, D);
    instance.valueFloat = initialValue;
    return instance;
  }
  // public static func Create(P: Float, I: Float, D: Float, initialValue: Vector4) -> ref<PID> {
  //   let instance: ref<PID> = PID.Create(P, I, D);
  //   instance.valueVector = initialValue;
  //   return instance;
  // }
  public func Update(P: Float, I: Float, D: Float) -> Void {
    this.P = P;
    this.I = I;
    this.D = D;
  }
  
  public func UpdateP(P: Float) -> Void {
    this.P = P;
  }
  
  public func UpdateI(I: Float) -> Void {
    this.I = I;
  }

  public func UpdateD(D: Float) -> Void {
    this.D = D;
  }

  public func SetInput(input: Float) {
    this.inputFloat = input;
  }
  // public func SetInput(input: Vector4) {
  //   this.inputVector = input;
  // }
  public func GetValue(timeDelta: Float) -> Float {
    let error: Float = this.inputFloat - this.valueFloat;
    this.valueFloat += this.GetCorrection(error, timeDelta);
    return this.valueFloat;
  } 
  public func GetValue() -> Float {
    return this.valueFloat;
  } 
  public func GetInput() -> Float {
    return this.inputFloat;
  }
  // public func GetValue(timeDelta: Float) -> Vector4 {
  //   let error: Vector4 = this.inputVector - this.valueVector;
  //   this.valueVector += this.GetCorrection(error)
  //   return this.valueVector;
  // }
  public func GetValue(input: Float, timeDelta: Float) -> Float {
    this.SetInput(input);
    return this.GetValue(timeDelta);
  }
  // public func GetValue(input: Vector4, timeDelta: Float) -> Vector4 {
  //   this.SetInput(input);
  //   return GetValue(timeDelta);
  // }
  public func GetCorrection(error: Float, timeDelta: Float) -> Float { 
    let derivative: Float = (error - this.lastErrorFloat) / timeDelta;
    // if error < 0.01 || error * this.lastErrorFloat < 0.0 {
    //   this.integralFloat = 0.0;
    // } else {
    this.integralFloat = ClampF(error * timeDelta + this.integralFloat, -100.0, 100.0);
    // }
    this.lastErrorFloat = error;
    return this.P * error + this.I * this.integralFloat + this.D * derivative;
  }
  // public func GetCorrection(error: Vector4, timeDelta: Float) -> Vector4 { 
  //   let derivative: Vector4 = (error - this.lastErrorVector) / timeDelta;
  //   this.integralVector = Vector4.ClampLength(error * timeDelta + this.integralVector, -100.0, 100.0);
  //   this.lastErrorVector = error;
  //   return this.P * error + this.I * this.integralFloat + this.D * derivative;
  // }
  public func GetCorrectionClamped(error: Float, timeDelta: Float, clamp: Float) -> Float {
    return ClampF(this.GetCorrection(error, timeDelta), -clamp, clamp);
  }
  public func Reset(opt input: Float) -> Void {
    this.inputFloat = input;
    this.integralFloat = 0.0;
    this.lastErrorFloat = 0.0;
  }
}

public class InputPID extends PID {
  private let P_dec: Float;
  public static func Create(P: Float, P_dec: Float) -> ref<InputPID> {
    let instance: ref<InputPID> = new InputPID();
    instance.P = P;
    instance.P_dec = P_dec;
    instance.Reset();
    return instance;
  }

  public func UpdatePd(Pd: Float) -> Void {
    this.P_dec = Pd;
  }

  public func GetCorrection(error: Float, timeDelta: Float) -> Float { 
    if AbsF(this.inputFloat) > AbsF(this.valueFloat) || this.inputFloat * this.valueFloat < 0.0 {
      return this.P * error;
    } else {
      return this.P_dec * error;
    }
  }
}

public class DualPID extends PID {
  private let P_aux: Float;
  private let I_aux: Float;
  private let D_aux: Float;
  private let ratio: Float;
  public static func Create(P: Float, I: Float, D: Float, P_aux: Float, I_aux: Float, D_aux: Float) -> ref<DualPID> {
    let instance: ref<DualPID> = new DualPID();
    instance.P = P;
    instance.I = I;
    instance.D = D;
    instance.P_aux = P_aux;
    instance.I_aux = I_aux;
    instance.D_aux = D_aux;
    instance.Reset();
    return instance;
  }
  public static func Create(P: Float, I: Float, D: Float, P_aux: Float, I_aux: Float, D_aux: Float, initialValue: Float) -> ref<DualPID> {
    let instance: ref<DualPID> = DualPID.Create(P, I, D, P_aux, I_aux, D_aux);
    instance.valueFloat = initialValue;
    return instance;
  }
  public func SetRatio(ratio: Float) {
    this.ratio = ratio;
  }
  public func GetCorrection(error: Float, timeDelta: Float) -> Float { 
    let derivative: Float = (error - this.lastErrorFloat) / timeDelta;
    // if error < 0.01 || error * this.lastErrorFloat < 0.0 {
    //   this.integralFloat = 0.0;
    // } else {
    this.integralFloat = ClampF(error * timeDelta + this.integralFloat, -100.0, 100.0) * 0.95;
    // }
    this.lastErrorFloat = error;
    let pri = this.P * error + this.I * this.integralFloat + this.D * derivative;
    let aux = this.P_aux * error + this.I_aux * this.integralFloat + this.D_aux * derivative;
    return pri * (1.0 - this.ratio) + aux * (this.ratio);
  }
  public func UpdateAux(P_aux: Float, I_aux: Float, D_aux: Float) -> Void {
    this.P_aux = P_aux;
    this.I_aux = I_aux;
    this.D_aux = D_aux;
  }
}

// vehicleTPPCameraComponent.reds

public native class vehicleChassisComponent extends IPlacedComponent {
    public native func GetComOffset() -> Transform;
}

native class vehicleTPPCameraComponent extends CameraComponent {
    // public native let isInAir: Bool;
    public native let drivingDirectionCompensationAngleSmooth: Float;
    public native let drivingDirectionCompensationSpeedCoef: Float;
    public native let lockedCamera: Bool;
    public native let worldPosition: WorldPosition;
    public native let worldTransform2: WorldTransform;
    public native let pitch: Float;
    public native let yaw: Float;
    public native let pitchDelta: Float; // positive moves camera down
    public native let yawDelta: Float; // positive moves camera right
    // public native let chassis: ref<vehicleChassisComponent>;
}

// @addMethod(vehicleChassisComponent)
// protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
//     super.OnRequestComponents(ri);
//     EntityRequestComponentsInterface.RequestComponent(ri, n"Chassis", n"vehicleChassisComponent", false);
// }


public native class vehicleDriveToPointEvent extends Event {
    public native let targetPos: Vector3;
    public native let useTraffic: Bool;
    public native let speedInTraffic: Float;
}

public importonly class EffectSpawnerComponent extends IVisualComponent {
    public native func AddEffect() -> Void;
}


// @addField(ColliderComponent)
// public native let mass: Float;

// @addField(ColliderComponent)
// public native let massOverride: Float;

// @addField(ColliderComponent)
// public native let inertia: Vector3;

// @addField(ColliderComponent)
// public native let comOffset: Transform;

// public native class exEntitySpawner {
//     public native static func Spawn(entityPath: ResRef, worldTransform: WorldTransform, opt appearance: CName, opt recordID: TweakDBID) -> EntityID;
//     public native static func SpawnRecord(recordID: TweakDBID, worldTransform: WorldTransform, opt appearance: CName) -> EntityID;
//     public native static func Despawn(entity: ref<Entity>) -> Void;
// }

// @addField(MappinSystem)
// public native let worldMappins: Array<Ptr<>>;

// @addField(entCameraComponent)
// native let fov: Float;

// @addField(entCameraComponent)
// native let zoom: Float;

// @addField(entCameraComponent)
// native let nearPlaneOverride: Float;

// @addField(entCameraComponent)
// native let farPlaneOverride: Float;

// @addField(entCameraComponent)
// native let motionBlurScale: Float;

//FindVehicleCameraManager


// @addMethod(FxSystem)
// public final native func SpawnEffect(resource: ResRef, transform: WorldTransform, opt ignoreTimeDilation: Bool) -> ref<FxInstance>;

// @addField(FxResource)
// public native let effect: ResRef;

// @addMethod(Entity)
// public native func AddComponent(component: ref<IComponent>) -> Bool;

// @addMethod(Entity)
// public native func AddWorldWidgetComponent() -> Bool;

// @addMethod(IPlacedComponent)
// public native func UpdateHardTransformBinding(bindName: CName, slotName: CName) -> Bool;

// vflightUIGameController.reds

public class vflightUIGameController extends inkHUDGameController {
  private let m_vehicleBlackboard: wref<IBlackboard>;
  private let m_vehicleFlightBlackboard: wref<IBlackboard>;
  private let m_vehicle: wref<VehicleObject>;
  private let m_vehiclePS: ref<VehicleComponentPS>;
  private let m_vehicleBBStateConectionId: ref<CallbackHandle>;
  private let m_vehicleCollisionBBStateID: ref<CallbackHandle>;
  private let m_vehiclePitchID: ref<CallbackHandle>;
  private let m_vehiclePositionID: ref<CallbackHandle>;
  private let m_vehicleBBUIActivId: ref<CallbackHandle>;
  private let m_rootWidget: wref<inkWidget>;
  private let m_UIEnabled: Bool;
  private let m_startAnimProxy: ref<inkAnimProxy>;
  private let m_loopAnimProxy: ref<inkAnimProxy>;
  private let m_endAnimProxy: ref<inkAnimProxy>;
  private let m_loopingBootProxy: ref<inkAnimProxy>;
  private let m_speedometerWidget: inkWidgetRef;
  private let m_tachometerWidget: inkWidgetRef;
  private let m_timeWidget: inkWidgetRef;
  private let m_instruments: inkWidgetRef;
  private let m_gearBox: inkWidgetRef;
  private let m_radio: inkWidgetRef;
  private let m_analogTachWidget: inkWidgetRef;
  private let m_analogSpeedWidget: inkWidgetRef;
  private let m_isVehicleReady: Bool;

  protected cb func OnInitialize() -> Bool {
    FlightLog.Info("[vflightUIGameController] OnInitialize");
    this.m_vehicle = this.GetOwnerEntity() as VehicleObject;
    this.m_vehiclePS = this.m_vehicle.GetVehiclePS();
    this.m_rootWidget = this.GetRootWidget();
    this.m_vehicleBlackboard = this.m_vehicle.GetBlackboard();
    this.m_vehicleFlightBlackboard = FlightController.GetInstance().GetBlackboard();
    if this.IsUIactive() {
      this.ActivateUI();
    };
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if !IsDefined(this.m_vehicleBBUIActivId) {
        this.m_vehicleBBUIActivId = this.m_vehicleFlightBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this, n"OnActivateUI");
      };
    };
    
    this.SetupInfoPanel();
    this.SetupPitchDisplay();
  }

  private let m_info: ref<inkCanvas>;
  private let m_position: ref<inkText>;

  protected cb func OnVehiclePositionChanged(position: Vector4) -> Bool {
    this.m_position.SetText(Vector4.ToStringPrec(position, 2));
  }

  private func SetupInfoPanel() {
    this.m_info = inkWidgetBuilder.inkCanvas(n"info")
      .Size(624.0, 200.0)
      .Reparent(this.GetRootCompoundWidget())
		  .Anchor(inkEAnchor.TopLeft)
      .Margin(380.0, 20.0, 0.0, 0.0)
      .BuildCanvas();

    let top = new inkHorizontalPanel();
		top.SetName(n"top");
		top.SetSize(500.0, 18.0);
    top.Reparent(this.m_info);
		// top.SetHAlign(inkEHorizontalAlign.Left);
		// top.SetVAlign(inkEVerticalAlign.Top);
    top.SetMargin(0.0, 0.0, 0.0, 0.0);

    let manufacturer = this.m_vehicle.GetRecord().Manufacturer().EnumName();
    let iconRecord = TweakDBInterface.GetUIIconRecord(TDBID.Create("UIIcon." + manufacturer));
    inkWidgetBuilder.inkImage(n"manufacturer")
      .Reparent(top)
      .Part(iconRecord.AtlasPartName())
      .Atlas(iconRecord.AtlasResourcePath())
		  .Size(174.0, 18.0)
      .Margin(20.0, 0.0, 0.0, 0.0)
		  .Opacity(1.0)
		  .Tint(FlightUtils.Bittersweet())
      .Tint(n"Default.accent_color1")
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .BuildImage();

    inkWidgetBuilder.inkImage(n"fluff")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"fluff_01")
      .Tint(FlightUtils.Bittersweet())
      .Tint(n"Default.accent_color1")
      .Opacity(1.0)
      .Margin(10.0, 0.0, 0.0, 0.0)
		  .Size(174.0, 18.0)
      .Anchor(inkEAnchor.LeftFillVerticaly)
      .Reparent(top)
      .BuildImage();

    inkWidgetBuilder.inkImage(n"fluff2")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"p20_sq_element")
      .Tint(FlightUtils.Bittersweet())
      .Tint(n"Default.accent_color1")
      .Opacity(1.0)
      .Margin(10.0, 0.0, 0.0, 0.0)
		  .Size(18.0, 18.0)
      .Anchor(inkEAnchor.LeftFillVerticaly)
      .Reparent(top)
      .BuildImage();

    let panel = inkWidgetBuilder.inkFlex(n"panel")
      .Margin(0.0, 24.0, 0.0, 0.0)
      .Padding(10.0, 10.0, 10.0, 10.0)
		  .FitToContent(true)
      .Reparent(this.m_info)
		  .Anchor(inkEAnchor.Fill)
      .BuildFlex();

    inkWidgetBuilder.inkImage(n"frame")
      .Reparent(panel)
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
		  .Part(n"tooltip_side_gap_flip")
		  .NineSliceScale(true)
      .Margin(0.0, 0.0, 0.0, 20.0)
		  .Tint(FlightUtils.ElectricBlue())
      .Tint(n"Default.main")
		  .Opacity(1.0)
      .BuildImage();
    //  .BindProperty(n"tintColor", n"Briefings.BackgroundColour");
      
   inkWidgetBuilder.inkImage(n"background")
      .Reparent(panel)
      .Anchor(inkEAnchor.Fill)
      .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
      .Part(n"frame_small_bg")
      .NineSliceScale(true)
      .Margin(20.0, 5.0, 10.0, 30.0)
      .Tint(FlightUtils.PureBlack())
      .Opacity(1.0)
      .BuildImage();
    
    inkWidgetBuilder.inkText(n"fluff_text1")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Text("[  0.0000@0]  VDECO(LOW)   :0x09A00000 - 0x19A00000 (256MiB)")
      .Margin(30.0, 15.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkText(n"fluff_text2")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Text("[  0.0000@0]  PPMGRO(HIGH) :0x07A00000 - 0x07000000 (46MiB)")
      .Margin(30.0, 35.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkText(n"fluff_text3")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Text("[  0.0000@0]  VDIN10(LOW)  :0x19A00000 - 0x1AA00000 (16MiB)")
      .Margin(30.0, 55.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkImage(n"position_fluff")
      .Reparent(this.m_info)
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"fluff_06_L")
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Opacity(1.0)
      .Margin(30.0, 100.0, 0.0, 0.0)
		  .Size(23.0, 24.0)
      .BuildImage();

    this.m_position = inkWidgetBuilder.inkText(n"position")
      .Reparent(this.m_info)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(18)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Text(" 1550.52,   850.68,    87.34")
      .Margin(60.0, 104.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();
    this.m_position;

  }

  private let m_pitch: ref<inkCanvas>;
  private let m_pitchMarkers: ref<inkCanvas>;
  private let m_lastPitchValue: Float;
  
  protected cb func OnVehiclePitchChanged(pitch: Float) -> Bool {
    this.m_pitchMarkers.SetTranslation(0.0, (pitch - 90.0) * 20.0);
    this.m_pitchMarkers.SetEffectParamValue(inkEffectType.BoxBlur, n"BoxBlur_0", n"intensity", AbsF(pitch - this.m_lastPitchValue) * 0.001);
    this.m_lastPitchValue = pitch;
  }

  private func SetupPitchDisplay() -> Void {

      let mark_scale = 20.0;
      let height = 520.0;
      let width = 60.0;

      this.m_pitch = inkWidgetBuilder.inkCanvas(n"m_pitch")
        .Size(width, height)
        .Reparent(this.GetRootCompoundWidget())
        .Anchor(inkEAnchor.TopLeft)
        .Anchor(0.5, 0.5)
        .Margin(0.0, 0.0, 0.0, 0.0)
        .Translation(314.0, 320.0)
        // .Opacity(0.5)
        .BuildCanvas();
      // this.m_pitch.SetChildOrder(inkEChildOrder.Backward);

      inkWidgetBuilder.inkImage(n"arrow")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_right_bg")
        .Size(20.0, 20.0)
        .Anchor(1.0, 0.5)
        .Anchor(inkEAnchor.CenterLeft)
        .Margin(-15.0, 0.0, 0.0, 0.0)
        .Opacity(1.0)
        .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
        .BuildImage();
        
      inkWidgetBuilder.inkText(n"fluff_text")
        .Reparent(this.m_pitch)
        .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
        .FontSize(12)
        .Anchor(0.0, 1.0)
        .Anchor(inkEAnchor.TopLeft)
        .Tint(FlightUtils.Bittersweet())
        .Tint(n"Default.accent_color1")
        .Text("89V_PITCH")
        .HAlign(inkEHorizontalAlign.Left)
        .Margin(-14.0, -10.0, 0.0, 0.0)
        // .Overflow(textOverflowPolicy.AdjustToSize) pArrayType was nullptr.
        .BuildText();

      inkWidgetBuilder.inkImage(n"border")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_cell_fg")
        .Size(width + 24.0, height)
        .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.CenterFillVerticaly)
        .Translation(-2.5, 0.0)
        .Opacity(1.0)
        .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
        .BuildImage();

      inkWidgetBuilder.inkImage(n"fill")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_cell_bg")
        .Size(width + 24.0, height)
        .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.CenterFillVerticaly)
        .Translation(-2.5, 0.0)
        .Opacity(0.1)
        .Tint(FlightUtils.PureBlack())
        .BuildImage();

      inkWidgetBuilder.inkMask(n"mask")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\quests\\q000\\aerondight_rayfield\\assets\\logo_and_mask.inkatlas")
        .Part(n"Maska_textura")
        // .Size(width, height)
        // .Opacity(1.0)     
        .FitToContent(true)
        // .MaskTransparency(1.0)
        // .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        // .InvertMask(true)
        .Anchor(inkEAnchor.Centered)
        // .MaskSource(inkMaskDataSource.TextureAtlas)
        .BuildMask();
      // mask.SetDynamicTextureMask(n"fill");
      // mask.SetEffectEnabled(inkEffectType.Mask, n"Mask_0", true);
      // mask.SetEffectParamValue(inkEffectType.Mask, n"Mask_0", n"opacity", 1.0);

      // FlightLog.Info("Does arrow_cell_bg exist on the mask atlas? " + ToString(mask.IsTexturePartExist(n"arrow_cell_bg")));

     this.m_pitchMarkers = inkWidgetBuilder.inkCanvas(n"markers")
        .Size(width, 180.0 * mark_scale)
        .Reparent(this.m_pitch)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.Centered)
        .Margin(0.0, 0.0, 0.0, 0.0)
        .BuildCanvas();
      this.m_pitchMarkers.CreateEffect(n"inkBoxBlurEffect", n"BoxBlur_0");
      this.m_pitchMarkers.SetEffectEnabled(inkEffectType.BoxBlur, n"BoxBlur_0", true);
      this.m_pitchMarkers.SetEffectParamValue(inkEffectType.BoxBlur, n"BoxBlur_0", n"intensity", 0.0);
      this.m_pitchMarkers.SetBlurDimension(n"BoxBlur_0", inkEBlurDimension.Vertical);
      // this.m_pitchMarkers.CreateEffect(n"inkMaskEffect", n"Mask_0");
      // this.m_pitchMarkers.SetEffectEnabled(inkEffectType.Mask, n"Mask_0", true);
		  // markers.SetRenderTransformPivot(new Vector2(0.0, 0.0));

      let midbar_size = 16.0;
      let marks: array<Float> = [-100.0, -90.0, -80.0, -70.0, -60.0, -50.0, -40.0, -30.0, -20.0, -10.0, 0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0];
      let marks_inc: array<Float> = [-4.0, -3.0, -2.0, -1.0, 1.0, 2.0, 3.0, 4.0, 5.0];

      inkWidgetBuilder.inkRectangle(n"m1_00000")
        .Tint(FlightUtils.Bittersweet())
        .Tint(n"Default.accent_color1")
        // .Opacity(mark == 0.0 ? 1.0 : 0.5)
        .Opacity(1.0)
        .Reparent(this.m_pitchMarkers)
        .Size(width, 19.0)
        .Anchor(0.0, 0.0)
        .Translation(0.0, 90.0 * mark_scale - 20.0)
        .BuildRectangle();

      inkWidgetBuilder.inkRectangle(n"m1_00001")
        .Tint(FlightUtils.Bittersweet())
        .Tint(n"Default.accent_color1")
        // .Opacity(mark == 0.0 ? 1.0 : 0.5)
        .Opacity(1.0)
        .Reparent(this.m_pitchMarkers)
        .Size(width, 19.0)
        .Anchor(0.0, 1.0)
        .Translation(0.0, 90.0 * mark_scale + 20.0)
        .BuildRectangle();

      // let text = inkWidgetBuilder.inkText(n"text")
      //   .Reparent(this.m_pitchMarkers)
      //   .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily", n"Heavy")
      //   .FontSize(16)
      //   .Anchor(0.5, 0.5)
      //   .Tint(FlightUtils.PureBlack())
      //   .Text("LEVEL")
      //   .Opacity(1.0)
      //   .HAlign(inkEHorizontalAlign.Center)
      //   .VAlign(inkEVerticalAlign.Center)
      //   .Margin(0.0, 0.0, 0.0, 0.0)
      //   .Translation(width / 2.0, 90.0 * mark_scale)
      //   // .Overflow(textOverflowPolicy.AdjustToSize)
      //   .BuildText();

      for mark in marks {
        if mark != 0.0 {
          inkWidgetBuilder.inkText(n"text")
            .Reparent(this.m_pitchMarkers)
            .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
            .FontSize(20)
            .Anchor(0.5, 0.5)
            .Tint(FlightUtils.ElectricBlue())
            .Tint(n"Default.main")
            .Text(FloatToStringPrec(AbsF(mark), 0))
            .HAlign(inkEHorizontalAlign.Center)
            .Margin(0.0, 0.0, 0.0, 0.0)
            .Translation(width / 2.0, (mark + 90.0) * mark_scale)
            // .Overflow(textOverflowPolicy.AdjustToSize)
            .BuildText();

          inkWidgetBuilder.inkRectangle(StringToName("m1_" + FloatToString(mark)))
            .Tint(FlightUtils.ElectricBlue())
            .Tint(n"Default.main")
            // .Opacity(mark == 0.0 ? 1.0 : 0.5)
            .Size(midbar_size, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(width - midbar_size, (mark + 90.0) * mark_scale)
            .Reparent(this.m_pitchMarkers)
            .BuildRectangle();

          inkWidgetBuilder.inkRectangle(StringToName("m2_" + FloatToString(mark)))
            .Tint(FlightUtils.ElectricBlue())
            .Tint(n"Default.main")
            // .Opacity(mark == 0.0 ? 1.0 : 0.5)
            .Size(midbar_size, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(0.0, (mark + 90.0) * mark_scale)
            .Reparent(this.m_pitchMarkers)
            .BuildRectangle();
        }
        for mark_inc in marks_inc {
          inkWidgetBuilder.inkRectangle(StringToName("m_" + FloatToString(mark + mark_inc)))
            .Tint(FlightUtils.ElectricBlue())
            .Tint(n"Default.main")
            .Opacity(mark_inc == 5.0 ? 0.5 : 0.1)
            .Size(width, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(0.0, ((mark + 90.0) + mark_inc) * mark_scale)
            .Reparent(this.m_pitchMarkers)
            .BuildRectangle();
        }
      } 
  }

  protected cb func OnUninitialize() -> Bool {
    if IsDefined(this.m_vehicleBBUIActivId) {
      this.m_vehicleFlightBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this.m_vehicleBBUIActivId);
    };
    this.UnregisterBlackBoardCallbacks();
  }

  private final func ActivateUI() -> Void {
    this.m_rootWidget.SetVisible(true);
    this.RegisterBlackBoardCallbacks();
    this.CheckIfVehicleShouldTurnOn();
  }

  private final func DeactivateUI() -> Void {
    this.UnregisterBlackBoardCallbacks();
    this.m_rootWidget.SetVisible(false);
  }

  protected cb func OnActivateUI(activate: Bool) -> Bool {
    let evt: ref<VehicleFlightUIActivationEvent> = new VehicleFlightUIActivationEvent();
    if activate {
      evt.m_activate = true;
    } else {
      evt.m_activate = false;
    };
    this.QueueEvent(evt);
  }

  protected cb func OnActivateUIEvent(evt: ref<VehicleFlightUIActivationEvent>) -> Bool {
    if evt.m_activate {
      this.ActivateUI();
    } else {
      this.DeactivateUI();
    };
  }

  protected cb func OnVehicleReady(ready: Bool) -> Bool {
    if ready {
      this.m_rootWidget.SetVisible(true);
    } else {
      if !ready {
        this.m_rootWidget.SetVisible(false);
      };
    };
    this.m_isVehicleReady = ready;
  }

  private final func RegisterBlackBoardCallbacks() -> Void {
    if IsDefined(this.m_vehicleBlackboard) {
      if !IsDefined(this.m_vehicleBBStateConectionId) {
        this.m_vehicleBBStateConectionId = this.m_vehicleBlackboard.RegisterListenerInt(GetAllBlackboardDefs().Vehicle.VehicleState, this, n"OnVehicleStateChanged");
      };
      if !IsDefined(this.m_vehicleCollisionBBStateID) {
        this.m_vehicleCollisionBBStateID = this.m_vehicleBlackboard.RegisterListenerBool(GetAllBlackboardDefs().Vehicle.Collision, this, n"OnVehicleCollision");
      };
    }
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if !IsDefined(this.m_vehiclePitchID) {
        this.m_vehiclePitchID = this.m_vehicleFlightBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this, n"OnVehiclePitchChanged");
      };
      if !IsDefined(this.m_vehiclePositionID) {
        this.m_vehiclePositionID = this.m_vehicleFlightBlackboard.RegisterListenerVector4(GetAllBlackboardDefs().VehicleFlight.Position, this, n"OnVehiclePositionChanged");
      };
      this.InitializeWidgetStyleSheet(this.m_vehicle);
    };
  }

  private final func UnregisterBlackBoardCallbacks() -> Void {
    if IsDefined(this.m_vehicleBlackboard) {
      if IsDefined(this.m_vehicleBBStateConectionId) {
        this.m_vehicleBlackboard.UnregisterListenerInt(GetAllBlackboardDefs().Vehicle.VehicleState, this.m_vehicleBBStateConectionId);
      };
      if IsDefined(this.m_vehicleCollisionBBStateID) {
        this.m_vehicleBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().Vehicle.Collision, this.m_vehicleCollisionBBStateID);
      };
    }
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if IsDefined(this.m_vehiclePitchID) {
        this.m_vehicleFlightBlackboard.UnregisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this.m_vehiclePitchID);
      };
      if IsDefined(this.m_vehiclePositionID) {
        this.m_vehicleFlightBlackboard.UnregisterListenerVector4(GetAllBlackboardDefs().VehicleFlight.Position, this.m_vehiclePositionID);
      };
    };
  }

  private final func IsUIactive() -> Bool {
    if IsDefined(this.m_vehicleFlightBlackboard) && this.m_vehicleFlightBlackboard.GetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive) {
      return true;
    };
    return false;
  }

  private final func InitializeWidgetStyleSheet(veh: wref<VehicleObject>) -> Void {
    let record: wref<Vehicle_Record> = veh.GetRecord();
    let styleSheetPath: ResRef = record.WidgetStyleSheetPath();
    this.m_rootWidget.SetStyle(styleSheetPath);
  }

  private final func CheckIfVehicleShouldTurnOn() -> Void {
    if this.m_vehiclePS.GetIsUiQuestModified() {
      if this.m_vehiclePS.GetUiQuestState() {
        this.TurnOn();
      };
      return;
    };
    if this.m_vehicleBlackboard.GetInt(GetAllBlackboardDefs().Vehicle.VehicleState) == EnumInt(vehicleEState.On) {
      this.TurnOn();
    };
  }

  protected cb func OnVehicleStateChanged(state: Int32) -> Bool {
    if this.m_vehiclePS.GetIsUiQuestModified() {
      return false;
    };
    if state == EnumInt(vehicleEState.On) {
      this.TurnOn();
    };
    if state == EnumInt(vehicleEState.Default) {
      this.TurnOff();
    };
    if state == EnumInt(vehicleEState.Disabled) {
      this.TurnOff();
    };
    if state == EnumInt(vehicleEState.Destroyed) {
      this.TurnOff();
    };
  }

  private final func TurnOn() -> Void {
    this.KillBootupProxy();
    if this.m_UIEnabled {
      this.PlayIdleLoop();
    } else {
      this.m_UIEnabled = true;
      this.m_startAnimProxy = this.PlayLibraryAnimation(n"start");
      this.m_startAnimProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnStartAnimFinished");
      this.EvaluateWidgetStyle(GameInstance.GetTimeSystem(this.m_vehicle.GetGame()).GetGameTime());
    };
  }

  private final func TurnOff() -> Void {
    this.m_UIEnabled = false;
    this.KillBootupProxy();
    if IsDefined(this.m_startAnimProxy) {
      this.m_startAnimProxy.Stop();
    };
    if IsDefined(this.m_loopAnimProxy) {
      this.m_loopAnimProxy.Stop();
    };
    this.m_endAnimProxy = this.PlayLibraryAnimation(n"end");
    this.m_endAnimProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnEndAnimFinished");
  }

  protected cb func OnStartAnimFinished(anim: ref<inkAnimProxy>) -> Bool {
    this.PlayIdleLoop();
  }

  private final func PlayIdleLoop() -> Void {
    let animOptions: inkAnimOptions;
    animOptions.loopType = inkanimLoopType.Cycle;
    animOptions.loopInfinite = true;
    this.m_loopAnimProxy = this.PlayLibraryAnimation(n"loop", animOptions);
  }

  protected cb func OnEndAnimFinished(anim: ref<inkAnimProxy>) -> Bool {
    this.m_rootWidget.SetState(n"inactive");
  }

  private final func PlayLibraryAnim(animName: CName) -> Void {
    this.PlayLibraryAnimation(animName);
  }

  public final func EvaluateWidgetStyle(time: GameTime) -> Void {
    let currTime: GameTime;
    let sunRise: GameTime;
    let sunSet: GameTime;
    if this.m_UIEnabled {
      sunSet = GameTime.MakeGameTime(0, 20, 0, 0);
      sunRise = GameTime.MakeGameTime(0, 5, 0, 0);
      currTime = GameTime.MakeGameTime(0, GameTime.Hours(time), GameTime.Minutes(time), GameTime.Seconds(time));
      if currTime <= sunSet && currTime >= sunRise {
        if NotEquals(this.m_rootWidget.GetState(), n"day") {
          this.m_rootWidget.SetState(n"day");
        };
      } else {
        if NotEquals(this.m_rootWidget.GetState(), n"night") {
          this.m_rootWidget.SetState(n"night");
        };
      };
    };
  }

  protected cb func OnVehicleCollision(collision: Bool) -> Bool {
    this.PlayLibraryAnimation(n"glitch");
  }

  protected cb func OnForwardVehicleQuestEnableUIEvent(evt: ref<ForwardVehicleQuestEnableUIEvent>) -> Bool {
    switch evt.mode {
      case vehicleQuestUIEnable.Gameplay:
        this.CheckIfVehicleShouldTurnOn();
        break;
      case vehicleQuestUIEnable.ForceEnable:
        this.TurnOn();
        break;
      case vehicleQuestUIEnable.ForceDisable:
        this.TurnOff();
    };
  }

  protected cb func OnVehiclePanzerBootupUIQuestEvent(evt: ref<VehiclePanzerBootupUIQuestEvent>) -> Bool {
    let animOptions: inkAnimOptions;
    this.m_UIEnabled = true;
    this.m_rootWidget.SetVisible(true);
    animOptions.loopType = inkanimLoopType.Cycle;
    animOptions.loopInfinite = true;
    switch evt.mode {
      case panzerBootupUI.UnbootedIdle:
        this.KillBootupProxy();
        this.m_loopingBootProxy = this.PlayLibraryAnimation(n"1_unbooted_idle", animOptions);
        break;
      case panzerBootupUI.BootingAttempt:
        this.KillBootupProxy();
        this.m_loopingBootProxy = this.PlayLibraryAnimation(n"2_booting_attempt", animOptions);
        break;
      case panzerBootupUI.BootingSuccess:
        this.KillBootupProxy();
        this.m_loopingBootProxy = this.PlayLibraryAnimation(n"3_booting_success");
        break;
      case panzerBootupUI.Loop:
        this.KillBootupProxy();
        this.m_loopingBootProxy = this.PlayLibraryAnimation(n"loop", animOptions);
    };
  }

  private final func KillBootupProxy() -> Void {
    if IsDefined(this.m_loopingBootProxy) {
      this.m_loopingBootProxy.Stop();
    };
  }

  protected cb func OnForwardVehicleQuestUIEffectEvent(evt: ref<ForwardVehicleQuestUIEffectEvent>) -> Bool {
    if evt.glitch {
      this.PlayLibraryAnimation(n"glitch");
    };
    if evt.panamVehicleStartup {
      this.PlayLibraryAnimation(n"start_panam");
    };
    if evt.panamScreenType1 {
      this.PlayLibraryAnimation(n"panam_screen_type1");
    };
    if evt.panamScreenType2 {
      this.PlayLibraryAnimation(n"panam_screen_type2");
    };
    if evt.panamScreenType3 {
      this.PlayLibraryAnimation(n"panam_screen_type3");
    };
    if evt.panamScreenType4 {
      this.PlayLibraryAnimation(n"panam_screen_type4");
    };
  }
}


// _blackboardDefinitions.reds


public class VehicleFlightDef extends BlackboardDefinition {

  public let IsActive: BlackboardID_Bool;
  public let Mode: BlackboardID_Int;
  public let IsUIActive: BlackboardID_Bool;
  public let Orientation: BlackboardID_Quat;
  public let Force: BlackboardID_Vector4;
  public let Torque: BlackboardID_Vector4;
  public let Position: BlackboardID_Vector4;
  public let Pitch: BlackboardID_Float;
  public let Roll: BlackboardID_Float;

  public const func AutoCreateInSystem() -> Bool {
    return true;
  }
}

@addField(AllBlackboardDefinitions)
public let VehicleFlight: ref<VehicleFlightDef>;

// @addField(VehicleDef)
// public let IsFlightActive: BlackboardID_Bool;

// @addField(VehicleDef)
// public let FlightMode: BlackboardID_Int;

// @addField(VehicleDef)
// public let IsFlightUIActive: BlackboardID_Bool;

// @addField(VehicleDef)
// public let Orientation: BlackboardID_Quat;

// @addField(VehicleDef)
// public let Pitch: BlackboardID_Float;

// @addField(VehicleDef)
// public let Force: BlackboardID_Vector4;

// @addField(VehicleDef)
// public let Torque: BlackboardID_Vector4;

// @addField(VehicleDef)
// public let Position: BlackboardID_Vector4;

// _hudCarController.reds

// @wrapMethod(hudCarController)
// private final func Reset() -> Void {
//   wrappedMethod();
//   this.OnFlightActiveChanged(false);
// }

// @addField(hudCarController)
// private let m_flightActiveBBConnectionId: ref<CallbackHandle>;

// @addField(hudCarController)
// private let m_flightModeBBConnectionId: ref<CallbackHandle>;

// @addField(hudCarController)
// private let m_flightControllerStatus: wref<inkText>;

// @wrapMethod(hudCarController)
// private final func RegisterToVehicle(register: Bool) -> Void {
//   wrappedMethod(register);
  // let flightControllerBlackboard: wref<IBlackboard>;
  // let vehicle: ref<VehicleObject> = this.m_activeVehicle;
  // if vehicle == null {
  //   return;
  // };
  // flightControllerBlackboard = FlightController.GetInstance().GetBlackboard();
  // if IsDefined(flightControllerBlackboard) {
  //   if register {
  //     // GetRootWidget() returns root widget of base type inkWidget
  //     // GetRootCompoundWidget() returns root widget casted to inkCompoundWidget
  //     if !IsDefined(this.m_flightControllerStatus) {
  //       this.m_flightControllerStatus = FlightController.HUDStatusSetup(this.GetRootCompoundWidget());
  //     }
  //     this.m_flightActiveBBConnectionId = flightControllerBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this, n"OnFlightActiveChanged");
  //     this.m_flightModeBBConnectionId = flightControllerBlackboard.RegisterListenerInt(GetAllBlackboardDefs().VehicleFlight.Mode, this, n"OnFlightModeChanged");
  //     this.FlightActiveChanged(FlightController.GetInstance().active);
  //   } else {
  //     flightControllerBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this.m_flightActiveBBConnectionId);
  //     flightControllerBlackboard.UnregisterListenerInt(GetAllBlackboardDefs().VehicleFlight.Mode, this.m_flightModeBBConnectionId);
  //   };
  // };
// }

// @addMethod(hudCarController)
// protected cb func OnFlightActiveChanged(active: Bool) -> Bool {
//   if !IsDefined(this.m_flightControllerStatus) {
//     this.m_flightControllerStatus = FlightController.HUDStatusSetup(this.GetRootCompoundWidget());
//   }
//   this.FlightActiveChanged(active);
// }

// @addMethod(hudCarController)
// protected func FlightActiveChanged(active: Bool) -> Void {
//   if active {
//     this.m_flightControllerStatus.SetText("Flight Active: " + fs().playerComponent.GetFlightMode().GetDescription());
//   } else {
//     this.m_flightControllerStatus.SetText("Flight Available");
//   }
// }

// @addMethod(hudCarController)
// protected cb func OnFlightModeChanged(mode: Int32) -> Bool {
//   this.m_flightControllerStatus.SetText("Flight Active: " + fs().playerComponent.GetFlightMode().GetDescription());
// }

@wrapMethod(hudCarController)
protected cb func OnSpeedValueChanged(speedValue: Float) -> Bool {
  // speedValue = AbsF(speedValue);
  // let multiplier: Float = GameInstance.GetStatsDataSystem(this.m_activeVehicle.GetGame()).GetValueFromCurve(n"vehicle_ui", speedValue, n"speed_to_multiplier");
  // inkTextRef.SetText(this.m_SpeedValue, IntToString(RoundMath(speedValue)));

  let fc = fs().playerComponent;
  if fc.active {
    let speed = AbsF(fc.stats.d_speed);
    let multiplier: Float = GameInstance.GetStatsDataSystem(this.m_activeVehicle.GetGame()).GetValueFromCurve(n"vehicle_ui", speed, n"speed_to_multiplier");
    inkTextRef.SetText(this.m_SpeedValue, IntToString(RoundMath(speed * multiplier)));
    this.drawRPMGaugeFull(AbsF(fc.surge) * 5000.0);
  } else {
    wrappedMethod(speedValue);
  }
}
@wrapMethod(hudCarController)
protected cb func OnRpmValueChanged(rpmValue: Float) -> Bool {
  let fc = fs().playerComponent;
  if !fc.active {
    wrappedMethod(rpmValue);
  }
}

// _inkBorder.reds

@addField(inkBorder)
native let thickness: Float;

@addMethod(inkBorder)
public func SetThickness(thickness: Float) {
    this.thickness = thickness;
}
@addMethod(inkBorder)
public func GetThickness() -> Float{
    return this.thickness;
}

// _inkMask.reds

enum inkMaskDataSource {
    TextureAtlas = 0,
    DynamicTexture = 1
}

// public native class inkTextureAtlas {

// }

// @addField(inkMask)
// native let textureAtlas: inkTextureAtlas;
// @addField(inkMask)
// native let texturePart: CName;
@addField(inkMask)
native let dynamicTextureMask: CName;

@addMethod(inkMask)
func SetDynamicTextureMask(value: CName) {
    this.dynamicTextureMask = value;
}

@addField(inkMask)
native let dataSource: inkMaskDataSource;

@addMethod(inkMask)
func SetDataSource(value: inkMaskDataSource) {
    this.dataSource = value;
}

@addField(inkMask)
let useNineSliceScale: Bool;

@addField(inkMask)
let nineSliceScale: inkMargin;

@addMethod(inkMask)
public func UsesNineSliceScale() -> Bool {
	return this.useNineSliceScale;
}

@addMethod(inkMask)
public func SetNineSliceScale(enable: Bool) -> Void {
	this.useNineSliceScale = enable;
}

@addMethod(inkMask)
public func GetNineSliceGrid() -> inkMargin {
	return this.nineSliceScale;
}

@addMethod(inkMask)
public func SetNineSliceGrid(grid: inkMargin) -> Void {
	this.nineSliceScale = grid;
}

@addField(inkMask)
native let invertMask: Bool;

@addMethod(inkMask)
func SetInvertMask(value: Bool) {
    this.invertMask = value;
}

@addField(inkMask)
native let maskTransparency: Float;

@addMethod(inkMask)
func SetMaskTransparency(value: Float) {
    this.maskTransparency = value;
}


// @addMethod(inkMask)
// func SetAtlasResource(textureAtlas: ResRef) {
//     this.textureAtlas = textureAtlas;
// }
@addMethod(inkMask)
public native func SetAtlasResource(atlasResourcePath: ResRef) -> Bool;

// _inkQuadShape.reds

public native class inkQuadShape extends inkBaseShapeWidget {
    // native let textureAtlas: ResRef;
    native let texturePart: CName;
    native let vertexList: array<Vector2>;

    // public func GetTextureAtlas() -> ResRef {
    //     return this.textureAtlas;
    // }
    public func GetTexturePart() -> CName {
        return this.texturePart;
    }
    public func GetVertexList() -> array<Vector2> {
        return this.vertexList;
    }
}

// _inkWidget.reds

@addMethod(inkWidget)
public native func CreateEffect(typeName: CName, effectName: CName) -> Void;

enum inkEBlurDimension
{
   Horizontal = 0,
   Vertical = 1
}

@addMethod(inkWidget)
public native func SetBlurDimension(effectName: CName, blurDimension : inkEBlurDimension) -> Bool;

// _MeshComponent.reds

@addField(MeshComponent)
public native let visualScale: Vector3;

// _Transitions.reds

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
    if scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active) {
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

// _VehicleComponent.reds

@replaceMethod(VehicleComponent)
protected cb func OnVehicleWaterEvent(evt: ref<VehicleWaterEvent>) -> Bool {
  if evt.isInWater  && !this.GetPS().GetIsSubmerged() {
    if !Equals(GetMountedVehicle(FlightController.GetInstance().player), this.GetVehicle()) && FlightController.GetInstance().IsActive() {
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

@wrapMethod(VehicleComponent)
private final func ExplodeVehicle(instigator: wref<GameObject>) -> Void {
  wrappedMethod(instigator);
  this.GetVehicle().GetFlightComponent().isDestroyed = true;
  this.GetVehicle().GetFlightComponent().hasExploded = true;
  this.GetVehicle().GetFlightComponent().hasUpdate = false;
  this.GetVehicle().GetFlightComponent().Deactivate(true);
}

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

// _VehicleObject.reds

@addField(VehicleObject)
private let m_flightComponent: wref<FlightComponent>;

@wrapMethod(VehicleObject)
protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
  EntityRequestComponentsInterface.RequestComponent(ri, n"flightComponent", n"FlightComponent", true);
  // EntityRequestComponentsInterface.RequestComponent(ri, n"flight_ui", n"worlduiWidgetComponent", true);
  // EntityRequestComponentsInterface.RequestComponent(ri, n"flight_ui_info", n"worlduiWidgetComponent", true);
  wrappedMethod(ri);
}

@wrapMethod(VehicleObject)
protected cb func OnTakeControl(ri: EntityResolveComponentsInterface) -> Bool {
  //FlightLog.Info("[VehicleObject] OnTakeControl: " + this.GetDisplayName());
  this.m_flightComponent = EntityResolveComponentsInterface.GetComponent(ri, n"flightComponent") as FlightComponent;
  // this.m_flightComponent.ui = EntityResolveComponentsInterface.GetComponent(ri, n"flight_ui") as worlduiWidgetComponent;
  // this.m_flightComponent.ui_info = EntityResolveComponentsInterface.GetComponent(ri, n"flight_ui_info") as worlduiWidgetComponent;
  // this.m_flightComponent.Toggle(false);
  wrappedMethod(ri);
}

@addMethod(VehicleObject)
public const func GetFlightComponent() -> ref<FlightComponent> {
  return this.m_flightComponent;
}

@addMethod(VehicleObject)
public func ToggleFlightComponent(state: Bool) -> Void {
  this.m_flightComponent.Toggle(state);
}

@addMethod(VehicleObject)
public func GetLocalToWorld() -> Matrix {
  return WorldTransform.ToMatrix(this.GetWorldTransform());
}

@addField(VehicleObject)
public let chassis: ref<vehicleChassisComponent>;

@addField(VehicleObject)
@runtimeProperty("offset", "0x24C")
public native let isOnGround: Bool;

@addField(VehicleObject)
@runtimeProperty("offset", "0x254")
public native let acceleration: Float;

@addField(VehicleObject)
@runtimeProperty("offset", "0x258")
public native let deceleration: Float;

@addField(VehicleObject)
@runtimeProperty("offset", "0x25C")
public native let handbrake: Float;

// @addField(VehicleObject)
// public native let turnX: Float;

// @addField(VehicleObject)
// public native let turnX2: Float;

// @addField(VehicleObject)
// public native let turnX3: Float;

@addField(VehicleObject)
@runtimeProperty("offset", "0x268")
public native let turnX: Float;

@addField(VehicleObject)
@runtimeProperty("offset", "0x950")
public native let tracePosition: Vector3;

@addMethod(VehicleObject)
public native func UsesInertiaTensor() -> Bool;

@addMethod(VehicleObject)
public native func GetInertiaTensor() -> Matrix;

// @addMethod(VehicleObject)
// public native func GetWorldInertiaTensor() -> Matrix;

@addMethod(VehicleObject)
public native func GetMomentOfInertiaScale() -> Vector3;

@addMethod(VehicleObject)
public native func GetCenterOfMass() -> Vector3;

@addMethod(VehicleObject)
public native func GetAngularVelocity() -> Vector3;

@addMethod(VehicleObject)
public native func TurnOffAirControl() -> Bool;

public native class vehicleFlightHelper extends IScriptable {
    public native let force: Vector4;
    public native let torque: Vector4;
}

@addMethod(VehicleObject)
public native func AddFlightHelper() -> ref<vehicleFlightHelper>;

@addMethod(VehicleObject)
public native func GetComponentsUsingSlot(slotName: CName) -> array<ref<IComponent>>;

@addMethod(VehicleObject)
public native func GetWeaponPlaceholderOrientation(index: Int32) -> Quaternion;

@addMethod(VehicleObject)
public native func GetWeapons() -> array<ref<WeaponObject>>;

// working
// @addMethod(VehicleObject)
// protected cb func OnPhysicalCollision(evt: ref<PhysicalCollisionEvent>) -> Bool {
//   FlightLog.Info("[VehicleObject] OnPhysicalCollision");
//   let vehicle = evt.otherEntity as VehicleObject;
//   if IsDefined(vehicle) {
//     let gameInstance: GameInstance = this.GetGame();
//     let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
//     let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), vehicle);
//     if isPlayerMounted {
//       // FlightController.GetInstance().ProcessImpact(evt.attackData.vehicleImpactForce);
//     } else {
//       let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
//       impulseEvent.radius = 1.0;
//       impulseEvent.worldPosition = Vector4.Vector4To3(evt.worldPosition);
//       impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 10000.0);
//       vehicle.QueueEvent(impulseEvent);
//     }
//   }
// }

@wrapMethod(VehicleObject)
public final func IsOnPavement() -> Bool {
  return wrappedMethod() || FlightController.GetInstance().IsActive();
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

// @addMethod(VehicleObject)
// public const func IsQuickHackAble() -> Bool {
//   return true;
// }

// @addMethod(VehicleObject)
// public const func IsQuickHacksExposed() -> Bool {
//   return true;
// }

// _weaponRoster.reds

@addField(weaponRosterGameController)
let m_FlightStateBlackboardId: ref<CallbackHandle>;

@wrapMethod(weaponRosterGameController)
private final func RegisterBB() -> Void {
  wrappedMethod();
  let flightBB = FlightController.GetInstance().GetBlackboard();
  if IsDefined(flightBB) {
    if !IsDefined(this.m_FlightStateBlackboardId) {
      this.m_FlightStateBlackboardId = flightBB.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this, n"OnFlightActivate");
    }
  }
}

@wrapMethod(weaponRosterGameController)
private final func UnregisterBB() -> Void {
  wrappedMethod();
  let flightBB = FlightController.GetInstance().GetBlackboard();
  if IsDefined(flightBB) {
    if IsDefined(this.m_FlightStateBlackboardId) {
       flightBB.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this.m_FlightStateBlackboardId);
    }
  }
}

@addMethod(weaponRosterGameController)
private cb func OnFlightActivate() -> Void {
  this.PlayFold();
}

