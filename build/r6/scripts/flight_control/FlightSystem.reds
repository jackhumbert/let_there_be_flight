public class FlightSystem extends ScriptableSystem {
  
  public static func Get(gameInstance: GameInstance) -> ref<FlightSystem> {
    return GameInstance.GetScriptableSystemsContainer(gameInstance).Get(n"FlightSystem") as FlightSystem;
  }

  private func OnAttach() -> Void {
    FlightLog.Info("[FlightSystem] OnAttach");
  }

  private final func OnPlayerAttach(request: ref<PlayerAttachRequest>) -> Void {
    FlightLog.Info("[FlightSystem] OnPlayerAttach");
  }

  private func OnDetach() -> Void {
    FlightLog.Info("[FlightSystem] OnDetach");
  }

  private func OnRestored(saveVersion: Int32, gameVersion: Int32) -> Void {
    FlightLog.Info("[FlightSystem] OnRestored");
  }

  // private func IsSavingLocked() -> Bool {
  //   return FlightController.GetInstance().IsActive();
  // }
}

