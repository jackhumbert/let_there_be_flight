enum FlightVehicleType {
  Streetkid = 0,
  Nomad = 1,
  Corpo = 2,
  Poor = 3,
  Suburban = 4,
  Urban = 5
}

public abstract native class IFlightConfiguration extends IScriptable {
  @runtimeProperty("offset", "0x40")
  public native let component: wref<FlightComponent>;

  @runtimeProperty("offset", "0x50")
  public native let thrusters: array<ref<IFlightThruster>>;

  @runtimeProperty("offset", "0x60")
  public native let flightCameraBone: CName; // "root_border_front"

  @runtimeProperty("offset", "0x68")
  public native let flightCameraOffset: Vector3; // 0, 0, 0

  public native func AddColliders();
  public native func RemoveColliders();
  public native func OnActivationCore();
  public native func OnDeactivationCore();

  public func CanActivate() -> Bool = true;

  public let type: FlightVehicleType = FlightVehicleType.Corpo;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Crashes")
  @runtimeProperty("ModSettings.category.order", "4")
  @runtimeProperty("ModSettings.order", "1")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Detachable-Thrusters")
  @runtimeProperty("ModSettings.description", "UI-Settings-Detachable-Thrusters-Description")
  public let areThrustersDetachable: Bool = true;

  public let percentageAttached: Float = 1.0;

  public func AreThrustersDetachable() -> Bool {
    return this.areThrustersDetachable;
  }

  public func OnSetup(vehicle: ref<VehicleObject>) {
    let name = NameToString(vehicle.GetCurrentAppearanceName());

    if StrFindFirst(name, "poor") > -1 {
      this.type = FlightVehicleType.Poor;
    }
    if StrFindFirst(name, "urban") > -1 {
      this.type = FlightVehicleType.Urban;
    }
    if StrFindFirst(name, "suburban") > -1 {
      this.type = FlightVehicleType.Suburban;
    }

    if StrFindFirst(name, "nomad") > -1 {
      this.type = FlightVehicleType.Nomad;
    }

    if StrFindFirst(name, "tyger") > -1 
    || StrFindFirst(name, "6th_street") > -1 
    || StrFindFirst(name, "animals") > -1 
    || StrFindFirst(name, "valentinos") > -1 {
      this.type = FlightVehicleType.Streetkid;
    }
  }

  public func CreateMesh(vehicle: ref<VehicleObject>) -> ref<MeshComponent> {

    let defaultMesh = r"user\\jackhumbert\\meshes\\engine_nomad.mesh";
    
    if (Equals(this.type, FlightVehicleType.Corpo)) {
      defaultMesh = r"user\\jackhumbert\\meshes\\engine_corpo.mesh";
    }

    let mc = new PhysicalMeshComponent();
    mc.SetMesh(vehicle.GetResRef("thrusterMesh", defaultMesh));
    mc.meshApperance = vehicle.GetCName("thrusterAppearance");
    mc.motionBlurScale = 0.1;
    // not Appearance: a car whose flight first comes on while it is far away or in the air (a summon spawns it 120 m out,
    // 20 m up) has its full appearance shown later without these added meshes, and so does every car of that model
    // after it in the session
    mc.LODMode = entMeshComponentLODMode.AlwaysVisible;
    return mc;
  }

  public func OnActivation() {
    FlightLog.Info("[FlightConfiguration] Activating " + NameToString(this.GetClassName()) + ": " + NameToString(this.component.GetVehicle().GetCurrentAppearanceName()));
  }

  public func OnDeactivation() {
    
  }

  public func GetThrusterTensor() -> Vector4 {
    let total = new Vector4(0.0, 0.0, 0.0, 0.0);
    let vt = this.component.GetVehicle().GetWorldTransform();
    let totalThrusters = 0.0;
    let attachedThrusters = 0.0;
    for thruster in this.thrusters {
      if thruster.attached {
        attachedThrusters += 1.0;
        let v = WorldTransform.TransformInvPoint(vt, thruster.GetLocalToWorld() * Vector4.EmptyVector());
        v -= Vector4.Vector3To4(this.component.GetVehicle().GetCenterOfMass());
        total.X += SqrtF(PowF(v.Y, 2.0) + PowF(v.Z, 2.0));
        total.Y += SqrtF(PowF(v.X, 2.0) + PowF(v.Z, 2.0));
        total.Z += SqrtF(PowF(v.X, 2.0) + PowF(v.Y, 2.0));
      }
      totalThrusters += 1.0;
    }
    this.percentageAttached = attachedThrusters / totalThrusters;
    return total;
  }
}
