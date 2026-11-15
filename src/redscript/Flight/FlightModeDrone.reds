public class FlightModeDrone extends FlightMode {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.order", "4")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Drone-Mode-Enabled")
  public let enabled: Bool = true;

  // Scales the three turn factors below.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.order", "5")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Drone-Turn-Speed")
  @runtimeProperty("ModSettings.description", "UI-Settings-Drone-Turn-Speed-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.25")
  @runtimeProperty("ModSettings.max", "3.0")
  public let droneTurnSpeed: Float = 1.0;

  // Scales the three thrust factors below.
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.order", "6")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Drone-Thrust")
  @runtimeProperty("ModSettings.description", "UI-Settings-Drone-Thrust-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.25")
  @runtimeProperty("ModSettings.max", "3.0")
  public let droneThrust: Float = 1.0;

  // Developer values (no longer Mod Settings, docs/design/flight-modes.md, Developer values): thrust, m/s2 at full input, and
  // turn, torque per unit of input. Drone Thrust and Drone Turn Speed scale them.
  public let droneModeLiftFactor: Float = 40.0;
  public let droneModePitchFactor: Float = 5.0;
  public let droneModeRollFactor: Float = 8.0;
  public let droneModeSurgeFactor: Float = 15.0;
  public let droneModeYawFactor: Float = 5.0;
  public let droneModeSwayFactor: Float = 15.0;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.usesRightStickInput = true;
    LTBF_RegisterListener(this);
  }

  public func Deinitialize() -> Void {
    LTBF_UnregisterListener(this);
  }

  public func IsEnabled() -> Bool = this.enabled;

  public func Activate() -> Void {
  }

  public func Deactivate() -> Void {
  }

  public func GetDescription() -> String = "Drone";

  // the maths in C++ (FlightModeControl.Drone, Flight::Drone)
  public func Update(timeDelta: Float) -> Void {
    let k = this.Control(timeDelta);
    k.droneModeLiftFactor = this.droneModeLiftFactor * this.droneThrust;
    k.droneModePitchFactor = this.droneModePitchFactor * this.droneTurnSpeed;
    k.droneModeRollFactor = this.droneModeRollFactor * this.droneTurnSpeed;
    k.droneModeSurgeFactor = this.droneModeSurgeFactor * this.droneThrust;
    k.droneModeYawFactor = this.droneModeYawFactor * this.droneTurnSpeed;
    k.droneModeSwayFactor = this.droneModeSwayFactor * this.droneThrust;
    k.Drone(this.component.stats, this.component);
    this.force = k.force;
    this.torque = k.torque;
  }
}