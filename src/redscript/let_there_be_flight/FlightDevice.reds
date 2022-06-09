public class FlightDevice extends GameObject {
  public let flightController: ref<FlightController>;

  protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
    super.OnRequestComponents(ri);
    EntityRequestComponentsInterface.RequestComponent(ri, n"updateComponent", n"UpdateComponent", false);
    FlightLog.Info("[FlightDevice] OnRequestComponents");
  }

  protected cb func OnTakeControl(ri: EntityResolveComponentsInterface) -> Bool {
    super.OnTakeControl(ri);
    FlightLog.Info("[FlightDevice] OnTakeControl");
  }

  protected cb func OnGameAttached() -> Bool {
    super.OnGameAttached();
    FlightLog.Info("[FlightDevice] OnGameAttached");
  }

  protected cb func OnDetach() -> Bool {
    super.OnDetach();
    FlightLog.Info("[FlightDevice] OnDetach");
  }
}