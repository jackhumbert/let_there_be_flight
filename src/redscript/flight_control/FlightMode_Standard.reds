public abstract class FlightModeStandard extends FlightMode {
  protected func UpdateWithNormalDistance(timeDelta: Float, normal: Vector4, heightDifference: Float) -> Void {
    let hoverCorrection = this.component.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, this.sys.settings.hoverClamp()) / this.sys.settings.hoverClamp();
    let liftForce: Float = hoverCorrection * this.sys.settings.hoverFactor() * (9.81000042) * this.gravityFactor;
    this.UpdateWithNormalLift(timeDelta, normal, liftForce);
  }

  protected func UpdateWithNormalLift(timeDelta: Float, normal: Vector4, liftForce: Float) -> Void {
    let pitchCorrection: Float = 0.0;
    let rollCorrection: Float = 0.0;
    let yawCorrection: Float = 0.0;

    normal = Vector4.RotateAxis(normal, this.component.stats.d_forward, this.component.yaw * this.sys.settings.rollWithYaw());
    normal = Vector4.RotateAxis(normal, this.component.stats.d_right, this.component.lift * this.sys.settings.pitchWithLift() * Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction));
    normal = Vector4.RotateAxis(normal, this.component.stats.d_right, this.component.surge * this.sys.settings.pitchWithSurge());

    this.component.pitchPID.SetRatio(this.component.stats.d_speedRatio * AbsF(Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_forward)));
    this.component.rollPID.SetRatio(this.component.stats.d_speedRatio * AbsF(Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_right)));

    // pitchCorrection = this.component.pitchPID.GetCorrectionClamped(FlightUtils.IdentCurve(Vector4.Dot(normal, FlightUtils.Forward())) + this.lift.GetValue() * this.pitchWithLift, timeDelta, 10.0) + this.pitch.GetValue() / 10.0;
    // rollCorrection = this.component.rollPID.GetCorrectionClamped(FlightUtils.IdentCurve(Vector4.Dot(normal, FlightUtils.Right())), timeDelta, 10.0) + this.yaw.GetValue() * this.rollWithYaw + this.roll.GetValue() / 10.0;
    let pitchDegOff = 90.0 - AbsF(Vector4.GetAngleDegAroundAxis(normal, this.component.stats.d_forward, this.component.stats.d_right));
    let rollDegOff = 90.0 - AbsF(Vector4.GetAngleDegAroundAxis(normal, this.component.stats.d_right, this.component.stats.d_forward));
    if AbsF(pitchDegOff) < 80.0  {
      // pitchCorrection = this.component.pitchPID.GetCorrectionClamped(pitchDegOff / 90.0 + this.lift.GetValue() * this.pitchWithLift, timeDelta, 10.0) + this.pitch.GetValue() / 10.0;
      pitchCorrection = this.component.pitchPID.GetCorrectionClamped(pitchDegOff / 90.0, timeDelta, 10.0) + this.component.pitch / 10.0;
    }
    if AbsF(rollDegOff) < 80.0 {
      // rollCorrection = this.component.rollPID.GetCorrectionClamped(rollDegOff / 90.0 + this.yaw.GetValue() * this.rollWithYaw, timeDelta, 10.0) + this.roll.GetValue() / 10.0;
      rollCorrection = this.component.rollPID.GetCorrectionClamped(rollDegOff / 90.0, timeDelta, 10.0) + this.component.roll / 10.0;
    }
    // adjust with speed ratio 
    // pitchCorrection = pitchCorrection * (this.pitchCorrectionFactor + 1.0 * this.pitchCorrectionFactor * this.component.stats.d_speedRatio);
    // rollCorrection = rollCorrection * (this.rollCorrectionFactor + 1.0 * this.rollCorrectionFactor * this.component.stats.d_speedRatio);
    pitchCorrection *= this.sys.settings.pitchCorrectionFactor();
    rollCorrection *= this.sys.settings.rollCorrectionFactor();
    let changeAngle: Float = Vector4.GetAngleDegAroundAxis(Quaternion.GetForward(this.component.stats.d_lastOrientation), this.component.stats.d_forward, this.component.stats.d_up);
    if AbsF(pitchDegOff) < 30.0 && AbsF(rollDegOff) < 30.0 {
      let directionAngle: Float = Vector4.GetAngleDegAroundAxis(this.component.stats.d_direction, this.component.stats.d_forward, this.component.stats.d_up);
      this.component.yawPID.integralFloat *= (1.0 - AbsF(this.component.yaw));
      yawCorrection = this.component.yawPID.GetCorrectionClamped(directionAngle, timeDelta, 10.0) / 10.0;
    }
    yawCorrection += this.sys.settings.yawD() * changeAngle / timeDelta;

    let velocityDamp: Vector4 = this.component.stats.d_localVelocity * -MaxF(this.component.brake * this.sys.settings.brakeFactor() * this.component.stats.s_brakingFrictionFactor, this.sys.settings.airResistance() * this.component.stats.s_airResistanceFactor);
    let angularDamp: Vector4 = this.component.stats.d_angularVelocity * -MaxF(this.component.brake * this.sys.settings.angularBrakeFactor() * this.component.stats.s_brakingFrictionFactor, this.sys.settings.angularDampFactor());

    // let yawDirectionality: Float = (this.component.stats.d_speedRatio + AbsF(this.yaw.GetValue()) * this.swayWithYaw) * this.yawDirectionalityFactor;
    let yawDirectionality: Float = this.component.stats.d_speedRatio * this.sys.settings.yawDirectionalityFactor();
    // actual in-game mass (i think)
    // this.averageMass = this.averageMass * 0.99 + (liftForce / 9.8) * 0.01;
    // FlightLog.Info(ToString(this.averageMass) + " vs " + ToString(this.component.stats.s_mass));
    let surgeForce: Float = this.component.surge * this.sys.settings.surgeFactor();

    //this.CreateImpulse(this.component.stats.d_position, FlightUtils.Right() * Vector4.Dot(FlightUtils.Forward() - direction, FlightUtils.Right()) * yawDirectionality / 2.0);

    let aeroFactor = Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction);

    this.force = new Vector4(0.0, 0.0, 0.0, 0.0);
    this.torque = new Vector4(0.0, 0.0, 0.0, 0.0);

    // yawDirectionality - redirect non-directional velocity to vehicle forward
    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * AbsF(aeroFactor);
    // lift
    // force += new Vector4(0.00, 0.00, liftForce + this.component.stats.d_speedRatio * liftForce, 0.00);
    this.force += liftForce * this.component.stats.d_localUp;
    // surge
    this.force += FlightUtils.Forward() * surgeForce;
    // directional brake
    this.force += velocityDamp;

    // pitch correction
    this.torque.X = -(pitchCorrection + angularDamp.X);
    // roll correction
    this.torque.Y = (rollCorrection - angularDamp.Y);
    // yaw correction
    this.torque.Z = -((yawCorrection * this.sys.settings.yawCorrectionFactor() + this.component.yaw * this.sys.settings.yawFactor()) + angularDamp.Z);
    // rotational brake
    // torque = torque + (angularDamp);

    // if this.showOptions {
    //   this.component.stats.s_centerOfMass.position.X -= torque.Y * 0.1;
    //   this.component.stats.s_centerOfMass.position.Y -= torque.X * 0.1;
    // }
  }
}