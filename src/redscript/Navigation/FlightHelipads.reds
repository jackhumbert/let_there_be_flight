// Landing pads on the map (docs/helipads.md): a pin for each AV landing pad from FlightNavMap's
// helipad list, shown once discovered (flown or walked within discoverDistance) or all of them.
// Pins are registered like the game's drop points (a static point of interest that shows on the
// world map and can be tracked); FlightHelipadMappinData marks them as ours, so the map, the
// minimap and the world markers give them their own icon and text, and the Autopilot lands on
// the pad when its pin is tracked. Registered pins live only for the session: they are
// registered again on each load, from the discovered pad ids saved with the game.

public class FlightHelipadMappinData extends MappinScriptData {
  public let index: Int32;
  public let id: String;
}

public class FlightHelipadTickCallback extends DelayCallback {
  public let system: wref<FlightHelipadSystem>;

  public func Call() -> Void {
    if IsDefined(this.system) {
      this.system.Tick();
    }
  }
}

public class FlightHelipadSystem extends ScriptableSystem {
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Landing-Pads")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Landing-Pads-Show")
  @runtimeProperty("ModSettings.description", "UI-Settings-Landing-Pads-Show-Description")
  public let showOnMap: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Landing-Pads")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Landing-Pads-Discovered")
  @runtimeProperty("ModSettings.description", "UI-Settings-Landing-Pads-Discovered-Description")
  public let onlyDiscovered: Bool = true;

  public let discoverDistance: Float = 150.0; // m (horizontal) from a pad that discovers it
  public let tickInterval: Float = 2.0;       // s between discovery checks and pin updates
  public let icon: CName = n"vehicle";        // mappin_icons.inkatlas part (there is no AV icon)
  public let lastMapPin: wref<BaseWorldMapMappinController>; // last of our pins drawn on the world map

  private persistent let discovered: array<CName>; // pad ids, saved with the game
  private let mappins: array<NewMappinID>;         // per pad index, valid where pinned is set
  private let pinned: array<Bool>;
  private let player: wref<PlayerPuppet>;
  private let ticking: Bool;

  public static func GetInstance(game: GameInstance) -> ref<FlightHelipadSystem> {
    return GameInstance.GetScriptableSystemsContainer(game).Get(n"FlightHelipadSystem") as FlightHelipadSystem;
  }

  // The pad behind a mappin, or -1 when it is not one of ours.
  public static func PadOf(mappin: wref<IMappin>) -> Int32 {
    if !IsDefined(mappin) {
      return -1;
    }
    let data = mappin.GetScriptData() as FlightHelipadMappinData;
    return IsDefined(data) ? data.index : -1;
  }

  private func OnAttach() -> Void {
    LTBF_RegisterListener(this);
  }

  private func OnDetach() -> Void {
    LTBF_UnregisterListener(this);
    this.UnregisterAll();
  }

  private func OnPlayerAttach(request: ref<PlayerAttachRequest>) -> Void {
    this.Start(GameInstance.GetPlayerSystem(request.owner.GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet);
  }

  // Called by OnPlayerAttach and by FlightController when it gets the player, whichever comes
  // first; later calls only update the player.
  public func Start(player: ref<PlayerPuppet>) -> Void {
    this.player = player;
    if this.ticking {
      return;
    }
    // a new session: pins from before are gone with the old mappin system state
    let none: NewMappinID;
    ArrayClear(this.mappins);
    ArrayClear(this.pinned);
    let i = 0;
    while i < FlightNavMap.GetHelipadCount() {
      ArrayPush(this.mappins, none);
      ArrayPush(this.pinned, false);
      i += 1;
    }
    FlightLog.Info("[Helipads] " + ToString(ArraySize(this.discovered)) + " of " + ToString(FlightNavMap.GetHelipadCount()) + " landing pads discovered");
    this.ticking = true;
    this.Schedule();
  }

  private func OnPlayerDetach(request: ref<PlayerDetachRequest>) -> Void {
    this.UnregisterAll();
    this.ticking = false;
  }

  private func Schedule() -> Void {
    let callback = new FlightHelipadTickCallback();
    callback.system = this;
    GameInstance.GetDelaySystem(this.GetGameInstance()).DelayCallback(callback, this.tickInterval, false);
  }

  // every tickInterval s of game time (paused with the game, e.g. on the load screen)
  public func Tick() -> Void {
    if !this.ticking || !IsDefined(this.player) {
      return;
    }
    this.Discover(this.player.GetWorldPosition());
    this.SyncPins();
    this.Schedule();
  }

  public func IsDiscovered(index: Int32) -> Bool {
    return ArrayContains(this.discovered, StringToName(FlightNavMap.GetHelipadId(index)));
  }

  private func Discover(position: Vector4) -> Void {
    for index in FlightNavMap.FindHelipadsInRange(position, this.discoverDistance) {
      let id = FlightNavMap.GetHelipadId(index);
      if !ArrayContains(this.discovered, StringToName(id)) {
        ArrayPush(this.discovered, StringToName(id));
        FlightLog.Info("[Helipads] discovered " + id + " (" + ToString(ArraySize(this.discovered)) + " of " + ToString(FlightNavMap.GetHelipadCount()) + ")");
      }
    }
  }

  // Register or remove pins so that exactly the pads the settings ask for have one.
  private func SyncPins() -> Void {
    let i = 0;
    while i < ArraySize(this.mappins) {
      let want = this.showOnMap && (!this.onlyDiscovered || this.IsDiscovered(i));
      if want && !this.pinned[i] {
        this.mappins[i] = this.RegisterPin(i);
        this.pinned[i] = true;
      } else if !want && this.pinned[i] {
        GameInstance.GetMappinSystem(this.GetGameInstance()).UnregisterMappin(this.mappins[i]);
        this.pinned[i] = false;
      }
      i += 1;
    }
  }

  private func RegisterPin(index: Int32) -> NewMappinID {
    let data: MappinData;
    data.mappinType = t"Mappins.DropPointStaticMappin";
    data.variant = gamedataMappinVariant.ServicePointDropPointVariant;
    data.active = true;
    let script = new FlightHelipadMappinData();
    script.index = index;
    script.id = FlightNavMap.GetHelipadId(index);
    data.scriptData = script;
    return GameInstance.GetMappinSystem(this.GetGameInstance()).RegisterMappin(data, FlightNavMap.GetHelipadPosition(index));
  }

  private func UnregisterAll() -> Void {
    let i = 0;
    while i < ArraySize(this.mappins) {
      if this.pinned[i] {
        GameInstance.GetMappinSystem(this.GetGameInstance()).UnregisterMappin(this.mappins[i]);
        this.pinned[i] = false;
      }
      i += 1;
    }
  }

  // The pad whose pin the player tracks on the map, or -1.
  public func GetTrackedPad() -> Int32 {
    let mappinSystem = GameInstance.GetMappinSystem(this.GetGameInstance());
    return FlightHelipadSystem.PadOf(mappinSystem.GetMappin(mappinSystem.GetManuallyTrackedMappinID()));
  }

  // Title for a pad's pin: its name, or "AV Landing Pad".
  public static func Title(index: Int32) -> String {
    let name = FlightNavMap.GetHelipadName(index);
    return StrLen(name) > 0 ? name : GetLocalizedTextByKey(n"LTBF-Landing-Pad");
  }
}

// --- the pins' look ------------------------------------------------------------------------------

@wrapMethod(BaseWorldMapMappinController)
protected func UpdateIcon() {
  wrappedMethod();
  if FlightHelipadSystem.PadOf(this.m_mappin) >= 0 {
    let system = FlightHelipadSystem.GetInstance(GetGameInstance());
    inkImageRef.SetTexturePart(this.iconWidget, system.icon);
    system.lastMapPin = this; // for tests from CET (select / track it through the map menu)
  }
}

@wrapMethod(MinimapPOIMappinController)
protected func UpdateIcon() {
  wrappedMethod();
  if FlightHelipadSystem.PadOf(this.m_mappin) >= 0 {
    inkImageRef.SetTexturePart(this.iconWidget, FlightHelipadSystem.GetInstance(GetGameInstance()).icon);
  }
}

@wrapMethod(QuestMappinController)
protected func UpdateIcon() {
  wrappedMethod();
  if FlightHelipadSystem.PadOf(this.m_mappin) >= 0 {
    inkImageRef.SetTexturePart(this.iconWidget, FlightHelipadSystem.GetInstance(GetGameInstance()).icon);
  }
}

@wrapMethod(WorldMapTooltipController)
public func SetData(const data: script_ref<WorldMapTooltipData>, menu: ref<WorldMapMenuGameController>) {
  wrappedMethod(data, menu);
  let pad = FlightHelipadSystem.PadOf(Deref(data).mappin);
  if pad >= 0 {
    inkTextRef.SetText(this.m_titleText, FlightHelipadSystem.Title(pad));
    inkTextRef.SetVisible(this.m_descText, true);
    inkTextRef.SetText(this.m_descText, GetLocalizedTextByKey(n"LTBF-Landing-Pad-Description"));
    inkWidgetRef.SetVisible(this.m_lineBreak, true);
  }
}
