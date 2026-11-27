// The car's limits, worked out in C++ (src/red4ext/Navigation/Vehicle.hpp) so the planner, the
// Autopilot and the offline tools (navplan) use the same numbers. FlightModeAutopilot fills the
// fields (UpdateVehicleModel) and hands it to FlightNavMap.PlanRaceAsync.
public native class FlightVehicleModel extends IScriptable {
  public native let mass: Float;
  public native let airResistance: Float;
  public native let brakingFriction: Float;
  public native let surge: Float;           // m/s2, standardModeSurgeFactor
  public native let surgeShare: Float;      // share of it the car gets in level flight (fitted)
  public native let sway: Float;            // m/s2, standardModeSwayFactor
  public native let lift: Float;            // m/s2 over gravity, standardModeLiftFactor
  public native let gravityFactor: Float;
  public native let brakeLinear: Float;     // FlightSettings.brakeFactorLinear
  public native let drag: Bool;             // FlightSettings.physxFlightPhysics
  public native let drone: Bool;
  public native let maxTilt: Float;         // deg, droneMaxTilt
  public native let levelPitch: Bool;       // spinner style (attitudeStyle 1)
  public native let brakeShare: Float;      // droneBrakeShare
  public native let tiltFloor: Float;       // droneTiltFloor
  public native let driftMax: Float;        // deg, raceDriftMax
  public native let speedCap: Float;        // m/s
  public native let margin: Float;          // raceMargin
  public native let planDecel: Float;       // m/s2, racePlanDecel
  public native let pushOver: Float;        // m/s2, dronePushOver / racePushOver
  // the car's measured shape and thrust (FlightStats.s_dragArea.Y, s_thrusters); they count as far as the
  // developer settings shapeDrag and thrustStrength turn them on
  public native let cdaFront: Float;        // m2
  public native let cdaSide: Float;         // m2
  public native let cdaTop: Float;          // m2
  public native let thrusters: Float;

  // air drag at speed v, m/s2
  public native func Drag(speed: Float) -> Float;
  // what the air takes from the car at that velocity (world), m/s2, to thrust back: with the developer setting
  // shapeDrag the shape's drag on each body axis of the car turned by orientation, else Drag along the velocity
  public native func DragVector(velocity: Vector4, orientation: Quaternion) -> Vector4;
  // surge and sway as the car flies them: scaled by its thrust (developer setting thrustStrength)
  public native func Surge() -> Float;
  public native func Sway() -> Float;
  // drone style: the most horizontal acceleration with `thrusters` m/s2 across the tilted body
  public native func DroneAccel(thrusters: Float) -> Float;
  // where full surge meets the drag, at most speedCap
  public native func TopSpeed() -> Float;
  // sideways acceleration in a level turn
  public native func Lateral() -> Float;
  // speeding up at v: surge less drag
  public native func Accel(speed: Float) -> Float;
  // slowing at v: reverse surge, the brake and drag
  public native func Decel(speed: Float) -> Float;
  // what a speed plan brakes at: margin of Decel, at most planDecel
  public native func PlanDecel(speed: Float) -> Float;
  // drone style: the body's tilt on one axis for a thrust direction `angle` from vertical (Navigation::DroneBodyAngle)
  public native func DroneBodyAngle(angle: Float, thrust: Float, thrusters: Float, share: Float) -> Float;
  public native func Describe() -> String;
}
