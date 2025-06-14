public class VehicleFlightDef extends BlackboardDefinition {

  public let IsActive: BlackboardID_Bool;
  public let Mode: BlackboardID_Int;
  public let IsUIActive: BlackboardID_Bool;
  public let Orientation: BlackboardID_Quat;
  public let Force: BlackboardID_Vector4;
  public let Torque: BlackboardID_Vector4;
  public let Position: BlackboardID_Vector4;
  public let Pitch: BlackboardID_Float;
  public let Roll: BlackboardID_Float;
  public let InMountedVehicleCombat: BlackboardID_Bool;

  public const func AutoCreateInSystem() -> Bool {
    return true;
  }
}

@addField(AllBlackboardDefinitions)
public let VehicleFlight: ref<VehicleFlightDef>;

@addField(PlayerStateMachineDef)
public let VehicleFlight: BlackboardID_Int;

// @addField(VehicleDef)
// public let IsFlightActive: BlackboardID_Bool;

// @addField(VehicleDef)
// public let FlightMode: BlackboardID_Int;

// @addField(VehicleDef)
// public let IsFlightUIActive: BlackboardID_Bool;

// @addField(VehicleDef)
// public let Orientation: BlackboardID_Quat;

// @addField(VehicleDef)
// public let Pitch: BlackboardID_Float;

// @addField(VehicleDef)
// public let Force: BlackboardID_Vector4;

// @addField(VehicleDef)
// public let Torque: BlackboardID_Vector4;

// @addField(VehicleDef)
// public let Position: BlackboardID_Vector4;