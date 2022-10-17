// Let There Be Flight
// (C) 2022 Jack Humbert
// https://github.com/jackhumbert/let_there_be_flight
// This file was automatically generated on 2022-10-17 14:57:27.7959844

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
  public let roll: Float;
  public let pitch: Float;
  public let sway: Float;
}


public native class FlightAudio extends IScriptable {
  // defined in red4ext part
  public native func Start(emitterName: String, eventName: String) -> Void;
  public native func StartWithPitch(emitterName: String, eventName: String, pitch: Float) -> Void;
  public native func Play(eventName: String) -> Void;
  public native func Stop(emitterName: String) -> Void;
  // public native func Update(emitterName: String, eventLocation: Vector3, eventForward: Vector3, eventUp: Vector3, volume: Float) -> Void;
  public native func UpdateEvent(emitterName: String, eventMatrix: Matrix, volume: Float, update: ref<FlightAudioUpdate>) -> Void;
  public native func UpdateEventMatrix(emitterName: String, eventMatrix: Matrix) -> Void;
  public native func UpdateListenerMatrix(matrix: Matrix) -> Void;
  public native func UpdateParameter(parameterName: String, value: Float) -> Void;


  public static func Get() -> ref<FlightAudio> {
    return FlightSystem.GetInstance().audio;
  }

  private let m_positionProviders: ref<inkHashMap>;
  private let m_orientationProviders: ref<inkHashMap>;
  private let m_positions: ref<inkHashMap>;
  private let m_orientations: ref<inkHashMap>;
  private let slots: array<CName>;
  private let uiBlackboard: wref<IBlackboard>;
  private let menuCallback: ref<CallbackHandle>;
  public let isInMenu: Bool;

  private let uiGameDataBlackboard: wref<IBlackboard>;
  private let popupCallback: ref<CallbackHandle>;
  public let isPopupShown: Bool;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Audio-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Engine-Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let engineVolume: Float = 1.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Audio-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Wind-Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let windVolume: Float = 0.6;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Audio-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Warning-Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let warningVolume: Float = 0.5;

  public static func Create() -> ref<FlightAudio> {
    let self = new FlightAudio();

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
      // n"wheel_front",
      // n"wheel_back",
      // n"wheel_front_left",
      // n"wheel_front_right",
      // n"wheel_back_left",
      // n"wheel_back_right",
      // n"bumper_back",
      n"window_front_left_a",
      n"window_front_right_a"
    ];

    // if IsDefined(self.uiBlackboard) && IsDefined(self.menuCallback) {
    //   self.uiBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_System.IsInMenu, self.menuCallback);
    // }
    // if IsDefined(self.uiGameDataBlackboard) && IsDefined(self.popupCallback) {
    //   self.uiGameDataBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown, self.popupCallback);
    // }
    
    LTBF_RegisterListener(self);

    return self;
  }
  
  public cb func Initialize() {
    this.m_positionProviders = new inkHashMap();
    this.m_positions = new inkHashMap();
    this.m_orientationProviders = new inkHashMap();
    this.m_orientations = new inkHashMap();
    this.slots = [
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
      // n"wheel_front",
      // n"wheel_back",
      // n"wheel_front_left",
      // n"wheel_front_right",
      // n"wheel_back_left",
      // n"wheel_back_right",
      // n"bumper_back",
      n"window_front_left_a",
      n"window_front_right_a"
    ];
    
    LTBF_RegisterListener(this);
  }

  protected cb func OnWorldAttached() {
    let gameInstance = FlightSystem.GetInstance().gameInstance;
    this.uiBlackboard = GameInstance.GetBlackboardSystem(gameInstance).Get(GetAllBlackboardDefs().UI_System);
    if IsDefined(this.uiBlackboard) {
      if !IsDefined(this.menuCallback) {
        this.menuCallback = this.uiBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_System.IsInMenu, this, n"OnIsInMenu");
        this.isInMenu = this.uiBlackboard.GetBool(GetAllBlackboardDefs().UI_System.IsInMenu);
      }
    }

    this.uiGameDataBlackboard = GameInstance.GetBlackboardSystem(gameInstance).Get(GetAllBlackboardDefs().UIGameData);
    if IsDefined(this.uiGameDataBlackboard) {
      if !IsDefined(this.popupCallback) {
        this.popupCallback = this.uiGameDataBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown, this, n"OnPopupIsShown");
        this.isPopupShown = this.uiGameDataBlackboard.GetBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown);
      }
    }
  }

  protected cb func OnWorldPendingDetach() {
    if IsDefined(this.uiBlackboard) && IsDefined(this.menuCallback) {
      this.uiBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_System.IsInMenu, this.menuCallback);
    }
    if IsDefined(this.uiGameDataBlackboard) && IsDefined(this.popupCallback) {
      this.uiGameDataBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown, this.popupCallback);
    }
  }

  protected cb func OnIsInMenu(inMenu: Bool) -> Bool {
    this.isInMenu = inMenu;
  }
  protected cb func OnPopupIsShown(isShown: Bool) -> Bool {
    this.isPopupShown = isShown;
  }

  public func GetGameVolume() -> Float {
    let gameInstance = FlightSystem.GetInstance().gameInstance;
    if this.isInMenu || GameInstance.GetTimeSystem(gameInstance).IsPausedState() ||
      GameInstance.GetTimeSystem(gameInstance).IsTimeDilationActive(n"HubMenu") || 
      GameInstance.GetTimeSystem(gameInstance).IsTimeDilationActive(n"WorldMap")
    {
      return 0.0;
    }
    let volume = 1.0;
    let master = Cast<Float>((GameInstance.GetSettingsSystem(gameInstance).GetVar(n"/audio/volume", n"MasterVolume") as ConfigVarInt).GetValue()) / 100.0;
    let sfx = Cast<Float>((GameInstance.GetSettingsSystem(gameInstance).GetVar(n"/audio/volume", n"SfxVolume") as ConfigVarInt).GetValue()) / 100.0;
    volume *= master * sfx;

    // might need to handle just the scanning system's dilation, and the pause menu
    if GameInstance.GetTimeSystem(gameInstance).IsTimeDilationActive(n"radialMenu") {
      volume *= 0.1;
    }
  
    return volume;
  }

  public func GetEngineVolume() -> Float {
    return this.engineVolume;
  }

  public func GetWindVolume() -> Float {
    return this.windVolume;
  }

  public func GetWarningVolume() -> Float {
    return this.warningVolume;
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

  // public func DrawSlotPositions(ui: ref<FlightControllerUI>) {
  //   for slot in this.slots {
  //     let position = this.GetPosition(slot);
  //     ui.DrawMark(position);
  //     ui.DrawText(position, NameToString(slot));
  //   }
  // }

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
  public static func Create(v: Vector4) -> ref<Vector4Wrapper> {
    let vw = new Vector4Wrapper();
    vw.vector = v;
    return vw;
  }
}

public class OrientationWrapper {
  public let quaternion: Quaternion;
}

// FlightComponent.reds


public native class FlightComponentPS extends GameComponentPS {

}

public native class FlightComponent extends GameComponent {
  @runtimeProperty("offset", "0xA8")
  public native let sys: ref<FlightSystem>;
  
  @runtimeProperty("offset", "0xB8")
  public native let active: Bool;

  @runtimeProperty("offset", "0xB9")
  public native let hasUpdate: Bool;

  @runtimeProperty("offset", "0xC0")
  public native let force: Vector4;

  @runtimeProperty("offset", "0xD0")
  public native let torque: Vector4;

  public native func ChaseTarget(target: wref<GameObject>) -> Void;
  // public native func ChaseTarget() -> Void;

  public let thrusters: array<ref<FlightThruster>>;
  private let helper: ref<vehicleFlightHelper>;
  private let stats: ref<FlightStats>;

  public let m_interaction: ref<InteractionComponent>;
  public let m_healthStatPoolListener: ref<VehicleHealthStatPoolListener>;
  public let m_vehicleBlackboard: wref<IBlackboard>;
  public let m_vehicleTPPCallbackID: ref<CallbackHandle>;

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

  // public let ui: wref<worlduiWidgetComponent>;
  // public let ui_info: wref<worlduiWidgetComponent>;

  public let alarmIsPlaying: Bool;

  protected final const func GetVehicle() -> wref<VehicleObject> {
    return this.GetEntity() as VehicleObject;
  }
  
  private final const func GetMyPS() -> ref<FlightComponentPS> {
    return this.GetPS() as FlightComponentPS;
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
    this.sys.RegisterComponent(this);
    this.sqs = GameInstance.GetSpatialQueriesSystem(this.GetVehicle().GetGame());
    // this.fx = FlightFx.Create(this);
    // this.thrusters = FlightThruster.CreateThrusters(this);
    
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
    let hoverMode = FlightModeHover.Create(this);
    if hoverMode.enabled {
      ArrayPush(this.modes, hoverMode);
    } else {
      hoverMode.Deinitialize();
    }
    let automaticMode = FlightModeAutomatic.Create(this);
    if automaticMode.enabled {
      ArrayPush(this.modes, automaticMode);
    } else {
      automaticMode.Deinitialize();
    }
    let flyMode = FlightModeFly.Create(this);
    if flyMode.enabled {
      ArrayPush(this.modes, flyMode);
    } else {
      flyMode.Deinitialize();
    }
    let agDroneMode = FlightModeDroneAntiGravity.Create(this);
    if agDroneMode.agEnabled {
      ArrayPush(this.modes, agDroneMode);
    } else {
      agDroneMode.Deinitialize();
    }
    let droneMode = FlightModeDrone.Create(this);
    if droneMode.enabled {
      ArrayPush(this.modes, droneMode);
    } else {
      droneMode.Deinitialize();
    }

    this.audioUpdate = new FlightAudioUpdate();
    
    if ArraySize(this.thrusters) == 0 {
      this.thrusters = FlightThruster.CreateThrusters(this);
    }
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
    for mode in this.modes {
      mode.Deinitialize();
    }
    this.sys.UnregisterComponent(this);
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
    return ClampF(700.0 / this.GetVehicle().GetTotalMass() + 0.5, 0.5, 1.5);
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
    LTBF_RegisterListener(this);
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if mountChild.IsPlayer() {
      FlightLog.Info("[FlightComponent] OnMountingEvent: " + this.GetVehicle().GetDisplayName());
      // this.GetVehicle().TurnOffAirControl();
      this.SetupVehicleTPPBBListener();
      // FlightLog.Info("[FlightComponent] OnMountingEvent: " + this.GetVehicle().GetDisplayName());
      FlightAudio.Get().Start("windLeft", "wind_TPP");
      FlightAudio.Get().Start("windRight", "wind_TPP");
      // (this.GetVehicle().FindComponentByName(n"cars_sport_fx") as EffectSpawnerComponent).AddEffect();
      this.sys.playerComponent = this;
      this.isPlayerMounted = true;
      // this.uiControl = FlightControllerUI.Create(this.ui_info.GetGameController(), this.ui_info.GetGameController().GetRootCompoundWidget());
      // this.uiControl.Setup(this.stats);
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
    let wheeled = this.GetVehicle() as WheeledObject;
    if (this.isPlayerMounted && !this.FindGround(normal) || this.distance > FlightSettings.GetInstance().autoActivationHeight) && IsDefined(wheeled) && FlightSettings.GetInstance().autoActivationEnabled {
      this.Activate(true);
    }
  }

  protected cb func OnUnmountingEvent(evt: ref<UnmountingEvent>) -> Bool {
    
    LTBF_UnregisterListener(this);
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if IsDefined(mountChild) && mountChild.IsPlayer() {
      this.UnregisterVehicleTPPBBListener();
      FlightAudio.Get().Stop("windLeft");
      FlightAudio.Get().Stop("windRight");
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
      // this.audioUpdate.water = 1.0;
      FlightAudio.Get().UpdateParameter("water", 1.0);
    } else {
      // this.audioUpdate.water = 0.0;
      FlightAudio.Get().UpdateParameter("water", 0.0);
    }
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    FlightLog.Info("[FlightComponent] OnAction: " + this.GetVehicle().GetDisplayName());
  }

  protected cb func OnVehicleFlightActivationEvent(evt: ref<VehicleFlightActivationEvent>) -> Bool {
    this.Activate();
  }

  // Toggles flight on summon button press  
  // protected cb func OnSummonStartedEvent(evt: ref<SummonStartedEvent>) -> Bool {
  //   if Equals(evt.state, vehicleSummonState.EnRoute) || Equals(evt.state, vehicleSummonState.AlreadySummoned) {
  //     // this.CreateMappin();
  //     if Equals(evt.state, vehicleSummonState.EnRoute) {
  //       // this.SendParkEvent(false);
  //     };
  //     if Equals(evt.state, vehicleSummonState.AlreadySummoned) {
  //       // this.HonkAndFlash();
  //       if this.active {
  //         this.Deactivate(false);
  //       } else {
  //         this.Activate();
  //       }
  //       // this.GetVehicle().PhysicsWakeUp();
  //     };
  //   };
  // }

  public func Activate(opt silent: Bool) -> Void {
    // this.helper = this.GetVehicle().AddFlightHelper();
    FlightLog.Info("[FlightComponent] OnVehicleFlightActivationEvent: " + this.GetVehicle().GetDisplayName());
    this.GetVehicle().ScheduleAppearanceChange(this.GetVehicle().GetCurrentAppearanceName());
    if !this.active {

      this.stats = FlightStats.Create(this.GetVehicle());
      // this.sys.ctlr.ui.Setup(this.stats);

      this.SetupTires();
      for thruster in this.thrusters {
        thruster.Start();
      }
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
        FlightAudio.Get().Play("vehicle3_on");
        // this.sys.audio.StartWithPitch("playerVehicle", "vehicle3_TPP", this.GetPitch());
        // this.sys.audio.Start("leftFront", "vehicle3_TPP");
        // this.sys.audio.Start("rightFront", "vehicle3_TPP");
        // this.sys.audio.Start("leftRear", "vehicle3_TPP");
        // this.sys.audio.Start("rightRear", "vehicle3_TPP");
      }
      // this.sys.audio.StartWithPitch("vehicle" + this.GetUniqueID(), "vehicle3_TPP", this.GetPitch());
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
  let lastVelocity: Vector4;
  public let accelerationFx: ref<FxInstance>;
  public let smoothFx: Float = 1.0;
  public let accThreshold: Float = 1.0;

  protected func OnUpdate(timeDelta: Float) -> Void {
    if this.GetVehicle().IsDestroyed() {
      if !this.isDestroyed {
        FlightAudio.Get().Start("vehicleDestroyed" + this.GetUniqueID(), "vehicle3_destroyed");
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
      for thruster in this.thrusters {
        thruster.Update(this.smoothForce, this.smoothTorque);
      }
      return;
    }
    // something that responds to acceleration & collisions
    // maybe just need a custom effect
    // let velocity = this.GetVehicle().GetLinearVelocity();
    // let acceleration = velocity - this.lastVelocity;
    // this.lastVelocity = velocity;
    // if Vector4.Length(acceleration) > this.accThreshold {
    //   // if !IsDefined(this.accelerationFx) {
    //     let fx = Cast<FxResource>(r"base\\fx\\player\\p_damage\\p_health_low.effect");
    //     this.accelerationFx = GameInstance.GetFxSystem(this.GetVehicle().GetGame()).SpawnEffect(fx, new WorldTransform());
    //   // }
    //   this.smoothFx = LerpF(0.1, this.smoothFx, Vector4.Length(acceleration));
    //   this.accelerationFx.SetBlackboardValue(n"health_state", ClampF(1.0 - (this.smoothFx - this.accThreshold) * 0.5, 0.0, 1.0));
    // } else {
    //   // if IsDefined(this.accelerationFx) {
    //     this.accelerationFx.BreakLoop();
    //   // }
    // }
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



    // process user-inputted force/torque in visuals/audio
    this.UpdateAudioParams(timeDelta, force, torque);
    this.smoothForce = force;
    this.smoothTorque = torque;
    for thruster in this.thrusters {
      thruster.Update(force, torque);
    }
    
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

  protected cb func OnFlightFxCleanedUp(evt: ref<FlightFxCleanedUpEvent>) -> Bool {
    if !this.active {
      this.hasUpdate = false;
    }
  }

  public func Deactivate(silent: Bool) -> Void{
    this.active = false;
    for thruster in this.thrusters {
      thruster.Stop();
    }

    if this.isDestroyed && this.hasExploded && this.alarmIsPlaying {
        FlightAudio.Get().Stop("vehicleDestroyed" + this.GetUniqueID());
    }

    if !silent {
      this.GetVehicle().TurnEngineOn(true);
    }

    if !FlightSettings.GetInstance().generalApplyFlightPhysicsWhenDeactivated {
      let evt = new FlightFxCleanedUpEvent();
      GameInstance.GetDelaySystem(this.GetVehicle().GetGame()).DelayEvent(this.GetVehicle(), evt, evt.delay);
    }

    if this.isPlayerMounted {
      this.sys.ctlr.Deactivate(silent);
      if !silent {
        FlightAudio.Get().Play("vehicle3_off");
      }
      // this.sys.audio.Stop("playerVehicle");
      // this.sys.audio.Stop("leftFront");
      // this.sys.audio.Stop("rightFront");
      // this.sys.audio.Stop("leftRear");
      // this.sys.audio.Stop("rightRear");
    }
    // this.sys.audio.Stop("vehicle" + this.GetUniqueID());
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
        // let pc = FlightSystem.GetInstance().playerComponent;
        // if IsDefined(pc) && IsDefined(pc.GetVehicle()) {
        //   this.ChaseTarget(pc.GetVehicle());
        //   // this.ChaseTarget();
        // }
        // if biggestImpact > 0.00 {
        //   if !this.active {
        //     this.Activate();
        //   } else {
        //     // this.Deactivate(true);
        //   }
        // }
        if !this.active && this.GetVehicle().bouncy {
          this.FireVerticalImpulse(gridID);
        }
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
      this.audioUpdate.sway = this.sys.ctlr.sway.GetInput() * ratio;
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
    this.sys.audio.UpdateSlotProviders();

    // let leftFrontPosition = this.sys.audio.GetPosition(n"wheel_front_left") - (this.stats.d_velocity * timeDelta);
    // let rightFrontPosition = this.sys.audio.GetPosition(n"wheel_front_right") - (this.stats.d_velocity * timeDelta);
    // let leftRearPosition = this.sys.audio.GetPosition(n"wheel_back_left") - (this.stats.d_velocity * timeDelta);
    // let rightRearPosition = this.sys.audio.GetPosition(n"wheel_back_right") - (this.stats.d_velocity * timeDelta);

    let windLeftPosition = this.sys.audio.GetPosition(n"window_front_left_a"); // - (this.stats.d_velocity * timeDelta);
    let windRightPosition = this.sys.audio.GetPosition(n"window_front_right_a"); //- (this.stats.d_velocity * timeDelta);

    // let listenerMatrix = (this.sys.player.FindComponentByName(n"soundListener") as IPlacedComponent).GetLocalToWorld();
    // let listenerMatrix = this.sys.tppCamera.GetLocalToWorld();
    // FlightAudio.Get().UpdateListener(Matrix.GetTranslation(listenerMatrix), Matrix.GetAxisY(listenerMatrix), Matrix.GetAxisZ(listenerMatrix));
    // FlightAudio.Get().UpdateListener(listenerMatrix);

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
      FlightAudio.Get().UpdateEvent("windLeft", Matrix.BuiltTranslation(windLeftPosition), 1.0, this.audioUpdate);
      FlightAudio.Get().UpdateEvent("windRight",  Matrix.BuiltTranslation(windRightPosition), 1.0, this.audioUpdate);
    } else {
      this.audioUpdate.inside = 0.0;
    }
    if this.active {
      // this.sys.audio.Update("vehicle" + this.GetUniqueID(), this.GetVehicle().GetLocalToWorld(), engineVolume, this.audioUpdate);
    }
    if this.isDestroyed && !this.GetVehicle().GetVehicleComponent().GetPS().GetHasExploded() && this.alarmIsPlaying {
      FlightAudio.Get().UpdateEvent("vehicleDestroyed" + this.GetUniqueID(), this.GetVehicle().GetLocalToWorld(), 1.0, this.audioUpdate);
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
    // working
    // FlightLog.Info("[FlightComponent] OnHUDInstruction");
    if evt.quickhackInstruction.ShouldProcess() {
      // FlightLog.Info("[FlightComponent] quickhackInstructions.ShouldProcess");
      // this.GetVehicle().TryOpenQuickhackMenu(true);
      this.GetVehicle().TryOpenQuickhackMenu(evt.quickhackInstruction.ShouldOpen());
    };
  }

  
	// protected cb func OnFlightMalfunction(evt : ref<FlightMalfunction>) -> Bool {
  //     FlightLog.Info("[FlightComponent] OnFlightMalfunction");
	// 	if evt.IsCompleted() {
	// 		this.Activate(false);
	// 	} else {
	// 		this.Activate(true);
	// 	}
	// }


	// protected event OnHUDInstruction( evt : HUDInstruction )
	// {
	// 	super.OnHUDInstruction( evt );
	// 	if( evt.highlightInstructions.GetState() == InstanceState.ON )
	// 	{
	// 		GetDevicePS().SetFocusModeData( true );
	// 		ResolveDeviceOperationOnFocusMode( gameVisionModeType.Focus, true );
	// 	}
	// 	else
	// 	{
	// 		if( evt.highlightInstructions.WasProcessed() )
	// 		{
	// 			GetDevicePS().SetFocusModeData( false );
	// 			ToggleAreaIndicator( false );
	// 			ResolveDeviceOperationOnFocusMode( gameVisionModeType.Default, false );
	// 			NotifyConnectionHighlightSystem( false, false );
	// 		}
	// 	}
	// 	if( evt.quickhackInstruction.ShouldProcess() )
	// 	{
	// 		TryOpenQuickhackMenu( evt.quickhackInstruction.ShouldOpen() );
	// 	}
	// }


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
    if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"NoDriving") {
      return false;
    };
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
  // private let ui: ref<FlightControllerUI>;
  // public final func SetUI(ui: ref<FlightControllerUI>) {
  //   this.ui = ui;
  // }
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

    // if (this.showUI) {
      // this.ui.Show();
      // FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, true);
    // }
  
    if !silent {
      this.ShowSimpleMessage(n"Simple-Message-Flight-Control-Engaged");
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
      this.ShowSimpleMessage(n"Simple-Message-Flight-Control-Disengaged");
    }
    // if (this.showUI) {
    //   this.ui.Hide();
    // }
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
    //   evt.AddInputHint(FlightController.CreateInputHint(n"Aileron Roll", n"Yaw"), true);
    // } else {
      // evt.AddInputHint(FlightController.CreateInputHint("nTricks", n"Flight_Trick"), true);
    // }
    // we may want to look at something else besides this input so ForceBrakesUntilStoppedOrFor will work (not entirely sure it doesn't now)
    // vehicle.GetBlackboard().GetInt(GetAllBlackboardDefs().VehicleFlight.IsHandbraking)

    let usesRightStick = this.sys.playerComponent.GetFlightMode().usesRightStickInput;

    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Enable-Flight", n"Flight_Toggle"),       this.enabled && !this.active);

    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Disable-Flight", n"Flight_Toggle"),      this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Yaw", n"Yaw"),                           this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Pitch", n"Pitch"),                       this.active && !this.showOptions && (usesRightStick || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Roll", n"Roll"),                         this.active && !this.showOptions && (usesRightStick || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Lift", n"Lift"),                         this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Linear-Brake", n"Flight_LinearBrake"),   this.active && !this.showOptions && this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Angular-Brake", n"Flight_AngularBrake"), this.active && !this.showOptions && this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Brake", n"Flight_LinearBrake"),          this.active && !this.showOptions && !this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Flight-Options", n"Flight_Options"),     this.active && !this.showOptions);

    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Sway", n"Sway"),                         this.active && this.showOptions && this.usingKB);
    // let desc: String;
    // desc = this.sys.playerComponent.GetNextFlightModeDescription();
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Next-Mode", n"Flight_ModeSwitchForward"),     this.active && (this.showOptions || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Prev-Mode", n"Flight_ModeSwitchBackward"),     this.active && this.showOptions && !this.usingKB);
    // evt.AddInputHint(FlightController.CreateInputHint(n"Raise Hover Height", n"FlightOptions_Up"), true);
    // evt.AddInputHint(FlightController.CreateInputHint(n"Lower Hover Height", n"FlightOptions_Down"), true);
    evt.AddInputHint(FlightController.CreateInputHint(n"Input-Hint-Toggle-UI", n"Flight_UIToggle"),         this.active && this.showOptions);
    // evt.AddInputHint(FlightController.CreateInputHint(n"Fire", n"ShootPrimary"),                 this.active && !this.showOptions);

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
    this.ShowSimpleMessage(StringToName(newMode.GetDescription() + " " + GetLocalizedTextByKey(n"Simple-Message-Suffix-Enabled")));
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
          // if (this.showUI) {
          //   this.ui.ShowInfo();
          // }
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

  public static func CreateInputHint(label: CName, action: CName) -> InputHintData {
    let data: InputHintData;
    data.source = n"FlightController";
    data.action = action;
    data.localizedLabel = GetLocalizedTextByKey(label);
    if StrLen( data.localizedLabel) == 0 {
         data.localizedLabel = ToString(label);
    };
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

public class FlightFxCleanedUpEvent extends Event {
  public let delay: Float = 0.5;
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

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Automatic-Mode-Enabled")
  public let enabled: Bool = false;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Automatic-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Auto-Braking-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Auto-Braking-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "10")
  public let automaticModeAutoBrakingFactor: Float = 3.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Automatic-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Directionality")
  @runtimeProperty("ModSettings.description", "UI-Settings-Yaw-Directionality-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "1000")
  public let automaticModeYawDirectionality: Float = 300.0;

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
    let yawDirectionality: Float = this.component.stats.d_speedRatio * this.automaticModeYawDirectionality;

    let directionFactor = AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right));

    this.force += FlightUtils.Forward() * directionFactor * yawDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * directionFactor * yawDirectionality * AbsF(aeroFactor);

    if AbsF(this.component.surge) < 1.0 {    
      // let velocityDamp: Vector4 = (1.0 - AbsF(this.component.surge)) * this.automaticModeAutoBrakingFactor * this.component.stats.d_localDirection2D * (this.component.stats.d_speed2D / 100.0);
      // this.force -= velocityDamp;
      let velocityDamp: Vector4 = (1.0 - AbsF(this.component.surge)) * this.automaticModeAutoBrakingFactor * this.component.stats.s_brakingFrictionFactor * this.component.stats.d_localVelocity;
      this.force.X -= velocityDamp.X;
      this.force.Y -= velocityDamp.Y;
    }
  }
}

// FlightModeDrone.reds

public class FlightModeDrone extends FlightMode {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Drone-Mode-Enabled")
  public let enabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "Drone Mode Name")
  public let droneModeName: CName = n"Drone Mode";

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Lift-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeLiftFactor: Float = 40.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pitch-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100")
  public let droneModePitchFactor: Float = 5.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Roll-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100")
  public let droneModeRollFactor: Float = 12.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Surge-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "200")
  public let droneModeSurgeFactor: Float = 15.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100")
  public let droneModeYawFactor: Float = 5.0;
  
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Drone-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Sway-Factor")
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
    LTBF_RegisterListener(this);
  }

  public func Deinitialize() -> Void {
    LTBF_UnregisterListener(this);
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

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Anti-Gravity-Drone-Mode-Enabled")
  public let agEnabled: Bool = false;

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

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Fly-Mode-Enabled")
  public let enabled: Bool = false;

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

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Hover-Mode-Enabled")
  public let enabled: Bool = false;

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
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Hover-And-Fly-Enabled")
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
    // if angularDamp.X < 0.0 {
    //   angularDamp.X *= angularDamp.X * -1.0;
    // } else {
    //   angularDamp.X *= angularDamp.X;
    // }
    // if angularDamp.Y < 0.0 {
    //   angularDamp.Y *= angularDamp.Y * -1.0;
    // } else {
    //   angularDamp.Y *= angularDamp.Y;
    // }
    // if angularDamp.Z < 0.0 {
    //   angularDamp.Z *= angularDamp.Z * -1.0;
    // } else {
    //   angularDamp.Z *= angularDamp.Z;
    // }
    // if angularDamp.W < 0.0 {
    //   angularDamp.W *= angularDamp.W * -1.0;
    // } else {
    //   angularDamp.W *= angularDamp.W;
    // }
    

    let direction = this.component.stats.d_direction;
    if Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_forward) < 0.0 {
      direction = -this.component.stats.d_direction;
    }
    let yawDirectionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_up);
    let pitchDirectionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_right);

    // let aeroDynamicYaw = this.component.aeroYawPID.GetCorrectionClamped(yawDirectionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;
    // let aeroDynamicPitch = this.component.pitchAeroPID.GetCorrectionClamped(pitchDirectionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;
    let aeroDynamicYaw = yawDirectionAngle * this.component.stats.d_speedRatio;// / 10.0;
    let aeroDynamicPitch = pitchDirectionAngle * this.component.stats.d_speedRatio;// / 10.0;

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
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Surge-Factor")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "200.0")
  public let standardModeSurgeFactor: Float = 15.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Factor")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "50.0")
  public let standardModeYawFactor: Float = 5.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Sway-Factor")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "50.0")
  public let standardModeSwayFactor: Float = 5.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pitch-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "20.0")
  public let standardModePitchFactor: Float = 3.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pitch-Input-Angle")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "90.0")
  public let standardModePitchInputAngle: Float = 45.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Roll-Factor")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "20.0")
  public let standardModeRollFactor: Float = 15.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Roll-Input-Angle")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "90.0")
  public let standardModeRollInputAngle: Float = 45.0;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.collisionPenalty = 0.5;
    LTBF_RegisterListener(this);
  }

  public func Deinitialize() -> Void {
    LTBF_UnregisterListener(this);
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
    let pitchDegOffNormal = 90.0 - AbsF(Vector4.GetAngleDegAroundAxis(normal, this.component.stats.d_forward, this.component.stats.d_right));
    let pitchDegOffInput = this.component.pitch * this.standardModePitchInputAngle;
    let pitchDegOff = pitchDegOffNormal + pitchDegOffInput;
    let rollDegOffNormal = 90.0 - AbsF(Vector4.GetAngleDegAroundAxis(normal, this.component.stats.d_right, this.component.stats.d_forward));
    let rollDegOffInput = this.component.roll * this.standardModeRollInputAngle;
    let rollDegOff = rollDegOffNormal + rollDegOffInput;
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
    this.torque.Z = -((this.component.yaw - (rollDegOffInput * pitchDegOffInput / 1800.0)) * this.standardModeYawFactor + angularDamp.Z );
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
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Enable-Auto-Activation")
  @runtimeProperty("ModSettings.description", "UI-Settings-Enable-Auto-Activation-Description")
  public let autoActivationEnabled: Bool = false;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Auto-Activation-Height")
  @runtimeProperty("ModSettings.description", "UI-Settings-Auto-Activation-Height-Description")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.max", "10.0")
  public let autoActivationHeight: Float = 3.0;

  // Flight Control Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Linear-Brake-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Linear-Brake-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "10.0")
  public let brakeFactorLinear: Float = 1.2;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Angular-Brake-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Angular-Brake-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "50.0")
  public let brakeFactorAngular: Float = 5.0;
  
  // Flight Physics Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Apply-Flight-Physics-When-Deactivated")
  @runtimeProperty("ModSettings.description", "UI-Settings-Apply-Flight-Physics-When-Deactivated-Description")
  public let generalApplyFlightPhysicsWhenDeactivated: Bool = false;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Linear-Damp-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Linear-Damp-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.0001")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "0.01")
  public let generalDampFactorLinear: Float = 0.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Angular-Damp-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Angular-Damp-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.01")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "3.0")
  public let generalDampFactorAngular: Float = 1.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pitch-Aero-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Pitch-Aero-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalPitchAeroFactor: Float = 0.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Aero-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Yaw-Aero-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalYawAeroFactor: Float = 0.1;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pitch-Directionality-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Pitch-Directionality-Factor-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let generalPitchDirectionalityFactor: Float = 15.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Directionality-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Yaw-Directionality-Factor-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let generalYawDirectionalityFactor: Float = 5.0;

  // Flight Camera Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Driving-Direction-Compensation-Angle-Smoothing")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "180.0")
  public let drivingDirectionCompensationAngleSmooth: Float = 120.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Driving-Direction-Compensation-Speed-Coef")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let drivingDirectionCompensationSpeedCoef: Float = 0.1;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-FPV-Camera-Pitch-Offset")
  @runtimeProperty("ModSettings.description", "UI-Settings-FPV-Camera-Pitch-Offset-Description")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "-60.0")
  @runtimeProperty("ModSettings.max", "60.0")
  public let fpvCameraPitchOffset: Float = 0.0;

  // public cb func OnModSettingsUpdate(variable: CName, value: Variant) {
  //   switch (variable) {
  //     case n"autoActivationHeight":
  //       this.autoActivationHeight = FromVariant<Float>(value);
  //       break;
  //   }
  // }

  private func OnAttach() -> Void {
    FlightLog.Info("[FlightSettings] OnAttach");
    LTBF_RegisterListener(this);
    
    FlightSettings.SetVector3("inputPitchPID", 1.0, 0.5, 0.5);
    FlightSettings.SetVector3("inputRollPID", 1.0, 0.5, 0.5);
    
    FlightSettings.SetVector3("aeroYawPID", 1.0, 0.01, 1.0);
    FlightSettings.SetVector3("aeroPitchPID", 1.0, 0.01, 1.0);

    FlightSettings.SetVector3("hoverModePID", 1.0, 0.005, 0.5);

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
  public native func RegisterComponent(component: wref<FlightComponent>) -> Void;
  public native func UnregisterComponent(component: wref<FlightComponent>) -> Void;

  @runtimeProperty("offset", "0x48")
  public native let cameraIndex: Int32;

  @runtimeProperty("offset", "0x50")
  public native let soundListener: wref<IPlacedComponent>;

  @runtimeProperty("offset", "0x70")
  public native let audio: ref<FlightAudio>;

  public let gameInstance: GameInstance;
  public let player: wref<PlayerPuppet>;
  public let ctlr: ref<FlightController>;
  public let stats: ref<FlightStats>;
  // public let fx: ref<FlightFx>;
  public let tppCamera: wref<vehicleTPPCameraComponent>;
  public let playerComponent: wref<FlightComponent>;

  public func Setup(player: ref<PlayerPuppet>) -> Void {
    FlightLog.Info("[FlightSystem] Player updated");
    this.player = player;
    this.soundListener = player.FindComponentByName(n"soundListener") as IPlacedComponent;
    this.gameInstance = player.GetGame();
    // if !IsDefined(this.audio) {
    //   this.audio = FlightAudio.Create();
    //   FlightLog.Info("[FlightSystem] FlightAudio Created");
    // }
    this.ctlr = FlightController.GetInstance();
    this.tppCamera = player.FindComponentByName(n"vehicleTPPCamera") as vehicleTPPCameraComponent;
    // this.soundListener = this.tppCamera;
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



// FlightThruster.reds

enum FlightThrusterType {
  FrontLeft = 0,
  FrontRight = 1,
  BackLeft = 2,
  BackRight = 3,
  FrontLeftB = 4,
  FrontRightB = 5,
  Front = 6,
  Back = 7
}

public class FlightThruster {
  public let flightComponent: ref<FlightComponent>;
  public let bone: Float = 0.0;
  public let boneLerpAmount: Float = 0.25;
  public let maxThrusterAnglePitch: Float = 90.0;
  public let maxThrusterAngleOutside: Float = 60.0;
  public let maxThrusterAngleInside: Float = 15.0;
  public let thrusterAngleAllowance: Float = 15.0;
  public let ogComponents: array<ref<IComponent>>;
  public let meshComponent: ref<MeshComponent>;
  public let mainResRef: ResRef = r"user\\jackhumbert\\effects\\ion_thruster.effect";
  public let mainFxRes: FxResource;
  public let retroResRef: ResRef = r"user\\jackhumbert\\effects\\retro_thruster.effect";
  public let retroFxRes: FxResource;
  public let mainFx: ref<FxInstance>;
  // public let mainThrusterFactor: Float = 0.05;
  public let mainThrusterYawFactor: Float = 0.5;
  public let retroFx: ref<FxInstance>;
  public let retroThrusterFactor: Float = 0.1;
  public let force: Vector4;
  public let torque: Vector4;
  public let isRight: Bool;
  public let isFront: Bool;
  public let isMotorcycle: Bool;
  public let isB: Bool;
  public let id: String;
  public let audioUpdate: ref<FlightAudioUpdate>;
  public let audioPitch: Float;
  public let audioPitchSeparation: Float = 0.001;

  public static func CreateThrusters(fc: ref<FlightComponent>) -> array<ref<FlightThruster>> {
    let thrusters: array<ref<FlightThruster>>;
    let vehicleComponent = fc.GetVehicle().GetVehicleComponent();

    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterFL") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.FrontLeft));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterFR") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.FrontRight));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterBL") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.BackLeft));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterBR") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.BackRight));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterFLB") as MeshComponent) {
      ArrayPush(thrusters, new FlightThrusterFLB().Create(fc));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterFRB") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.FrontRightB));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterF") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.Front));
    }
    if IsDefined(vehicleComponent.FindComponentByName(n"ThrusterB") as MeshComponent) {
      ArrayPush(thrusters, new FlightThruster().Initialize(fc, FlightThrusterType.Back));
    }

    return thrusters;
  }

  public func Initialize(fc: ref<FlightComponent>, type: FlightThrusterType) -> ref<FlightThruster> {
    this.flightComponent = fc;
    this.mainFxRes = Cast<FxResource>(this.mainResRef);
    this.retroFxRes = Cast<FxResource>(this.retroResRef);

    if Equals(type, FlightThrusterType.FrontRight) {
      this.isRight = true;
      this.isFront = true;
    }
    if Equals(type, FlightThrusterType.FrontLeft) {
      this.isRight = false;
      this.isFront = true;
    }
    if Equals(type, FlightThrusterType.FrontRightB) {
      this.isRight = true;
      this.isFront = true;
      this.isB = true;
    }
    if Equals(type, FlightThrusterType.FrontLeftB) {
      this.isRight = false;
      this.isFront = true;
      this.isB = true;
    }
    if Equals(type, FlightThrusterType.BackRight) {
      this.isRight = true;
      this.isFront = false;
    }
    if Equals(type, FlightThrusterType.BackLeft) {
      this.isRight = false;
      this.isFront = false;
    }
    if Equals(type, FlightThrusterType.Front) {
      this.isFront = true;
      this.isMotorcycle = true;
    }
    if Equals(type, FlightThrusterType.Back) {
      this.isMotorcycle = true;
    }

    let vehicleComponent = this.flightComponent.GetVehicle().GetVehicleComponent();

    this.meshComponent = vehicleComponent.FindComponentByName(this.GetComponentName()) as MeshComponent;
    this.meshComponent.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.meshComponent.Toggle(false);
    this.meshComponent.SetLocalOrientation(EulerAngles.ToQuat(this.GetEulerAngles()));

    this.id = "vehicle";
    this.audioPitch = this.flightComponent.GetPitch();
    if this.isFront {
      this.id += "F";
      // this.audioPitch *= 1.02;
    } else {
      this.id += "B";
      // this.audioPitch *= 0.5;
      // this.audioPitch *= 2.0;
    }
    if this.isRight {
      this.id += "R";
      // this.audioPitch *= (1.0 + this.audioPitchSeparation);
    } else {
      this.id += "L";
      // this.audioPitch /= (1.0 + this.audioPitchSeparation);
    }
    if this.isB {
      this.id += "B";
      // this.audioPitch *= 0.5;
    }
    this.id += this.flightComponent.GetUniqueID();
    this.audioUpdate = new FlightAudioUpdate();

    return this;
  }

  public func Start() {
    let vehicle = this.flightComponent.GetVehicle();
    let effectTransform: WorldTransform;
    let wt = new WorldTransform();

    this.ogComponents = this.flightComponent.GetVehicle().GetComponentsUsingSlot(this.GetSlotName());
    this.HideOGComponents();

    WorldTransform.SetPosition(effectTransform, this.flightComponent.stats.d_position);
    this.mainFx = GameInstance.GetFxSystem(vehicle.GetGame()).SpawnEffect(this.mainFxRes, effectTransform);
    this.mainFx.SetBlackboardValue(n"thruster_amount", 0.0);
    this.mainFx.AttachToComponent(vehicle, entAttachmentTarget.Transform, this.GetComponentName(), wt);
    this.meshComponent.Toggle(true);

    let wt_retro: WorldTransform;
    WorldTransform.SetOrientation(wt_retro, EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, -90.0)));
    this.retroFx =  GameInstance.GetFxSystem(vehicle.GetGame()).SpawnEffect(this.retroFxRes, effectTransform);
    // this.retroFx.AttachToSlot(this.component.GetVehicle(), entAttachmentTarget.Transform, n"Base", wt_retro);
    this.retroFx.AttachToComponent(vehicle, entAttachmentTarget.Transform, this.GetComponentName(), wt_retro);

    FlightAudio.Get().StartWithPitch(this.id, "vehicle3_TPP", this.audioPitch);

  }

  let forceThreshold: Float = 10.0;
  let torqueThreshold: Float = 1.0;

  let animDeviation: Float = 0.3;
  let animRadius: Float = 0.0;


  public func Update(force: Vector4, torque: Vector4) {
    if Vector4.Length(force) > this.forceThreshold {
      this.force = Vector4.Normalize(force);
    } else {
      this.force = force / this.forceThreshold;
    }
    if Vector4.Length(torque) > this.torqueThreshold {
      this.torque = Vector4.Normalize(torque);
    } else {
      this.torque = torque / this.torqueThreshold;
    }
    
    let vec = new Vector4(1.0, 1.0, 1.0, 1.0);
    if !this.flightComponent.active {
      vec = Vector4.EmptyVector();
    }
    this.meshComponent.visualScale = Vector4.Vector4To3(Vector4.Interpolate(Vector4.Vector3To4(this.meshComponent.visualScale), vec, 0.1));

    this.meshComponent.SetLocalOrientation(Quaternion.Slerp(this.meshComponent.GetLocalOrientation(), EulerAngles.ToQuat(this.GetEulerAngles()), 0.1));

    let amount = Vector4.Dot(Quaternion.GetUp(this.meshComponent.GetLocalOrientation()), this.force);
    amount += this.GetMainThrusterTorqueAmount();
    // let amount = Vector4.Dot(Quaternion.GetUp(this.meshComponent.GetLocalOrientation()), Quaternion.GetUp(EulerAngles.ToQuat(this.GetEulerAngles())));
    // amount *= this.mainThrusterFactor;
    amount = ClampF(amount, -1.0, 1.0);
    this.mainFx.SetBlackboardValue(n"thruster_amount", amount);

    // -4, 4 / -10, 10
    let animDeviationCenter = 0.0;
    let animDeviationScale = 0.1;
    // 0, 16
    // let animRadiusCenter = 1.0;
    // let animRadiusScale = -1.0;

    // this.bone = LerpF(this.boneLerpAmount, this.bone, -animScale + ClampF(amount, -1.0, 1.0) * animScale);
    this.animDeviation = LerpF(this.boneLerpAmount, this.animDeviation, animDeviationCenter + amount * animDeviationScale);
    // this.animDeviation = animDeviationCenter + amount * animDeviationScale;
    // this.animRadius = animRadiusCenter + amount * animRadiusScale;
    // AnimationControllerComponent.SetInputFloatToReplicate(this.flightComponent.GetVehicle(), this.GetDeviationName(), this.animDeviation);
    // AnimationControllerComponent.SetInputFloatToReplicate(this.flightComponent.GetVehicle(), this.GetRadiusName(), this.animRadius);

    let acc = this.flightComponent.FindComponentByName(n"AnimationController") as AnimationControllerComponent;
    if IsDefined(acc) {
      acc.SetInputFloat(this.GetDeviationName(), this.animDeviation);
    }
    // AnimationControllerComponent.SetInputFloat(this.flightComponent.GetVehicle(), this.GetDeviationName(), this.animDeviation);

    // acc.SetInputFloat(this.GetRadiusName(), this.animRadius);

    let retroAmount = this.GetRetroThrusterAmount();
    this.retroFx.SetBlackboardValue(n"thruster_amount", retroAmount);
    
    this.audioUpdate = this.flightComponent.audioUpdate;
    // amount *= 0.5;
    // this.audioUpdate.surge *= amount;
    // this.audioUpdate.pitch *= amount;
    // this.audioUpdate.yaw *= retroAmount;
    // this.audioUpdate.sway *= retroAmount;
    // this.audioUpdate.lift *= amount;
    // this.audioUpdate.roll *= amount;
    let volume = 1.0;
    if !this.isFront {
      volume = ClampF(this.flightComponent.stats.d_speed / 100.0, 0.0, 1.0);
    }
    // this.audioUpdate.pitch = retroAmount;
    
    let matrix = this.meshComponent.GetLocalToWorld();
    // rotates the event cone down
    let quat = Matrix.ToQuat(matrix) * new Quaternion(-0.707, 0.0, 0.0, 0.707);
    let rotatedMatrix = Quaternion.ToMatrix(quat);
    rotatedMatrix.W = matrix.W;
    FlightAudio.Get().UpdateEvent(this.id, rotatedMatrix, volume, this.audioUpdate);
  }

  public func Stop() {
    FlightAudio.Get().Stop(this.id);
    if IsDefined(this.mainFx) {
      this.mainFx.BreakLoop();
    }
    if IsDefined(this.retroFx) {
      this.retroFx.BreakLoop();
    }
    this.ShowOGComponents();
  }

  public func GetEulerAngles() -> EulerAngles {
    return new EulerAngles(this.GetPitch(), this.GetRoll(), this.GetYaw());
  }

  public let componentSizeArray: array<Vector3>;

  public func HideOGComponents() {
    for c in this.ogComponents {
      let mc = c as MeshComponent;
      if IsDefined(mc) {
        ArrayPush(this.componentSizeArray, mc.visualScale);
        mc.visualScale = new Vector3(0.0, 0.0, 0.0);
      }
    }
  }

  public func ShowOGComponents() {
    let i = 0;
    for c in this.ogComponents {
      let mc = c as MeshComponent;
      if IsDefined(mc) {
        mc.visualScale = this.componentSizeArray[i];
        i += 1;
      }
    }
    ArrayClear(this.componentSizeArray);
  }

  public func GetMainThrusterTorqueAmount() -> Float {
    let amount: Float = 0;
    if this.isFront {
      amount += this.torque.X;
    } else {
      amount -= this.torque.X;
    }
    if this.isRight {
      amount -= this.torque.Y;
    } else {
      amount += this.torque.Y;
    }
    return amount;
  }

  public func GetRetroThrusterAmount() -> Float {
    let vec: Vector4;
    if this.isRight {
      vec = new Vector4(-1.0, 0.0, 0.0, 0.0);
    } else {
      vec = new Vector4(1.0, 0.0, 0.0, 0.0);
    }
    let tor: Float;
    if this.isFront ^ this.isRight { // FL, BR
      tor = -this.torque.Z;
    } else { // FR, BL
      tor = this.torque.Z;
    }
    return (Vector4.Dot(vec, this.force) + tor) * this.retroThrusterFactor;
  }

  public func GetComponentName() -> CName {
    if this.isMotorcycle {
        if this.isFront {
          return n"ThrusterF";
        } else {
          return n"ThrusterB";
        }
    } else {
      if this.isRight {
        if this.isFront {
          if this.isB {
            return n"ThrusterFRB";
          } else {
            return n"ThrusterFR";
          }
        } else {
          return n"ThrusterBR";
        }
      } else {
        if this.isFront {
          if this.isB {
            return n"ThrusterFLB";
          } else {
            return n"ThrusterFL";
          }
        } else {
          return n"ThrusterBL";
        }
      }
    }
  }

  public func GetSlotName() -> CName {
    if this.isMotorcycle {
        if this.isFront {
          return n"wheel_front_spring";
        } else {
          return n"axel_back";
        }
    } else {
      if this.isRight {
        if this.isFront {
          if this.isB {
            return n"wheel_front_right_b";
          } else {
            return n"wheel_front_right";
          }
        } else {
          return n"wheel_back_right";
        }
      } else {
        if this.isFront {
          if this.isB {
            return n"wheel_front_left_b";
          } else {
            return n"wheel_front_left";
          }
        } else {
          return n"wheel_back_left";
        }
      }
    }
  }

  public func GetRadiusName() -> CName {
    if this.isRight {
      if this.isFront {
        if this.isB {
          return n"veh_rad_w_1_r";
        } else {
          return n"veh_rad_w_f_r";
        }
      } else {
        return n"veh_rad_w_b_r";
      }
    } else {
      if this.isFront {
        if this.isB {
          return n"veh_rad_w_1_l";
        } else {
          return n"veh_rad_w_f_l";
        }
      } else {
        return n"veh_rad_w_b_l";
      }
    }
  }

  public func GetDeviationName() -> CName {
    if this.isRight {
      if this.isFront {
        if this.isB {
          return n"veh_press_w_1_r";
        } else {
          return n"veh_press_w_f_r";
        }
      } else {
        return n"veh_press_w_b_r";
      }
    } else {
      if this.isFront {
        if this.isB {
          return n"veh_press_w_1_l";
        } else {
          return n"veh_press_w_f_l";
        }
      } else {
        return n"veh_press_w_b_l";
      }
    }
  }

  public func GetPitch() -> Float {
    let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Right());
    if angle < (-this.maxThrusterAnglePitch - this.thrusterAngleAllowance) || angle > (this.maxThrusterAnglePitch + this.thrusterAngleAllowance) {
      angle = 0.0;
    }
    let dir: Float;
    if this.isRight {
      dir = 1.0;
    } else {
      dir = -1.0;
    }
    angle *= (1.0 - AbsF(this.torque.Y) * 0.5);
    return ClampF(angle, -this.maxThrusterAnglePitch, this.maxThrusterAnglePitch) * dir;
  }

  public func GetRoll() -> Float {
    if this.isRight {
      return 0.0;
    } else {
      return 180.0;
    }
  }

  public func GetYaw() -> Float {
    if this.flightComponent.active {
      let dir: Float;
      let outside: Float;
      let inside: Float;
      if this.isRight {
        dir = 1.0;
        outside = this.maxThrusterAngleInside;
        inside = this.maxThrusterAngleOutside;
      } else {
        dir = -1.0;
        outside = this.maxThrusterAngleOutside;
        inside = this.maxThrusterAngleInside;
      }
      let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Forward());
      if angle < (-inside - this.thrusterAngleAllowance) || angle > (outside + this.thrusterAngleAllowance) {
        angle = 0.0;
      }
      let tor: Float;
      if this.isFront ^ this.isRight { // FL, BR
        tor = this.torque.Z;
      } else { // FR, BL
        tor = -this.torque.Z;
      }
      return tor * this.mainThrusterYawFactor + ClampF(angle, -inside, outside) * dir;
    } else {
      return 180.0;
    }
  }
}


public class FlightThrusterFLB extends FlightThruster {
  public func Create(fc: ref<FlightComponent>) -> ref<FlightThruster> {
    return this.Initialize(fc, FlightThrusterType.FrontLeftB);
  }

  public func GetComponentName() -> CName {
    return n"ThrusterFLB";
  }

  public func GetSlotName() -> CName {
    return n"wheel_front_left_b";
  }

  public func GetRadiusName() -> CName {
    return n"veh_rad_w_1_l";
  }

  public func GetDeviationName() -> CName {
    return n"veh_press_w_1_l";
  }
}

public class Vector3Wrapper {
  public let vector: Vector3;
  public static func Create(v: Vector3) -> ref<Vector3Wrapper> {
    let vw = new Vector3Wrapper();
    vw.vector = v;
    return vw;
  }
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
      camera.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(FlightSettings.GetInstance().fpvCameraPitchOffset, 0.0, 0.0)));
    }

    // let workspotSystem: ref<WorkspotGameSystem> = scriptInterface.GetWorkspotSystem();
    // workspotSystem.SwitchSeatVehicle(scriptInterface.owner, scriptInterface.executionOwner, n"OccupantSlots", n"CustomFlightCamera");
  }

  public func ExitCustomCamera(scriptInterface: ref<StateGameScriptInterface>) {
    let camera = (scriptInterface.executionOwner as PlayerPuppet).GetFPPCameraComponent();
    if IsDefined(camera) {
      camera.SetLocalPosition(new Vector4(0.0, 0.0, 0.0, 0.0));
      camera.SetLocalOrientation(EulerAngles.ToQuat(new EulerAngles(0.0, 0.0, 0.0)));
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

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-UI-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Enabled")
  public let enabled: Bool = true;

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
    GameInstance.GetBlackboardSystem(vehicle.GetGame()).Get(GetAllBlackboardDefs().UI_ActiveVehicleData).SetBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsPlayerMounted, true); 
  }

  protected cb func OnUninitialize() -> Bool {
    FlightLog.Info("[hudFlightController] OnUninitialize");
    // TakeOverControlSystem.CreateInputHint(this.GetPlayerControlledObject().GetGame(), false);
    // SecurityTurret.CreateInputHint(this.GetPlayerControlledObject().GetGame(), false);
    if IsDefined(this.m_healthStatPoolListener) {
      GameInstance.GetStatPoolsSystem(this.m_gameInstance).RequestUnregisteringListener(Cast(this.m_healthStatPoolListener.m_vehicle.GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    }
    GameInstance.GetBlackboardSystem(this.m_gameInstance).Get(GetAllBlackboardDefs().UI_ActiveVehicleData).SetBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsPlayerMounted, false); 
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
      if this.enabled {
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
      }
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


// ModSettings.reds

@if(ModuleExists("ModSettingsModule")) 
public func LTBF_RegisterListener(listener: ref<IScriptable>) {
  ModSettings.RegisterListenerToClass(listener);
}

@if(!ModuleExists("ModSettingsModule")) 
public func LTBF_RegisterListener(listener: ref<IScriptable>) { }

@if(ModuleExists("ModSettingsModule")) 
public func LTBF_UnregisterListener(listener: ref<IScriptable>) {
  ModSettings.UnregisterListenerToClass(listener);
}

@if(!ModuleExists("ModSettingsModule")) 
public func LTBF_UnregisterListener(listener: ref<IScriptable>) { }

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

// Quickhacks.reds

@wrapMethod(QuickhackModule)
protected func Process(out task: HUDJob, mode: ActiveMode) -> Void {
  let instruction: ref<QuickhackInstance>;
  if !IsDefined(task.actor) {
    return;
  };
  if IsDefined(this.m_hud.GetCurrentTarget()) && (Equals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.DEVICE) || Equals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.BODY_DISPOSAL_DEVICE) || Equals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.PUPPET) || Equals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.VEHICLE)) {
    if task.actor == this.m_hud.GetCurrentTarget() {
      if this.m_hud.GetCurrentTarget().GetShouldRefreshQHack() {
        this.m_calculateClose = true;
        this.m_hud.GetCurrentTarget().SetShouldRefreshQHack(false);
        instruction = task.instruction.quickhackInstruction;
        if IsDefined(instruction) && IsDefined(task.actor) {
          instruction.SetState(InstanceState.ON, this.DuplicateLastInstance(task.actor));
          instruction.SetContext(this.BaseOpenCheck());
        };
      };
    };
  } else {
    if this.m_calculateClose {
      if !IsDefined(this.m_hud.GetCurrentTarget()) || NotEquals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.DEVICE) || NotEquals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.BODY_DISPOSAL_DEVICE) || NotEquals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.PUPPET) || NotEquals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.VEHICLE) {
        this.m_calculateClose = false;
        this.m_hud.GetLastTarget().SetShouldRefreshQHack(true);
        QuickhackModule.SendRevealQuickhackMenu(this.m_hud, this.m_hud.GetPlayer().GetEntityID(), false);
      };
    };
  };
}

@wrapMethod(HUDManager)
private final func CanShowHintMessage() -> Bool {
  let attitudeCheck: Bool;
  let currentTargetObj: wref<GameObject>;
  if Equals(this.m_currentTarget.GetType(), HUDActorType.PUPPET) || Equals(this.m_currentTarget.GetType(), HUDActorType.DEVICE) || Equals(this.m_currentTarget.GetType(), HUDActorType.BODY_DISPOSAL_DEVICE) || Equals(this.m_currentTarget.GetType(), HUDActorType.VEHICLE) {
    currentTargetObj = GameInstance.FindEntityByID(this.GetGameInstance(), this.m_currentTarget.GetEntityID()) as GameObject;
    attitudeCheck = NotEquals(GameObject.GetAttitudeTowards(this.GetPlayer(), currentTargetObj), EAIAttitude.AIA_Friendly);
    return this.IsCyberdeckEquipped() && attitudeCheck;
  };
  if Equals(this.m_currentTarget.GetType(), HUDActorType.ITEM) {
    return false;
  };
  return this.IsCyberdeckEquipped();
}


@addMethod(VehicleObject)
public const func IsQuickHackAble() -> Bool {
  return true;
}

@addMethod(VehicleObject)
public const func IsQuickHacksExposed() -> Bool {
  return true;
}

@addMethod(VehicleObject)
public const func ShouldEnableRemoteLayer() -> Bool {
  return this.IsTechie() || this.IsQuickHacksExposed() && this.IsNetrunner();
}

@addMethod(VehicleObject)
public const func CanRevealRemoteActionsWheel() -> Bool {
  if !this.ShouldRegisterToHUD() {
    return false;
  };
  if !this.IsQuickHackAble() {
    return false;
  };
  return true;
}

// @addMethod(VehicleObject)
// protected cb func OnQuickHackPanelStateChanged(evt: ref<QuickHackPanelStateEvent>) -> Bool {
  // this.DetermineInteractionStateByTask();
// }

@addMethod(VehicleObject)
public const func ShouldRegisterToHUD() -> Bool {
  return true;
}

// @addMethod(VehicleObject)
// protected final func DetermineInteractionStateByTask() -> Void {
//   GameInstance.GetDelaySystem(this.GetGame()).QueueTask(this, null, n"DetermineInteractionStateTask", gameScriptTaskExecutionStage.Any);
// }

// @addMethod(VehicleObject)
// protected final func DetermineInteractionStateTask(data: ref<ScriptTaskData>) -> Void {
//   this.DetermineInteractionState();
// }

// @addMethod(VehicleObject)
// protected final func DetermineInteractionState() -> Void {
//   let context: GetActionsContext;
//     context.requestorID = this.GetEntityID();
//     context.requestType = gamedeviceRequestType.Remote;
//     context.processInitiatorObject = GetPlayer(this.GetGame());
//     this.GetPS().DetermineInteractionState(this.m_interactionComponent, context, this.m_objectActionsCallbackCtrl);
// }

public class FlightAction extends ScriptableDeviceAction {
  public let m_owner: wref<VehicleObject>;
  public let title: String;
  public let description: String;
  public let icon: TweakDBID;
}

public class FlightEnable extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightEnable";
    this.title = "Enable Flight";
    this.description = "Grant this vehicle clear for take-off and use the default mode";
    this.icon = t"UIIcon.SystemCollapse";
    this.SetObjectActionID(t"DeviceAction.FlightAction");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.m_flightComponent.active;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[FlightEnable] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.UnsetPhysicsStates();
    this.m_owner.EndActions();
    this.m_owner.m_flightComponent.Activate(true);
  }
}

public class FlightDisable extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightDisable";
    this.title = "Disable Flight";
    this.description = "Turn off flight mode and let the occupants suffer the consequences";
    this.icon = t"UIIcon.SystemCollapse";
    this.SetObjectActionID(t"DeviceAction.FlightAction");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && v.m_flightComponent.active;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[FlightDisable] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.UnsetPhysicsStates();
    this.m_owner.EndActions();
    this.m_owner.m_flightComponent.Deactivate(true);
  }
}

public class FlightMalfunction extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightMalfunction";
    this.title = "Initiate Launch";
    this.description = "Rip a hole in the sky and throw this particular asshole in it";
    this.icon = t"UIIcon.TurretMalfunction";
    this.SetObjectActionID(t"DeviceAction.FlightMalfunction");
  }
}

public class DisableGravity extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"DisableGravity";
    this.title = "Enable Antigrav";
    this.description = "Free this vehicle the prison that is Earth's gravity";
    this.icon = t"UIIcon.SystemCollapse";
    this.SetObjectActionID(t"ChoiceIcons.EngineeringIcon");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && v.HasGravity();
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[DisableGravity] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.UnsetPhysicsStates();
    this.m_owner.EndActions();
    this.m_owner.EnableGravity(false);
  }
}

public class EnableGravity extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"EnableGravity";
    this.title = "Disable Antigrav";
    this.description = "Accept Newton's law and bring this vehicle back to Earth at that lovely 9.8 m/s^2";
    this.icon = t"UIIcon.SystemCollapse";
    this.SetObjectActionID(t"ChoiceIcons.EngineeringIcon");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.HasGravity();
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[EnableGravity] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.EnableGravity(true);
  }
}

public class Bouncy extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"Bouncy";
    this.title = "Funhouse";
    this.description = "Charm this vehicle with the folly of youth and make it a little bouncy";
    this.icon = t"ChoiceIcons.SabotageIcon";
    this.SetObjectActionID(t"DeviceAction.Bouncy");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.bouncy;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[Bouncy] CompleteAction");
    super.CompleteAction(gameInstance);
  }
}

// @addMethod(VehicleComponentPS)
// public func OnFlightMalfunction(evt: ref<FlightMalfunction>) -> EntityNotificationType {
//   FlightLog.Info("[VehicleComponentPS] OnFlightMalfunction");
//   return EntityNotificationType.SendThisEventToEntity;
// }

// @addMethod(VehicleObject)
// protected cb func OnFlightMalfunction(evt: ref<FlightMalfunction>) -> Bool {
//   FlightLog.Info("[VehicleObject] OnFlightMalfunction");
// }

// @addMethod(VehicleObject)
// protected cb func OnPerformedAction(evt: ref<PerformedAction>) -> Bool {
//     FlightLog.Info("[VehicleObject] OnPerformedAction");
//   let action: ref<ScriptableDeviceAction>;
//   // let sequenceQuickHacks: ref<ForwardAction>;
//   this.SetScannerDirty(true);
//   action = evt.m_action as ScriptableDeviceAction;
//   // this.ExecuteBaseActionOperation(evt.m_action.GetClassName());
//   if action.CanTriggerStim() {
//     // this.TriggerAreaEffectDistractionByAction(action);
//   };
//   if IsDefined(action) && action.IsIllegal() && !action.IsQuickHack() {
//     // this.ResolveIllegalAction(action.GetExecutor(), action.GetDurationValue());
//   };
//   // if this.IsConnectedToActionsSequencer() && !this.IsLockedViaSequencer() {
//     // sequenceQuickHacks = new ForwardAction();
//     // sequenceQuickHacks.requester = this.GetDevicePS().GetID();
//     // sequenceQuickHacks.actionToForward = action;
//     // GameInstance.GetPersistencySystem(this.GetGame()).QueuePSEvent(this.GetDevicePS().GetActionsSequencer().GetID(), this.GetDevicePS().GetActionsSequencer().GetClassName(), sequenceQuickHacks);
//   // };
//   // this.ResolveQuestImportanceOnPerformedAction(action);
// }

public func ActionFlightEnable(owner: ref<VehicleObject>) -> ref<FlightEnable> {
    let action: ref<FlightEnable> = new FlightEnable();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionFlightDisable(owner: ref<VehicleObject>) -> ref<FlightDisable> {
    let action: ref<FlightDisable> = new FlightDisable();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionFlightMalfunction(owner: ref<VehicleObject>) -> ref<FlightMalfunction> {
    let action: ref<FlightMalfunction> = new FlightMalfunction();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionDisableGravity(owner: ref<VehicleObject>) -> ref<DisableGravity> {
    let action: ref<DisableGravity> = new DisableGravity();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionEnableGravity(owner: ref<VehicleObject>) -> ref<EnableGravity> {
    let action: ref<EnableGravity> = new EnableGravity();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionBouncy(owner: ref<VehicleObject>) -> ref<Bouncy> {
    let action: ref<Bouncy> = new Bouncy();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    return action;
}

@addMethod(VehicleObject)
protected func SendQuickhackCommands(shouldOpen: Bool) {
  // FlightLog.Info("[VehicleObject] SendQuickhackCommands()");
  let actions: array<ref<DeviceAction>>;
  let commands: array<ref<QuickhackData>>;
  // let context: GetActionsContext;
  let quickSlotsManagerNotification: ref<RevealInteractionWheel> = new RevealInteractionWheel();
  quickSlotsManagerNotification.lookAtObject = this;
  quickSlotsManagerNotification.shouldReveal = shouldOpen;
  if shouldOpen {
    // FlightLog.Info("[VehicleObject] SendQuickhackCommands() shouldOpen == true");
    // context = this.GetPS().GenerateContext(gamedeviceRequestType.Remote, Device.GetInteractionClearance(), this.GetPlayerMainObject(), this.GetEntityID());
    // this.GetPS().GetRemoteActions(actions, context);
    
    // action.SetInactiveWithReason(false, "LocKey#49279");
    ArrayPush(actions, ActionFlightEnable(this));
    ArrayPush(actions, ActionFlightDisable(this));
    ArrayPush(actions, ActionFlightMalfunction(this));
    ArrayPush(actions, ActionDisableGravity(this));
    ArrayPush(actions, ActionEnableGravity(this));
    ArrayPush(actions, ActionBouncy(this));

    if this.m_isQhackUploadInProgerss {
      ScriptableDeviceComponentPS.SetActionsInactiveAll(actions, "LocKey#7020");
    };
    this.TranslateActionsIntoQuickSlotCommands(actions, commands);
    quickSlotsManagerNotification.commands = commands;
  };
  HUDManager.SetQHDescriptionVisibility(this.GetGame(), shouldOpen);
  GameInstance.GetUISystem(this.GetGame()).QueueEvent(quickSlotsManagerNotification);
}

@addMethod(VehicleObject)
protected cb func OnQuickSlotCommandUsed(evt: ref<QuickSlotCommandUsed>) -> Bool {
  this.ExecuteAction(evt.action, GameInstance.GetPlayerSystem(this.GetGame()).GetLocalPlayerControlledGameObject());
}

@addMethod(VehicleObject)
protected final const func ExecuteAction(choice: InteractionChoice, executor: wref<GameObject>, layerTag: CName) -> Void {
  let action: ref<ScriptableDeviceAction>;
  let sAction: ref<ScriptableDeviceAction>;
  let i: Int32 = 0;
  while i < ArraySize(choice.data) {
    action = FromVariant<ref<ScriptableDeviceAction>>(choice.data[i]);
    if IsDefined(action) {
      if ChoiceTypeWrapper.IsType(choice.choiceMetaData.type, gameinteractionsChoiceType.CheckFailed) {
        return;
      };
      this.ExecuteAction(action, executor);
    };
    sAction = action as ScriptableDeviceAction;
    if IsDefined(sAction) {
      sAction.SetInteractionLayer(layerTag);
    };
    i += 1;
  };
}

@addMethod(VehicleObject)
protected final const func ExecuteAction(action: ref<DeviceAction>, opt executor: wref<GameObject>) -> Bool {
  let sAction: ref<ScriptableDeviceAction> = action as ScriptableDeviceAction;
  if sAction != null {
    sAction.RegisterAsRequester(this.GetEntityID());
    if executor != null {
      sAction.SetExecutor(executor);
    };
    sAction.ProcessRPGAction(this.GetGame());
    return true;
  };
  return false;
}

@addField(VehicleObject)
public let m_isQhackUploadInProgerss: Bool;

@addMethod(VehicleObject)
protected cb func OnUploadProgressStateChanged(evt: ref<UploadProgramProgressEvent>) -> Bool {
  // FlightLog.Info("[VehicleObject] OnUploadProgressStateChanged");
  if Equals(evt.progressBarContext, EProgressBarContext.QuickHack) {
    if Equals(evt.progressBarType, EProgressBarType.UPLOAD) {
      if Equals(evt.state, EUploadProgramState.STARTED) {
        this.m_isQhackUploadInProgerss = true;
      } else {
        if Equals(evt.state, EUploadProgramState.COMPLETED) {
          this.m_isQhackUploadInProgerss = false;
        };
      };
    };
  };
}

@addMethod(VehicleObject)
protected cb func OnScanningLookedAt(evt: ref<ScanningLookAtEvent>) -> Bool {
  super.OnScanningLookedAt(evt);
  let playerPuppet: ref<PlayerPuppet> = GameInstance.FindEntityByID(this.GetGame(), evt.ownerID) as PlayerPuppet;
  if IsDefined(playerPuppet) && evt.state {
    if this.IsDead() {
      return IsDefined(null);
    };
    this.UpdateScannerLookAtBB(true);
  } else {
    this.UpdateScannerLookAtBB(false);
  };
}

@addMethod(VehicleObject)
private final func UpdateScannerLookAtBB(b: Bool) -> Void {
  let scannerBlackboard: wref<IBlackboard> = GameInstance.GetBlackboardSystem(this.GetGame()).Get(GetAllBlackboardDefs().UI_Scanner);
  scannerBlackboard.SetBool(GetAllBlackboardDefs().UI_Scanner.ScannerLookAt, b);
}

@addMethod(VehicleObject)
private final const func GetQuickHackDuration(quickHackRecord: wref<ObjectAction_Record>, rootObject: wref<GameObject>, targetID: StatsObjectID, instigatorID: EntityID) -> Float {
  let durationMods: array<wref<ObjectActionEffect_Record>>;
  if !IsDefined(quickHackRecord) {
    return 0.00;
  };
  quickHackRecord.CompletionEffects(durationMods);
  return this.GetObjectActionEffectDurationValue(durationMods, rootObject, targetID, instigatorID);
}

@addMethod(VehicleObject)
private final const func GetQuickHackDuration(quickHackID: TweakDBID, rootObject: wref<GameObject>, targetID: StatsObjectID, instigatorID: EntityID) -> Float {
  let durationMods: array<wref<ObjectActionEffect_Record>>;
  let actionRecord: wref<ObjectAction_Record> = TweakDBInterface.GetObjectActionRecord(quickHackID);
  if !IsDefined(actionRecord) {
    return 0.00;
  };
  actionRecord.CompletionEffects(durationMods);
  return this.GetObjectActionEffectDurationValue(durationMods, rootObject, targetID, instigatorID);
}


@addMethod(VehicleObject)
   private final const func GetIgnoredDurationStats() -> array<wref<StatusEffect_Record>> {
    let result: array<wref<StatusEffect_Record>>;
    ArrayPush(result, TweakDBInterface.GetStatusEffectRecord(t"BaseStatusEffect.WasQuickHacked"));
    ArrayPush(result, TweakDBInterface.GetStatusEffectRecord(t"BaseStatusEffect.QuickHackUploaded"));
    return result;
  }

@addMethod(VehicleObject)
  private final const func GetObjectActionEffectDurationValue(durationMods: array<wref<ObjectActionEffect_Record>>, rootObject: wref<GameObject>, targetID: StatsObjectID, instigatorID: EntityID) -> Float {
    let duration: wref<StatModifierGroup_Record>;
    let durationValue: Float;
    let effectToCast: wref<StatusEffect_Record>;
    let i: Int32;
    let ignoredDurationStats: array<wref<StatusEffect_Record>>;
    let lastMatchingEffect: wref<StatusEffect_Record>;
    let statModifiers: array<wref<StatModifier_Record>>;
    if ArraySize(durationMods) > 0 {
      ignoredDurationStats = this.GetIgnoredDurationStats();
      i = 0;
      while i < ArraySize(durationMods) {
        effectToCast = durationMods[i].StatusEffect();
        if IsDefined(effectToCast) {
          if !ArrayContains(ignoredDurationStats, effectToCast) {
            lastMatchingEffect = effectToCast;
          };
        };
        i += 1;
      };
      effectToCast = lastMatchingEffect;
      duration = effectToCast.Duration();
      duration.StatModifiers(statModifiers);
      durationValue = RPGManager.CalculateStatModifiers(statModifiers, this.GetGame(), rootObject, targetID, Cast<StatsObjectID>(instigatorID));
    };
    return durationValue;
  }

  
@addMethod(VehicleObject)
private final const func GetICELevel() -> Float {
  let statsSystem: ref<StatsSystem> = GameInstance.GetStatsSystem(this.GetGame());
  let playerLevel: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(GetPlayer(this.GetGame()).GetEntityID()), gamedataStatType.Level);
  let targetLevel: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatType.Level);
  let resistance: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatType.HackingResistance);
  return resistance + 0.50 * (targetLevel - playerLevel);
}

@addMethod(VehicleObject)
  private final func TranslateActionsIntoQuickSlotCommands(actions: array<ref<DeviceAction>>, out commands: array<ref<QuickhackData>>) -> Void {
    let actionCompletionEffects: array<wref<ObjectActionEffect_Record>>;
    let actionRecord: wref<ObjectAction_Record>;
    let actionStartEffects: array<wref<ObjectActionEffect_Record>>;
    let choice: InteractionChoice;
    let emptyChoice: InteractionChoice;
    let i: Int32;
    let i1: Int32;
    let newCommand: ref<QuickhackData>;    
    // let prereqsToCheck: array<wref<IPrereq_Record>>;
    let targetActivePrereqs: array<wref<ObjectActionPrereq_Record>>;
    let sAction: ref<FlightAction>;
    let isOngoingUpload: Bool = GameInstance.GetStatPoolsSystem(this.GetGame()).IsStatPoolAdded(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatPoolType.QuickHackUpload);
    let statModifiers: array<wref<StatModifier_Record>>;
    let playerRef: ref<PlayerPuppet> = GetPlayer(this.GetGame());
    let iceLVL: Float = this.GetICELevel();
    let actionOwnerName: CName = StringToName(this.GetDisplayName());
    if ArraySize(actions) == 0 {
      newCommand = new QuickhackData();
      newCommand.m_title = "LocKey#42171";
      newCommand.m_isLocked = true;
      newCommand.m_actionState = EActionInactivityReson.Invalid;
      newCommand.m_actionOwnerName = StringToName(this.GetDisplayName());
      newCommand.m_description = "LocKey#42172";
      ArrayPush(commands, newCommand);
    } else {
      i = 0;
      while i < ArraySize(actions) {
        newCommand = new QuickhackData();
        ArrayClear(actionStartEffects);
        sAction = actions[i] as FlightAction;
        actionRecord = sAction.GetObjectActionRecord();
        if NotEquals(actionRecord.ObjectActionType().Type(), gamedataObjectActionType.DeviceQuickHack) {
        } else {
          newCommand.m_uploadTime = sAction.GetActivationTime();
          // newCommand.m_duration = sAction.GetDurationValue();
          newCommand.m_duration = this.GetQuickHackDuration(actionRecord, this, Cast<StatsObjectID>(this.GetEntityID()), playerRef.GetEntityID());
          newCommand.m_title = sAction.title;
          newCommand.m_description = sAction.description;
          newCommand.m_actionOwnerName = actionOwnerName;
          // newCommand.m_title = LocKeyToString(actionRecord.ObjectActionUI().Caption());
          // newCommand.m_description = LocKeyToString(actionRecord.ObjectActionUI().Description());
          newCommand.m_icon = sAction.icon;
          newCommand.m_iconCategory = actionRecord.GameplayCategory().IconName();
          newCommand.m_type = actionRecord.ObjectActionType().Type();
          newCommand.m_actionOwner = this.GetEntityID();
          newCommand.m_isInstant = false;
          newCommand.m_ICELevel = iceLVL;
          newCommand.m_ICELevelVisible = true;
          // newCommand.m_vulnerabilities = this.GetPS().GetActiveQuickHackVulnerabilities();
          newCommand.m_actionState = EActionInactivityReson.Locked;
          // newCommand.m_quality = sAction.quality;
          newCommand.m_costRaw = BaseScriptableAction.GetBaseCostStatic(playerRef, actionRecord);
          newCommand.m_category = actionRecord.HackCategory();
          ArrayClear(actionCompletionEffects);
          actionRecord.CompletionEffects(actionCompletionEffects);
          newCommand.m_actionCompletionEffects = actionCompletionEffects;
          actionRecord.StartEffects(actionStartEffects);
          i1 = 0;
          while i1 < ArraySize(actionStartEffects) {
            if Equals(actionStartEffects[i1].StatusEffect().StatusEffectType().Type(), gamedataStatusEffectType.PlayerCooldown) {
              actionStartEffects[i1].StatusEffect().Duration().StatModifiers(statModifiers);
              newCommand.m_cooldown = RPGManager.CalculateStatModifiers(statModifiers, this.GetGame(), playerRef, Cast<StatsObjectID>(playerRef.GetEntityID()), Cast<StatsObjectID>(playerRef.GetEntityID()));
              newCommand.m_cooldownTweak = actionStartEffects[i1].StatusEffect().GetID();
              ArrayClear(statModifiers);
            };
            if newCommand.m_cooldown != 0.00 {
            }
            i1 += 1;
          };
          if !IsDefined(this as GenericDevice) {
            choice = emptyChoice;
            choice = sAction.GetInteractionChoice();
            if TDBID.IsValid(choice.choiceMetaData.tweakDBID) {
              newCommand.m_titleAlternative = LocKeyToString(TweakDBInterface.GetInteractionBaseRecord(choice.choiceMetaData.tweakDBID).Caption());
            };
          };

          newCommand.m_costRaw = sAction.GetBaseCost();
          newCommand.m_cost = sAction.GetCost();
          if !sAction.IsPossible(this) || !sAction.IsVisible(playerRef) {
            sAction.SetInactiveWithReason(false, "LocKey#7019");
          } else {
            newCommand.m_uploadTime = sAction.GetActivationTime();
            let interactionChoice = sAction.GetInteractionChoice();
            let i2 = 0;
            while i2 < ArraySize(interactionChoice.captionParts.parts) {
              if IsDefined(interactionChoice.captionParts.parts[i2] as InteractionChoiceCaptionStringPart) {
                newCommand.m_title = GetLocalizedText(interactionChoice.captionParts.parts[i2] as InteractionChoiceCaptionStringPart.content);
              };
              i2 += 1;
            };
            if sAction.IsInactive() {
            } else {
              if !sAction.CanPayCost(playerRef) {
                newCommand.m_actionState = EActionInactivityReson.OutOfMemory;
                sAction.SetInactiveWithReason(false, "LocKey#27398");
              };
              if actionRecord.GetTargetActivePrereqsCount() > 0 {
                ArrayClear(targetActivePrereqs);
                actionRecord.TargetActivePrereqs(targetActivePrereqs);
                // i2 = 0;
                // while i2 < ArraySize(targetActivePrereqs) {
                //   ArrayClear(prereqsToCheck);
                //   targetActivePrereqs[i2].FailureConditionPrereq(prereqsToCheck);
                //   if !RPGManager.CheckPrereqs(prereqsToCheck, this) {
                //     sAction.SetInactiveWithReason(false, targetActivePrereqs[i2].FailureExplanation());
                //   }
                //   i2 += 1;
                // };
              };
              if isOngoingUpload {
                sAction.SetInactiveWithReason(false, "LocKey#7020");
              };
            }
          }


          if sAction.IsInactive() {
            newCommand.m_isLocked = true;
            newCommand.m_inactiveReason = sAction.GetInactiveReason();
            if this.HasActiveQuickHackUpload() {
              newCommand.m_action = sAction;
            };
          } else {
            if !sAction.CanPayCost() {
              newCommand.m_actionState = EActionInactivityReson.OutOfMemory;
              newCommand.m_isLocked = true;
              newCommand.m_inactiveReason = "LocKey#27398";
            };
            if GameInstance.GetStatPoolsSystem(this.GetGame()).HasActiveStatPool(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatPoolType.QuickHackUpload) {
              newCommand.m_isLocked = true;
              newCommand.m_inactiveReason = "LocKey#27398";
            };
            if !sAction.IsInactive() || this.HasActiveQuickHackUpload() {
              newCommand.m_action = sAction;
            };
          };
          newCommand.m_actionMatchesTarget = true;
          if !newCommand.m_isLocked {
            newCommand.m_actionState = EActionInactivityReson.Ready;
          };
          ArrayPush(commands, newCommand);
        };
        i += 1;
      };
    };
    i = 0;
    while i < ArraySize(commands) {
      if commands[i].m_isLocked && IsDefined(commands[i].m_action) {
        (commands[i].m_action as ScriptableDeviceAction).SetInactiveWithReason(false, commands[i].m_inactiveReason);
      };
      i += 1;
    };
    QuickhackModule.SortCommandPriority(commands, this.GetGame());
  }


public class FlightMalfunctionEffector extends Effector {
  public let m_owner: wref<VehicleObject>;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOn");
    this.m_owner = owner as VehicleObject;
    if IsDefined(this.m_owner) {
      this.m_owner.UnsetPhysicsStates();
      this.m_owner.EndActions();
      this.m_owner.m_flightComponent.Activate(true);
      this.m_owner.m_flightComponent.lift = 5.0;
    }
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOff");
    if IsDefined(this.m_owner) {
      this.m_owner.m_flightComponent.Deactivate(true);
    }
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] Uninitialize");
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
    this.m_owner.m_flightComponent.Deactivate(true);
  }
}

public class DisableGravityEffector extends Effector {

}

public class BouncyEffector extends Effector {
  public let m_owner: wref<GameObject>;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[BouncyEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[BouncyEffector] ActionOn");
    this.m_owner = owner;
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.bouncy = true;
      vehicle.ignoreImpulses = false;
      vehicle.UnsetPhysicsStates();
      vehicle.EndActions();
      vehicle.m_flightComponent.FireVerticalImpulse(0);
    }
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[BouncyEffector] ActionOff");
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.bouncy = false;
    }
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
  }
}

// @wrapMethod(QuickhacksListGameController)
// private final func SelectData(data: ref<QuickhackData>) -> Void {
//   wrappedMethod(data);
//   // let description: String = GetLocalizedText(this.m_selectedData.m_description);
//   // if StrLen(description) == 0 || Equals(description, "Loading") {
//       let description = ToString(this.m_selectedData.m_description);
//   // };
//   inkTextRef.SetText(this.m_description, description);
// }


@wrapMethod(QuickHackDescriptionGameController)
protected cb func OnQuickHackDataChanged(value: Variant) -> Bool {
  this.m_selectedData = FromVariant<ref<QuickhackData>>(value);
  if IsDefined(this.m_selectedData) {
    let title: String = GetLocalizedText(this.m_selectedData.m_title);
    if StrLen(title) == 0 {
      title = ToString(this.m_selectedData.m_title);
    }
    inkTextRef.SetText(this.m_subHeader, title);

    let description: String = GetLocalizedText(this.m_selectedData.m_description);
    if StrLen(description) == 0 {
      description = ToString(this.m_selectedData.m_description);
    }
    inkTextRef.SetText(this.m_description, description);

    this.SetupTier();
    this.SetupDuration();
    this.SetupMaxCooldown();
    this.SetupUploadTime();
    this.SetupMemotyCost();
    this.SetupCategory();
    this.SetupDamage();
  };
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
    if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) &&
      GameInstance.GetQuestsSystem(scriptInterface.GetGame()).GetFact(n"map_blocked") == 0 &&
      Equals(this.GetCurrentTier(stateContext), GameplayTier.Tier1_FullGameplay) {
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
private let m_flightComponent: ref<FlightComponent>;

@wrapMethod(VehicleObject)
protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
  EntityRequestComponentsInterface.RequestComponent(ri, n"flightComponent", n"FlightComponent", false);
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
@runtimeProperty("offset", "0x611")
public native let ignoreImpulses: Bool;

@addField(VehicleObject)
@runtimeProperty("offset", "0x268")
public native let turnX: Float;

@addField(VehicleObject)
@runtimeProperty("offset", "0x950")
public native let tracePosition: Vector3;

@addMethod(VehicleObject)
public native func EndActions() -> Void;

@addMethod(VehicleObject)
public native func HasGravity() -> Bool;

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
public native func EnableGravity(enabled: Bool) -> Void;

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

@addMethod(VehicleObject)
public native func UnsetPhysicsStates() -> Void;

@addField(VehicleObject)
public let bouncy: Bool;

// working
// @addMethod(VehicleObject)
// protected cb func OnPhysicalCollision(evt: ref<PhysicalCollisionEvent>) -> Bool {
//   // FlightLog.Info("[VehicleObject] OnPhysicalCollision");
//   let vehicle = evt.otherEntity as VehicleObject;
//   if IsDefined(vehicle) {
//     let gameInstance: GameInstance = this.GetGame();
//     let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
//     let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), vehicle);
//     if !isPlayerMounted && this.bouncy {
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

