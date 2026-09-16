public abstract importonly class IFlightSystem extends IGameSystem {
}

public func fs() -> ref<FlightSystem> = FlightSystem.GetInstance();

// public native class FlightSystem extends IFlightSystem {
public native class FlightSystem extends IGameSystem {
  public static native func GetInstance() -> ref<FlightSystem>;
  public native func RegisterComponent(component: ref<FlightComponent>) -> Void;
  public native func UnregisterComponent(component: ref<FlightComponent>) -> Void;

  @runtimeProperty("offset", "0x48")
  public native let cameraIndex: Int32;

  @runtimeProperty("offset", "0x50")
  public native let soundListener: wref<IPlacedComponent>;

  @runtimeProperty("offset", "0x60")
  public native let audio: ref<FlightAudio>;

  public let gameInstance: GameInstance;
  public let player: wref<PlayerPuppet>;
  public let ctlr: ref<FlightController>;
  public let stats: ref<FlightStats>;
  // public let fx: ref<FlightFx>;
  // public let tppCamera: wref<vehicleTPPCameraComponent>;

  @runtimeProperty("offset", "0xB8")
  public native let playerComponent: wref<FlightComponent>;

  // Set by hudFlightController when its hud entry is spawned. With the HUD attached to the
  // player that happens once per session instead of once per mount, so the controller has
  // to be told which vehicle it is describing.
  public let hud: wref<hudFlightController>;

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
    // this.tppCamera = player.FindComponentByName(n"vehicleTPPCamera") as vehicleTPPCameraComponent;
    // this.soundListener = this.tppCamera;
  }

  public func SetPlayerComponent(component: wref<FlightComponent>) -> Void {
    FlightLog.Info("[FlightSystem] Player component set");
    this.playerComponent = component;
  }

  public func SetHud(hud: wref<hudFlightController>) -> Void {
    this.hud = hud;
    if IsDefined(hud) {
      FlightLog.Info("[FlightSystem] Flight HUD controller registered");
    } else {
      FlightLog.Info("[FlightSystem] Flight HUD controller cleared");
    }
  }

  // Called on every player mount and unmount so the flight HUD can follow the vehicle
  // without being destroyed and rebuilt with it. Passing null means "no vehicle".
  public func NotifyHudVehicleChanged(vehicle: wref<VehicleObject>) -> Void {
    if IsDefined(this.hud) {
      this.hud.OnFlightVehicleChanged(vehicle);
    }
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

