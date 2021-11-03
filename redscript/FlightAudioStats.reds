public static native func StartFlightAudio() -> Void
public static native func StopFlightAudio() -> Void
public static native func UpdateFlightAudio() -> Void

public class FlightAudioStats {
  public let volume: Float;
  public let playerPosition: Vector4;
  public let playerUp: Vector4;
  public let playerForward: Vector4;
  public let cameraPosition: Vector4;
  public let cameraUp: Vector4;
  public let cameraForward: Vector4;
  public let speed: Float;
  public let surge: Float;
  public let yawDiff: Float;
  public let lift: Float;
  public let yaw: Float;
  public let pitchDiff: Float;
  public let brake: Float;
  public static func Create() -> ref<FlightAudioStats> {
    let instance = new FlightAudioStats();
    instance.volume = 1.0;
    instance.playerPosition = Vector4.EmptyVector();
    instance.playerUp = new Vector4(0.0, 0.0, 1.0, 0.0);
    instance.playerForward =new Vector4(0.0, 1.0, 0.0, 0.0);
    instance.cameraPosition = Vector4.EmptyVector();
    instance.cameraUp = new Vector4(0.0, 0.0, 1.0, 0.0);
    instance.cameraForward = new Vector4(0.0, 1.0, 0.0, 0.0);
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