public native class FlightAudio extends IScriptable {
  // defined in red4ext part
  // LTBF's own samples as objects at the car (docs/design/audio.md, Samples): by the event's name,
  // once, or looping under an id until StopLoop
  public native func PlayOneShot(name: String, vehicle: ref<VehicleObject>) -> Void;
  public native func StartLoop(id: String, name: String, vehicle: ref<VehicleObject>) -> Void;
  public native func StopLoop(id: String) -> Void;
  // the engine synth's water low-pass: 1 under water, 0 in the air
  public native func SetWater(value: Float) -> Void;
  // every playing sound paused / resumed
  public native func Pause() -> Void;
  public native func Resume() -> Void;
  // the Autopilot's cinematic camera is on: the player's car is heard from outside, and from the camera
  public native func SetCinematic(on: Bool) -> Void;
  // the echoes' state as JSON, for tests
  public native func AcousticsState() -> String;

  // the nav map's and the live distances side by side to red4ext/logs/ltbf_acoustics_<time>.csv
  public native func AcousticsLog(on: Bool) -> Void;
  // LTBF's sound to red4ext/logs/ltbf_stems_<time>_{dry,wet,bus}.wav: the thrusters without and
  // with the echoes, the master bus; returns the base path ("" when stopped)
  public native func AcousticsStems(on: Bool) -> String;
  // a test sound through the echoes in place of LTBF's sound: 0 off, 1 clicks (two a second),
  // 2 pink noise, 3 a 440 Hz sine
  public native func AcousticsTestSource(source: Int32) -> Void;

  // a thruster's engine synth (docs/design/engine-synth.md); car is
  // the vehicle's entity hash (an NPC car's thrusters fold into one voice), player whether it is V's;
  // the plugin updates it each frame from the vehicle (Audio/Inputs/Gather)
  public native func StartEngine(emitterName: String, pitch: Float, car: Uint64, player: Bool) -> Void;
  public native func StopEngine(emitterName: String) -> Void;
  // its channels and cost as JSON, for tests
  public native func EngineState() -> String;
  // which of the old FMOD event's effects the synth keeps: "fmod", "clean", or any of
  // "cone=0,distortion=0,darkness=0,grit=0" (1 to keep); applies at once
  public native func EngineCharacter(text: String) -> String;
  // what the thrusters' synth starts from: "drone" (the old loops), "bullroarer" or "fan"; applies at once
  public native func EngineSound(name: String) -> String;
  // the voice budget: Windows Sonic objects and thruster synth voices (1 to 64 each); returns the audio
  // summary, or a line starting "error:"
  public native func VoicesBudget(maxObjects: Int32, maxSynth: Int32) -> String;
  // the voice pool as JSON: sources, who has channels, folded cars, the synth's cost
  public native func VoicesState() -> String;
  // the car's front windows (world positions) for the first person wind (the wind itself,
  // docs/design/audio.md, Wind, the plugin works out from the car each frame)
  public native func SetWindowSlots(vehicle: ref<VehicleObject>, left: Vector4, right: Vector4) -> Void;
  public native func WindOn(on: Bool) -> Void;
  // the wind's state as JSON, for tests
  public native func WindState() -> String;
  // air absorption on the echoes and reverb (on by default; false turns it off)
  public native func AcousticsAir(on: Bool) -> Void;
  // echoes off planes fitted to the live hits (the default) or off square-on surfaces
  public native func AcousticsPlanes(on: Bool) -> Void;
  // NPC cars heard through buildings: a low-pass and a duck while static geometry is between (on by default)
  public native func AcousticsOcclusion(on: Bool) -> Void;
  // one line: the thrusters' synth, its character and the echoes, as they are now
  public native func AudioSummary() -> String;

  // the echoes on or off, and their level (not a Mod Setting yet; set by test helpers)
  public let acoustics: Bool = true;
  public let acousticsGain: Float = 1.0;
  // the near surfaces from the game's own collision (live raycasts) rather than the nav map alone:
  // 0 off, 2 the plugin through PhysX
  public let acousticsRaySource: Int32 = 2;
  // the comparison log is being written (read by test helpers)
  public let acousticsLogging: Bool;

  public static func Get() -> ref<FlightAudio> {
    return FlightSystem.GetInstance().audio;
  }

  private let uiBlackboard: wref<IBlackboard>;
  private let menuCallback: ref<CallbackHandle>;
  public let isInMenu: Bool;

  private let uiGameDataBlackboard: wref<IBlackboard>;
  private let popupCallback: ref<CallbackHandle>;
  public let isPopupShown: Bool;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Hud-And-Sound")
  @runtimeProperty("ModSettings.category.order", "7")
  @runtimeProperty("ModSettings.order", "1")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Engine-Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let engineVolume: Float = 1.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Hud-And-Sound")
  @runtimeProperty("ModSettings.category.order", "7")
  @runtimeProperty("ModSettings.order", "2")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Wind-Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let windVolume: Float = 0.6;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Hud-And-Sound")
  @runtimeProperty("ModSettings.category.order", "7")
  @runtimeProperty("ModSettings.order", "3")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Warning-Volume")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let warningVolume: Float = 0.5;

  public static func Create() -> ref<FlightAudio> {
    let self = new FlightAudio();

    LTBF_RegisterListener(self);

    return self;
  }
  
  public cb func Initialize() {
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

  // the handles cleared too, so the next OnWorldAttached registers the listeners again
  protected cb func OnWorldPendingDetach() {
    if IsDefined(this.uiBlackboard) && IsDefined(this.menuCallback) {
      this.uiBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_System.IsInMenu, this.menuCallback);
    }
    this.menuCallback = null;
    if IsDefined(this.uiGameDataBlackboard) && IsDefined(this.popupCallback) {
      this.uiGameDataBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown, this.popupCallback);
    }
    this.popupCallback = null;
  }

  protected cb func OnIsInMenu(inMenu: Bool) -> Bool {
    this.isInMenu = inMenu;
  }
  protected cb func OnPopupIsShown(isShown: Bool) -> Bool {
    this.isPopupShown = isShown;
  }

  // where the entity's slot is in the world
  public static func SlotPosition(entity: ref<Entity>, slot: CName) -> Vector4 {
    let p: Vector4;
    IPositionProvider.CreateSlotPositionProvider(entity, slot).CalculatePosition(p);
    return p;
  }
}
