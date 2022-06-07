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

public class VehicleFlightUIActivationEvent extends vehicleFlightEvent {
  public let m_activate: Bool;
}


public class VehicleFlightModeChangeEvent extends vehicleFlightEvent {
  let mode: Int32;
}