// The areas the game keeps the player out of (gameWorldBoundaryNode), which the Autopilot keeps out of too
// (src/red4ext/Navigation/Boundaries.hpp, docs/design/world-boundaries.md).
public native class FlightBoundaries extends IScriptable {
  // the quest facts that decide which areas are in force
  public native static func GetFacts() -> array<CName>;
  // reads those facts and lays the areas in force over the nav map; false without boundaries or a nav map
  public native static func Refresh() -> Bool;
  // read only, for checks: areas in force, the mask's key ("0" for none), the column at the position blocked
  public native static func GetAreasInForce() -> Int32;
  public native static func GetMaskKey() -> String;
  public native static func IsBlocked(position: Vector4) -> Bool;
}

// Follows the boundary gates' facts as uiInventoryScriptableSystem follows its own: a listener per fact once the
// player is attached (a save's facts are in by then), dropped on detach.
public class FlightBoundariesSystem extends ScriptableSystem {
  private let facts: array<CName>;
  private let listeners: array<Uint32>;

  private func OnPlayerAttach(request: ref<PlayerAttachRequest>) {
    this.Unregister();
    let quests = GameInstance.GetQuestsSystem(this.GetGameInstance());
    this.facts = FlightBoundaries.GetFacts();
    for fact in this.facts {
      ArrayPush(this.listeners, quests.RegisterListener(fact, this, n"OnBoundaryFactChanged"));
    }
    FlightBoundaries.Refresh();
  }

  private func OnDetach() {
    this.Unregister();
  }

  private func Unregister() {
    let quests = GameInstance.GetQuestsSystem(this.GetGameInstance());
    let i = 0;
    while i < ArraySize(this.listeners) {
      quests.UnregisterListener(this.facts[i], this.listeners[i]);
      i += 1;
    }
    ArrayClear(this.listeners);
  }

  private func OnBoundaryFactChanged(value: Int32) {
    FlightBoundaries.Refresh();
  }
}
