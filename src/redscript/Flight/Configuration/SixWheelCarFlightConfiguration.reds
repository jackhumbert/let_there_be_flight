public class SixWheelCarFlightConfiguration extends CarFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    ArrayPush(this.thrusters, new FlightThrusterFLB().Create(vehicle, CreateNomadThruster()));
    ArrayPush(this.thrusters, new FlightThrusterFRB().Create(vehicle, CreateNomadThruster()));

    vehicle.AddComponent(this. thrusters[4].meshComponent);
    this.thrusters[4].OnSetup(this.component);
    ArrayPush(this.thrusters[4].fxs, new MainFlightThrusterFX().Create(this.thrusters[4]));
    ArrayPush(this.thrusters[4].fxs, new SideFlightThrusterFX().Create(this.thrusters[4]));

    vehicle.AddComponent(this. thrusters[5].meshComponent);
    this.thrusters[5].OnSetup(this.component);
    ArrayPush(this.thrusters[5].fxs, new MainFlightThrusterFX().Create(this.thrusters[5]));
    ArrayPush(this.thrusters[5].fxs, new SideFlightThrusterFX().Create(this.thrusters[5]));
  }
}
