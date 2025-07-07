public class SixWheelCarFlightConfiguration extends CarFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    ArrayPush(this.thrusters, new FlightThrusterFLB().Create(vehicle, this.CreateMesh(vehicle)));
    ArrayPush(this.thrusters, new FlightThrusterFRB().Create(vehicle, this.CreateMesh(vehicle)));

    let retroThrusterTweak = vehicle.GetRecordID();
    TDBID.Append(retroThrusterTweak, t".hasRetroThruster");
    let hasRetroThruster = TweakDBInterface.GetBool(retroThrusterTweak, false);

    this.thrusters[4].hasRetroThruster = hasRetroThruster;
    this.thrusters[5].hasRetroThruster = hasRetroThruster;

    if this.thrusters[4].hasRetroThruster {
      ArrayPush(this.thrusters[4].fxs, new MainFlightThrusterFX().Create(this.thrusters[4]));
      ArrayPush(this.thrusters[4].fxs, new SideFlightThrusterFX().Create(this.thrusters[4]));
    } else {
      ArrayPush(this.thrusters[4].fxs, new RegularFlightThrusterFX().Create(this.thrusters[4]));
    }
    this.thrusters[4].OnSetup(this.component);

    if this.thrusters[5].hasRetroThruster {
      ArrayPush(this.thrusters[5].fxs, new MainFlightThrusterFX().Create(this.thrusters[5]));
      ArrayPush(this.thrusters[5].fxs, new SideFlightThrusterFX().Create(this.thrusters[5]));
    } else {
      ArrayPush(this.thrusters[5].fxs, new RegularFlightThrusterFX().Create(this.thrusters[5]));
    }
    this.thrusters[5].OnSetup(this.component);
  }
}
