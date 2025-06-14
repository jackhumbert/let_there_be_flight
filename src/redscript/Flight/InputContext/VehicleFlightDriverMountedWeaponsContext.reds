public class VehicleFlightDriverMountedWeaponsContextDecisions extends VehicleFlightContextDecisions {

  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return Equals(driverCombatType, gamedataDriverCombatType.MountedWeapons);
  }
}

public class VehicleFlightDriverMountedWeaponsContextEvents extends VehicleFlightContextEvents {

}
