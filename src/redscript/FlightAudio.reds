public native class FlightAudio {
  // defined in red4ext part
  public native func Init(eventName: String) -> Void;
  public native func Deinit() -> Void;
  public native func Start() -> Void;
  public native func Stop() -> Void;
  public native func Update() -> Void;

  public let volume: Float;
  public let playerPosition: Vector3;
  public let playerUp: Vector3;
  public let playerForward: Vector3;
  public let cameraPosition: Vector3;
  public let cameraUp: Vector3;
  public let cameraForward: Vector3;
  public let speed: Float;
  public let surge: Float;
  public let yawDiff: Float;
  public let lift: Float;
  public let yaw: Float;
  public let pitchDiff: Float;
  public let brake: Float;
  public static func Create() -> ref<FlightAudio> {
    let instance = new FlightAudio();
    instance.volume = 1.0;
    instance.playerPosition = new Vector3(0.0, 0.0, 0.0);
    instance.playerUp = new Vector3(0.0, 0.0, 1.0);
    instance.playerForward =new Vector3(0.0, 1.0, 0.0);
    instance.cameraPosition = new Vector3(0.0, 0.0, 0.0);
    instance.cameraUp = new Vector3(0.0, 0.0, 1.0);
    instance.cameraForward = new Vector3(0.0, 1.0, 0.0);
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