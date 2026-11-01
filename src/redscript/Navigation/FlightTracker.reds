// The Autopilot's follower in C++ (src/red4ext/Navigation/Tracker.hpp). FlightModeAutopilot fills the
// params, the inputs and the state each frame, calls Track and Command and copies the state back.
public native class FlightTracker extends IScriptable {
  // params, as the Autopilot's fields of the same name (surge hold: AutopilotSurge's)
  public native let replanOffLine: Float;
  public native let replanDistance: Float;
  public native let cornerSpan: Float;
  public native let cornerOvershoot: Float;
  public native let collisionHorizon: Float;
  public native let raceOffLine: Float;
  public native let lineLead: Float;
  public native let speedGain: Float;
  public native let brakeAccel: Float;
  public native let brakeMax: Float;
  public native let velocityGain: Float;
  public native let accelJerk: Float;
  public native let raceJerk: Float;
  public native let lineSpring: Float;
  public native let lineVelocityGain: Float;
  public native let lineSpan: Float;
  public native let trajectoryCorrectionMax: Float;
  public native let slack: Float;
  public native let dwell: Float;
  public native let fullSurge: Bool;

  // inputs, this frame
  public native let dt: Float;
  public native let speed: Float;
  public native let replanTimer: Float;
  public native let position: Vector4;
  public native let velocity: Vector4;
  public native let forward: Vector4;             // the car's nose, where the route has no direction
  public native let aimOffset: Vector4;           // added to the aim point (the barrel roll's circle)
  public native let overlayVelocity: Vector4;
  public native let target: Vector4;              // Command: Track's aim point, or the hold point
  public native let rawAccel: Vector4;            // Command, not following: what the caller worked out
  public native let rawDesiredVelocity: Vector4;
  public native let rawJerk: Float;
  public native let following: Bool;             // the status is "following" with a route and no shaped manoeuvre
  public native let routeLine: Bool;
  public native let planned: Bool;               // a trajectory is followed
  public native let recovering: Bool;            // the controls are damped after a hit
  public native let requestPending: Bool;
  public native let surgeFree: Bool;             // nothing but the follower forbids the surge hold

  // state, in and out
  public native let segment: Int32;
  public native let closestS: Float;
  public native let offRouteNow: Float;
  public native let remaining: Float;
  public native let aheadSmooth: Float;
  public native let curvatureAhead: Float;
  public native let blockedTimer: Float;
  public native let targetSpeed: Float;
  public native let speedCmd: Float;
  public native let speedRate: Float;
  public native let closestNow: Vector4;
  public native let pathCurvature: Vector4;
  public native let accelCmd: Vector4;
  public native let blockedAhead: Bool;
  public native let emergencyBrake: Bool;
  public native let surgeHeld: Bool;
  public native let surgeChanging: Float;         // s the surge hold has wanted the other way

  // outputs
  public native let lookahead: Vector4;
  public native let desiredVelocity: Vector4;
  public native let accel: Vector4;
  public native let replanLine: Bool;            // off the line: a new trajectory from here
  public native let requestRoute: Bool;          // off the route: ask for a new one
  public native let onLine: Bool;

  // the car's place on the route through the emergency brake; sets target, the state and the replan flags
  public native func Track(route: ref<FlightRoute>, trajectory: ref<FlightTrajectory>, vehicle: ref<FlightVehicleModel>) -> Void;
  // the acceleration that follows the line or the aim point, rate limited, then the surge hold
  public native func Command(route: ref<FlightRoute>, trajectory: ref<FlightTrajectory>) -> Void;
}
