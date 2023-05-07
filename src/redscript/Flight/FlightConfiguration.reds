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
  public native let component: ref<FlightComponent>;

  @runtimeProperty("offset", "0x50")
  public native let thrusters: array<ref<IFlightThruster>>;

  @runtimeProperty("offset", "0x60")
  public native let flightCameraBone: CName; // "root_border_front"

  @runtimeProperty("offset", "0x68")
  public native let flightCameraOffset: Vector3; // 0, 0, 0

  public native func OnActivationCore();
  public native func OnDeactivationCore();

  public func CanActivate() -> Bool = true;

  public let type: FlightVehicleType = FlightVehicleType.Corpo;

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

  public func OnActivation() {
    
  }

  public func OnDeactivation() {
    
  }

  public func GetThrusterTensor() -> Vector4 {
    let total = new Vector4(0.0, 0.0, 0.0, 0.0);
    let vt = this.component.GetVehicle().GetWorldTransform();
    for thruster in this.thrusters {
      let v = WorldTransform.TransformInvPoint(vt, thruster.meshComponent.GetLocalToWorld() * Vector4.EmptyVector());
      v -= Vector4.Vector3To4(this.component.GetVehicle().GetCenterOfMass());
      total.X += SqrtF(PowF(v.Y, 2.0) + PowF(v.Z, 2.0));
      total.Y += SqrtF(PowF(v.X, 2.0) + PowF(v.Z, 2.0));
      total.Z += SqrtF(PowF(v.X, 2.0) + PowF(v.Y, 2.0));
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
    
    //#define PARTICLE R"(base\fx\vehicles\_damage\scratches\car_scratches_sparks.effect)"
    //#define PARTICLE R"(base\fx\_library\sparks\lib_sparks_impact_2-3m.effect)"
    //#define DECAL R"(base\fx\vehicles\_damage\scratches\car_scratches_sparks.effect)"
    //#define DECAL R"(base\fx\vehicles\_damage\scratches\car_scratches_concrete.effect)"
    //#define DECAL R"(base\fx\_library\fire\lib_burnt_mark_01_decal.effect)"

    // base\fx\vehicles\_wheels\tire_tracks\radial\blood\v_tire_track_blood_r_m_01.effect

    // this.particleEffect = r"base\\fx\\_library\\sparks\\lib_sparks_veryfast_constant_01.effect";
    // this.particleEffect = r"base\\fx\\environment\\sparks\\sparks_constant.effect";
    // good
    // this.particleEffect = r"base\\fx\\_library\\sparks\\lib_sparks_welding_big_01.effect";
    // this.particleEffect = r"user\\jackhumbert\\effects\\thruster_sparks.effect";
    // this.decalEffect = r"base\\fx\\_library\\concrete_breaks\\concrete_breaks_medium.effect";
    // this.decalEffect = r"base\\fx\\vehicles\\_damage\\scratches\\v_car_scratches_decal.effect";
    // this.decalEffect = r"base\\fx\\vehicles\\_wheels\\skid_marks\\radial\\v_skid_mark_r_m_01.effect";
    // this.decalEffect = r"base\\fx\\vehicles\\_wheels\\tire_tracks\\radial\\v_tire_track_r_m_01.effect";
    // this.decalEffect = r"user\\jackhumbert\\effects\\thruster_mark.effect";

    if (Equals(this.type, FlightVehicleType.Corpo)) {
      ArrayPush(this.thrusters, new FlightThrusterFL().Create(vehicle, CreateCorpoThruster()));
      ArrayPush(this.thrusters, new FlightThrusterFR().Create(vehicle, CreateCorpoThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBR().Create(vehicle, CreateCorpoThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBL().Create(vehicle, CreateCorpoThruster()));
    } else {
      ArrayPush(this.thrusters, new FlightThrusterFL().Create(vehicle, CreateNomadThruster()));
      ArrayPush(this.thrusters, new FlightThrusterFR().Create(vehicle, CreateNomadThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBR().Create(vehicle, CreateNomadThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBL().Create(vehicle, CreateNomadThruster()));
      
      this.thrusters[0].hasRetroThruster = false;
      this.thrusters[1].hasRetroThruster = false;
      this.thrusters[2].hasRetroThruster = false;
      this.thrusters[3].hasRetroThruster = false;
    }

    this.thrusters[0].wheelIndex = 0;
    this.thrusters[1].wheelIndex = 1;
    this.thrusters[2].wheelIndex = 2;
    this.thrusters[3].wheelIndex = 3;

    for thruster in this.thrusters {
      if (Equals(this.type, FlightVehicleType.Corpo)) {
        ArrayPush(thruster.fxs, new MainFlightThrusterFX().Create(thruster));
        ArrayPush(thruster.fxs, new SideFlightThrusterFX().Create(thruster));
      } else {
        ArrayPush(thruster.fxs, new RegularFlightThrusterFX().Create(thruster));
      }
      vehicle.AddComponent(thruster.meshComponent);
      thruster.OnSetup(this.component);
    }

    // this.thrusters[0].fxs[0].SetResource(r"base\\fx\\vehicles\\manticore\\smoke_exhaust.effect"); // black & grey smoke
    // this.thrusters[1].fxs[0].SetResource(r"base\\fx\\vehicles\\av\\av_panzer\\weapons\\v_panzer_bullet_trail.effect"); // nothing i saw? maybe noise
    // this.thrusters[2].fxs[0].SetResource(r"base\\fx\\vehicles\\av\\_lights\\v_av_distant_emissives.effect"); // directional smoke
    // this.thrusters[3].fxs[0].SetResource(r"base\\fx\\vehicles\\drone\\drone_missile_trail.effect"); // cool orange trail actually?
    // this.thrusters[3].fxs[0].SetResource(r"base\\fx\\vehicles\\stratospheric_plane\\v_stratospheric_plane_takeoff_engines.effect"); // giant flashing rings

    // this.thrusters[0].fxs[0].SetResource(r"base\\fx\\vehicles\\_exhaust\\veh_exhaust_backfire.effect"); // no noticable effect
    // this.thrusters[1].fxs[0].SetResource(r"base\\fx\\vehicles\\_exhaust\\v_exhaust_smoke_standard_round.effect");  // no noticable effect
    // this.thrusters[2].fxs[0].SetResource(r"base\\fx\\vehicles\\car\\car_exhaust_smoke.effect");  // no noticable effect
    // this.thrusters[3].fxs[0].SetResource(r"base\\fx\\vehicles\\_exhaust\\v_exhaust_smoke_standard_rectangular.effect"); // no noticable effect

    // this.thrusters[0].fxs[0].SetResource(r"base\\fx\\vehicles\\av\\av_zetatech\\av_zetatech_turret_trail.effect"); // cool trail
    // this.thrusters[1].fxs[0].SetResource(r"base\\fx\\vehicles\\_exhaust\\veh_exhaust_start_sport.effect");   // not sure
    // this.thrusters[2].fxs[0].SetResource(r"base\\fx\\vehicles\\av\\v_av_manticore_vapour_trails.effect");  // not sure
    // this.thrusters[3].fxs[0].SetResource(r"base\\fx\\vehicles\\_exhaust\\v_exhaust_backfire_sport_rectangular.effect"); // forward smoke thing?

    // this.thrusters[0].fxs[0].SetResource(r"base\\fx\\weapons\\firearms\\special\\vehicle_rocket_launcher\\w_special_vehicle_missile_trail.effect"); // tiny smokey trail, short
    // this.thrusters[1].fxs[0].SetResource(r"base\\fx\\quest\\q003\\boss_centaur\\forc e_attack\\force_trail.effect");  // idk
    // this.thrusters[2].fxs[0].SetResource(r"base\\fx\\quest\\q114\\q114_missile_barage_trail.effect"); // smoke with flame - good basis for combustion engine?, short
    // this.thrusters[3].fxs[0].SetResource(r"base\\fx\\weapons\\trails\\smart\\w_trail_smart_rifle_high_low_class.effect");  // tiny little trail, short

    // this.thrusters[0].fxs[0].SetResource(r"base\\fx\\weapons\\throwables\\granades\\basic\\granade_trail_default.effect"); // idk
    // this.thrusters[1].fxs[0].SetResource(r"base\\fx\\quest\\q114\\q114_cars_dust_trail.effect");  // long trail of dust
    // this.thrusters[2].fxs[0].SetResource(r"base\\fx\\weapons\\bullet_trail_green.effect"); // idk
    // this.thrusters[3].fxs[0].SetResource(r"base\\fx\\weapons\\bullet_trail_simple.effect"); // nice bright orange trail
  }
}

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

public class BikeFlightConfiguration extends IFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    this.flightCameraOffset = new Vector3(0.0, 1.0, 0.5);

    if (Equals(this.type, FlightVehicleType.Corpo)) {
      ArrayPush(this.thrusters, new FlightThrusterFront().Create(vehicle, CreateCorpoThruster()));
      ArrayPush(this.thrusters, new FlightThrusterBack().Create(vehicle, CreateCorpoThruster()));
    } else {
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
    }

    this.thrusters[0].hasRetroThruster = false;
    this.thrusters[1].hasRetroThruster = false;

    for thruster in this.thrusters {
      ArrayPush(thruster.fxs, new RegularFlightThrusterFX().Create(thruster));
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