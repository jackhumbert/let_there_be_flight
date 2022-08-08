public native abstract importonly class IFlightSystem extends IGameSystem {
}

public func fs() -> ref<FlightSystem> = FlightSystem.GetInstance();

public native class FlightSystem extends IFlightSystem {
  public static native func GetInstance() -> ref<FlightSystem>;

  @runtimeProperty("offset", "0x48")
  public native let cameraIndex: Int32;

  @runtimeProperty("offset", "0x50")
  public native let soundListener: wref<IPlacedComponent>;

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
    // this.soundListener = player.FindComponentByName(n"soundListener") as IPlacedComponent;
    this.gameInstance = player.GetGame();
    if !IsDefined(this.audio) {
      this.audio = FlightAudio.Create();
      FlightLog.Info("[FlightSystem] FlightAudio Created");
    }
    this.ctlr = FlightController.GetInstance();
    this.tppCamera = player.FindComponentByName(n"vehicleTPPCamera") as vehicleTPPCameraComponent;
    this.soundListener = this.tppCamera;
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

