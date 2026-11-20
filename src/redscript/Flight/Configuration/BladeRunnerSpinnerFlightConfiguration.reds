// The Blade Runner Spinner add-on vehicle (Nexus Mods 7916, ctxrlsec). Its hull already has
// thrusters modelled in, so the wheel thrusters carry no mesh of their own, only the flame.
// GetConfigurationClass finds a class by the vehicle's appearance name, so these do nothing
// unless that mod is installed. All three appearances share one entity (br_spinner__basic_01.ent).
public abstract class BladeRunnerSpinnerFlightConfiguration extends IFlightConfiguration {
  public func OnSetup(vehicle: ref<VehicleObject>) {
    super.OnSetup(vehicle);

    ArrayPush(this.thrusters, new FlightThrusterFL().Create(vehicle, CreateEmptyThruster()));
    ArrayPush(this.thrusters, new FlightThrusterFR().Create(vehicle, CreateEmptyThruster()));
    ArrayPush(this.thrusters, new FlightThrusterBR().Create(vehicle, CreateEmptyThruster()));
    ArrayPush(this.thrusters, new FlightThrusterBL().Create(vehicle, CreateEmptyThruster()));

    // the rear flames sit under the hull's rear thrusters, up and inboard of the wheels
    this.thrusters[2].meshComponent.SetLocalPosition(new Vector4(0.120, 0.110, 0.110, 0.0));
    this.thrusters[3].meshComponent.SetLocalPosition(new Vector4(-0.120, 0.110, 0.110, 0.0));

    let i = 0;
    for thruster in this.thrusters {
      thruster.hasRetroThruster = false;
      thruster.wheelIndex = i;
      ArrayPush(thruster.fxs, new MainFlightThrusterFX().Create(thruster));
      thruster.OnSetup(this.component);
      i += 1;
    }
  }
}

public class FlightConfiguration_br_spinner__basic_black extends BladeRunnerSpinnerFlightConfiguration {}
public class FlightConfiguration_br_spinner__basic_black2 extends BladeRunnerSpinnerFlightConfiguration {}
public class FlightConfiguration_br_spinner__basic_black3 extends BladeRunnerSpinnerFlightConfiguration {}
