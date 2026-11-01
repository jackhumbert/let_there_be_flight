// Flying with the surge held, as a player does (docs/design/autopilot.md): the thrust also holds
// off the air drag (ApplyPhysics takes it along the velocity), and where the trajectory asks for
// all the speed there is (top speed, or speeding up as hard as the car can) the surge is held full
// and the drag sets the speed. The drift that lets the surge push round a bend is the trajectory's
// (Trajectory.cpp). Owned by FlightModeAutopilot.
public class AutopilotSurge extends IScriptable {
  public let dragFeedForward: Bool = true;
  public let fullSurge: Bool = true;
  public let slack: Float = 4.0;       // m/s over the trajectory's speed the surge is still held full
  // s a change of the hold has to last before it is taken (braking for blocked space, a hit or
  // leaving the line let go at once): at a sample's edge the limit and the slack flicker it frame
  // to frame
  public let dwell: Float = 0.3;
  // s the demand the command follows, against audioTime for a deep drop: the raw target saws between
  // full and about half every 1.5 s at the hold's slack boundary (the car rides above the plan
  // speed, the hold lets go, the drag pulls it back under), and the follower's bend corrections
  // wobble it; a level averaged over this sits nearly constant (0.83 to 0.88 against 2.65/s of
  // ramps over 1.0, in game)
  public let audioWindow: Float = 3.0;
  // share of full the demand has to fall below the command before it follows at once, over
  // `audioTime`: braking for blocked space, a hit, off the line. The boundary cycle's dips reach
  // 0.45 below full at most, so only braking crosses this
  public let audioJump: Float = 0.6;

  // s the commanded surge and sway are eased over for the engine sound and the thrusters' look (the
  // thrust itself is not): a player's surge does not jump from reverse to full in a frame
  public let audioTime: Float = 0.25;

  public let held: Bool;               // the surge was held full this frame
  public let changing: Float;         // s the hold has wanted the other way

  // The acceleration the drag takes from the car, to be thrust back: along the velocity, m/s2.
  public func Drag(ap: ref<FlightModeAutopilot>, velocity: Vector4) -> Vector4 {
    let speed = Vector4.Length(velocity);
    if !this.dragFeedForward || speed < 0.5 {
      return new Vector4(0.0, 0.0, 0.0, 0.0);
    }
    return velocity * (ap.Vehicle().Drag(speed) / speed);
  }

  // Whether the surge is held full (held): following a trajectory on its line where the speed is set
  // by the car (its top speed, or speeding up), the car not more than slack ahead of the speed there,
  // each change lasting dwell first. `free`: nothing forbids it (not braking for blocked space, not
  // after a hit, on the line): when it does, it lets go at once.
  public func Update(ap: ref<FlightModeAutopilot>, s: Float, speed: Float, free: Bool, dt: Float) -> Void {
    if !this.fullSurge || !free || !ap.onLine || ap.blockedAhead || ap.emergencyBrake {
      this.held = false;
      this.changing = 0.0;
      return;
    }
    // with no trajectory the limit reads as Top, and the hold must not start
    let valid = IsDefined(ap.trajectory) && ap.trajectory.IsValid();
    // End is never read when not valid: want is false then
    let limit = FlightTrajectoryLimit.End;
    if valid {
      limit = ap.trajectory.Limit(s);
    }
    let want = valid && (Equals(limit, FlightTrajectoryLimit.Top) || Equals(limit, FlightTrajectoryLimit.Accel)) && speed < ap.trajectory.Speed(s) + this.slack;
    if NotEquals(want, this.held) {
      this.changing += dt;
      if this.changing >= this.dwell {
        this.held = want;
        this.changing = 0.0;
      }
    } else {
      this.changing = 0.0;
    }
  }
 
  // `current` eased toward `target`: over `audioWindow` (the hold's boundary cycle and the bend
  // corrections average out, so the engine sound reads a level), except a demand fallen further than
  // `audioJump` below it follows at once over `audioTime`: braking for blocked space, a hit, off the
  // line. Rises always follow the window: thrust coming back is not urgent.
  public func Ease(current: Float, target: Float, dt: Float) -> Float {
    let time = target < current - this.audioJump ? this.audioTime : this.audioWindow;
    return current + (target - current) * ClampF(dt / MaxF(time, 0.001), 0.0, 1.0);
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
