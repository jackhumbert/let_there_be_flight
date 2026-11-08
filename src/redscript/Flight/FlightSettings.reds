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

// How flying feels, as one choice over the ten handling values (FlightSettings.handling; the sets are
// Flight::HandlingFactors, Flight/Handling.hpp, whose Flight::Handling has the same numbers).
enum FlightHandling {
  Loose = 0,
  Default = 1,
  Tight = 2
}

public native class FlightSettings extends IScriptable {
  public native static func GetInstance() -> ref<FlightSettings>;
  public native static func GetFloat(name: String) -> Float;
  public native static func GetFloatN(name: CName) -> Float; // the same value, looked up by name: for per-frame callers
  public native static func SetFloat(name: String, value: Float) -> Float;
  public native static func GetBool(name: String) -> Bool;
  public native static func SetBool(name: String, value: Bool) -> Bool;
  public native static func GetVector3(name: String) -> Vector3;
  public native static func SetVector3(name: String, x: Float, y: Float, z: Float) -> Vector3;
  public native static func DebugBreak() -> Void;

  // The player's settings (Mod Settings), in the menu's order: General, Controls, Flight Modes (with the C++ modes'
  // and FlightModeDrone's), Crashes, Camera, Landing Pads and Summons, HUD and Sound, Key Bindings
  // (docs/design/settings-audit.md). The fields after them are developer values: no Mod Settings attributes, so a
  // value a player once saved is no longer read and the code default holds.

  // General

  // Restricts flight to a subset of vehicles without any redscript. See the readme's
  // Configuration section for the Whitelist Only yaml.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.category.order", "1")
  @runtimeProperty("ModSettings.order", "0")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Flight-Availability")
  @runtimeProperty("ModSettings.description", "UI-Settings-Flight-Availability-Description")
  @runtimeProperty("ModSettings.displayValues.AllVehicles", "UI-Settings-Flight-Availability-All-Vehicles")
  @runtimeProperty("ModSettings.displayValues.OwnedVehicles", "UI-Settings-Flight-Availability-Owned-Vehicles")
  @runtimeProperty("ModSettings.displayValues.PlayerVehicleOnly", "UI-Settings-Flight-Availability-Player-Vehicle-Only")
  @runtimeProperty("ModSettings.displayValues.WhitelistOnly", "UI-Settings-Flight-Availability-Whitelist-Only")
  public let flightAvailability: FlightAvailability = FlightAvailability.AllVehicles;

  // Turns flight on when V gets into a vehicle that is autoActivationHeight or more above the ground.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.category.order", "1")
  @runtimeProperty("ModSettings.order", "1")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Enable-Auto-Activation")
  @runtimeProperty("ModSettings.description", "UI-Settings-Enable-Auto-Activation-Description")
  public let autoActivationEnabled: Bool = false;

  // Controls

  // Flips the sign of the Flight_Lift axis for every device, so it covers the gamepad
  // stick and the keyboard up/down buttons without editing the input xml.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.category.order", "2")
  @runtimeProperty("ModSettings.order", "0")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Invert-Lift")
  @runtimeProperty("ModSettings.description", "UI-Settings-Invert-Lift-Description")
  public let invertLift: Bool = false;

  // The ten handling values below as one choice (Flight/Handling.hpp): Default is their values here, Loose and Tight
  // the sets there. Applied where they are read (the force tail, the Autopilot), never written into the fields.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.category.order", "2")
  @runtimeProperty("ModSettings.order", "1")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Handling")
  @runtimeProperty("ModSettings.description", "UI-Settings-Handling-Description")
  @runtimeProperty("ModSettings.displayValues.Loose", "UI-Settings-Handling-Loose")
  @runtimeProperty("ModSettings.displayValues.Default", "UI-Settings-Handling-Default")
  @runtimeProperty("ModSettings.displayValues.Tight", "UI-Settings-Handling-Tight")
  public let handling: FlightHandling = FlightHandling.Default;

  // Scales both brakes (brakeFactorLinear and brakeFactorAngular below): LinearBrake() and AngularBrake().
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.category.order", "2")
  @runtimeProperty("ModSettings.order", "2")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Brake-Strength")
  @runtimeProperty("ModSettings.description", "UI-Settings-Brake-Strength-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.5")
  @runtimeProperty("ModSettings.max", "3.0")
  public let brakeStrength: Float = 1.0;

  // Seconds the gamepad flight toggle has to be held; 0 is a tap. The keyboard toggle stays a tap. Vanilla autodrive is
  // a 0.5 s hold on the same button (L3). It replaced gamepadToggleRequiresHold and gamepadToggleHoldTime.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Control-Settings")
  @runtimeProperty("ModSettings.category.order", "2")
  @runtimeProperty("ModSettings.order", "3")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Gamepad-Toggle-Hold")
  @runtimeProperty("ModSettings.description", "UI-Settings-Gamepad-Toggle-Hold-Description")
  @runtimeProperty("ModSettings.step", "0.1")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "3.0")
  public let gamepadToggleHold: Float = 0.0;

  // Flight Modes (the rest of the category is the C++ modes' and FlightModeDrone's)

  // Hover, Fly and the anti-gravity drone in the mode switch (their IsEnabled).
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.order", "6")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Extra-Flight-Modes")
  @runtimeProperty("ModSettings.description", "UI-Settings-Extra-Flight-Modes-Description")
  public let extraModesEnabled: Bool = false;

  // Camera

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.category.order", "5")
  @runtimeProperty("ModSettings.order", "0")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-FPV-Camera-Pitch-Offset")
  @runtimeProperty("ModSettings.description", "UI-Settings-FPV-Camera-Pitch-Offset-Description")
  @runtimeProperty("ModSettings.step", "5.0")
  @runtimeProperty("ModSettings.min", "-60.0")
  @runtimeProperty("ModSettings.max", "60.0")
  public let fpvCameraPitchOffset: Float = 0.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Camera-Settings")
  @runtimeProperty("ModSettings.category.order", "5")
  @runtimeProperty("ModSettings.order", "1")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-TPP-Camera-Pitch-Offset")
  @runtimeProperty("ModSettings.description", "UI-Settings-TPP-Camera-Pitch-Offset-Description")
  @runtimeProperty("ModSettings.step", "1.0")
  @runtimeProperty("ModSettings.min", "-60.0")
  @runtimeProperty("ModSettings.max", "60.0")
  public let tppCameraPitchOffset: Float = 20.0;

  // Developer values (no longer Mod Settings)

  // Read by the plugin (FlightComponent::OnUpdate) from the class default each frame. On: a
  // flying vehicle is simulated by PhysX alone (gravity, ground contacts and surface friction
  // solved together), see src/red4ext/Physics/ChassisContacts.cpp. Off: the game's own
  // vehicle integrator moves it and PhysX only reports contacts once per frame.
  public let physxFlightPhysics: Bool = true;

  // Read by the plugin each frame. Extra friction against the ground for flying vehicles
  // (PhysX Flight Physics only), as a Coulomb coefficient on top of PhysX's own.
  public let groundFriction: Float = 0.5;

  // How far above the ground (m) a vehicle has to be for autoActivationEnabled to turn flight on.
  public let autoActivationHeight: Float = 3.0;

  // The brakes before Brake Strength: read through LinearBrake() and AngularBrake().
  public let brakeFactorLinear: Float = 1.0;
  public let brakeFactorAngular: Float = 1.0;

  // Skips the cleanup that stops flight's updates when flight is switched off.
  public let generalApplyFlightPhysicsWhenDeactivated: Bool = false;

  // The handling values: Handling's Default set (Loose's and Tight's are Flight::HandlingFactors in C++).
  public let generalDampFactorLinear: Float = 0.0;
  public let generalDampFactorAngular: Float = 2.0;
  public let generalDampFactorAngularMax: Float = 3.5;
  public let generalPitchAeroFactor: Float = 0.0;
  // How much the vehicle is rotated (yaw) towards its velocity
  public let generalYawAeroFactor: Float = 0.0;
  public let generalPitchDirectionalityFactor: Float = 15.0;
  // How much the vehicle's yaw affects its velocity
  public let generalYawDirectionalityFactor: Float = 5.0;
  // Extra multiplier on the yaw (Z) part of the angular damping, applied after the
  // "no damping while the stick is deflected" term and before the Angular Damp Max clamp.
  public let generalYawDampFactor: Float = 1.0;
  // Adds yaw input to the yaw directionality term, so turning redirects more of the
  // existing velocity into the new heading.
  public let generalSwayWithYaw: Float = 0.0;
  // How much yaw input banks the vehicle into the turn (the Autopilot's manual flight, Flight::AttitudeLift).
  public let generalRollWithYaw: Float = 0.15;

  // The chase camera while flying (the C++ hooks TPPCameraComponent_SetVehicleData and _CalculateLookAtPosition).
  public let drivingDirectionCompensationAngleSmooth: Float = 120.0;
  public let drivingDirectionCompensationSpeedCoef: Float = 0.1;
  public let tppCameraCenterOnMass: Bool = true;

  // The brakes as the modes and the force tail apply them: the developer factor times Brake Strength.
  public final func LinearBrake() -> Float = this.brakeFactorLinear * this.brakeStrength;
  public final func AngularBrake() -> Float = this.brakeFactorAngular * this.brakeStrength;

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
