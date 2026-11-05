// Cars with more than three axles, a thruster on every wheel (FlightConfiguration.cpp picks these from the wheel slots
// the appearance binds; tools/flightsim/thruster_slots.py checks every vanilla rig against them).

// front, front B, back B and back: the MT28 coach
public class EightWheelCarFlightConfiguration extends CarFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);
    this.AddAxleThruster(vehicle, new FlightThrusterFLB().Create(vehicle, this.CreateMesh(vehicle)));
    this.AddAxleThruster(vehicle, new FlightThrusterFRB().Create(vehicle, this.CreateMesh(vehicle)));
    this.AddAxleThruster(vehicle, new FlightThrusterBLB().Create(vehicle, this.CreateMesh(vehicle)));
    this.AddAxleThruster(vehicle, new FlightThrusterBRB().Create(vehicle, this.CreateMesh(vehicle)));
  }

  // as SixWheelCarFlightConfiguration sets up its extra pair: the record's retro thruster flag, the effects, OnSetup
  protected func AddAxleThruster(vehicle: ref<VehicleObject>, thruster: ref<IFlightThruster>) {
    ArrayPush(this.thrusters, thruster);
    let retroThrusterTweak = vehicle.GetRecordID();
    TDBID.Append(retroThrusterTweak, t".hasRetroThruster");
    thruster.hasRetroThruster = TweakDBInterface.GetBool(retroThrusterTweak, false);
    if thruster.hasRetroThruster {
      ArrayPush(thruster.fxs, new MainFlightThrusterFX().Create(thruster));
      ArrayPush(thruster.fxs, new SideFlightThrusterFX().Create(thruster));
    } else {
      ArrayPush(thruster.fxs, new RegularFlightThrusterFX().Create(thruster));
    }
    thruster.OnSetup(this.component);
  }
}

// and the back C and D axles too: the Bratsk U4020 extended (six axles)
public class TwelveWheelCarFlightConfiguration extends EightWheelCarFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);
    this.AddAxleThruster(vehicle, new FlightThrusterBLC().Create(vehicle, this.CreateMesh(vehicle)));
    this.AddAxleThruster(vehicle, new FlightThrusterBRC().Create(vehicle, this.CreateMesh(vehicle)));
    this.AddAxleThruster(vehicle, new FlightThrusterBLD().Create(vehicle, this.CreateMesh(vehicle)));
    this.AddAxleThruster(vehicle, new FlightThrusterBRD().Create(vehicle, this.CreateMesh(vehicle)));
  }
}
