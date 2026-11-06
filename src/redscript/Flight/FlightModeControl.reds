// The mode controllers' maths in C++ (Flight/Modes.hpp): the mode fills the params and the inputs from its fields each
// frame, calls one method and copies the outputs back (and the in/out fields for the modes that keep state).
public native class FlightModeControl extends IScriptable {
  // params, as the mode's and FlightSettings' fields of the same name
  public native let standardModeSurgeFactor: Float;
  public native let standardModeYawFactor: Float;
  public native let standardModeSwayFactor: Float;
  public native let standardModePitchFactor: Float;
  public native let standardModeLiftFactor: Float;
  public native let standardModeHoverFactor: Float;
  public native let standardModePitchInputAngle: Float;
  public native let standardModeRollFactor: Float;
  public native let standardModeRollInputAngle: Float;
  public native let generalRollWithYaw: Float;
  public native let pitchWithLift: Float;
  public native let pitchWithSurge: Float;
  public native let brakeFactorLinear: Float;
  public native let brakeFactorAngular: Float;
  public native let hoverClamp: Float;
  public native let hoverModeMinHoverHeight: Float;
  public native let hoverModeMaxHoverHeight: Float;
  public native let gravityFactor: Float;
  public native let altitudeHoldFactor: Float;
  public native let heightDampening: Float;
  public native let heightCorrectionFactor: Float;
  public native let automaticModeAutoBrakingFactor: Float;
  public native let automaticModeYawDirectionality: Float;
  public native let automaticModeLiftFactor: Float;
  public native let automaticModeHoverFactor: Float;
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
  public native let foundGround: Bool;       // HoverFly and Automatic: what FindGround returned
  public native let distance: Float;         // FlightComponent.distance after FindGround
  public native let normal: Vector4;         // FindGround's out parameter (zero when nothing was found)

  // in and out
  public native let hoverHeight: Float;      // FlightComponent.hoverHeight
  public native let hovering: Float;
  public native let referenceZ: Float;
  public native let holdingAltitude: Bool;

  // outputs, car frame
  public native let force: Vector4;
  public native let torque: Vector4;

  // FlightModeStandard.UpdateWithNormalLift and UpdateWithNormalDistance; set force and torque
  public native func StandardLift(stats: ref<FlightStats>, pitchPID: ref<PID>, rollPID: ref<PID>, normal: Vector4, liftForce: Float) -> Void;
  public native func StandardDistance(stats: ref<FlightStats>, hoverPID: ref<PID>, pitchPID: ref<PID>, rollPID: ref<PID>, normal: Vector4, heightDifference: Float) -> Void;
  // FlightModeAutomatic.Update after FindGround (HoverFly: the script Hover & Fly's, deleted 2026-11-05, no caller); they
  // write the in/out fields back
  public native func HoverFly(stats: ref<FlightStats>, hoverPID: ref<PID>, pitchPID: ref<PID>, rollPID: ref<PID>) -> Void;
  public native func Automatic(stats: ref<FlightStats>, hoverPID: ref<PID>, pitchPID: ref<PID>, rollPID: ref<PID>) -> Void;
  // FlightModeDrone.Update
  // component: the mode's FlightComponent, for Drone's skid just over the ground (Flight::DroneSkid)
  public native func Drone(stats: ref<FlightStats>, component: ref<IScriptable>) -> Void;

  // Vector4.RotateAxis, Vector4.Interpolate and LerpF as the native code computes them, to compare with the game's own
  public native static func RotateAxis(v: Vector4, axis: Vector4, angle: Float) -> Vector4;
  public native static func Interpolate(a: Vector4, b: Vector4, t: Float) -> Vector4;
  public native static func Lerp(alpha: Float, a: Float, b: Float) -> Float;
}
