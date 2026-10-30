// A trajectory along a route (C++, src/red4ext/Navigation/Trajectory.hpp): speed, time, velocity and
// acceleration at every 2 m, inside the car's limits. The Autopilot builds one for its route and
// flies the speeds and tilts from it (docs/design/trajectory.md).
public native class FlightTrajectory extends IScriptable {
  // the trajectory along route for the car in vehicle, from startSpeed m/s, stopping at its end when stop
  public native static func Build(route: array<Vector4>, vehicle: ref<FlightVehicleModel>, startSpeed: Float, stop: Bool) -> ref<FlightTrajectory>;
  public native func IsValid() -> Bool;
  public native func Length() -> Float;
  public native func Duration() -> Float;
  // at distance s along the route, interpolated
  public native func Speed(s: Float) -> Float;
  public native func Along(s: Float) -> Float;
  public native func Time(s: Float) -> Float;
  public native func Position(s: Float) -> Vector4;
  public native func Velocity(s: Float) -> Vector4;
  public native func Acceleration(s: Float) -> Vector4;
  // the drone's planned nose yaw, body pitch (nose down speeding up) and roll (into the turn), rad
  public native func Yaw(s: Float) -> Float;
  public native func Pitch(s: Float) -> Float;
  public native func Roll(s: Float) -> Float;
  // what sets the speed at s
  public native func Limit(s: Float) -> String;
  // samples outside the car's limits, "; " between them (empty when none)
  public native func Violations() -> String;
}
