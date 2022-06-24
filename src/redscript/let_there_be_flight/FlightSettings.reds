public native class FlightSettings extends ScriptableSystem {
  public native static func GetFloat(name: CName) -> Float;
  public native static func SetFloat(name: CName, value: Float) -> Void;

  private func OnAttach() -> Void {
    FlightLog.Info("[FlightSettings] OnAttach");
    FlightSettings.SetFloat(n"airResistance", 0.001);
    FlightSettings.SetFloat(n"angularBrakeFactor", 10.0);
    FlightSettings.SetFloat(n"angularDampFactor", 3.0);
    FlightSettings.SetFloat(n"automaticModeAutoBrakingFactor", 200.0);
    FlightSettings.SetFloat(n"automaticModeYawDirectionality", 300.0);
    FlightSettings.SetFloat(n"brakeFactor", 1.2);
    FlightSettings.SetFloat(n"brakeOffset", 0.0);
    FlightSettings.SetFloat(n"collisionRecoveryDelay", 0.8);
    FlightSettings.SetFloat(n"collisionRecoveryDuration", 0.8);
    FlightSettings.SetFloat(n"defaultHoverHeight", 3.50);
    FlightSettings.SetFloat(n"distance", 0.0);
    FlightSettings.SetFloat(n"distanceEase", 0.1);
    FlightSettings.SetFloat(n"fwtfCorrection", 0.0);
    FlightSettings.SetFloat(n"hoverClamp", 10.0);
    FlightSettings.SetFloat(n"hoverFactor", 40.0);
    FlightSettings.SetFloat(n"hoverLiftFactor", 8.0);
    FlightSettings.SetFloat(n"isFlightUIActive", 1.0);
    FlightSettings.SetFloat(n"liftFactor", 8.0);
    FlightSettings.SetFloat(n"liftFactorDrone", 40.0);
    FlightSettings.SetFloat(n"lookAheadMax", 10.0);
    FlightSettings.SetFloat(n"lookAheadMin", 1.0);
    FlightSettings.SetFloat(n"maxHoverHeight", 7.0);
    FlightSettings.SetFloat(n"minHoverHeight", 1.0);
    FlightSettings.SetFloat(n"normalEase", 0.3);
    FlightSettings.SetFloat(n"pitchAeroCorrectionFactor", 0.5);
    FlightSettings.SetFloat(n"pitchCorrectionFactor", 3.0);
    FlightSettings.SetFloat(n"pitchDirectionalityFactor", 80.0);
    FlightSettings.SetFloat(n"pitchFactorDrone", 5.0);
    FlightSettings.SetFloat(n"pitchWithLift", 0.0);
    FlightSettings.SetFloat(n"pitchWithSurge", 0.0);
    FlightSettings.SetFloat(n"referenceZ", 0.0);
    FlightSettings.SetFloat(n"rollCorrectionFactor", 15.0);
    FlightSettings.SetFloat(n"rollFactorDrone", 12.0);
    FlightSettings.SetFloat(n"rollWithYaw", 0.15);
    FlightSettings.SetFloat(n"secondCounter", 0.0);
    FlightSettings.SetFloat(n"surgeFactor", 15.0);
    FlightSettings.SetFloat(n"surgeOffset", 0.5);
    FlightSettings.SetFloat(n"swayFactor", 5.0);
    FlightSettings.SetFloat(n"swayWithYaw", 0.5);
    FlightSettings.SetFloat(n"thrusterFactor", 0.05);
    FlightSettings.SetFloat(n"yawCorrectionFactor", 0.1);
    FlightSettings.SetFloat(n"yawD", 3.0);
    FlightSettings.SetFloat(n"yawDirectionalityFactor", 50.0);
    FlightSettings.SetFloat(n"yawFactor", 5.0);
    FlightSettings.SetFloat(n"yawFactorDrone", 5.0);
  }
}