public native abstract importonly class IFlightSystem extends IGameSystem {
}

public native class FlightSystem extends IFlightSystem {
  public let gameInstance: GameInstance;
  public let player: ref<PlayerPuppet>;
  public let ctlr: ref<FlightController>;
  public let stats: ref<FlightStats>;
  public let settings: ref<FlightSettings>;
  public let audio: ref<FlightAudio>;
  public let fx: ref<FlightFx>;
  public let tppCamera: ref<vehicleTPPCameraComponent>;
  public let components: array<ref<FlightComponent>>;

  public static native func GetInstance() -> ref<FlightSystem>;

  public func Setup(player: ref<PlayerPuppet>) -> Void {
    this.player = player;
    this.gameInstance = player.GetGame();
    this.settings = new FlightSettings();
    this.audio = FlightAudio.Create();
  }

  public func OnUpdate(timeDelta: Float) -> Void {
    let i = 0;
    while (i < ArraySize(this.components)) {
      this.components[i].OnUpdate(timeDelta);
    }
  }

//   public static func Get(gameInstance: GameInstance) -> ref<FlightSystem> {
//     return GameInstance.GetScriptableSystemsContainer(gameInstance).Get(n"FlightSystem") as FlightSystem;
//   }

  // public final func OnFlightSystemAttach() -> Void {
  //   FlightLog.Info("[FlightSystem] OnFlightSystemAttach");
  // }

  // public final func OnPlayerAttach(request: ref<PlayerAttachRequest>) -> Void {
  //   FlightLog.Info("[FlightSystem] OnPlayerAttach");
  // }

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

