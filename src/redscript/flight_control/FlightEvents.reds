public abstract class VehicleFlightEvent extends Event {
  public let vehicle: ref<VehicleObject>;
}

public class VehicleFlightActivationEvent extends VehicleFlightEvent {
  public let activated: Bool;
}