// Hover & Fly as releases up to 0.3.20 flew it, in C++ (Flight/FlightModeHoverFlyClassic.hpp): the old attitude and
// lift under a hover band. Second in the mode list, after Standard. A mod extends it through FlightMode's hooks.
public native class FlightModeHoverFlyClassic extends FlightMode {
  // its state (read only)
  public native let hovering: Float;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.collisionPenalty = 0.5; // as the script Hover & Fly had it: a hit takes half the recovery time off at full impact
  }
}
