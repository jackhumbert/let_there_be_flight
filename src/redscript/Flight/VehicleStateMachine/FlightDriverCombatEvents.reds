public class FlightDriverCombatEvents extends DriverCombatEvents {

  protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightDriverCombatEvents] OnEnter");
    super.OnEnter(stateContext, scriptInterface);
    this.OnEnterFlight(stateContext, scriptInterface);
  }

  public func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightDriverCombatEvents] OnExit");
    super.OnExit(stateContext, scriptInterface);
    this.OnExitFlight(stateContext, scriptInterface);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightDriverCombatEvents] OnForcedExit");
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
    FlightLog.Info("[FlightDriverCombatEvents] OnEnter");
    super.OnEnter(stateContext, scriptInterface);
    this.OnEnterFlight(stateContext, scriptInterface);
  }

  public func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightDriverCombatEvents] OnExit");
    super.OnExit(stateContext, scriptInterface);
    this.OnExitFlight(stateContext, scriptInterface);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightDriverCombatEvents] OnForcedExit");
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
    FlightLog.Info("[FlightDriverCombatEvents] OnEnter");
    super.OnEnter(stateContext, scriptInterface);
    this.OnEnterFlight(stateContext, scriptInterface);
  }

  public func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightDriverCombatEvents] OnExit");
    super.OnExit(stateContext, scriptInterface);
    this.OnExitFlight(stateContext, scriptInterface);
  }

  public func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    FlightLog.Info("[FlightDriverCombatEvents] OnForcedExit");
    this.OnForcedExitFlight(stateContext, scriptInterface);
    super.OnForcedExit(stateContext, scriptInterface);
  }

  public func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
    super.OnUpdate(timeDelta, stateContext, scriptInterface);
    this.OnUpdateFlight(timeDelta, stateContext, scriptInterface);
  }
}