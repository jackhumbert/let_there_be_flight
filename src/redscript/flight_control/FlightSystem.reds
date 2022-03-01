
public class FlightSystem extends ScriptableSystem {
  
  public static func Get(gameInstance: GameInstance) -> ref<FlightSystem> {
    return GameInstance.GetScriptableSystemsContainer(gameInstance).Get(n"FlightSystem") as FlightSystem;
  }

  private func OnAttach() -> Void {
    FlightLog.Info("[FlightSystem] OnAttach");
  }

  private func OnDetach() -> Void {
    FlightLog.Info("[FlightSystem] OnDetach");
  }

  // private func IsSavingLocked() -> Bool {
  //   return FlightController.GetInstance().IsActive();
  // }
}