
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
      // vehicle.AddComponent(thruster.meshComponent);
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