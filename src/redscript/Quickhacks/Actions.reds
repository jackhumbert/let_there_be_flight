public class FlightAction extends ScriptableDeviceAction {
  public let m_owner: wref<VehicleObject>;
  public let title: CName;
  public let description: CName;
  public let icon: TweakDBID;
}

public class FlightEnable extends FlightAction {
  public final func SetProperties() -> Void {
    this.SetObjectActionID(t"DeviceAction.FlightEnable");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.m_flightComponent.active;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[FlightEnable] CompleteAction");
    super.CompleteAction(gameInstance);
    if !this.m_owner.m_flightComponent.active {
      this.m_owner.UnsetPhysicsStates();
      this.m_owner.EndActions();
      this.m_owner.m_flightComponent.Activate(true);
    }
  }
}

public class FlightDisable extends FlightAction {
  public final func SetProperties() -> Void {
    this.SetObjectActionID(t"DeviceAction.FlightDisable");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && v.m_flightComponent.active;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[FlightDisable] CompleteAction");
    super.CompleteAction(gameInstance);
    if this.m_owner.m_flightComponent.active {
      this.m_owner.UnsetPhysicsStates();
      this.m_owner.EndActions();
      this.m_owner.m_flightComponent.Deactivate(true);
    }
  }
}

public class FlightMalfunction extends FlightAction {
  public final func SetProperties() -> Void {
    this.SetObjectActionID(t"DeviceAction.FlightMalfunction");
  }
}

public class DisableGravity extends FlightAction {
  public final func SetProperties() -> Void {
    this.SetObjectActionID(t"DeviceAction.DisableGravity");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && v.HasGravity();
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[DisableGravity] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.UnsetPhysicsStates();
    this.m_owner.EndActions();
    this.m_owner.EnableGravity(false);
  }
}

public class EnableGravity extends FlightAction {
  public final func SetProperties() -> Void {
    this.SetObjectActionID(t"DeviceAction.EnableGravity");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.HasGravity();
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[EnableGravity] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.EnableGravity(true);
  }
}

public class Funhouse extends FlightAction {
  public final func SetProperties() -> Void {
    this.SetObjectActionID(t"DeviceAction.Funhouse");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.bouncy;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[Funhouse] CompleteAction");
    super.CompleteAction(gameInstance);
  }
}

// Individual QH Actions

public func ActionFlightEnable(owner: ref<VehicleObject>) -> ref<FlightEnable> {
    let action: ref<FlightEnable> = new FlightEnable();
    action.clearanceLevel = DefaultActionsParametersHolder.GetTakeOverControl();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.AddDeviceName(owner.GetVehiclePS().GetDeviceName());
    action.CreateInteraction();
    action.CreateActionWidgetPackage();

    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionFlightDisable(owner: ref<VehicleObject>) -> ref<FlightDisable> {
    let action: ref<FlightDisable> = new FlightDisable();
    action.clearanceLevel = DefaultActionsParametersHolder.GetTakeOverControl();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.AddDeviceName(owner.GetVehiclePS().GetDeviceName());
    action.CreateInteraction();
    action.CreateActionWidgetPackage();

    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionFlightMalfunction(owner: ref<VehicleObject>) -> ref<FlightMalfunction> {
    let action: ref<FlightMalfunction> = new FlightMalfunction();
    action.clearanceLevel = DefaultActionsParametersHolder.GetTakeOverControl();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.AddDeviceName(owner.GetVehiclePS().GetDeviceName());
    action.CreateInteraction();
    action.CreateActionWidgetPackage();

    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionDisableGravity(owner: ref<VehicleObject>) -> ref<DisableGravity> {
    let action: ref<DisableGravity> = new DisableGravity();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetTakeOverControl();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.AddDeviceName(owner.GetVehiclePS().GetDeviceName());
    action.CreateInteraction();
    action.CreateActionWidgetPackage();

    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionEnableGravity(owner: ref<VehicleObject>) -> ref<EnableGravity> {
    let action: ref<EnableGravity> = new EnableGravity();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetTakeOverControl();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.AddDeviceName(owner.GetVehiclePS().GetDeviceName());
    action.CreateInteraction();
    action.CreateActionWidgetPackage();

    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionFunhouse(owner: ref<VehicleObject>) -> ref<Funhouse> {
    let action: ref<Funhouse> = new Funhouse();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetTakeOverControl();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.AddDeviceName(owner.GetVehiclePS().GetDeviceName());
    action.CreateActionWidgetPackage();
    action.CreateInteraction();

    action.SetExecutor(GetPlayer(owner.GetGame()));
    // action.ProcessRPGAction(owner.GetGame());
    return action;
}