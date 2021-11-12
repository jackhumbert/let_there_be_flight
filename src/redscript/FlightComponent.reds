public class FlightComponent extends ScriptableDC {

  public let m_interaction: ref<InteractionComponent>;

  public let m_healthStatPoolListener: ref<VehicleHealthStatPoolListener>;

  public let m_vehicleBlackboard: wref<IBlackboard>;

  protected final const func GetVehicle() -> wref<VehicleObject> {
    return this.GetEntity() as VehicleObject;
  }

  private final func OnGameAttach() -> Void {
    // FlightLog.Info("[FlightComponent] OnGameAttach: " + this.GetVehicle().GetDisplayName());
    this.m_interaction = this.FindComponentByName(n"interaction") as InteractionComponent;
    this.m_healthStatPoolListener = new VehicleHealthStatPoolListener();
    this.m_healthStatPoolListener.m_owner = this.GetVehicle();
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestRegisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    this.m_vehicleBlackboard = this.GetVehicle().GetBlackboard();
  }

  private final func OnGameDetach() -> Void {
    // FlightLog.Info("[FlightComponent] OnGameDetach: " + this.GetVehicle().GetDisplayName());
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestUnregisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
  }
  
  private final func RegisterInputListener() -> Void {
    let playerPuppet: ref<PlayerPuppet> = GameInstance.GetPlayerSystem(this.GetVehicle().GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet;
    playerPuppet.RegisterInputListener(this, n"VehicleInsideWheel");
    playerPuppet.RegisterInputListener(this, n"VehicleHorn");
  }

  private final func UnregisterInputListener() -> Void {
    let playerPuppet: ref<PlayerPuppet> = GameInstance.GetPlayerSystem(this.GetVehicle().GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet;
    if IsDefined(playerPuppet) {
      playerPuppet.UnregisterInputListener(this);
    };
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {

  }
}

@addField(VehicleObject)
private let m_flightComponent: wref<FlightComponent>;

@wrapMethod(VehicleObject)
protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
  EntityRequestComponentsInterface.RequestComponent(ri, n"flightComponent", n"FlightComponent", true);
  wrappedMethod(ri);
}

@wrapMethod(VehicleObject)
protected cb func OnTakeControl(ri: EntityResolveComponentsInterface) -> Bool {
  this.m_flightComponent = EntityResolveComponentsInterface.GetComponent(ri, n"flightComponent") as FlightComponent;
  this.m_flightComponent.Toggle(false);
  wrappedMethod(ri);
}

@addMethod(VehicleObject)
public const func GetFlightComponent() -> ref<FlightComponent> {
  return this.m_flightComponent;
}

@addMethod(VehicleObject)
public func ToggleFlightComponent(state: Bool) -> Void {
  this.m_flightComponent.Toggle(state);
}
