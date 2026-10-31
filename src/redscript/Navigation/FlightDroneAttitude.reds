// The drone attitude in C++ (src/red4ext/Navigation/DroneAttitude.hpp). FlightModeAutopilot fills the
// params, the inputs and the state each frame, calls Step and copies the state back.
public native class FlightDroneAttitude extends IScriptable {
  // params, as the Autopilot's fields of the same name
  public native let droneBodyShare: Float;
  public native let droneTiltTime: Float;
  public native let attitudeLead: Float;
  public native let droneYawLead: Float;
  public native let lineSpan: Float;
  public native let droneYawFrequency: Float;
  public native let droneYawRateMax: Float;
  public native let droneYawAccelMax: Float;
  public native let racePitchFrequency: Float;
  public native let racePitchRateMax: Float;
  public native let racePitchAccelMax: Float;
  public native let raceRollFrequency: Float;
  public native let raceRollRateMax: Float;
  public native let raceRollAccelMax: Float;
  public native let trajectoryAttitudeLead: Float;
  public native let droneTakeoffTilt: Float;
  public native let droneTakeoffIn: Float;
  public native let droneTakeoffFull: Float;
  public native let droneTakeoffOut: Float;
  public native let droneTakeoffLength: Float;
  public native let dragFeedForward: Bool;   // AutopilotSurge.dragFeedForward

  // inputs, this frame
  public native let dt: Float;
  public native let blend: Float;            // how much the nose follows the path rather than the hover heading
  public native let speed: Float;
  public native let closestS: Float;
  public native let accel: Vector4;          // the commanded acceleration
  public native let velocity: Vector4;
  public native let forward: Vector4;        // the car's nose, where the route has no direction
  public native let live: Bool;              // false: the state is set to the target (the first frame)
  public native let onLine: Bool;
  public native let following: Bool;         // a trajectory is followed and the status is "following"
  public native let descending: Bool;        // the status is "descending" or "landed"

  // state, in and out
  public native let droneYaw: Float;
  public native let droneYawRate: Float;
  public native let dronePitch: Float;
  public native let dronePitchRate: Float;
  public native let droneRoll: Float;
  public native let droneRollRate: Float;
  public native let attAccel: Vector4;
  public native let attRef: Vector4;
  public native let headingRef: Vector4;

  // outputs
  public native let fwd: Vector4;            // the nose
  public native let up: Vector4;             // the top

  // the heading the nose is wanted at, before the landing turn's choice
  public native func HeadingYaw(route: ref<FlightRoute>, trajectory: ref<FlightTrajectory>) -> Float;
  // eases the state towards wantedYaw and the tilt the thrust needs; sets fwd and up
  public native func Step(wantedYaw: Float, route: ref<FlightRoute>, trajectory: ref<FlightTrajectory>, vehicle: ref<FlightVehicleModel>) -> Void;
}
