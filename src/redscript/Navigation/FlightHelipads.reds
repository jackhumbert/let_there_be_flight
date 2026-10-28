// Landing pads on the map (docs/helipads.md): a pin for each AV landing pad from FlightNavMap's
// helipad list, shown once discovered (flown or walked within discoverDistance) or all of them.
// Pins are registered with the drop point variant and our own mappin type (MappinSystem.RegisterMappin
// makes a RuntimeMappin, which shows on the world map and can be tracked; it always reads as
// visited, so it never plays the map's "new" pulse). FlightHelipadMappinData marks them as ours,
// so the map, the minimap and the world markers give them their own icon (UIIcon.LTBF_LandingPad),
// text and spawn range (MappinUISpawnProfile.LTBF_LandingPad, src/tweaks/helipads.yaml), and the
// Autopilot lands on the pad when its pin is tracked. Registered pins live only for the session:
// they are registered again on each load, from the discovered pad ids saved with the game.
// How the game handles these pins (from the 2.5 binary): docs/helipads.md, section 5.
// The first touchdown on each pad (in a vehicle, inside its radius, at deck height, nearly
// stopped) gives a one-off supply drop of eddies and crafting components, also saved.

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
  public let touchdownHeight: Float = 3.0;    // m from the deck to the car's position
  public let touchdownSpeed: Float = 1.5;     // m/s
  public let supplyMoney: Int32 = 750;        // eddies on the first touchdown on a pad
  public let supplyCommon: Int32 = 8;         // common crafting components
  public let supplyUncommon: Int32 = 3;       // uncommon crafting components
  public let lastMapPin: wref<BaseWorldMapMappinController>; // last of our pins drawn on the world map

  private persistent let discovered: array<CName>; // pad ids, saved with the game
  private persistent let supplied: array<CName>;   // pad ids whose supplies were collected
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
    this.CheckTouchdown();
    this.SyncPins();
    this.Schedule();
  }

  public func IsDiscovered(index: Int32) -> Bool {
    return ArrayContains(this.discovered, StringToName(FlightNavMap.GetHelipadId(index)));
  }

  private func Discover(position: Vector4) -> Void {
    let found = 0;
    for index in FlightNavMap.FindHelipadsInRange(position, this.discoverDistance) {
      let id = FlightNavMap.GetHelipadId(index);
      if !ArrayContains(this.discovered, StringToName(id)) {
        ArrayPush(this.discovered, StringToName(id));
        found += 1;
        FlightLog.Info("[Helipads] discovered " + id + " (" + ToString(ArraySize(this.discovered)) + " of " + ToString(FlightNavMap.GetHelipadCount()) + ")");
      }
    }
    if found > 0 && this.showOnMap {
      this.Notify(found == 1
        ? GetLocalizedTextByKey(n"LTBF-Landing-Pad-Discovered")
        : StrReplace(GetLocalizedTextByKey(n"LTBF-Landing-Pads-Discovered"), "{count}", ToString(found)),
        GetLocalizedTextByKey(n"LTBF-Landing-Pad-Discovered-Description"));
    }
  }

  // The pad the player's vehicle stands on: inside its radius, within touchdownHeight of the deck
  // and slower than touchdownSpeed; -1 otherwise.
  public func GetTouchdownPad() -> Int32 {
    let vehicle = GetMountedVehicle(this.player);
    if !IsDefined(vehicle) || Vector4.Length(vehicle.GetLinearVelocity()) > this.touchdownSpeed {
      return -1;
    }
    let position = vehicle.GetWorldPosition();
    let pad = FlightNavMap.FindNearestHelipad(position, 50.0);
    if pad < 0 {
      return -1;
    }
    let deck = FlightNavMap.GetHelipadPosition(pad);
    if Vector4.Length2D(position - deck) > FlightNavMap.GetHelipadRadius(pad) || AbsF(position.Z - deck.Z) > this.touchdownHeight {
      return -1;
    }
    return pad;
  }

  public func IsSupplied(index: Int32) -> Bool {
    return ArrayContains(this.supplied, StringToName(FlightNavMap.GetHelipadId(index)));
  }

  private func CheckTouchdown() -> Void {
    let pad = this.GetTouchdownPad();
    if pad < 0 || this.IsSupplied(pad) {
      return;
    }
    let id = FlightNavMap.GetHelipadId(pad);
    ArrayPush(this.supplied, StringToName(id));
    let transactions = GameInstance.GetTransactionSystem(this.GetGameInstance());
    transactions.GiveItem(this.player, MarketSystem.Money(), this.supplyMoney);
    transactions.GiveItem(this.player, ItemID.FromTDBID(t"Items.CommonMaterial1"), this.supplyCommon);
    transactions.GiveItem(this.player, ItemID.FromTDBID(t"Items.UncommonMaterial1"), this.supplyUncommon);
    FlightLog.Info("[Helipads] supplies at " + id + " (" + ToString(ArraySize(this.supplied)) + " of " + ToString(FlightNavMap.GetHelipadCount()) + ")");
    this.Notify(GetLocalizedTextByKey(n"LTBF-Landing-Pad-Supplies"),
      StrReplace(GetLocalizedTextByKey(n"LTBF-Landing-Pad-Supplies-Description"), "{money}", ToString(this.supplyMoney)));
  }

  // A toast like the game's location reveals, title only. It plays the warning animation and the
  // chip malfunction jingle, so it reads as an alert; the quest-update style notification meant to
  // replace it (title plus `text`) did not draw in game yet, see docs/helipads.md section 6.
  private func Notify(title: String, text: String) -> Void {
    let msg: SimpleScreenMessage;
    msg.isShown = true;
    msg.duration = 3.0;
    msg.message = title;
    msg.type = SimpleMessageType.Reveal;
    GameInstance.GetBlackboardSystem(this.GetGameInstance()).Get(GetAllBlackboardDefs().UI_Notifications).SetVariant(GetAllBlackboardDefs().UI_Notifications.WarningMessage, ToVariant(msg), true);
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
    data.mappinType = t"Mappins.LTBF_LandingPadMappin"; // src/tweaks/helipads.yaml
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

  // The pins' icon (src/tweaks/helipads.yaml).
  public static func Icon() -> wref<UIIcon_Record> {
    return TweakDBInterface.GetUIIconRecord(t"UIIcon.LTBF_LandingPad");
  }

  // Title for a pad's pin: its name, or "AV Landing Pad".
  public static func Title(index: Int32) -> String {
    let name = FlightNavMap.GetHelipadName(index);
    return StrLen(name) > 0 ? name : GetLocalizedTextByKey(n"LTBF-Landing-Pad");
  }
}

// --- the pins' look ------------------------------------------------------------------------------

// Spawn range on the minimap and as in-world markers: the containers ask this once per pin (and
// again when it is tracked or untracked); the same widget and runtime profile as a drop point,
// with our longer spawn profile.
@wrapMethod(MinimapContainerController)
public func CreateMappinUIProfile(mappin: wref<IMappin>, mappinVariant: gamedataMappinVariant, customData: ref<MappinControllerCustomData>) -> MappinUIProfile {
  if FlightHelipadSystem.PadOf(mappin) >= 0 {
    return MappinUIProfile.Create(r"base\\gameplay\\gui\\widgets\\minimap\\minimap_poi_mappin.inkwidget", t"MappinUISpawnProfile.LTBF_LandingPad", t"MinimapMappinUIProfile.DropPoint");
  }
  return wrappedMethod(mappin, mappinVariant, customData);
}

@wrapMethod(WorldMappinsContainerController)
public func CreateMappinUIProfile(mappin: wref<IMappin>, mappinVariant: gamedataMappinVariant, customData: ref<MappinControllerCustomData>) -> MappinUIProfile {
  if FlightHelipadSystem.PadOf(mappin) >= 0 {
    return MappinUIProfile.Create(r"base\\gameplay\\gui\\widgets\\mappins\\quest\\default_mappin.inkwidget", t"MappinUISpawnProfile.LTBF_LandingPad", t"WorldMappinUIProfile.DropPoint");
  }
  return wrappedMethod(mappin, mappinVariant, customData);
}

// Icon: SetTexture with the UIIcon record sets our atlas and part together (the TweakDBID overload
// goes through an async image request that left a blank white square). Each spawn gets its own
// widget (no pooling, checked in the binary), so this never leaks to a vanilla pin.
@wrapMethod(BaseWorldMapMappinController)
protected func UpdateIcon() {
  wrappedMethod();
  if FlightHelipadSystem.PadOf(this.m_mappin) >= 0 {
    this.SetTexture(this.iconWidget, FlightHelipadSystem.Icon());
    FlightHelipadSystem.GetInstance(GetGameInstance()).lastMapPin = this; // test handle
  }
}

@wrapMethod(MinimapPOIMappinController)
protected func UpdateIcon() {
  wrappedMethod();
  if FlightHelipadSystem.PadOf(this.m_mappin) >= 0 {
    this.SetTexture(this.iconWidget, FlightHelipadSystem.Icon());
  }
}

@wrapMethod(QuestMappinController)
protected func UpdateIcon() {
  wrappedMethod();
  if FlightHelipadSystem.PadOf(this.m_mappin) >= 0 {
    this.SetTexture(this.iconWidget, FlightHelipadSystem.Icon());
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