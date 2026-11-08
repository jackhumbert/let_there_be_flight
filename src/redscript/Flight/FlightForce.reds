// The force tail in C++ (src/red4ext/Flight/Force.hpp): FlightMode.ApplyPhysics and the scaling after it in
// FlightComponent.OnUpdate. The component fills the params and the inputs, calls Step, then copies the outputs
// back.
public native class FlightForce extends IScriptable {
  // params, as FlightSettings' fields of the same name
  public native let physxFlightPhysics: Bool;
  public native let generalDampFactorAngular: Float;
  public native let brakeFactorAngular: Float;
  public native let generalYawDampFactor: Float;
  public native let dampStabilityK: Float;
  public native let dampMaxPerFrame: Float;
  public native let generalDampFactorAngularMax: Float;
  public native let generalDampFactorLinear: Float;
  public native let brakeFactorLinear: Float;
  public native let generalSwayWithYaw: Float;
  public native let generalYawDirectionalityFactor: Float;
  public native let generalPitchDirectionalityFactor: Float;
  public native let generalYawAeroFactor: Float;
  public native let generalPitchAeroFactor: Float;
  public native let collisionRecoveryDelay: Float;
  public native let collisionRecoveryDuration: Float;
  public native let handling: Int32; // FlightSettings.handling: Loose and Tight replace the handling values above

  // inputs, this frame
  public native let dt: Float;
  public native let force: Vector4;          // car frame, before ApplyPhysics (the trick plus the mode's force)
  public native let torque: Vector4;
  public native let applyPhysics: Bool;      // false when the mode index is out of range
  public native let pitch: Float;
  public native let roll: Float;
  public native let yaw: Float;
  public native let linearBrake: Float;
  public native let angularBrake: Float;
  public native let thrusterTensor: Vector4;
  public native let contactSpin: Vector4;    // world, FlightComponent.GetContactSpin
  public native let collisionTimer: Float;   // in and out

  // outputs
  public native let physicsForce: Vector4;   // what ApplyPhysics leaves in the mode's force (car frame)
  public native let physicsTorque: Vector4;
  public native let linearDamp: Float;       // the mode's linearDamp
  public native let worldForce: Vector4;     // what OnUpdate adds to the component's force
  public native let worldTorque: Vector4;

  // the force tail from the fields; sets the outputs and collisionTimer
  public native func Step(stats: ref<FlightStats>) -> Void;
}
