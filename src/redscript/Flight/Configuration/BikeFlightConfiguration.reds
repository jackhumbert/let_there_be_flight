public class BikeFlightConfiguration extends IFlightConfiguration {
  public let frontChild: ref<MeshComponent>;
  public let backChild: ref<MeshComponent>;

  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    this.flightCameraOffset = new Vector3(0.0, 1.0, 0.5);

    ArrayPush(this.thrusters, new FlightThrusterFront().Create(vehicle, this.CreateMesh(vehicle)));
    ArrayPush(this.thrusters, new FlightThrusterBack().Create(vehicle, this.CreateMesh(vehicle)));

    // if (!Equals(this.type, FlightVehicleType.Corpo)) {
    //   this.frontChild = CreateNomadThruster();
    //   this.frontChild.name = n"FrontThrusterChild";
    //   this.frontChild.visualScale = new Vector3(0.0, 0.0, 0.0);
    //   // this.frontChild.SetLocalPosition(new Vector4(0.0, 0.0, -0.2, 1.0));
    //   this.frontChild.SetLocalOrientation(new Quaternion(0.0, 0.0, 0.707, -0.707));
    //   this.frontChild.SetParentTransform(this.thrusters[0].meshComponent.name, n"None");
    //   // vehicle.AddComponent(this.frontChild);

    //   this.backChild = CreateNomadThruster();
    //   this.backChild.name = n"BackThrusterChild";
    //   this.backChild.visualScale = new Vector3(0.0, 0.0, 0.0);
    //   // this.backChild.SetLocalPosition(new Vector4(0.0, 0.0, -0.2, 1.0));
    //   this.backChild.SetLocalOrientation(new Quaternion(0.0, 0.0, -0.707, -0.707));
    //   this.backChild.SetParentTransform(this.thrusters[1].meshComponent.name, n"None");
    //   // vehicle.AddComponent(this.backChild);
    // }

    this.thrusters[0].hasRetroThruster = false;
    this.thrusters[1].hasRetroThruster = false;

    for thruster in this.thrusters {
      ArrayPush(thruster.fxs, new RegularFlightThrusterFX().Create(thruster));
      // vehicle.AddComponent(thruster.meshComponent);
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