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

  public func OnSetup(vehicle: ref<VehicleObject>) {
    let name = NameToString(vehicle.GetCurrentAppearanceName());

    // if (VehicleComponent.CheckVehicleDesiredTag(vehicle, n"Sport"))
    
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

    // let appearanceTweak = vehicle.GetRecordID();
    // TDBID.Append(appearanceTweak, t".thrusterAppearance");
    // let thrusterAppearance = TweakDBInterface.GetCName(appearanceTweak, n"default");
    
    // let meshTweak = vehicle.GetRecordID();
    // TDBID.Append(meshTweak, t".thrusterMesh");
    // let thrusterMesh = TweakDBInterface.GetResRef(meshTweak, defaultMesh);


    let mc = new PhysicalMeshComponent();
    mc.SetMesh(vehicle.GetResRef("thrusterMesh", defaultMesh));
    mc.meshApperance = vehicle.GetCName("thrusterAppearance");
    mc.motionBlurScale = 0.1;
    mc.LODMode = entMeshComponentLODMode.Appearance;
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
    for thruster in this.thrusters {
      if thruster.attached {
        let v = WorldTransform.TransformInvPoint(vt, thruster.GetLocalToWorld() * Vector4.EmptyVector());
        v -= Vector4.Vector3To4(this.component.GetVehicle().GetCenterOfMass());
        total.X += SqrtF(PowF(v.Y, 2.0) + PowF(v.Z, 2.0));
        total.Y += SqrtF(PowF(v.X, 2.0) + PowF(v.Z, 2.0));
        total.Z += SqrtF(PowF(v.X, 2.0) + PowF(v.Y, 2.0));
      }
    }
    return total;
  }

  public func GetEffectForMaterial(material: CName, originalFx: MaterialFx) -> MaterialFx {
    if Equals(material, n"concrete.physmat") ||
       Equals(material, n"asphalt.physmat") ||
       Equals(material, n"metal.physmat") ||
       Equals(material, n"metal_painted.physmat")||
       Equals(material, n"default_material.physmat") {
        originalFx.normal.particle.skidMarks = r"user\\jackhumbert\\effects\\thruster_sparks.effect";
        originalFx.normal.particle.tireTracks = r"user\\jackhumbert\\effects\\thruster_sparks.effect";
        originalFx.normal.particle.loaded = true;
        originalFx.wet.particle.skidMarks = r"user\\jackhumbert\\effects\\thruster_sparks.effect";
        originalFx.wet.particle.tireTracks = r"user\\jackhumbert\\effects\\thruster_sparks.effect";
        originalFx.wet.particle.loaded = true;
        originalFx.rain.particle.skidMarks = r"user\\jackhumbert\\effects\\thruster_sparks.effect";
        originalFx.rain.particle.tireTracks = r"user\\jackhumbert\\effects\\thruster_sparks.effect";
        originalFx.rain.particle.loaded = true;
        
        originalFx.normal.decal.skidMarks = r"user\\jackhumbert\\effects\\thruster_mark.effect";
        originalFx.normal.decal.tireTracks = r"user\\jackhumbert\\effects\\thruster_mark.effect";
        originalFx.normal.decal.loaded = true;
        originalFx.wet.decal.skidMarks = r"user\\jackhumbert\\effects\\thruster_mark.effect";
        originalFx.wet.decal.tireTracks = r"user\\jackhumbert\\effects\\thruster_mark.effect";
        originalFx.wet.decal.loaded = true;
        originalFx.rain.decal.skidMarks = r"user\\jackhumbert\\effects\\thruster_mark.effect";
        originalFx.rain.decal.tireTracks = r"user\\jackhumbert\\effects\\thruster_mark.effect";
        originalFx.rain.decal.loaded = true;
    }

    //   case n"dirt.physmat":
    //   case n"grass.physmat":
    //   case n"sand.physmat":
    //   case n"mud.physmat":
 
    return originalFx;
  }
}