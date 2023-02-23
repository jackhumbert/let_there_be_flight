enum FlightVehicleType {
  Streetkid = 0,
  Nomad = 1,
  Corpo = 2
}

public abstract native class IFlightConfiguration extends IScriptable {
  @runtimeProperty("offset", "0x40")
  public native let component: ref<FlightComponent>;

  @runtimeProperty("offset", "0x50")
  public native let thrusters: array<ref<IFlightThruster>>;

  @runtimeProperty("offset", "0x60")
  public native let flightCameraBone: CName; // "root_border_front"

  @runtimeProperty("offset", "0x68")
  public native let flightCameraOffset: Vector3; // 0, 0, 0

  public let type: FlightVehicleType = FlightVehicleType.Corpo;

  public func OnSetup(vehicle: ref<VehicleObject>) {
    // switch (vehicle.currentAppearance) {
    //   case n"thorton_galena_nomad_player_01":
    //     this.type = FlightVehicleType.Nomad;
    //     break;
    //   case n"chevalier_emperor__basic_police":
    //     this.type = FlightVehicleType.Corpo;
    //     break;
    //   case n"quadra_type66__basic_poor_03":
    //   case n"arch_nemesis_basic_jackie":
    //   case n"mahir_supron__basic_urban_02":
    //   case n"thorton_galena__basic_player_01":
    //   default:
    //     this.type = FlightVehicleType.Streetkid;
    //     break;
    // }
  }
}

public func CreateEmptyThruster() -> ref<MeshComponent> {
  let mc = new MeshComponent();
  return mc;
}

public func CreateCorpoThruster() -> ref<MeshComponent> {
  let mc = new PhysicalMeshComponent();
  mc.SetMesh(r"user\\jackhumbert\\meshes\\engine_corpo.mesh");
  mc.meshApperance = n"default";
  mc.motionBlurScale = 0.1;
  mc.LODMode = entMeshComponentLODMode.Appearance;
  return mc;
}

public func CreateNomadThruster() -> ref<MeshComponent> {
  let mc = new PhysicalMeshComponent();
  mc.SetMesh(r"user\\jackhumbert\\meshes\\engine_nomad.mesh");
  mc.meshApperance = n"default";
  mc.motionBlurScale = 0.1;
  mc.LODMode = entMeshComponentLODMode.Appearance;
  return mc;
}

public class CarFlightConfiguration extends IFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    if (Equals(this.type, FlightVehicleType.Corpo)) {
      ArrayPush(this.thrusters, new FlightThrusterFL().Create(vehicle, CreateCorpoThruster()));
      ArrayPush(this.thrusters, new FlightThrusterFR().Create(vehicle, CreateCorpoThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBL().Create(vehicle, CreateCorpoThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBR().Create(vehicle, CreateCorpoThruster()));
    } else {
      ArrayPush(this.thrusters, new FlightThrusterFL().Create(vehicle, CreateNomadThruster()));
      ArrayPush(this.thrusters, new FlightThrusterFR().Create(vehicle, CreateNomadThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBL().Create(vehicle, CreateNomadThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBR().Create(vehicle, CreateNomadThruster()));
    }

    for thruster in this.thrusters {
      vehicle.AddComponent(thruster.meshComponent);
      thruster.OnSetup(this.component);
    }
  }
}

public class SixWheelCarFlightConfiguration extends CarFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    ArrayPush(this.thrusters, new FlightThrusterFLB().Create(vehicle, CreateNomadThruster()));
    ArrayPush(this.thrusters, new FlightThrusterFRB().Create(vehicle, CreateNomadThruster()));

    vehicle.AddComponent(this. thrusters[4].meshComponent);
    this.thrusters[4].OnSetup(this.component);

    vehicle.AddComponent(this. thrusters[5].meshComponent);
    this.thrusters[5].OnSetup(this.component);
  }
}

public class BikeFlightConfiguration extends IFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    this.flightCameraOffset = new Vector3(0.0, 1.0, 0.5);

    ArrayPush(this.thrusters, new FlightThrusterFront().Create(vehicle, CreateEmptyThruster()));
    ArrayPush(this.thrusters, new FlightThrusterBack().Create(vehicle, CreateEmptyThruster()));

    let mesh: ref<MeshComponent>;
    mesh = CreateNomadThruster();
    // mesh.visualScale = new Vector3(1.0, 0.5, 1.0);
    // mesh.SetLocalPosition(new Vector4(0.0, 0.0, -0.2, 1.0));
    mesh.SetLocalOrientation(new Quaternion(0.0, 0.0, 0.707, -0.707));
    mesh.SetParentTransform(this.thrusters[0].meshComponent.name, n"None");
    vehicle.AddComponent(mesh);

    mesh = CreateNomadThruster();
    // mesh.visualScale = new Vector3(1.0, 0.5, 1.0);
    // mesh.SetLocalPosition(new Vector4(0.0, 0.0, -0.2, 1.0));
    mesh.SetLocalOrientation(new Quaternion(0.0, 0.0, -0.707, -0.707));
    mesh.SetParentTransform(this.thrusters[1].meshComponent.name, n"None");
    vehicle.AddComponent(mesh);

    for thruster in this.thrusters {
      vehicle.AddComponent(thruster.meshComponent);
      thruster.OnSetup(this.component);
    }
  }
}

// public class FlightConfiguration_quadra_type66__basic_jen_rowley extends IFlightConfiguration {
//   public func OnSetup() {
//     ArrayPush(this.thrusters, new FlightThrusterFL().Create());
//     ArrayPush(this.thrusters, new FlightThrusterFR().Create());
//     ArrayPush(this.thrusters, new FlightThrusterBL().Create());
//     ArrayPush(this.thrusters, new FlightThrusterBR().Create());

//     // this.thrusters[0].relativePosition = new Vector3(0.0, 0.0, 1.0);

//     this.thrusters[2].meshPath = n"user\\jackhumbert\\meshes\\engine_nomad.mesh";
//     this.thrusters[3].meshPath = n"user\\jackhumbert\\meshes\\engine_nomad.mesh";

//     for thruster in this.thrusters {
//       thruster.OnSetup(this.component);
//     }
//   }
// }