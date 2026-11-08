// The Drone modes' maths in C++ (Flight/Modes.hpp): the mode fills the params and the inputs from its fields each
// frame, calls Drone and copies the outputs back.
public native class FlightModeControl extends IScriptable {
  // params, as the mode's and FlightSettings' fields of the same name: the brakes scaled by Brake Strength, the
  // drone's factors by Drone Thrust and Drone Turn Speed
  public native let brakeFactorLinear: Float;
  public native let brakeFactorAngular: Float;
  public native let droneModeLiftFactor: Float;
  public native let droneModePitchFactor: Float;
  public native let droneModeRollFactor: Float;
  public native let droneModeSurgeFactor: Float;
  public native let droneModeYawFactor: Float;
  public native let droneModeSwayFactor: Float;

  // inputs, this frame
  public native let dt: Float;
  public native let surge: Float;
  public native let lift: Float;
  public native let sway: Float;
  public native let yaw: Float;
  public native let roll: Float;
  public native let pitch: Float;
  public native let linearBrake: Float;
  public native let angularBrake: Float;

  // outputs, car frame
  public native let force: Vector4;
  public native let torque: Vector4;

  // FlightModeDrone.Update
  // component: the mode's FlightComponent, for Drone's skid just over the ground (Flight::DroneSkid)
  public native func Drone(stats: ref<FlightStats>, component: ref<IScriptable>) -> Void;

  // Vector4.RotateAxis, Vector4.Interpolate and LerpF as the native code computes them, to compare with the game's own
  public native static func RotateAxis(v: Vector4, axis: Vector4, angle: Float) -> Vector4;
  public native static func Interpolate(a: Vector4, b: Vector4, t: Float) -> Vector4;
  public native static func Lerp(alpha: Float, a: Float, b: Float) -> Float;
}
