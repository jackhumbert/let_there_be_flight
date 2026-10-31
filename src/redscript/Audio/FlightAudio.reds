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
  public let scrape: Float;
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
  // hear from here this frame instead of the player's soundListener (the cinematic camera's shots)
  public native func SetCameraListener(position: Vector4, forward: Vector4, up: Vector4) -> Void;
  public native func UpdateParameter(parameterName: String, value: Float) -> Void;
  // every playing event paused / resumed
  public native func Pause() -> Void;
  public native func Resume() -> Void;
  // each frame the player's car flies: the flying sound's echoes off the city (docs/design/acoustics.md);
  // gain 0 = off, inside 1 = first person, raySource who casts the near rays (acousticsRaySource)
  public native func UpdateAcoustics(carMatrix: Matrix, gain: Float, inside: Float, raySource: Int32) -> Void;
  // the echoes' state as JSON, for tests
  public native func AcousticsState() -> String;

  // the near rays' directions for the car, for CastAcousticRays
  public native func AcousticsRays(carMatrix: Matrix) -> array<Vector4>;
  // what they hit: distance (< 0: nothing) and physics material, cast from origin
  public native func AcousticsHits(origin: Vector4, distances: array<Float>, materials: array<CName>) -> Void;
  // the nav map's and the live distances side by side to red4ext/logs/ltbf_acoustics_<time>.csv
  public native func AcousticsLog(on: Bool) -> Void;
  // LTBF's FMOD sound to red4ext/logs/ltbf_stems_<time>_{dry,wet,bus}.wav: the thrusters without and
  // with the echoes, the master bus; returns the base path ("" when stopped)
  public native func AcousticsStems(on: Bool) -> String;
  // a test sound through the echoes in place of LTBF's FMOD sound: 0 off, 1 clicks (two a second),
  // 2 pink noise, 3 a 440 Hz sine; the thrusters are muted while it plays
  public native func AcousticsTestSource(source: Int32) -> Void;

  // the engine synth in place of the thruster event vehicle3_TPP (docs/design/engine-synth.md)
  public native func StartEngine(emitterName: String, pitch: Float) -> Void;
  public native func UpdateEngine(emitterName: String, eventMatrix: Matrix, volume: Float, update: ref<FlightAudioUpdate>) -> Void;
  public native func StopEngine(emitterName: String) -> Void;
  // its channels and cost as JSON, for tests
  public native func EngineState() -> String;
  // who makes the thrusters' sound: 0 the FMOD event, 1 the engine synth (not a Mod Setting yet:
  // LtbfBridge.EngineSource for tests; takes effect when the thrusters next start)
  public let engineSource: Int32 = 0;

  // the echoes on or off, and their level (not a Mod Setting yet: LtbfBridge.Acoustics for tests)
  public let acoustics: Bool = true;
  public let acousticsGain: Float = 1.0;
  // the near surfaces from the game's own collision (live raycasts) rather than the nav map alone:
  // 0 off, 1 cast here (CastAcousticRays), 2 cast by the plugin through PhysX
  public let acousticsRaySource: Int32 = 2;
  public let acousticsRayRange: Float = 100.0;
  private let acousticsRayTimer: Float;
  // the comparison log is being written (LtbfBridge.AcousticsLive)
  public let acousticsLogging: Bool;

  // The near rays through the game's collision (static world, terrain, blockers: what a flying car
  // hits), about 20 times a second while the echoes are on; the plugin uses a hit in place of the
  // nav map's answer for that ray (a miss may only be collision not streamed in yet). Only with
  // acousticsRaySource 1: the plugin's own PhysX casts (2) need no script time.
  public func CastAcousticRays(carMatrix: Matrix, timeDelta: Float) -> Void {
    if !this.acoustics || this.acousticsRaySource != 1 {
      return;
    }
    this.acousticsRayTimer -= timeDelta;
    if this.acousticsRayTimer > 0.0 {
      return;
    }
    this.acousticsRayTimer = 0.05;
    let filter: QueryFilter;
    QueryFilter.AddGroup(filter, n"Static");
    QueryFilter.AddGroup(filter, n"Terrain");
    QueryFilter.AddGroup(filter, n"VehicleBlocker");
    QueryFilter.AddGroup(filter, n"PlayerBlocker");
    let queries = GameInstance.GetSpatialQueriesSystem(FlightSystem.GetInstance().gameInstance);
    let origin = Matrix.GetTranslation(carMatrix);
    let distances: array<Float>;
    let materials: array<CName>;
    for d in this.AcousticsRays(carMatrix) {
      // from the edge of a car-sized ellipsoid (3 m sideways, 1.5 m up and down, as the nav map
      // grows its solids): the query sees the car's own chassis
      let start = 1.0 / SqrtF(PowF(SqrtF(d.X * d.X + d.Y * d.Y) / 3.0, 2.0) + PowF(d.Z / 1.5, 2.0));
      let result: TraceResult;
      queries.SyncRaycastByQueryFilter(origin + d * start, origin + d * this.acousticsRayRange, filter, result, false, false);
      if TraceResult.IsValid(result) && !StrBeginsWith(NameToString(result.material), "vehicle") {
        ArrayPush(distances, Vector4.Distance(origin, Vector4.Vector3To4(result.position)));
        ArrayPush(materials, result.material);
      } else {
        ArrayPush(distances, -1.0);
        ArrayPush(materials, n"");
      }
    }
    this.AcousticsHits(origin, distances, materials);
  }


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
    // Do not check TimeSystem.IsPausedState() here: that flag is the in-game clock pause that
    // quests and races set (Chipping In, act 2 transitions, race starts) and sometimes never clear,
    // which left flight audio silent for the rest of the session. The real pause menu is covered by
    // isInMenu and by FlightSystem::OnGamePaused/OnGameResumed on the native side.
    if this.isInMenu ||
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