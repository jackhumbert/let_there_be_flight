public native class FlightSettings extends IScriptable {
  public native static func GetFloat(name: String) -> Float;
  public native static func SetFloat(name: String, value: Float) -> Float;
  public native static func GetVector3(name: String) -> Vector3;
  public native static func SetVector3(name: String, x: Float, y: Float, z: Float) -> Vector3;

  private func OnAttach() -> Void {
    FlightLog.Info("[FlightSettings] OnAttach");

    FlightSettings.SetFloat("autoActivationHeight", 2.0);
    
    FlightSettings.SetVector3("inputPitchPID", 1.0, 0.5, 0.5);
    FlightSettings.SetVector3("inputRollPID", 1.0, 0.5, 0.5);
    
    FlightSettings.SetVector3("aeroYawPID", 1.0, 0.01, 1.0);
    FlightSettings.SetVector3("aeroPitchPID", 1.0, 0.01, 1.0);

    FlightSettings.SetVector3("hoverModePID", 1.0, 0.005, 0.5);

    FlightSettings.SetFloat("generalDampFactorLinear", 0.001);
    FlightSettings.SetFloat("generalDampFactorAngular", 3.0);
    // FlightSettings.SetFloat("generalPitchAeroFactor", 0.25);
    FlightSettings.SetFloat("generalPitchAeroFactor", 0.0);
    FlightSettings.SetFloat("generalPitchDirectionalityFactor", 80.0);
    FlightSettings.SetFloat("generalYawAeroFactor", 0.1);
    FlightSettings.SetFloat("generalYawDirectionalityFactor", 50.0);

    FlightSettings.SetFloat("brakeFactorAngular", 10.0);
    FlightSettings.SetFloat("brakeFactorLinear", 1.2);

    FlightSettings.SetFloat("automaticModeAutoBrakingFactor", 200.0);
    FlightSettings.SetFloat("automaticModeYawDirectionality", 300.0);
    FlightSettings.SetFloat("brakeOffset", 0.0);
    FlightSettings.SetFloat("collisionRecoveryDelay", 0.8);
    FlightSettings.SetFloat("collisionRecoveryDuration", 0.8);
    FlightSettings.SetFloat("defaultHoverHeight", 3.50);
    FlightSettings.SetFloat("distance", 0.0);
    FlightSettings.SetFloat("distanceEase", 0.1);

    FlightSettings.SetFloat("droneModeLiftFactor", 40.0);
    FlightSettings.SetFloat("droneModePitchFactor", 5.0);
    FlightSettings.SetFloat("droneModeRollFactor", 12.0);
    FlightSettings.SetFloat("droneModeSurgeFactor", 15.0);
    FlightSettings.SetFloat("droneModeYawFactor", 5.0);
    FlightSettings.SetFloat("droneModeSwayFactor", 15.0);

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
    FlightSettings.SetFloat("pitchWithLift", 0.0);
    FlightSettings.SetFloat("pitchWithSurge", 0.0);
    FlightSettings.SetFloat("referenceZ", 0.0);
    FlightSettings.SetFloat("rollWithYaw", 0.15);
    FlightSettings.SetFloat("secondCounter", 0.0);

    FlightSettings.SetFloat("standardModePitchFactor", 3.0);
    FlightSettings.SetFloat("standardModePitchInputAngle", 45.0);
    FlightSettings.SetFloat("standardModeRollFactor", 15.0);
    FlightSettings.SetFloat("standardModeRollInputAngle", 45.0);
    FlightSettings.SetFloat("standardModeSurgeFactor", 15.0);
    FlightSettings.SetFloat("standardModeSwayFactor", 5.0);
    FlightSettings.SetFloat("standardModeYawFactor", 5.0);
    
    FlightSettings.SetFloat("surgeOffset", 0.5);
    FlightSettings.SetFloat("swayWithYaw", 0.5);
    FlightSettings.SetFloat("thrusterFactor", 0.05);
    FlightSettings.SetFloat("yawD", 3.0);
  }
}