public abstract class vehicleFlightEvent extends Event {
  // public native let vehicle: ref<VehicleObject>;
}

// public native class vehicleFlightPhysicsUpdateEvent extends vehicleFlightEvent {
//   public native let timeDelta: Float;
// }

public class VehicleFlightActivationEvent extends vehicleFlightEvent {
}

public class VehicleFlightDeactivationEvent extends vehicleFlightEvent {
  let silent: Bool;
}