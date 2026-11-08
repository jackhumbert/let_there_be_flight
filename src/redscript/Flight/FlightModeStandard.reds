// Standard, in C++ (Flight/FlightModeStandard.hpp): the mode flight starts in (the Hover & Fly redesign). A mod
// extends it through FlightMode's hooks.
public native class FlightModeStandard extends FlightMode {
  // its state (read only)
  public native let hovering: Float;
  public native let referenceZ: Float;
  public native let holdingAltitude: Bool;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.collisionPenalty = 0.5; // a hit takes half the recovery time off at full impact
  }
}
