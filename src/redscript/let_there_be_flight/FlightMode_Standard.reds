public abstract class FlightModeStandard extends FlightMode {
  public func Initialize(component: ref<FlightComponent>) -> Void {
    super.Initialize(component);
    this.collisionPenalty = 0.5;
  }

  protected func UpdateWithNormalDistance(timeDelta: Float, normal: Vector4, heightDifference: Float) -> Void {
    let hoverCorrection = this.component.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, FlightSettings.GetFloat(n"hoverClamp"));// / FlightSettings.GetFloat(n"hoverClamp");
    let liftForce: Float = hoverCorrection * FlightSettings.GetFloat(n"hoverFactor") + (9.81000042) * this.gravityFactor;
    this.UpdateWithNormalLift(timeDelta, normal, liftForce);
  }

  protected func UpdateWithNormalLift(timeDelta: Float, normal: Vector4, liftForce: Float) -> Void {
    let pitchCorrection: Float = 0.0;
    let rollCorrection: Float = 0.0;

    normal = Vector4.RotateAxis(normal, this.component.stats.d_forward, this.component.yaw * FlightSettings.GetFloat(n"rollWithYaw"));
    normal = Vector4.RotateAxis(normal, this.component.stats.d_right, this.component.lift * FlightSettings.GetFloat(n"pitchWithLift") * Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction));
    normal = Vector4.RotateAxis(normal, this.component.stats.d_right, this.component.surge * FlightSettings.GetFloat(n"pitchWithSurge"));
    // normal = Vector4.RotateAxis(normal, this.component.stats.d_right, this.component.surge * this.component.stats.s_forwardWeightTransferFactor);
    

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
    pitchCorrection *= FlightSettings.GetFloat(n"pitchCorrectionFactor");
    rollCorrection *= FlightSettings.GetFloat(n"rollCorrectionFactor");
    // let changeAngle: Float = Vector4.GetAngleDegAroundAxis(Quaternion.GetForward(this.component.stats.d_lastOrientation), this.component.stats.d_forward, this.component.stats.d_up);
    // if AbsF(pitchDegOff) < 30.0 && AbsF(rollDegOff) < 30.0 {

    // }
    // yawCorrection += FlightSettings.GetFloat(n"yawD") * changeAngle / timeDelta;

    let velocityDamp: Vector4 = this.component.linearBrake * FlightSettings.GetFloat(n"brakeFactor") * this.component.stats.s_brakingFrictionFactor * this.component.stats.d_localVelocity;
    let angularDamp: Vector4 = this.component.stats.d_angularVelocity * this.component.angularBrake * FlightSettings.GetFloat(n"angularBrakeFactor") * this.component.stats.s_brakingFrictionFactor;

    // let yawDirectionality: Float = (this.component.stats.d_speedRatio + AbsF(this.yaw.GetValue()) * this.swayWithYaw) * this.yawDirectionalityFactor;
    // actual in-game mass (i think)
    // this.averageMass = this.averageMass * 0.99 + (liftForce / 9.8) * 0.01;
    // FlightLog.Info(ToString(this.averageMass) + " vs " + ToString(this.component.stats.s_mass));
    let surgeForce: Float = this.component.surge * FlightSettings.GetFloat(n"surgeFactor");

    //this.CreateImpulse(this.component.stats.d_position, FlightUtils.Right() * Vector4.Dot(FlightUtils.Forward() - direction, FlightUtils.Right()) * yawDirectionality / 2.0);


    this.force = new Vector4(0.0, 0.0, 0.0, 0.0);
    this.torque = new Vector4(0.0, 0.0, 0.0, 0.0);

    // lift
    // force += new Vector4(0.00, 0.00, liftForce + this.component.stats.d_speedRatio * liftForce, 0.00);
    this.force += liftForce * this.component.stats.d_localUp;
    // surge
    this.force += FlightUtils.Forward() * surgeForce;
    // sway
    this.force += FlightUtils.Right() * this.component.sway * FlightSettings.GetFloat(n"swayFactor");
    // directional brake
    this.force -= velocityDamp;

    // pitch correction
    this.torque.X = -(pitchCorrection + angularDamp.X);
    // roll correction
    this.torque.Y = (rollCorrection - angularDamp.Y);
    // yaw correction
    this.torque.Z = -(this.component.yaw * FlightSettings.GetFloat(n"yawFactor") + angularDamp.Z);
    // rotational brake
    // torque = torque + (angularDamp);

    // if this.showOptions {
    //   this.component.stats.s_centerOfMass.position.X -= torque.Y * 0.1;
    //   this.component.stats.s_centerOfMass.position.Y -= torque.X * 0.1;
    // }
  }
}