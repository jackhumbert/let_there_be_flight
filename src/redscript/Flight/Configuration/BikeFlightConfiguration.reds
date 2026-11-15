public class BikeFlightConfiguration extends IFlightConfiguration {
  public let frontChild: ref<MeshComponent>;
  public let backChild: ref<MeshComponent>;

  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    this.flightCameraOffset = new Vector3(0.0, 1.0, 0.5);

    ArrayPush(this.thrusters, new FlightThrusterFront().Create(vehicle, this.CreateMesh(vehicle)));
    ArrayPush(this.thrusters, new FlightThrusterBack().Create(vehicle, this.CreateMesh(vehicle)));

    this.thrusters[0].hasRetroThruster = false;
    this.thrusters[1].hasRetroThruster = false;

    for thruster in this.thrusters {
      ArrayPush(thruster.fxs, new RegularFlightThrusterFX().Create(thruster));
      thruster.OnSetup(this.component);
    }
  }

  public func OnActivation() {
    super.OnActivation();
    this.frontChild.visualScale = new Vector3(1.0, 1.0, 1.0);
    this.backChild.visualScale = new Vector3(1.0, 1.0, 1.0);
  }

  public func OnDeactivation() {
    this.frontChild.visualScale = new Vector3(0.0, 0.0, 0.0);
    this.backChild.visualScale = new Vector3(0.0, 0.0, 0.0);
  }
}