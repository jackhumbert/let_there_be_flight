public class VehicleFlightDriverMountedWeaponsContextDecisions extends VehicleFlightContextDecisions {

  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return Equals(driverCombatType, gamedataDriverCombatType.MountedWeapons);
  }
}

public class VehicleFlightDriverMountedWeaponsContextEvents extends VehicleFlightContextEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnEnter(stateContext, scriptInterface);
    this.SetInMountedVehicleCombat(true);
  }

  protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnExit(stateContext, scriptInterface);
    this.SetInMountedVehicleCombat(false);
  }
}
