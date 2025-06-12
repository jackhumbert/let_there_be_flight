
public class FlightEvents extends VehicleEventsTransition {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] Flight Enter");
    super.OnEnter(stateContext, scriptInterface);
    this.OnEnterFlight(stateContext, scriptInterface);
  }

  public final func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] Flight Exit");
    this.OnExitFlight(stateContext, scriptInterface);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] Flight ForcedExit");
    this.OnForcedExitFlight(stateContext, scriptInterface);
    super.OnForcedExit(stateContext, scriptInterface);
  }

  public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    this.OnUpdateFlight(timeDelta, stateContext, scriptInterface);
  }
}

public class FlightDriverCombatEvents extends DriverCombatEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombat Enter");
    super.OnEnter(stateContext, scriptInterface);
    this.OnEnterFlight(stateContext, scriptInterface);
  }

  public func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombat Exit");
    super.OnExit(stateContext, scriptInterface);
    this.OnExitFlight(stateContext, scriptInterface);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombat ForcedExit");
    this.OnForcedExitFlight(stateContext, scriptInterface);
    super.OnForcedExit(stateContext, scriptInterface);
  }

  public func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnUpdate(timeDelta, stateContext, scriptInterface);
    this.OnUpdateFlight(timeDelta, stateContext, scriptInterface);
  }
}

public class FlightDriverCombatFirearmsEvents extends DriverCombatFirearmsEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombatFirearmsEvents Enter");
    super.OnEnter(stateContext, scriptInterface);
    this.OnEnterFlight(stateContext, scriptInterface);
  }

  public func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombatFirearmsEvents Exit");
    super.OnExit(stateContext, scriptInterface);
    this.OnExitFlight(stateContext, scriptInterface);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombatFirearmsEvents ForcedExit");
    this.OnForcedExitFlight(stateContext, scriptInterface);
    super.OnForcedExit(stateContext, scriptInterface);
  }

  public func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnUpdate(timeDelta, stateContext, scriptInterface);
    this.OnUpdateFlight(timeDelta, stateContext, scriptInterface);
  }
}

public class FlightDriverCombatMountedWeaponsEvents extends DriverCombatMountedWeaponsEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombatMountedWeaponsEvents Enter");
    super.OnEnter(stateContext, scriptInterface);
    this.OnEnterFlight(stateContext, scriptInterface);
  }

  public func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombatMountedWeaponsEvents Exit");
    super.OnExit(stateContext, scriptInterface);
    this.OnExitFlight(stateContext, scriptInterface);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[VehicleStateMachine] FlightDriverCombatMountedWeaponsEvents ForcedExit");
    this.OnForcedExitFlight(stateContext, scriptInterface);
    super.OnForcedExit(stateContext, scriptInterface);
  }

  public func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnUpdate(timeDelta, stateContext, scriptInterface);
    this.OnUpdateFlight(timeDelta, stateContext, scriptInterface);
  }
}