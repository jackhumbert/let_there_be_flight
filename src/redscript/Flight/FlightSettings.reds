public static func GetFlightSettings() -> ref<FlightSettings> {
  return FlightSettings.GetInstance();
}

// Which vehicles the player is allowed to fly. Read live by VehicleObject.IsFlightAvailable(),
// which is the single gate for activation, input hints, auto activation and the flight
// quickhacks. It never changes which vehicles get a FlightComponent, so NPC and traffic
// vehicles keep theirs in every mode.
enum FlightAvailability {
  AllVehicles = 0,
  OwnedVehicles = 1,
  PlayerVehicleOnly = 2,
  WhitelistOnly = 3
}

public native class FlightSettings extends IScriptable {
  public native static func GetInstance() -> ref<FlightSettings>;
  public native static func GetFloat(name: String) -> Float;
  public native static func SetFloat(name: String, value: Float) -> Float;
  public native static func GetBool(name: String) -> Bool;
  public native static func SetBool(name: String, value: Bool) -> Bool;
  public native static func GetVector3(name: String) -> Vector3;
  public native static func SetVector3(name: String, x: Float, y: Float, z: Float) -> Vector3;
  public native static func DebugBreak() -> Void;

  // Restricts flight to a subset of vehicles without any redscript. See the readme's
  // Configuration section for the Whitelist Only yaml.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Flight-Availability")
  @runtimeProperty("ModSettings.description", "UI-Settings-Flight-Availability-Description")
  @runtimeProperty("ModSettings.displayValues.AllVehicles", "UI-Settings-Flight-Availability-All-Vehicles")
  @runtimeProperty("ModSettings.displayValues.OwnedVehicles", "UI-Settings-Flight-Availability-Owned-Vehicles")
  @runtimeProperty("ModSettings.displayValues.PlayerVehicleOnly", "UI-Settings-Flight-Availability-Player-Vehicle-Only")
  @runtimeProperty("ModSettings.displayValues.WhitelistOnly", "UI-Settings-Flight-Availability-Whitelist-Only")
  public let flightAvailability: FlightAvailability = FlightAvailability.AllVehicles;

  // Read by the plugin (FlightComponent::OnUpdate) from the class default each frame. On: a
  // flying vehicle is simulated by PhysX alone (gravity, ground contacts and surface friction
  // solved together), see src/red4ext/Physics/ChassisContacts.cpp. Off: the game's own
  // vehicle integrator moves it and PhysX only reports contacts once per frame.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-PhysX-Flight-Physics")
  @runtimeProperty("ModSettings.description", "UI-Settings-PhysX-Flight-Physics-Description")
  public let physxFlightPhysics: Bool = false;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Enable-Auto-Activation")
  @runtimeProperty("ModSettings.description", "UI-Settings-Enable-Auto-Activation-Description")
  public let autoActivationEnabled: Bool = false;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Auto-Activation-Height")
  @runtimeProperty("ModSettings.description", "UI-Settings-Auto-Activation-Height-Description")
  @runtimeProperty("ModSettings.step", "0.5")
  @runtimeProperty("ModSettings.max", "10.0")
  public let autoActivationHeight: Float = 3.0;

  // Flight Control Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Linear-Brake-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Linear-Brake-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "5.0")
  public let brakeFactorLinear: Float = 1.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Angular-Brake-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Angular-Brake-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "5.0")
  public let brakeFactorAngular: Float = 1.0;

  // Flips the sign of the Flight_Lift axis for every device, so it covers the gamepad
  // stick and the keyboard up/down buttons without editing the input xml.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Invert-Lift")
  @runtimeProperty("ModSettings.description", "UI-Settings-Invert-Lift-Description")
  public let invertLift: Bool = false;

  // When on, the gamepad flight toggle has to be held for gamepadToggleHoldTime seconds
  // instead of tapped. The keyboard toggle stays a tap either way.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Gamepad-Toggle-Requires-Hold")
  @runtimeProperty("ModSettings.description", "UI-Settings-Gamepad-Toggle-Requires-Hold-Description")
  public let gamepadToggleRequiresHold: Bool = false;

  // Seconds the gamepad toggle has to be held when gamepadToggleRequiresHold is on. Vanilla
  // autodrive is a 0.5 s hold on the same button, so the default stays clear of it.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Gamepad-Toggle-Hold-Time")
  @runtimeProperty("ModSettings.description", "UI-Settings-Gamepad-Toggle-Hold-Time-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.2")
  @runtimeProperty("ModSettings.max", "3.0")
  public let gamepadToggleHoldTime: Float = 1.0;

  // Flight Physics Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Apply-Flight-Physics-When-Deactivated")
  @runtimeProperty("ModSettings.description", "UI-Settings-Apply-Flight-Physics-When-Deactivated-Description")
  public let generalApplyFlightPhysicsWhenDeactivated: Bool = false;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Linear-Damp-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Linear-Damp-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "20.0")
  public let generalDampFactorLinear: Float = 0.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Angular-Damp-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Angular-Damp-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "20.0")
  public let generalDampFactorAngular: Float = 2.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Angular-Damp-Max")
  @runtimeProperty("ModSettings.description", "UI-Settings-Angular-Damp-Max-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "50.0")
  public let generalDampFactorAngularMax: Float = 3.5;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pitch-Aero-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Pitch-Aero-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalPitchAeroFactor: Float = 0.0;

  // How much the vehicle is rotated (yaw) towards its velocity
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Aero-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Yaw-Aero-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalYawAeroFactor: Float = 0.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pitch-Directionality-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Pitch-Directionality-Factor-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let generalPitchDirectionalityFactor: Float = 15.0;

  // How much the vehicle's yaw affects its velocity
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Directionality-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Yaw-Directionality-Factor-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let generalYawDirectionalityFactor: Float = 5.0;

  // Extra multiplier on the yaw (Z) part of the angular damping, applied after the
  // "no damping while the stick is deflected" term and before the Angular Damp Max clamp.
  // 1.0 is the shipped behaviour.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Yaw-Damp-Factor")
  @runtimeProperty("ModSettings.description", "UI-Settings-Yaw-Damp-Factor-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "5.0")
  public let generalYawDampFactor: Float = 1.0;

  // Adds yaw input to the yaw directionality term, so turning redirects more of the
  // existing velocity into the new heading. 0.0 is the shipped behaviour.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Sway-With-Yaw")
  @runtimeProperty("ModSettings.description", "UI-Settings-Sway-With-Yaw-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "2.0")
  public let generalSwayWithYaw: Float = 0.0;

  // How much yaw input banks the vehicle into the turn (Standard and Automatic modes).
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Physics-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Roll-With-Yaw")
  @runtimeProperty("ModSettings.description", "UI-Settings-Roll-With-Yaw-Description")
  @runtimeProperty("ModSettings.step", "0.01")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let generalRollWithYaw: Float = 0.15;

  // Flight Camera Settings

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Driving-Direction-Compensation-Angle-Smoothing")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "180.0")
  public let drivingDirectionCompensationAngleSmooth: Float = 120.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Driving-Direction-Compensation-Speed-Coef")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "1.0")
  public let drivingDirectionCompensationSpeedCoef: Float = 0.1;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-FPV-Camera-Pitch-Offset")
  @runtimeProperty("ModSettings.description", "UI-Settings-FPV-Camera-Pitch-Offset-Description")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "-60.0")
  @runtimeProperty("ModSettings.max", "60.0")
  public let fpvCameraPitchOffset: Float = 0.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-TPP-Camera-Pitch-Offset")
  @runtimeProperty("ModSettings.description", "UI-Settings-TPP-Camera-Pitch-Offset-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "-60.0")
  @runtimeProperty("ModSettings.max", "60.0")
  public let tppCameraPitchOffset: Float = 20.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Enable-Center-On-Mass")
  @runtimeProperty("ModSettings.description", "UI-Settings-Enable-Center-On-Mass-Description")
  public let tppCameraCenterOnMass: Bool = true;

  // public cb func OnModSettingsUpdate(variable: CName, value: Variant) {
  //   switch (variable) {
  //     case n"autoActivationHeight":
  //       this.autoActivationHeight = FromVariant<Float>(value);
  //       break;
  //   }
  // }

  private func OnAttach() -> Void {
    FlightLog.Info("[FlightSettings] OnAttach");
    LTBF_RegisterListener(this);

    FlightSettings.SetVector3("inputPitchPID", 1.0, 0.2, 0.05);
    FlightSettings.SetVector3("inputRollPID", 1.0, 0.5, 0.0);

    FlightSettings.SetVector3("aeroYawPID", 1.0, 0.01, 1.0);
    FlightSettings.SetVector3("aeroPitchPID", 1.0, 0.01, 1.0);

    FlightSettings.SetVector3("hoverModePID", 1.0, 0.005, 0.5);

    FlightSettings.SetFloat("brakeOffset", 0.0);
    FlightSettings.SetFloat("collisionRecoveryDelay", 0.8);
    FlightSettings.SetFloat("collisionRecoveryDuration", 0.8);
    FlightSettings.SetFloat("defaultHoverHeight", 3.50);
    FlightSettings.SetFloat("distance", 0.0);
    FlightSettings.SetFloat("distanceEase", 0.1);

    // FlightSettings.SetFloat("flyModeLiftFactor", 20.0);

    FlightSettings.SetVector3("FPVCameraOffset", 0.0, 0.0, -0.5);

    FlightSettings.SetFloat("fwtfCorrection", 0.0);
    FlightSettings.SetFloat("hoverClamp", 10.0);
    // FlightSettings.SetFloat("hoverModeLiftFactor", 8.0);
    FlightSettings.SetFloat("isFlightUIActive", 1.0);
    FlightSettings.SetFloat("lockFPPCameraForDrone", 1.0);
    FlightSettings.SetFloat("lookAheadMax", 10.0);
    FlightSettings.SetFloat("lookAheadMin", 1.0);
    FlightSettings.SetFloat("hoverModeMaxHoverHeight", 7.0);
    FlightSettings.SetFloat("hoverModeMinHoverHeight", 1.0);
    FlightSettings.SetFloat("normalEase", 0.3);
    FlightSettings.SetFloat("referenceZ", 0.0);
    FlightSettings.SetFloat("secondCounter", 0.0);

    FlightSettings.SetFloat("surgeOffset", 0.5);

    // angular damping stability cap, see FlightMode.CapDamping
    FlightSettings.SetFloat("dampStabilityK", 0.19);
    FlightSettings.SetFloat("dampMaxPerFrame", 1.0);

    FlightSettings.SetFloat("pitchWithLift", 0.0);
    FlightSettings.SetFloat("pitchWithSurge", 0.0);

    FlightSettings.SetFloat("yawD", 3.0);

    FlightSettings.SetBool("inTPPDriverCombat", false);
  }
}
