// The attitude control of FlyAerobatic in C++ (src/red4ext/Navigation/Aerobatics.hpp). FlightModeAutopilot fills the
// params, the state and the inputs each frame, calls the eases and Step, and copies the state back.
public native class FlightAttitudeControl extends IScriptable {
  // params, as the fields of the same name on FlightModeAutopilot, FlightSettings and FlightMode
  public native let raceRollFrequency: Float;
  public native let raceRollRateMax: Float;
  public native let raceRollAccelMax: Float;
  public native let racePitchFrequency: Float;
  public native let racePitchRateMax: Float;
  public native let racePitchAccelMax: Float;
  public native let attitudeFrequency: Float;
  public native let attitudeDamping: Float;
  public native let maxAngularAccel: Float;
  public native let torqueGainPitch: Float;
  public native let torqueGainRoll: Float;
  public native let torqueGainYaw: Float;
  public native let generalDampFactorAngular: Float;
  public native let generalYawDampFactor: Float;
  public native let generalDampFactorAngularMax: Float;
  public native let dampStabilityK: Float;
  public native let dampMaxPerFrame: Float;
  public native let standardModeSurgeFactor: Float;
  public native let standardModeSwayFactor: Float;
  public native let standardModeLiftFactor: Float;
  public native let gravityFactor: Float;

  // state, in and out
  public native let targetRight: Vector4;
  public native let targetForward: Vector4;
  public native let targetUp: Vector4;
  public native let omegaRef: Vector4;       // the smoothed rate of the target
  public native let bankAngle: Float;
  public native let bankRateNow: Float;
  public native let pitchAngle: Float;
  public native let pitchRateNow: Float;

  // inputs, this frame
  public native let dt: Float;
  public native let shape: Bool;             // a manoeuvre's shape set the attitude: the bank easing carries on from it
  public native let accel: Vector4;          // the commanded acceleration
  public native let drag: Vector4;           // the surge hold's drag
  public native let dragAccel: Float;
  public native let raceDecel: Float;
  public native let thrusterTensor: Vector4;
  public native let inertiaX: Vector4;       // FlightComponent.GetWorldInertiaRow; W > 0.5: the rows are valid
  public native let inertiaY: Vector4;
  public native let inertiaZ: Vector4;
  public native let fwd: Vector4;            // the nose wanted
  public native let up: Vector4;             // the top wanted

  // outputs
  public native let torque: Vector4;
  public native let thrust: Vector4;         // in the car's frame (sway, surge, lift)
  public native let rateRef: Vector4;        // the target's rate in the car's frame
  public native let attitudeError: Float;

  // the wanted up vector with its bank eased; updates bankAngle and bankRateNow
  public native func EaseBank(dt: Float, fwd: Vector4, up: Vector4) -> Vector4;
  // the nose with its pitch eased; tangent is the line's (LineFrameOver), read when onLine; updates pitchAngle and pitchRateNow
  public native func EasePitch(dt: Float, fwd: Vector4, onLine: Bool, tangent: Vector4, headingRef: Vector4) -> Vector4;
  // the target frame, the torque and the thrust from the fields; updates the state
  public native func Step(stats: ref<FlightStats>) -> Void;
}
