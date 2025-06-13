public class VehicleFlightContextDecisions extends InputContextTransitionDecisions {

  protected let m_stateCallbackID: ref<CallbackHandle>;
  protected let m_flightCallbackID: ref<CallbackHandle>;

  protected let m_psmVehicle: Int32;
  protected let m_isFlying: Bool;

  protected let m_context: ref<StateContext>;

  protected func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleFlightContextDecisions] OnAttach");
    this.m_context = stateContext;
    let allBlackboardDef = GetAllBlackboardDefs();
      
    if IsDefined(scriptInterface.localBlackboard) {
      this.m_stateCallbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this, n"OnVehicleStateChanged");
      this.OnVehicleStateChanged(scriptInterface.localBlackboard.GetInt(allBlackboardDef.PlayerStateMachine.Vehicle));
      
      FlightController.GetInstance().SetupMountedToCallback(scriptInterface.localBlackboard);
    }
      
    let bb = FlightController.GetInstance().GetBlackboard();

    if IsDefined(bb) {
      this.m_flightCallbackID = bb.RegisterListenerBool(allBlackboardDef.VehicleFlight.IsActive, this, n"OnVehicleFlightChanged");
      this.OnVehicleFlightChanged(bb.GetBool(allBlackboardDef.VehicleFlight.IsActive));
    };

    (scriptInterface.owner as VehicleObject).ToggleFlightComponent(true);
  }

  protected func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);

    let allBlackboardDef: ref<AllBlackboardDefinitions> = GetAllBlackboardDefs();

    if IsDefined(scriptInterface.localBlackboard) {
      scriptInterface.localBlackboard.UnregisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this.m_stateCallbackID);
    };

    let bb = FlightController.GetInstance().GetBlackboard();

    if IsDefined(bb) {
      scriptInterface.localBlackboard.UnregisterListenerBool(allBlackboardDef.VehicleFlight.IsActive, this.m_flightCallbackID);
    };

    this.m_stateCallbackID = null;
    this.m_flightCallbackID = null;
  }

  protected const func GetContextName() -> String {
    return "VehicleFlight";
  }

  protected func UpdateEnterConditionForFlight() -> Void {
    let value = this.m_psmVehicle == EnumInt(gamePSMVehicle.Driving) && this.m_isFlying;
    // let value = this.m_isFlying;
    this.EnableOnEnterCondition(value);
    // FlightLog.Info("[InputContext] " + this.GetContextName() + " EnterCondition: " + ToString(value));
    
    // let inputState = this.m_context.GetStateMachineCurrentState(n"InputContext");
    // FlightLog.Info("[InputContext] Current: " + NameToString(inputState));
  }

  protected cb func OnVehicleStateChanged(value: Int32) -> Bool {
    this.m_psmVehicle = value;
    this.UpdateEnterConditionForFlight();
  }

  protected cb func OnVehicleFlightChanged(value: Bool) -> Bool {
    this.m_isFlying = value;
    this.UpdateEnterConditionForFlight();
  }

  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return NotEquals(driverCombatType, gamedataDriverCombatType.MountedWeapons);
  }

  protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    if !this.DriverCombatTypeEnterCondition(stateContext) {
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

public class VehicleFlightDriverMountedWeaponsContextDecisions extends VehicleFlightContextDecisions {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverMountedWeapons";
  }

  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return Equals(driverCombatType, gamedataDriverCombatType.MountedWeapons);
  }
}

public class VehicleFlightDriverCombatContextDecisions extends VehicleFlightContextDecisions {

  protected let m_tppCallbackID: ref<CallbackHandle>;
  protected let m_upperBodyCallbackID: ref<CallbackHandle>;

  protected let m_inTpp: Bool;
  protected let m_isAiming: Bool;

  protected func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnAttach(stateContext, scriptInterface);

    let allBlackboardDef = GetAllBlackboardDefs();
      
    if IsDefined(scriptInterface.localBlackboard) {
      this.m_tppCallbackID = scriptInterface.localBlackboard.RegisterListenerBool(allBlackboardDef.PlayerStateMachine.IsDriverCombatInTPP, this, n"OnVehiclePerspectiveChanged", true);
      this.OnVehiclePerspectiveChanged(scriptInterface.localBlackboard.GetBool(allBlackboardDef.PlayerStateMachine.IsDriverCombatInTPP));

      this.m_upperBodyCallbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.UpperBody, this, n"OnUpperBodyStateChanged", true);
      this.OnUpperBodyStateChanged(scriptInterface.localBlackboard.GetInt(allBlackboardDef.PlayerStateMachine.UpperBody));
    }
  }

  protected func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnDetach(stateContext, scriptInterface);

    let allBlackboardDef: ref<AllBlackboardDefinitions> = GetAllBlackboardDefs();

    if IsDefined(scriptInterface.localBlackboard) {
      scriptInterface.localBlackboard.UnregisterListenerBool(allBlackboardDef.PlayerStateMachine.IsDriverCombatInTPP, this.m_tppCallbackID);
      scriptInterface.localBlackboard.UnregisterListenerInt(allBlackboardDef.PlayerStateMachine.UpperBody, this.m_upperBodyCallbackID);
    };

    this.m_tppCallbackID = null;
    this.m_upperBodyCallbackID = null;
  }

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverCombat";
  }

  protected func UpdateEnterConditionForFlight() -> Void {
    let value = this.m_psmVehicle == EnumInt(gamePSMVehicle.DriverCombat) && this.m_isFlying;
    // let value = this.m_isFlying;
    this.EnableOnEnterCondition(value);
    // FlightLog.Info("[InputContext] " + this.GetContextName() + " EnterCondition: " + ToString(value));
  }

  protected cb func OnVehiclePerspectiveChanged(value: Bool) -> Bool {
    this.m_inTpp = value;
  }

  protected cb func OnUpperBodyStateChanged(value: Int32) -> Bool {
    this.m_isAiming = value == EnumInt(gamePSMUpperBodyStates.Aim);
  }

  protected const func CameraPerspectiveEnterCondition() -> Bool {
    return !this.m_inTpp;
  }

  protected const func IsAimingEnterCondition() -> Bool {
    return !this.m_isAiming;
  }
  
  protected const func DriverCombatTypeEnterCondition(const stateContext: ref<StateContext>) -> Bool {
    let driverCombatType: gamedataDriverCombatType = this.GetDriverCombatType(stateContext);
    return Equals(driverCombatType, gamedataDriverCombatType.Standard) || Equals(driverCombatType, gamedataDriverCombatType.Doors);
  }

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
    // FlightLog.Info("[InputContext] " + this.GetContextName() + " Entering");
    return true;
  }
}

public class VehicleFlightDriverCombatTPPContextDecisions extends VehicleFlightDriverCombatContextDecisions {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverCombatTPP";
  }

  protected const func CameraPerspectiveEnterCondition() -> Bool {
    return this.m_inTpp;
  }
}

public class VehicleFlightDriverCombatAimContextDecisions extends VehicleFlightDriverCombatContextDecisions {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverCombatAim";
  }

  protected const func IsAimingEnterCondition() -> Bool {
    return this.m_isAiming;
  }

  protected const func CameraPerspectiveEnterCondition() -> Bool {
    return true;
  }
}

public class VehicleFlightDriverCombatMountedWeaponsContextDecisions extends VehicleFlightDriverCombatContextDecisions {

  protected const func GetContextName() -> String {
    return "VehicleFlightDriverCombatMountedWeapons";
  }

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