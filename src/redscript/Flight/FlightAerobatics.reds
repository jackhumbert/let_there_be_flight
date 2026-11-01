// Aerobatic flying for the Autopilot (FlightModeAutopilot.aerobatics): the car's attitude is a
// full 3D target (forward and up) instead of world up plus a bank and a yaw rate, and manoeuvres
// that turn the car past 90 degrees are flown as time-parameterised 3D shapes.
//
// The nav map (tools/navmap_bake) is baked for a car cylinder of 3 m radius and 1.5 m half height.
// Any yaw or roll stays inside it (rolled on its side a car is about 2 m tall), so rolls fit
// wherever the map is free; pitched past about 35 degrees the car is up to 2.4 m tall, so the
// pitch manoeuvres (loop, Immelmann, split-S) keep pitchMargin of extra room above and below.

public class FlightAttitude {
  // Rotation vector (axis * angle, rad) that turns the frame (r, f, u) into (tr, tf, tu), in the
  // first frame's coordinates (X right, Y forward, Z up, as the body's angular velocity). The
  // shortest way round; at exactly 180 degrees the axis is arbitrary.
  public static func RotationError(r: Vector4, f: Vector4, u: Vector4, tr: Vector4, tf: Vector4, tu: Vector4) -> Vector4 {
    // m[row][col] = dot(axis row of the first frame, axis col of the second): the second frame's
    // axes in the first frame's coordinates, i.e. the rotation matrix between them
    let m00 = Vector4.Dot(r, tr);
    let m01 = Vector4.Dot(r, tf);
    let m02 = Vector4.Dot(r, tu);
    let m10 = Vector4.Dot(f, tr);
    let m11 = Vector4.Dot(f, tf);
    let m12 = Vector4.Dot(f, tu);
    let m20 = Vector4.Dot(u, tr);
    let m21 = Vector4.Dot(u, tf);
    let m22 = Vector4.Dot(u, tu);
    // to a quaternion (Shepperd: divide by the largest of the four candidates)
    let w: Float;
    let x: Float;
    let y: Float;
    let z: Float;
    let trace = m00 + m11 + m22;
    if trace > 0.0 {
      let s = SqrtF(trace + 1.0) * 2.0;
      w = 0.25 * s;
      x = (m21 - m12) / s;
      y = (m02 - m20) / s;
      z = (m10 - m01) / s;
    } else if m00 > m11 && m00 > m22 {
      let s = SqrtF(MaxF(1.0 + m00 - m11 - m22, 0.000001)) * 2.0;
      w = (m21 - m12) / s;
      x = 0.25 * s;
      y = (m01 + m10) / s;
      z = (m02 + m20) / s;
    } else if m11 > m22 {
      let s = SqrtF(MaxF(1.0 + m11 - m00 - m22, 0.000001)) * 2.0;
      w = (m02 - m20) / s;
      x = (m01 + m10) / s;
      y = 0.25 * s;
      z = (m12 + m21) / s;
    } else {
      let s = SqrtF(MaxF(1.0 + m22 - m00 - m11, 0.000001)) * 2.0;
      w = (m10 - m01) / s;
      x = (m02 + m20) / s;
      y = (m12 + m21) / s;
      z = 0.25 * s;
    }
    if w < 0.0 {
      w = -w;
      x = -x;
      y = -y;
      z = -z;
    }
    let sinHalf = SqrtF(x * x + y * y + z * z);
    if sinHalf < 0.000001 {
      return new Vector4(0.0, 0.0, 0.0, 0.0);
    }
    let angle = 2.0 * AtanF(sinHalf, w);
    return new Vector4(x * angle / sinHalf, y * angle / sinHalf, z * angle / sinHalf, 0.0);
  }

  // u made perpendicular to the unit vector f and normalised; zero length when it is along f.
  public static func Across(u: Vector4, f: Vector4) -> Vector4 {
    return u - f * Vector4.Dot(u, f);
  }
}

public enum FlightManeuverKind {
  None = 0,
  AileronRoll = 1, // 360 degrees about the nose, on the route
  BarrelRoll = 2,  // a helix around the route, rolling with it
  Loop = 3,        // a vertical circle back to where it started
  Immelmann = 4,   // half loop up, half roll upright: reverses the heading, gains 2 r
  SplitS = 5       // half roll inverted, half loop down: reverses the heading, loses 2 r
}

public class FlightManeuver {
  public let kind: FlightManeuverKind;
  public let side: Float;        // +1 / -1: which way it rolls
  public let time: Float;
  public let duration: Float;
  public let origin: Vector4;    // car position at the start
  public let dir: Vector4;       // horizontal unit direction at the start
  public let right: Vector4;     // horizontal, dir x up
  public let speed: Float;       // m/s flown through the shape
  public let radius: Float;      // m: loop radius, barrel roll radius
  public let rollTime: Float;    // s of a half roll (Immelmann, split-S)
  public let shape: ref<FlightManeuverShape>;  // the geometry in C++ when created with useNative (then the script bodies below are not run)

  // useNative: the shape is sized and sampled in C++ (FlightManeuverShape); false: the script below
  public static func Create(kind: FlightManeuverKind, side: Float, position: Vector4, velocity: Vector4, accel: Float, rollTime: Float, useNative: Bool) -> ref<FlightManeuver> {
    let self = new FlightManeuver();
    self.kind = kind;
    if useNative {
      self.shape = new FlightManeuverShape();
      self.shape.Create(EnumInt(kind), side, position, velocity, accel, rollTime);
      self.side = self.shape.side;
      self.origin = self.shape.origin;
      self.dir = self.shape.dir;
      self.right = self.shape.right;
      self.speed = self.shape.speed;
      self.radius = self.shape.radius;
      self.rollTime = self.shape.rollTime;
      self.duration = self.shape.duration;
      return self;
    }
    self.side = side >= 0.0 ? 1.0 : -1.0;
    self.origin = position;
    self.dir = Vector4.Normalize2D(velocity);
    self.right = Vector4.Cross(self.dir, FlightUtils.Up());
    self.speed = MaxF(Vector4.Length2D(velocity), 10.0);
    self.rollTime = rollTime;
    if Equals(kind, FlightManeuverKind.AileronRoll) {
      self.duration = 2.2;
    } else if Equals(kind, FlightManeuverKind.BarrelRoll) {
      self.radius = 5.0;
      self.duration = 3.5;
    } else {
      // tight enough for the city: v^2 / r at most accel, within 14 to 35 m
      self.radius = ClampF(self.speed * self.speed / accel, 14.0, 35.0);
      let half = 3.1415927 * self.radius / self.speed;
      self.duration = Equals(kind, FlightManeuverKind.Loop) ? 2.0 * half : half + rollTime;
    }
    return self;
  }

  public func IsOverlay() -> Bool {
    return Equals(this.kind, FlightManeuverKind.AileronRoll) || Equals(this.kind, FlightManeuverKind.BarrelRoll);
  }

  public func IsDone() -> Bool {
    return this.time >= this.duration;
  }

  // 0 -> 1 eased in and out over the duration, and its rate
  private func Ease(t: Float, d: Float, out rate: Float) -> Float {
    let x = ClampF(t / d, 0.0, 1.0);
    rate = 0.5 * 3.1415927 / d * SinF(3.1415927 * x);
    return 0.5 * (1.0 - CosF(3.1415927 * x));
  }

  // Aileron roll: the up vector turned about the nose by the roll at time t.
  public func RollUp(t: Float, f: Vector4, u: Vector4) -> Vector4 {
    if IsDefined(this.shape) {
      return this.shape.RollUpAt(t, f, u);
    }
    let rate: Float;
    let phi = 6.2831853 * this.Ease(t, this.duration, rate) * this.side;
    return u * CosF(phi) + Vector4.Cross(f, u) * SinF(phi);
  }

  // Barrel roll: offset from the route (a circle across the flight direction, starting and ending
  // on it), its rate of change, and the up vector (towards the circle's centre), at time t.
  public func BarrelOffset(t: Float, out offset: Vector4, out offsetVelocity: Vector4, out up: Vector4) -> Void {
    if IsDefined(this.shape) {
      this.shape.BarrelAt(t);
      offset = this.shape.offset;
      offsetVelocity = this.shape.offsetVelocity;
      up = this.shape.up;
      return;
    }
    let rate: Float;
    let theta = 6.2831853 * this.Ease(t, this.duration, rate);
    let thetaRate = 6.2831853 * rate;
    let z = FlightUtils.Up();
    let s = this.right * this.side;
    offset = (z * (1.0 - CosF(theta)) + s * SinF(theta)) * this.radius;
    offsetVelocity = (z * SinF(theta) + s * CosF(theta)) * (this.radius * thetaRate);
    up = z * this.radius - offset;
  }

  // Pitch manoeuvres: reference position, velocity, acceleration and attitude at time t.
  public func Sample(t: Float, out pos: Vector4, out vel: Vector4, out acc: Vector4, out fwd: Vector4, out up: Vector4) -> Void {
    if IsDefined(this.shape) {
      this.shape.SampleAt(t);
      pos = this.shape.pos;
      vel = this.shape.vel;
      acc = this.shape.acc;
      fwd = this.shape.fwd;
      up = this.shape.up;
      return;
    }
    let z = FlightUtils.Up();
    let h = this.dir;
    let r = this.radius;
    let v = this.speed;
    let w = v / r;
    let half = 3.1415927 * r / v;
    let s = this.right * this.side;
    let rate: Float;
    acc = new Vector4(0.0, 0.0, 0.0, 0.0);
    if Equals(this.kind, FlightManeuverKind.Loop) || (Equals(this.kind, FlightManeuverKind.Immelmann) && t < half) {
      // up and over a circle of radius r, the car's top towards its centre
      let a = w * ClampF(t, 0.0, 2.0 * half);
      pos = this.origin + (h * SinF(a) + z * (1.0 - CosF(a))) * r;
      vel = (h * CosF(a) + z * SinF(a)) * v;
      acc = (z * CosF(a) - h * SinF(a)) * (v * w);
      up = z * CosF(a) - h * SinF(a);
    } else if Equals(this.kind, FlightManeuverKind.Immelmann) {
      // inverted at the top heading back, rolling upright
      let psi = 3.1415927 * this.Ease(t - half, this.rollTime, rate);
      pos = this.origin + z * (2.0 * r) - h * (v * (t - half));
      vel = h * -v;
      up = z * -CosF(psi) + s * SinF(psi);
    } else if Equals(this.kind, FlightManeuverKind.SplitS) && t < this.rollTime {
      // rolling inverted, straight on
      let psi = 3.1415927 * this.Ease(t, this.rollTime, rate);
      pos = this.origin + h * (v * MaxF(t, 0.0));
      vel = h * v;
      up = z * CosF(psi) + s * SinF(psi);
    } else if Equals(this.kind, FlightManeuverKind.SplitS) {
      // down and round a circle under the top, the car's top towards its centre
      let b = w * MinF(t - this.rollTime, half);
      pos = this.origin + h * (v * this.rollTime) + (h * SinF(b) - z * (1.0 - CosF(b))) * r;
      vel = (h * CosF(b) - z * SinF(b)) * v;
      acc = (z * -CosF(b) - h * SinF(b)) * (v * w);
      up = z * -CosF(b) - h * SinF(b);
    } else {
      pos = this.origin + h * (v * t);
      vel = h * v;
      up = z;
    }
    fwd = Vector4.Normalize(vel);
  }

  // A point the car can be at in any attitude: inside a free gap with margin to spare above and
  // below (the bake's 1.5 m half height holds the car level or rolled, not pitched up).
  public static func PointClear(p: Vector4, margin: Float) -> Bool {
    for g in FlightNavMap.GetGaps(p) {
      if p.Z > g.X + margin && p.Z < g.Y - margin {
        return true;
      }
    }
    return false;
  }

  // The whole shape of a pitch manoeuvre is free on the map, with margin.
  public func Fits(margin: Float) -> Bool {
    let samples = 32;
    let pos: Vector4;
    let vel: Vector4;
    let acc: Vector4;
    let fwd: Vector4;
    let up: Vector4;
    this.Sample(0.0, pos, vel, acc, fwd, up);
    let prev = pos;
    let i = 1;
    while i <= samples {
      this.Sample(this.duration * Cast<Float>(i) / Cast<Float>(samples), pos, vel, acc, fwd, up);
      if !FlightManeuver.PointClear(pos, margin) || FlightNavMap.SegmentClear(prev, pos) < 1.0 {
        return false;
      }
      prev = pos;
      i += 1;
    }
    return true;
  }

  public func GetName() -> String {
    switch this.kind {
      case FlightManeuverKind.AileronRoll: return "aileron roll";
      case FlightManeuverKind.BarrelRoll: return "barrel roll";
      case FlightManeuverKind.Loop: return "loop";
      case FlightManeuverKind.Immelmann: return "Immelmann";
      case FlightManeuverKind.SplitS: return "split-S";
      default: return "none";
    }
  }
}
