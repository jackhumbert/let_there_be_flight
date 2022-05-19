public class FlightModeDrone extends FlightMode {
  public func Update(timeDelta: Float, out force: Vector4, out torque: Vector4) -> Void {
      this.sys.tppCamera.yawDelta = 0.0;
      this.sys.tppCamera.pitchDelta = 0.0;

      let dampFactor = -MaxF(this.sys.ctlr.brake.GetValue() * this.sys.settings.brakeFactor() * this.component.stats.s_brakingFrictionFactor, this.sys.settings.airResistance() * this.component.stats.s_airResistanceFactor);
      let velocityDamp: Vector4 = this.component.stats.d_localVelocity * dampFactor;

      // lift
      force += FlightUtils.Up() * this.sys.ctlr.lift.GetValue() * this.sys.settings.liftFactorDrone();
      // surge
      force += FlightUtils.Forward() * this.sys.ctlr.surge.GetValue() * this.sys.settings.surgeFactor();
      // directional brake
      force += velocityDamp;

      // pitch correction
      torque.X = -(this.sys.ctlr.pitch.GetValue() * this.sys.settings.pitchFactorDrone());
      // roll correction
      torque.Y = (this.sys.ctlr.roll.GetValue() * this.sys.settings.rollFactorDrone());
      // yaw correction
      torque.Z = -(this.sys.ctlr.yaw.GetValue() * this.sys.settings.yawFactorDrone());
  }
}