// Flying with the surge held, as a player does (docs/design/autopilot.md): the thrust also holds
// off the air drag (ApplyPhysics takes it along the velocity), and where the trajectory asks for
// all the speed there is (top speed, or speeding up as hard as the car can) the surge is held full
// and the drag sets the speed. The drift that lets the surge push round a bend is the trajectory's
// (Trajectory.cpp). Owned by FlightModeAutopilot.
public class AutopilotSurge extends IScriptable {
  public let dragFeedForward: Bool = true;
  public let fullSurge: Bool = true;
  public let slack: Float = 4.0;       // m/s over the trajectory's speed the surge is still held full

  public let held: Bool;               // the surge was held full this frame

  // The acceleration the drag takes from the car, to be thrust back: along the velocity, m/s2.
  public func Drag(ap: ref<FlightModeAutopilot>, velocity: Vector4) -> Vector4 {
    let speed = Vector4.Length(velocity);
    if !this.dragFeedForward || speed < 0.5 {
      return new Vector4(0.0, 0.0, 0.0, 0.0);
    }
    return velocity * (ap.Vehicle().Drag(speed) / speed);
  }

  // Whether the surge is held full: following a trajectory on its line where the speed is set by
  // the car (its top speed, or speeding up), the car not more than slack ahead of the speed there.
  public func Hold(ap: ref<FlightModeAutopilot>, s: Float, speed: Float, following: Bool) -> Bool {
    if !this.fullSurge || !following || !ap.onLine || ap.blockedAhead || ap.emergencyBrake {
      return false;
    }
    let limit = ap.trajectory.Limit(s);
    if !Equals(limit, "top") && !Equals(limit, "accel") {
      return false;
    }
    return speed < ap.trajectory.Speed(s) + this.slack;
  }

  // The thrust in the car's frame with more along the path (`path`, unit, in the car's frame) until
  // the surge is full (`surge` m/s2): the speed is left to the drag, the turn to the rest.
  public func Full(local: Vector4, path: Vector4, surge: Float) -> Vector4 {
    if path.Y < 0.2 || local.Y >= surge {
      return local;
    }
    return local + path * ((surge - local.Y) / path.Y);
  }
}
