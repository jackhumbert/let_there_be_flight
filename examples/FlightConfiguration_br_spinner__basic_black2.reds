@if(ModuleExists("LetThereBeFlight")) 
public class FlightConfiguration_br_spinner__basic_black2 extends IFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    ArrayPush(this.thrusters, new FlightThrusterFL().Create(vehicle, CreateEmptyThruster()));
    ArrayPush(this.thrusters, new FlightThrusterFR().Create(vehicle, CreateEmptyThruster()));
    ArrayPush(this.thrusters, new FlightThrusterBR().Create(vehicle, CreateEmptyThruster()));
    ArrayPush(this.thrusters, new FlightThrusterBL().Create(vehicle, CreateEmptyThruster()));
    
    this.thrusters[0].hasRetroThruster = false;
    this.thrusters[1].hasRetroThruster = false;
    this.thrusters[2].hasRetroThruster = false;
    this.thrusters[3].hasRetroThruster = false;

    this.thrusters[0].wheelIndex = 0;
    this.thrusters[1].wheelIndex = 1;
    this.thrusters[2].wheelIndex = 2;
    this.thrusters[3].wheelIndex = 3;

    for thruster in this.thrusters {
      ArrayPush(thruster.fxs, new MainFlightThrusterFX().Create(thruster));
      vehicle.AddComponent(thruster.meshComponent);
      thruster.OnSetup(this.component);
    }
  }

  public func OnActivation() {
    FlightLog.Info("[FlightConfiguration] Activating Custom Spinner Configuration");
  }

  public func OnDeactivation() {
    
  }
}