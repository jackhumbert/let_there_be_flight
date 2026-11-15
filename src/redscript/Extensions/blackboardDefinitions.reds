public class VehicleFlightDef extends BlackboardDefinition {

  public let IsActive: BlackboardID_Bool;
  public let Mode: BlackboardID_Int;
  public let IsUIActive: BlackboardID_Bool;
  public let Pitch: BlackboardID_Float;
  public let Roll: BlackboardID_Float;
  public let InMountedVehicleCombat: BlackboardID_Bool;
  public let ExitedWhileActive: BlackboardID_Bool;

  public const func AutoCreateInSystem() -> Bool {
    return true;
  }
}

@addField(AllBlackboardDefinitions)
public let VehicleFlight: ref<VehicleFlightDef>;

@addField(PlayerStateMachineDef)
public let VehicleFlight: BlackboardID_Int;

