
public class CarFlightConfiguration extends IFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);
    
    //#define PARTICLE R"(base\fx\vehicles\_damage\scratches\car_scratches_sparks.effect)"
    //#define PARTICLE R"(base\fx\_library\sparks\lib_sparks_impact_2-3m.effect)"
    //#define DECAL R"(base\fx\vehicles\_damage\scratches\car_scratches_sparks.effect)"
    //#define DECAL R"(base\fx\vehicles\_damage\scratches\car_scratches_concrete.effect)"
    //#define DECAL R"(base\fx\_library\fire\lib_burnt_mark_01_decal.effect)"

    // base\fx\vehicles\_wheels\tire_tracks\radial\blood\v_tire_track_blood_r_m_01.effect

    ArrayPush(this.thrusters, new FlightThrusterFL().Create(vehicle, this.CreateMesh(vehicle)));
    ArrayPush(this.thrusters, new FlightThrusterFR().Create(vehicle, this.CreateMesh(vehicle)));
    ArrayPush(this.thrusters, new FlightThrusterBR().Create(vehicle, this.CreateMesh(vehicle)));
    ArrayPush(this.thrusters, new FlightThrusterBL().Create(vehicle, this.CreateMesh(vehicle)));

    let retroThrusterTweak = vehicle.GetRecordID();
    TDBID.Append(retroThrusterTweak, t".hasRetroThruster");
    let hasRetroThruster = TweakDBInterface.GetBool(retroThrusterTweak, false);
      
    this.thrusters[0].hasRetroThruster = hasRetroThruster;
    this.thrusters[1].hasRetroThruster = hasRetroThruster;
    this.thrusters[2].hasRetroThruster = hasRetroThruster;
    this.thrusters[3].hasRetroThruster = hasRetroThruster;

    this.thrusters[0].wheelIndex = 0;
    this.thrusters[1].wheelIndex = 1;
    this.thrusters[2].wheelIndex = 2;
    this.thrusters[3].wheelIndex = 3;

    for thruster in this.thrusters {
      if thruster.hasRetroThruster {
        ArrayPush(thruster.fxs, new MainFlightThrusterFX().Create(thruster));
        ArrayPush(thruster.fxs, new SideFlightThrusterFX().Create(thruster));
      } else {
        ArrayPush(thruster.fxs, new RegularFlightThrusterFX().Create(thruster));
      }
      thruster.OnSetup(this.component);
    }

  }
}