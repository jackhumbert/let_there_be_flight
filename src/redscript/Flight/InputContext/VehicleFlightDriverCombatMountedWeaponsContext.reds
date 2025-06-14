public class VehicleFlightDriverCombatMountedWeaponsContextDecisions extends VehicleFlightDriverCombatContextDecisions {

  protected const func CameraPerspectiveEnterCondition() -> Bool {
    return true;
  }

  protected const func IsAimingEnterCondition() -> Bool {
    return true;
  }

  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return Equals(driverCombatType, gamedataDriverCombatType.MountedWeapons);
  }
}

public class VehicleFlightDriverCombatMountedWeaponsContextEvents extends VehicleFlightDriverCombatContextEvents {

}
