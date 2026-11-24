// The flight route shown to the player, for other mods to draw (src/red4ext/Navigation/FlightGuidance.hpp,
// docs/design/guidance.md). While the player's vehicle flies with something tracked: the Autopilot's own
// route while it is engaged, else a route planned for flying by hand. Another mod can test for LTBF with
// @if(ModuleExists("LetThereBeFlight")) around the code that calls this.
public native class FlightGuidance extends IScriptable {
  // the player's vehicle is flying (flight on, not only mounted)
  public native static func IsFlying() -> Bool;
  // there is a flight route to show: flying, something tracked, and a route planned to it
  public native static func HasRoute() -> Bool;
  // the route is the Autopilot's own (it is engaged); false when planned for flying by hand, or none
  public native static func IsAutopilotRoute() -> Bool;
  // the route's world points (W 1), car end first: the point on the line nearest the car, then the line's
  // points after it, about 5 m apart, to the destination end; empty when HasRoute is false
  public native static func GetRoute() -> array<Vector4>;
  // changes whenever the route is replaced (a new plan, a rejoin, the Autopilot's replan), not as the car
  // moves along it
  public native static func GetRouteRevision() -> Int32;
  // m from the car to the route; 0 without a route
  public native static func GetDistanceToRoute() -> Float;
}
