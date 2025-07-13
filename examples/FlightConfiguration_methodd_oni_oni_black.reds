@if(ModuleExists("LetThereBeFlight")) 
public class FlightConfiguration_methodd_oni_oni_black extends IFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    this.flightCameraOffset = new Vector3(0.0, 1.0, 0.5);

    ArrayPush(this.thrusters, new FlightThrusterFront().Create(vehicle, CreateEmptyThruster()));
    ArrayPush(this.thrusters, new FlightThrusterBack().Create(vehicle, CreateEmptyThruster()));

    this.thrusters[0].hasRetroThruster = false;
    this.thrusters[1].hasRetroThruster = false;

    for thruster in this.thrusters {
      // ArrayPush(thruster.fxs, new RegularFlightThrusterFX().Create(thruster));
      thruster.OnSetup(this.component);
    }
  }

  public func OnActivation() {
    super.OnActivation();
  }

  public func OnDeactivation() {
  }
}