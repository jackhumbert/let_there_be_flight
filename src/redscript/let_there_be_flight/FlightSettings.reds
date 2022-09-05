public static func FlightSettings() -> ref<FlightSettings> {
  return FlightSettings.GetInstance();
}

public native class FlightSettings extends IScriptable {
  public native static func GetInstance() -> ref<FlightSettings>;
  public native static func GetFloat(name: String) -> Float;
  public native static func SetFloat(name: String, value: Float) -> Float;
  public native static func GetVector3(name: String) -> Vector3;
  public native static func SetVector3(name: String, x: Float, y: Float, z: Float) -> Vector3;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "General Flight Settings")
  @runtimeProperty("ModSettings.displayName", "Auto Activation Height")
  @runtimeProperty("ModSettings.description", "In-game units for detecting when flight should automatically be activated on spawn")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.max", "10.0")
  public let autoActivationHeight: Float = 3.0;

  // Flight Control Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Control Settings")
  @runtimeProperty("ModSettings.displayName", "Linear Brake Factor")
  @runtimeProperty("ModSettings.description", "How much the linear brake button slows the vehicle's velocity")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "10.0")
  public let brakeFactorLinear: Float = 1.2;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Control Settings")
  @runtimeProperty("ModSettings.displayName", "Angular Brake Factor")
  @runtimeProperty("ModSettings.description", "How much the angular brake button slows the vehicle's rotation")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "50.0")
  public let brakeFactorAngular: Float = 5.0;
  
  // Flight Physics Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Apply Flight Physics When Deactivated")
  @runtimeProperty("ModSettings.description", "Useful for continuing to control the vehicle mid-air when deactivating")
  public let generalApplyFlightPhysicsWhenDeactivated: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Linear Damp Factor")
  @runtimeProperty("ModSettings.description", "How much resistance any linear movement is given")
  @runtimeProperty("ModSettings.step", "0.0001")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "0.01")
  public let generalDampFactorLinear: Float = 0.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Angular Damp Factor")
  @runtimeProperty("ModSettings.description", "How much resistance any angular movement is given")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "10.0")
  public let generalDampFactorAngular: Float = 3.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Pitch Aero Factor")
  @runtimeProperty("ModSettings.description", "How much the vehicle is rotated (pitch) towards its velocity")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalPitchAeroFactor: Float = 0.25;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Yaw Aero Factor")
  @runtimeProperty("ModSettings.description", "How much the vehicle is rotated (yaw) towards its velocity")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalYawAeroFactor: Float = 0.1;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Pitch Directionality Factor")
  @runtimeProperty("ModSettings.description", "How much the vehicle's pitch affects its velocity")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let generalPitchDirectionalityFactor: Float = 80.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Physics Settings")
  @runtimeProperty("ModSettings.displayName", "Yaw Directionality Factor")
  @runtimeProperty("ModSettings.description", "How much the vehicle's yaw affects its velocity")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let generalYawDirectionalityFactor: Float = 50.0;

  // Flight Camera Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Camera Settings")
  @runtimeProperty("ModSettings.displayName", "Driving Direction Compensation Angle Smoothing")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "180.0")
  public let drivingDirectionCompensationAngleSmooth: Float = 120.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Camera Settings")
  @runtimeProperty("ModSettings.displayName", "Driving Direction Compensation Speed Coef")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let drivingDirectionCompensationSpeedCoef: Float = 0.1;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Camera Settings")
  @runtimeProperty("ModSettings.displayName", "FPV Camera Pitch Offset")
  @runtimeProperty("ModSettings.description", "Angle in degrees")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "-60.0")
  @runtimeProperty("ModSettings.max", "60.0")
  public let fpvCameraPitchOffset: Float = 0.0;

  // public cb func OnModSettingsUpdate(variable: CName, value: Variant) {
  //   switch (variable) {
  //     case n"autoActivationHeight":
  //       this.autoActivationHeight = FromVariant<Float>(value);
  //       break;
  //   }
  // }

  private func OnAttach() -> Void {
    FlightLog.Info("[FlightSettings] OnAttach");
    ModSettings.RegisterListenerToClass(this);
    
    FlightSettings.SetVector3("inputPitchPID", 1.0, 0.5, 0.5);
    FlightSettings.SetVector3("inputRollPID", 1.0, 0.5, 0.5);
    
    FlightSettings.SetVector3("aeroYawPID", 1.0, 0.01, 1.0);
    FlightSettings.SetVector3("aeroPitchPID", 1.0, 0.01, 1.0);

    FlightSettings.SetVector3("hoverModePID", 1.0, 0.005, 0.5);

    FlightSettings.SetFloat("automaticModeAutoBrakingFactor", 200.0);
    FlightSettings.SetFloat("automaticModeYawDirectionality", 300.0);

    FlightSettings.SetFloat("brakeOffset", 0.0);
    FlightSettings.SetFloat("collisionRecoveryDelay", 0.8);
    FlightSettings.SetFloat("collisionRecoveryDuration", 0.8);
    FlightSettings.SetFloat("defaultHoverHeight", 3.50);
    FlightSettings.SetFloat("distance", 0.0);
    FlightSettings.SetFloat("distanceEase", 0.1);

    FlightSettings.SetFloat("flyModeLiftFactor", 20.0);

    FlightSettings.SetVector3("FPVCameraOffset", 0.0, 0.0, -0.5);

    FlightSettings.SetFloat("fwtfCorrection", 0.0);
    FlightSettings.SetFloat("hoverClamp", 10.0);
    FlightSettings.SetFloat("hoverFactor", 40.0);
    FlightSettings.SetFloat("hoverModeLiftFactor", 8.0);
    FlightSettings.SetFloat("isFlightUIActive", 1.0);
    FlightSettings.SetFloat("liftFactor", 8.0);
    FlightSettings.SetFloat("lockFPPCameraForDrone", 1.0);
    FlightSettings.SetFloat("lookAheadMax", 10.0);
    FlightSettings.SetFloat("lookAheadMin", 1.0);
    FlightSettings.SetFloat("hoverModeMaxHoverHeight", 7.0);
    FlightSettings.SetFloat("hoverModeMinHoverHeight", 1.0);
    FlightSettings.SetFloat("normalEase", 0.3);
    FlightSettings.SetFloat("referenceZ", 0.0);
    FlightSettings.SetFloat("secondCounter", 0.0);
    
    FlightSettings.SetFloat("surgeOffset", 0.5);

    FlightSettings.SetFloat("swayWithYaw", 0.5);
    FlightSettings.SetFloat("rollWithYaw", 0.15);
    FlightSettings.SetFloat("pitchWithLift", 0.0);
    FlightSettings.SetFloat("pitchWithSurge", 0.0);

    FlightSettings.SetFloat("yawD", 3.0);
  }
}