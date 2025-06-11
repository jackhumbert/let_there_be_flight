public class VehicleFlightDriverCombatContextDecisions extends VehicleDriverCombatContextDecisions {

  private let m_callbackIDFlight: ref<CallbackHandle>;
  protected let m_psmVehicle: Int32;
  protected let m_flightEnabled: Bool;

  protected func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightDriverCombatContextDecisions] OnAttach");
    super.OnAttach(stateContext, scriptInterface);
    let allBlackboardDef: ref<AllBlackboardDefinitions>;
    let bb = FlightController.GetInstance().GetBlackboard();
    if IsDefined(bb) {
      allBlackboardDef = GetAllBlackboardDefs();

      this.m_callbackIDFlight = bb.RegisterListenerBool(allBlackboardDef.VehicleFlight.IsActive, this, n"OnVehicleFlightChanged");
      this.OnVehicleFlightChanged(bb.GetBool(allBlackboardDef.VehicleFlight.IsActive));

      FlightController.GetInstance().SetupMountedToCallback(scriptInterface.localBlackboard);
      (scriptInterface.owner as VehicleObject).ToggleFlightComponent(true);
    };
  }

  protected func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightDriverCombatContextDecisions] OnDetach");
    super.OnDetach(stateContext, scriptInterface);
    (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
    this.m_callbackIDFlight = null;
  }

  protected cb func OnVehicleStateChanged(value: Int32) -> Bool {
    FlightLog.Info("[VehicleFlightDriverCombatContextDecisions] OnVehicleStateChanged: " + ToString(value));
    this.m_psmVehicle = value;
    this.EnableOnEnterCondition(this.m_psmVehicle == EnumInt(gamePSMVehicle.DriverCombat) && this.m_flightEnabled);
  }

  protected cb func OnVehicleFlightChanged(value: Bool) -> Bool {
    FlightLog.Info("[VehicleFlightDriverCombatContextDecisions] OnVehicleFlightChanged: " + ToString(value));
    this.m_flightEnabled = value;
    this.EnableOnEnterCondition(this.m_psmVehicle == EnumInt(gamePSMVehicle.DriverCombat) && this.m_flightEnabled);
  }

  // protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  //   // FlightLog.Info("[VehicleFlightDriverCombatContextDecisions] EnterCondition");
  //   // if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleOnlyForward") {
  //   //   return false;
  //   // };
  //   if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"NoDriving") {
  //     return false;
  //   };
  //   return true;
  // }
  
  protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    if !this.CameraPerspectiveEnterCondition() || !this.IsAimingEnterCondition() || !this.DriverCombatTypeEnterCondition(stateContext) {
      return false;
    };
    if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"VehicleOnlyForward") {
      return false;
    };
    if StatusEffectSystem.ObjectHasStatusEffectWithTag(scriptInterface.executionOwner, n"NoDriving") {
      return false;
    };
    return true;
  }
}

public class VehicleFlightDriverCombatTPPContextDecisions extends VehicleFlightDriverCombatContextDecisions {

  private const func CameraPerspectiveEnterCondition() -> Bool {
    return this.m_inTpp;
  }
}

public class VehicleFlightDriverCombatAimContextDecisions extends VehicleFlightDriverCombatContextDecisions {

  protected const func IsAimingEnterCondition() -> Bool {
    return this.m_isAiming;
  }

  protected const func CameraPerspectiveEnterCondition() -> Bool {
    return true;
  }
}

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