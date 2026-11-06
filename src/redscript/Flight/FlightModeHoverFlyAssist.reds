// Hover & Fly, in C++ (Flight/FlightModeHoverFlyAssist.hpp); a mod extends it through FlightMode's hooks.
public native class FlightModeHoverFlyAssist extends FlightMode {
  // its state, as the script Hover & Fly named it (read only)
  public native let hovering: Float;
  public native let referenceZ: Float;
  public native let holdingAltitude: Bool;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.collisionPenalty = 0.5; // as the standard modes: a hit takes half the recovery time off at full impact
  }
}
