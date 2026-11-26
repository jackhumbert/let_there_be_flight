// Pad summon (docs/design/vehicle-summon.md): with V on or near a landing pad, a summoned
// vehicle takes off and flies to the pad with the Autopilot, and lands on it, instead of
// vanilla's road summon. Away from pads vanilla's own summon runs, and the fallback below only
// steps in when it cannot bring the car to V (it refuses at a pad without a summon spot - no
// car, no crash).
//
// Why the spawn is ours (docs, section 4): the vanilla summon's spawn-point search is tied to
// road-side summon spots, so at a pad it refuses; the record-path summon native crashes the
// game on 2.5 (a null function object read at the call); and the garage's
// summon-to-transform path only feeds the garage machine, which drives on traffic lanes - none
// at a pad. The spawn is instead the VehicleSystem's own record spawn,
// game::VehicleSystem::RequestNewVehicle (FlightCarSummon.cpp calls it) with a transform we
// pick: no garage machine, no traffic slots; the Autopilot flies the car in from wherever the
// spawn put it.
//
// The player flow (the dpad tap or the wheel's summon item) is
// QuickSlotsManager.SummonActiveVehicle: its wrap takes over when the pad gate passes, with the
// record of the garage's active vehicle. The tick (2 s, paused with the game) then finds the spawned vehicle (the summon
// blackboard's SummonedVehicleEntityID, else the record's vehicle nearest the spawn spot),
// engages the Autopilot on it with the pad as this flight's destination, watches the flight and
// lands it; the completion closes vanilla's state (Arrived, the SummonFinishedEvent to the
// vehicle's component for its arrival effects) when the game has not already.
//
// The fallback (docs/design/vehicle-summon.md section 5): anywhere else, vanilla's summon runs and
// the tick watches it. When it cannot bring the car to V, the car flies the rest of the way and
// lands next to V on the ground (the Autopilot's own landing spot search, kept clear of V):
//   1. no car at all: the summon was pressed (not on cooldown) and nothing came of it - the
//      garage found no place to spawn it (a high roof); spawned here as at a pad, out from V
//   2. PathfindingFailed: the garage spawned the car but found no traffic lane to drive it on
//   3. EnRoute, but the car has not got closer to V for stallTime
//   4. Arrived, but far from V: the drive's target is the road nearest V, a level below V on a roof
//
// Always fly (the alwaysFlySummon setting, off by default): every summon away from a pad flies in
// and lands next to V without waiting for vanilla to fail. The car already out is taken over where
// it is; with none out the record is spawned out from V as in case 1.

public class FlightSummonTickCallback extends DelayCallback {
  public let system: wref<FlightSummonSystem>;

  public func Call() -> Void {
    if IsDefined(this.system) {
      this.system.Tick();
    }
  }
}

public class FlightSummonSystem extends ScriptableSystem {
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Landing-Pads")
  @runtimeProperty("ModSettings.category.order", "6")
  @runtimeProperty("ModSettings.order", "3")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Pad-Summon")
  @runtimeProperty("ModSettings.description", "UI-Settings-Pad-Summon-Description")
  public let padSummonEnabled: Bool = true;

  // a developer value, not a Mod Setting (docs/design/flight-modes.md, Developer values)
  public let padSummonRange: Float = 60.0;    // m beyond a pad's radius V counts as near it

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Landing-Pads")
  @runtimeProperty("ModSettings.category.order", "6")
  @runtimeProperty("ModSettings.order", "4")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Summon-Fallback")
  @runtimeProperty("ModSettings.description", "UI-Settings-Summon-Fallback-Description")
  public let summonFallbackEnabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Landing-Pads")
  @runtimeProperty("ModSettings.category.order", "6")
  @runtimeProperty("ModSettings.order", "5")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Summon-Always-Fly")
  @runtimeProperty("ModSettings.description", "UI-Settings-Summon-Always-Fly-Description")
  public let alwaysFlySummon: Bool = false;

  // tuning, not in Mod Settings
  public let tickInterval: Float = 2.0;       // s between ticks (paused with the game)
  public let spawnWaitTicks: Int32 = 5;       // ticks (10 s) to look for the spawned vehicle
  public let spawnDistance: Float = 120.0;    // m out from the pad the vehicle spawns
  public let spawnHeight: Float = 20.0;       // m over the deck it spawns
  public let planningTime: Float = 30.0;      // s of the autopilot without a route before giving up
  public let flightTime: Float = 180.0;       // s of the whole summon flight at most
  public let deckMaxDrop: Float = 15.0;       // m of deck height difference for V counting as near
  public let noSpawnTime: Float = 4.0;        // s after a summon press with nothing from the garage (case 1)
  public let stallTime: Float = 12.0;         // s EnRoute without getting closer to V (case 3)
  public let stallProgress: Float = 5.0;      // m closer that counts as progress
  public let arrivedLevel: Float = 6.0;       // m of height between V and an arrived car: another level (case 4)
  public let pressWatchTime: Float = 60.0;    // s the car of V's press is watched once the press has an answer (not counting EnRoute)
  public let arrivedFar: Float = 80.0;        // m (2D) from V an arrived car is too far (vanilla stops 40 m off on a street)
  public let groundKeepClear: Float = 4.0;    // m (2D) from V a landing car's centre stays (a car is 2.5 m half-long)

  // the flight this system owns (empty between summons); public for tests (ltbf_bridge.reds)
  public let summoning: Bool;
  public let engaged: Bool;
  public let pad: Int32;                      // the pad being flown to, -1 when none
  public let target: Vector4;                 // with no pad: where V stood, the car lands next to it
  public let reason: String;                  // why this summon flies: "pad", "always fly" or the fallback's case
  private let ownSpawn: Bool;                 // the car is this system's spawn, not one vanilla put somewhere
  public let vehicle: wref<VehicleObject>;
  private let ticking: Bool;
  private let vehicleID: EntityID;
  private let lastSpawnID: EntityID;          // the car this system last spawned, for a second press
  private let recordID: TweakDBID;            // the record of the vehicle this summon spawned, none for vanilla's car
  public let padSpot: Vector4;                // where the transform summon put the vehicle
  private let awaitingSpawnTicks: Int32;
  private let pendingTicks: Int32;
  private let tries: Int32;
  private let flightTimer: Float;
  private let planningTimer: Float;
  // the vanilla summon being watched (the fallback)
  private let pressPending: Bool;             // a summon press whose outcome is not known yet
  private let pressAge: Float;
  private let pressRecord: TweakDBID;
  private let pressState: Int32;
  private let pressID: EntityID;
  private let pressPos: Vector4;
  private let pressed: Bool;                  // V's own press is being watched: cases 2 to 4 are for it only
  private let pressedID: EntityID;            // the car the garage answered that press with
  private let pressedAge: Float;              // s since that answer, not counting EnRoute
  private let watchID: EntityID;              // the car seen with this summon, none between summons
  private let watchBest: Float;               // its closest yet to V, m
  private let watchStall: Float;              // s since it last got stallProgress closer
  private let lastState: Int32;

  public static func GetInstance(game: GameInstance) -> ref<FlightSummonSystem> {
    return GameInstance.GetScriptableSystemsContainer(game).Get(n"FlightSummonSystem") as FlightSummonSystem;
  }

  private func OnAttach() -> Void {
    LTBF_RegisterListener(this);
    this.Reset();
    this.ticking = true;
    this.Schedule();
  }

  private func OnDetach() -> Void {
    LTBF_UnregisterListener(this);
    this.ticking = false;
  }

  private func Schedule() -> Void {
    if !this.ticking {
      return;
    }
    let callback = new FlightSummonTickCallback();
    callback.system = this;
    GameInstance.GetDelaySystem(this.GetGameInstance()).DelayCallback(callback, this.tickInterval, false);
  }

  // --- the summon blackboard ------------------------------------------------------------------------

  // The blackboard vanilla's summon machine writes (GarageComponent::BB_UpdateSummonState, docs
  // section 1); this system reads the summoned vehicle's EntityID from it and may write the
  // summon's own terminal states.
  private func SummonData() -> ref<VehicleSummonDataDef> {
    return GetAllBlackboardDefs().VehicleSummonData;
  }

  private func SummonBB() -> wref<IBlackboard> {
    return GameInstance.GetBlackboardSystem(this.GetGameInstance()).Get(this.SummonData());
  }

  // the summon state, -1 when the blackboard is not there
  public func State() -> Int32 {
    let bb = this.SummonBB();
    if !IsDefined(bb) {
      return -1;
    }
    return Cast<Int32>(bb.GetUint(this.SummonData().SummonState));
  }

  private func SetState(state: vehicleSummonState) -> Void {
    let bb = this.SummonBB();
    if !IsDefined(bb) {
      return;
    }
    bb.SetUint(this.SummonData().SummonState, Cast<Uint32>(EnumInt<vehicleSummonState>(state)), true);
  }

  public func SummonedID() -> EntityID {
    let bb = this.SummonBB();
    if !IsDefined(bb) {
      let none: EntityID;
      return none;
    }
    return bb.GetEntityID(this.SummonData().SummonedVehicleEntityID);
  }

  private func IsState(a: Int32, b: vehicleSummonState) -> Bool {
    return a == EnumInt<vehicleSummonState>(b);
  }

  // The nearest pad V is on or near: within its radius (on it) or padSummonRange beyond it, and
  // within deckMaxDrop of its deck height. -1 otherwise.
  private func NearPad(p: Vector4) -> Int32 {
    let pad = FlightNavMap.FindNearestHelipad(p, 300.0);
    if pad < 0 {
      return -1;
    }
    let deck = FlightNavMap.GetHelipadPosition(pad);
    if Vector4.Distance2D(p, deck) > FlightNavMap.GetHelipadRadius(pad) + this.padSummonRange
      || AbsF(p.Z - deck.Z) > this.deckMaxDrop {
      return -1;
    }
    return pad;
  }

  // --- the player's summon: the wrapped flow -----------------------------------------------------

  // The wrap's gate: the setting on, the map in, the player on or near a pad, and the flight
  // machinery able to take the vehicle at all (mode settings and the flight component are
  // checked at the engage; the pad gate here is the cheap one).
  public func ShouldFlySummon(p: Vector4) -> Bool {
    if !this.padSummonEnabled || !FlightNavMap.IsLoaded() {
      return false;
    }
    return this.NearPad(p) >= 0;
  }

  // The summon wrap's replacement: spawn the player's picked vehicle at a spot out from the pad
  // and take it over. False = not for us (vanilla runs on). The wrap passes the garage's active
  // vehicle's record.
  public func FlySummonToPad(game: GameInstance, aRecord: TweakDBID) -> Bool {
    let player = GetPlayer(game);
    if !IsDefined(player) {
      return false;
    }
    let pad = this.NearPad(player.GetWorldPosition());
    if pad < 0 || IsDefined(this.OwnCarOut(game, aRecord)) {
      return false;
    }
    if !FlightCarSummon.AutopilotLands() {
      // spawned in the air, the car could only be dropped
      FlightLog.Info("[Summon] the Autopilot or Land at Destination is off: leaving the summon to vanilla");
      return false;
    }
    // the record goes straight to the spawn (the VehicleSystem's RequestNewVehicle takes the
    // TweakDBID; the C++ side refuses one that is not in TweakDB). No TDBID natives in this
    // flow - their runtime wrappers can be unbound in a modded launch and the script call
    // crashes, whose frame it runs in (docs/design/vehicle-summon.md section 3): the callers
    // hand the record over.
    let spot: Vector4;
    let yaw: Float;
    let padYaw = Deg2Rad(FlightNavMap.GetHelipadYaw(pad));
    let padSide = new Vector4(-SinF(padYaw), CosF(padYaw), 0.0, 0.0);
    if !this.PickSpawnSpot(FlightNavMap.GetHelipadPosition(pad), padSide, player.GetWorldPosition(), spot, yaw) {
      FlightLog.Warn("[Summon] no free air to spawn the vehicle in around pad " + FlightNavMap.GetHelipadId(pad)
        + ": leaving the summon to vanilla");
      return false;
    }
    if !FlightCarSummon.SpawnVehicleAt(aRecord, spot.X, spot.Y, spot.Z, yaw) {
      FlightLog.Warn("[Summon] the spawn request was refused: leaving the summon to vanilla");
      return false;
    }
    this.summoning = true;
    this.engaged = false;
    this.padSpot = spot;
    this.vehicle = null;
    // the spawned vehicle's EntityID is not returned by the spawn; the system's tick finds it
    // (the machine's blackboard id, else the record's vehicle nearest the spot)
    this.recordID = aRecord;
    this.pad = pad;
    this.reason = "pad";
    this.awaitingSpawnTicks = 0;
    this.pendingTicks = 0;
    this.tries = 0;
    this.flightTimer = 0.0;
    this.planningTimer = 0.0;
    this.pressPending = false;
    this.ClearWatch();
    FlightLog.Info("[Summon] spawn at " + FloatToStringPrec(spot.X, 1) + ", " + FloatToStringPrec(spot.Y, 1) + ", "
      + FloatToStringPrec(spot.Z, 1) + " yaw " + FloatToStringPrec(yaw, 0) + ": the Autopilot flies it to pad "
      + FlightNavMap.GetHelipadId(pad));
    return true;
  }

  // A press while this system's own spawn from an earlier summon is still out. The garage never
  // learns of that car, so vanilla's summon would bring a second one, and the pad and no-spawn
  // paths would spawn another. True when the press is taken: the car is flying in already, or is
  // next to V (or on V's pad) and stays put, or is flown in again from where it is.
  public func ResummonOwnCar(game: GameInstance, aRecord: TweakDBID) -> Bool {
    if this.summoning {
      return true;
    }
    let car = this.OwnCarOut(game, aRecord);
    let player = GetPlayer(game);
    if !IsDefined(car) || !IsDefined(player) {
      return false;
    }
    let v = player.GetWorldPosition();
    let pad = this.NearPad(v);
    let onPad = pad >= 0 && Vector4.Distance2D(car.GetWorldPosition(), FlightNavMap.GetHelipadPosition(pad)) <= FlightNavMap.GetHelipadRadius(pad);
    if onPad || this.CloseEnough(car, v) {
      FlightLog.Info("[Summon] summoned again: the vehicle is already here");
      return true;
    }
    if !this.IsFlyableVehicle(car) || !FlightCarSummon.AutopilotLands() {
      return false;
    }
    this.TakeOver(car, "summoned again");
    return true;
  }

  // This system's last spawn of the record, when it is still in the world and not wrecked.
  private func OwnCarOut(game: GameInstance, aRecord: TweakDBID) -> ref<VehicleObject> {
    if !EntityID.IsDefined(this.lastSpawnID) {
      return null;
    }
    let car = GameInstance.FindEntityByID(game, this.lastSpawnID) as VehicleObject;
    if !IsDefined(car) || car.GetRecordID() != aRecord || car.IsDestroyed() {
      return null;
    }
    return car;
  }

  // The always-fly summon (the wraps, away from a pad, for a press vanilla would act on): true when
  // the press is taken and vanilla's summon must not run. The summoned car already out is flown
  // from where it is (left to vanilla when it is already next to V, or cannot fly); with none out
  // the record is spawned out from V and flown in.
  public func FlySummonAlways(game: GameInstance, aRecord: TweakDBID) -> Bool {
    let none: TweakDBID;
    if !this.alwaysFlySummon || !FlightNavMap.IsLoaded() || Equals(aRecord, none) {
      return false;
    }
    if this.summoning {
      // already flying one in: vanilla's summon would start driving the car out of the air
      return true;
    }
    let player = GetPlayer(game);
    if !IsDefined(player) {
      return false;
    }
    let car = GameInstance.FindEntityByID(game, this.SummonedID()) as VehicleObject;
    if IsDefined(car) && car.GetRecordID() == aRecord {
      if !this.IsFlyableVehicle(car) || this.CloseEnough(car, player.GetWorldPosition()) {
        return false;
      }
      this.TakeOver(car, "always fly");
      return true;
    }
    return this.FlySummonToGround(game, aRecord, "always fly");
  }

  // --- the tick ------------------------------------------------------------------------------------

  public func Tick() -> Void {
    if !this.ticking {
      return;
    }
    if this.summoning {
      this.TickSummon();
    } else if this.IsState(this.State(), vehicleSummonState.EnRoute) && this.OnForeignCandidate() {
      // a summon this system did not start (vanilla's, a quest's), EnRoute near a pad: flown there
    } else {
      this.TickWatch();
    }
    this.lastState = this.State();
    this.Schedule();
  }

  // --- the fallback: watching vanilla's summon ------------------------------------------------------

  // The summon wraps, after vanilla's own call: what the summon looked like before it, so the tick
  // can tell whether the garage did anything (case 1). Only for a press vanilla acted on (not on
  // cooldown: the wraps make vanilla's own check).
  public func NoteSummonPressed(record: TweakDBID, state: Int32, id: EntityID) -> Void {
    if !this.summonFallbackEnabled || this.summoning {
      return;
    }
    this.pressPending = true;
    this.pressed = true;
    let none: EntityID;
    this.pressedID = none;
    this.pressedAge = 0.0;
    this.pressAge = 0.0;
    this.pressRecord = record;
    this.pressState = state;
    this.pressID = id;
    let car = GameInstance.FindEntityByID(this.GetGameInstance(), id) as VehicleObject;
    this.pressPos = IsDefined(car) ? car.GetWorldPosition() : new Vector4(0.0, 0.0, 0.0, 0.0);
  }

  // The summon blackboard and the summoned car against V, every tick between flights.
  private func TickWatch() -> Void {
    if !this.summonFallbackEnabled || !FlightNavMap.IsLoaded() {
      this.pressPending = false;
      this.pressed = false;
      return;
    }
    let game = this.GetGameInstance();
    let player = GetPlayer(game);
    if !IsDefined(player) {
      return;
    }
    let state = this.State();
    let id = this.SummonedID();
    let car = GameInstance.FindEntityByID(game, id) as VehicleObject;
    let v = player.GetWorldPosition();
    // a re-summon of a car already out moves it without a new state: its place is checked once
    let placed = false;

    if this.pressPending {
      this.pressAge += this.tickInterval;
      let moved = IsDefined(car) && Vector4.Distance(car.GetWorldPosition(), this.pressPos) > 5.0;
      if state != this.pressState || NotEquals(id, this.pressID) || moved {
        // the garage answered: a new car, a new state, or the same car put somewhere else
        this.pressPending = false;
        this.pressedID = id;
        placed = moved && state == this.pressState;
      } else if this.pressAge >= this.noSpawnTime {
        // case 1: nothing at all
        this.pressPending = false;
        this.pressed = false;
        let none: TweakDBID;
        if this.NearPad(v) < 0 && NotEquals(this.pressRecord, none) {
          FlightLog.Info("[Summon] nothing from the garage " + FloatToStringPrec(this.pressAge, 0) + " s after the summon: flying the car in");
          this.FlySummonToGround(game, this.pressRecord, "no spawn");
        }
        return;
      }
    }

    // Cases 2 to 4 only for the car V's own press brought, for pressWatchTime after the garage answered
    // (EnRoute not counted). A summon a quest or another mod makes later (a car put in a parking spot by
    // V's apartment while V is upstairs) is left where it put it.
    if this.pressed && !this.pressPending {
      if !this.IsState(state, vehicleSummonState.EnRoute) {
        this.pressedAge += this.tickInterval;
      }
      if NotEquals(id, this.pressedID) || this.pressedAge > this.pressWatchTime {
        this.pressed = false;
      }
    }
    if !this.pressed {
      this.ClearWatch();
      return;
    }
    if !IsDefined(car) || !this.IsFlyableVehicle(car) {
      this.ClearWatch();
      if !this.pressPending {
        this.pressed = false;
      }
      return;
    }
    if this.IsState(state, vehicleSummonState.EnRoute) {
      // case 3: on the way, but not getting closer
      let separation = Vector4.Distance(car.GetWorldPosition(), v);
      if NotEquals(id, this.watchID) {
        this.watchID = id;
        this.watchBest = separation;
        this.watchStall = 0.0;
      } else if separation < this.watchBest - this.stallProgress {
        this.watchBest = separation;
        this.watchStall = 0.0;
      } else {
        this.watchStall += this.tickInterval;
        if this.watchStall >= this.stallTime && !this.CloseEnough(car, v) {
          this.TakeOver(car, "stalled " + FloatToStringPrec(separation, 0) + " m from V for "
            + FloatToStringPrec(this.watchStall, 0) + " s");
        }
      }
      return;
    }
    // the ends only as they happen: a PathfindingFailed or Arrived left from an earlier summon is not
    // this one's
    let changed = state != this.lastState;
    if changed && this.IsState(state, vehicleSummonState.PathfindingFailed) {
      // case 2
      this.TakeOver(car, "no path for the drive");
    } else if (changed || placed) && this.IsState(state, vehicleSummonState.Arrived) && !this.CloseEnough(car, v) {
      // case 4
      let p = car.GetWorldPosition();
      this.TakeOver(car, "arrived " + FloatToStringPrec(Vector4.Distance2D(p, v), 0) + " m away, "
        + FloatToStringPrec(p.Z - v.Z, 1) + " m of height");
    }
    this.ClearWatch();
  }

  // where vanilla would have left it anyway: on V's level, no further off than its own street
  // summons stop
  private func CloseEnough(car: ref<VehicleObject>, v: Vector4) -> Bool {
    let p = car.GetWorldPosition();
    return AbsF(p.Z - v.Z) < this.arrivedLevel && Vector4.Distance2D(p, v) < this.arrivedFar;
  }

  private func ClearWatch() -> Void {
    let none: EntityID;
    this.watchID = none;
    this.watchBest = 0.0;
    this.watchStall = 0.0;
  }

  // A vanilla summon's car taken over: flown to the pad V is near, else landed next to V.
  private func TakeOver(car: ref<VehicleObject>, why: String) -> Void {
    let v = GetPlayer(this.GetGameInstance()).GetWorldPosition();
    this.summoning = true;
    this.engaged = false;
    this.vehicle = car;
    this.vehicleID = car.GetEntityID();
    this.ownSpawn = false;
    let none: TweakDBID;
    this.recordID = none;
    this.pad = this.NearPad(v);
    this.target = v;
    this.padSpot = this.pad >= 0 ? FlightNavMap.GetHelipadPosition(this.pad) : v;
    this.reason = why;
    this.awaitingSpawnTicks = 0;
    this.pendingTicks = 0;
    this.tries = 0;
    this.flightTimer = 0.0;
    this.planningTimer = 0.0;
    this.pressPending = false;
    this.pressed = false;
    this.ClearWatch();
    FlightLog.Info("[Summon] vanilla's summon " + why + ": flying it to " + this.Where());
  }

  // Case 1: the record spawned out from V as at a pad (no pad here), and landed next to V.
  public func FlySummonToGround(game: GameInstance, aRecord: TweakDBID, why: String) -> Bool {
    let player = GetPlayer(game);
    if !IsDefined(player) {
      return false;
    }
    if !FlightCarSummon.AutopilotLands() {
      // spawned in the air, the car could only be dropped
      FlightLog.Info("[Summon] the Autopilot or Land at Destination is off: no flying summon");
      return false;
    }
    if IsDefined(this.OwnCarOut(game, aRecord)) {
      return false;
    }
    let v = player.GetWorldPosition();
    let spot: Vector4;
    let yaw: Float;
    if !this.PickSpawnSpot(v, player.GetWorldForward(), v, spot, yaw) {
      FlightLog.Warn("[Summon] no free air to spawn the vehicle in around V");
      return false;
    }
    if !FlightCarSummon.SpawnVehicleAt(aRecord, spot.X, spot.Y, spot.Z, yaw) {
      FlightLog.Warn("[Summon] the spawn request was refused");
      return false;
    }
    this.summoning = true;
    this.engaged = false;
    this.padSpot = spot;
    this.vehicle = null;
    this.ownSpawn = true;
    this.recordID = aRecord;
    this.pad = -1;
    this.target = v;
    this.reason = why;
    this.awaitingSpawnTicks = 0;
    this.pendingTicks = 0;
    this.tries = 0;
    this.flightTimer = 0.0;
    this.planningTimer = 0.0;
    this.pressPending = false;
    this.ClearWatch();
    FlightLog.Info("[Summon] spawn at " + FloatToStringPrec(spot.X, 1) + ", " + FloatToStringPrec(spot.Y, 1) + ", "
      + FloatToStringPrec(spot.Z, 1) + " yaw " + FloatToStringPrec(yaw, 0) + ": the Autopilot flies it to V (" + why + ")");
    return true;
  }

  // A vanilla summon this system did not spawn went EnRoute near a pad (the machine's own
  // confirmation): take it over, one tick later as for the spawn flow.
  private func OnForeignCandidate() -> Bool {
    if !this.padSummonEnabled || !FlightNavMap.IsLoaded() {
      return false;
    }
    let player = GetPlayer(this.GetGameInstance());
    if !IsDefined(player) {
      return false;
    }
    let pad = this.NearPad(player.GetWorldPosition());
    if pad < 0 {
      return false;
    }
    this.summoning = true;
    this.engaged = false;
    this.vehicleID = this.SummonedID();
    let none: TweakDBID;
    this.recordID = none;
    this.padSpot = FlightNavMap.GetHelipadPosition(pad);
    this.pad = pad;
    this.awaitingSpawnTicks = 0;
    this.pendingTicks = 0;
    this.tries = 0;
    this.flightTimer = 0.0;
    this.planningTimer = 0.0;
    this.reason = "pad";
    this.pressPending = false;
    this.ClearWatch();
    FlightLog.Info("[Summon] foreign summon EnRoute near a landing pad (pad " + FlightNavMap.GetHelipadId(pad)
      + "): taking it over");
    return true;
  }

  private func TickSummon() -> Void {
    if IsDefined(this.vehicle) && this.vehicle.IsDestroyed() {
      FlightLog.Info("[Summon] vehicle destroyed; summon ownership dropped");
      this.Reset();
      return;
    }
    if !this.engaged {
      this.awaitingSpawnTicks += 1;
      if this.awaitingSpawnTicks > this.spawnWaitTicks {
        this.FailOnGround(null, "the spawned vehicle never showed up");
        return;
      }
      if IsDefined(this.vehicle) {
        // the vehicle is ours; the takeover (its flight component's readiness is checked there,
        // a null component returns for the next tick, bounded by the wait above)
        this.TryEngage(this.vehicle.GetFlightComponent());
        return;
      }
      this.vehicle = this.FindSpawnedVehicle();
      if IsDefined(this.vehicle) {
        this.vehicleID = this.vehicle.GetEntityID();
        this.lastSpawnID = this.vehicleID;
        // V's own car: the garage's spawn marks it V's, ours does not, and without the mark its trunk has no stash
        // (VehicleComponentPS.GetTrunkActions, the flag's only reader)
        this.vehicle.GetVehiclePS().SetIsPlayerVehicle(true);
        // and the stash it opens is V's: the garage's spawn queues this on the car
        // (GarageComponent::ProcessSpawnedVehicles), without it the trunk holds the car's own, empty, inventory
        this.vehicle.QueueEvent(new MakeInventoryShareableEvent());
        // the spawn confirmed: the summon blackboard, as the vanilla machine writes it
        // (SummonedVehicleEntityID + EnRoute), so the poll side (and any listener) sees this
        // summon as a vanilla one from the spawn on
        let bb = this.SummonBB();
        if IsDefined(bb) {
          bb.SetEntityID(this.SummonData().SummonedVehicleEntityID, this.vehicleID, true);
          if !this.IsState(this.State(), vehicleSummonState.EnRoute) {
            this.SetState(vehicleSummonState.EnRoute);
          }
        }
      }
      return;
    }
    let component = this.vehicle.GetFlightComponent();
    if !IsDefined(component) {
      this.Reset();
      return;
    }
    // engaged: watching the flight
    if !component.active {
      // flight switched off underneath us (a quickhack, a reset): the summon is over either way;
      // if vanilla's own state never closed, say PathfindingFailed, the honest native terminal state
      if this.IsState(this.State(), vehicleSummonState.EnRoute) {
        this.SetState(vehicleSummonState.PathfindingFailed);
      }
      this.Reset();
      return;
    }
    let mode = component.GetFlightMode() as FlightModeAutopilot;
    if !IsDefined(mode) {
      this.FailOnGround(component, "the autopilot mode is gone");
      return;
    }
    this.flightTimer += this.tickInterval;
    this.planningTimer = this.RouteStarted(mode) ? 0.0 : this.planningTimer + this.tickInterval;
    if Equals(mode.status, "landed") {
      this.Complete(component, mode);
      return;
    }
    if Equals(mode.status, "arrived") {
      // the descent was held up over the pad (something solid the map lacks): leave it hovering
      // rather than dropping it, the summon is failed as far as vanilla is concerned
      this.ReleaseAirborne(mode, "the vehicle would not land at " + this.Where());
      return;
    }
    if this.planningTimer > this.planningTime {
      this.FailOnGround(component, "no route in " + FloatToStringPrec(this.planningTime, 0) + " s");
      return;
    }
    if this.flightTimer > this.flightTime && !Equals(mode.status, "descending") {
      this.ReleaseAirborne(mode, "the flight is taking too long");
      return;
    }
  }

  // The spawned vehicle: from ownership, then the summon blackboard's id (vanilla's machine
  // writes it), then this class's own spawn record (LastSpawnEntity, our callback's stub - the
  // pad summon's route, docs/design/vehicle-summon.md section 2), and else the record's vehicle
  // nearest the spawn spot via the player's targeting.
  private func FindSpawnedVehicle() -> wref<VehicleObject> {
    let game = this.GetGameInstance();
    if IsDefined(this.vehicle) {
      return this.vehicle;
    }
    let id = this.SummonedID();
    if EntityID.IsDefined(id) {
      let found = GameInstance.FindEntityByID(game, id) as VehicleObject;
      if IsDefined(found) && this.IsFlyableVehicle(found) {
        return found;
      }
    }
    let spawnEntity = FlightCarSummon.LastSpawnEntity();
    if EntityID.IsDefined(spawnEntity) {
      let recent = GameInstance.FindEntityByID(game, spawnEntity) as VehicleObject;
      if IsDefined(recent) && this.IsFlyableVehicle(recent) {
        return recent;
      }
    }
    if TDBID.IsValid(this.recordID) {
      let player = GetPlayer(game);
      if IsDefined(player) {
        let query = TSQ_ALL();
        query.testedSet = TargetingSet.Complete;
        query.maxDistance = 80.0;
        let parts: array<TS_TargetPartInfo>;
        GameInstance.GetTargetingSystem(game).GetTargetParts(player, query, parts);
        let best: wref<VehicleObject>;
        let bestDistance = 80.0;
        for part in parts {
          let vehicle = TS_TargetPartInfo.GetComponent(part).GetEntity() as VehicleObject;
          if IsDefined(vehicle) && vehicle.GetRecordID() == this.recordID && this.IsFlyableVehicle(vehicle) {
            let d = Vector4.Distance(vehicle.GetWorldPosition(), this.padSpot);
            if d < bestDistance {
              best = vehicle;
              bestDistance = d;
            }
          }
        }
        if IsDefined(best) {
          return best;
        }
      }
    }
    let none: wref<VehicleObject>;
    return none;
  }

  private func IsFlyableVehicle(v: wref<VehicleObject>) -> Bool {
    return !v.IsDestroyed() && IsDefined(v.GetFlightComponent()) && !v.GetFlightComponent().active
      && v.GetFlightComponent().GetAutopilotModeIndex() >= 0;
  }

  // --- the flight ---------------------------------------------------------------------------------

  // The takeover: end whatever vanilla is driving the car with (nothing, at a spawn; the call
  // guards the race), hand it to the Autopilot with the pad as this flight's destination, and
  // let the mode's own landing take it from there.
  private func TryEngage(component: ref<FlightComponent>) -> Void {
    let vehicle = this.vehicle;
    if !IsDefined(component) || component.active {
      return; // another tick decides (the wait above bounds it)
    }
    let index = component.GetAutopilotModeIndex();
    if index < 0 {
      this.Reset();
      return;
    }
    component.mode = index;
    let mode = component.GetFlightMode() as FlightModeAutopilot;
    if !IsDefined(mode) {
      this.Reset();
      return;
    }
    if mode.landingMode == 0 {
      // the autopilot would hover at the pad and then this system could only drop the car:
      // leave the summon to vanilla
      FlightLog.Info("[Summon] land at destination is off: leaving the summon to vanilla");
      this.Reset();
      return;
    }
    // next to V, there has to be room to land (with no spot the Autopilot would fly to V's own point
    // and hover there): checked before the car leaves the ground
    let ground = this.target;
    if this.pad < 0 && !this.GroundSpot(mode, ground) {
      if !this.ownSpawn {
        // vanilla's car: better where vanilla left it than in the air
        FlightLog.Info("[Summon] no room to land next to V: vanilla's car left where it is");
        this.Reset();
        return;
      }
      // our spawn, in the air out from V, with nowhere to put it: rare (no free column on any floor
      // under V); flight off drops it from the spawn height
      FlightLog.Warn("[Summon] no room to land on any floor under V: the spawned car is left as it is");
      this.Reset();
      return;
    }
    vehicle.ForceEnablePhysics();
    vehicle.EndActions();
    component.Activate(true);
    // the pad is this flight's destination, and the landing spot is put across the deck from V
    // standing on it (the mode's ChooseRouteGoal applies the offset). Set after Activate: the
    // mode's Activate clears these fields, and its first Update (next frame) is when they are
    // first read.
    if this.pad >= 0 {
      mode.externalDestination = FlightNavMap.GetHelipadPosition(this.pad);
      mode.padLandingOffset = this.LandingOffset(this.pad);
    } else {
      // on the ground next to V: the mode's own landing spot search around the level GroundSpot
      // found room on (V's, else one under V for a car of our own spawn), off V
      mode.externalDestination = ground;
      mode.landingKeepClear = this.target;
      mode.landingKeepClearRadius = this.groundKeepClear;
    }
    mode.hasExternalDestination = true;
    // the activate-time free-roam decision pre-dates these fields; free roam would fly the car
    // out to its own legs and never look at the destination again (a 1.6 km
    // route for a pad 150 m off)
    mode.freeRoamChosen = false;
    // and turn onto the pad heading at approach speed, before the sink, not with it: the
    // descent's own z-blend rotates the target yaw over the whole sink (~5 deg/s) and the
    // hovering car never follows it, touching down about 90 degrees of yaw
    // off the pad heading
    mode.droneBrakeTurn = true;
    this.engaged = true;
    this.flightTimer = 0.0;
    this.planningTimer = 0.0;
    FlightLog.Info("[Summon] flight engaged on the summoned vehicle, autopilot flying to " + this.Where() + " (" + this.reason + ")");
  }

  // Where a car without a pad lands: the mode's landing spot search a metre over where V stood (V's
  // feet are under the bake's floor, which is where a resting car's centre sits), kept groundKeepClear
  // off V. With no room on V's level, a car of this system's own spawn (case 1 and always fly: no other car exists)
  // tries each floor under V, nearest first: the street below beats no car. `at` comes back as the
  // destination to fly to.
  private func GroundSpot(mode: ref<FlightModeAutopilot>, out at: Vector4) -> Bool {
    let spot: Vector4;
    let v = this.target;
    at = v + new Vector4(0.0, 0.0, 1.0, 0.0);
    if mode.FindGroundSpot(at, v, this.groundKeepClear, spot) {
      return true;
    }
    if !this.ownSpawn {
      return false;
    }
    let floors = FlightNavMap.GetGaps(v);
    let i = ArraySize(floors) - 1;
    while i >= 0 {
      let below = new Vector4(v.X, v.Y, floors[i].X + 1.0, 1.0);
      if floors[i].X < v.Z - 3.0 && mode.FindGroundSpot(below, v, this.groundKeepClear, spot) {
        FlightLog.Info("[Summon] no room to land on V's level: landing " + FloatToStringPrec(v.Z - floors[i].X, 0) + " m below V");
        at = below;
        return true;
      }
      i -= 1;
    }
    return false;
  }

  // The landing spot's offset from the deck centre: across the deck from V, at least 4 m from V's
  // standing spot, never past radius - 5 (the deck's edge and the car's overhang). A pad too
  // small for the 4 m (radius under 9 m) lands at the centre.
  private func LandingOffset(pad: Int32) -> Vector4 {
    let limit = FlightNavMap.GetHelipadRadius(pad) - 5.0;
    if limit < 4.0 {
      return new Vector4(0.0, 0.0, 0.0, 0.0);
    }
    let deck = FlightNavMap.GetHelipadPosition(pad);
    let player = GetPlayer(this.GetGameInstance());
    let dir = new Vector4(0.0, 0.0, 0.0, 0.0);
    let distance = limit;
    if IsDefined(player) {
      let across = deck - player.GetWorldPosition();
      dir = FlightModeAutopilot.Flat(across);
      distance = MinF(limit, Vector4.Distance2D(deck, player.GetWorldPosition()) + 4.0);
    }
    if Vector4.Length2D(dir) < 0.5 {
      // V is at the deck centre: across, along the pad's own heading, to one side
      let yaw = Deg2Rad(FlightNavMap.GetHelipadYaw(pad));
      dir = new Vector4(-SinF(yaw), CosF(yaw), 0.0, 0.0);
    } else {
      dir = Vector4.Normalize(dir);
    }
    return dir * distance;
  }

  // Where the vehicle appears: spawnDistance out from `deck` (a pad's, or where V stands) at
  // spawnHeight over it, facing it, in a column the nav map holds free. The direction is away from
  // V across the pad, or `side` when V is at the centre (the pad's own side, V's facing), widened
  // when a direction is inside something the map does not know.
  private func PickSpawnSpot(deck: Vector4, side: Vector4, playerPos: Vector4, out spot: Vector4, out yaw: Float) -> Bool {
    let away = FlightModeAutopilot.Flat(deck - playerPos);
    if Vector4.Length2D(away) < 0.5 {
      away = Vector4.Normalize(FlightModeAutopilot.Flat(side));
    } else {
      away = Vector4.Normalize(away);
    }
    let angles = [0.0, 25.0, -25.0, 50.0, -50.0];
    let heights = [this.spawnHeight, 8.0, 32.0];
    let distances = [this.spawnDistance, 80.0, 50.0];
    let i = 0;
    while i < ArraySize(angles) {
      let d = this.Turn(away, Deg2Rad(angles[i]));
      let j = 0;
      while j < ArraySize(heights) {
        let k = 0;
        while k < ArraySize(distances) {
          let candidate = deck + d * distances[k] + new Vector4(0.0, 0.0, heights[j], 0.0);
          if FlightNavMap.IsFree(candidate) {
            spot = candidate;
            yaw = this.HeadingTo(candidate, deck);
            return true;
          }
          k += 1;
        }
        j += 1;
      }
      i += 1;
    }
    return false;
  }

  // v rotated by angle about +Z
  private func Turn(v: Vector4, angleRad: Float) -> Vector4 {
    let c = CosF(angleRad);
    let s = SinF(angleRad);
    return new Vector4(v.X * c - v.Y * s, v.X * s + v.Y * c, v.Z, 0.0);
  }

  // The pads' yaw of a heading: degrees, 0..360 (the pads' convention, +Y turned by it, CCW)
  private func HeadingTo(from: Vector4, to: Vector4) -> Float {
    let d = FlightModeAutopilot.Flat(to - from);
    if Vector4.Length2D(d) < 0.001 {
      return 0.0;
    }
    let yaw = Rad2Deg(AtanF(-d.X, d.Y));
    return yaw < 0.0 ? yaw + 360.0 : yaw;
  }

  // The flight has somewhere to be: a route to follow, or it is already over for good at one of
  // its ends (descending: the arrival flow cleared the route; landed/arrived: done either way).
  // Without this the planning timer would keep counting through a slow descent and FailOnGround
  // would switch flight off mid-air.
  private func RouteStarted(mode: ref<FlightModeAutopilot>) -> Bool {
    let status = mode.status;
    return Equals(status, "descending") || Equals(status, "landed") || Equals(status, "arrived")
      || (mode.RouteSize() >= 2 && !Equals(status, "planning") && !Equals(status, "no route")
        && !Equals(status, "no nav map") && !Equals(status, "idle"));
  }

  // --- the ends ------------------------------------------------------------------------------------

  // Touchdown on the pad: the flight comes down, then the mode and the component are deactivated
  // in the FlightController order (mode first), the summon fields cleared, and vanilla's own
  // arrival (its blackboard state and SummonFinishedEvent, to its VehicleComponent handler) only
  // when the native has not already done it at the spawn.
  private func Complete(component: ref<FlightComponent>, mode: ref<FlightModeAutopilot>) -> Void {
    FlightLog.Info("[Summon] landed at " + this.Where() + ", completing the summon");
    this.ClearSummonFields(mode);
    if IsDefined(component) && component.active {
      mode.Deactivate();
      component.Deactivate(false);
    }
    if this.State() != EnumInt<vehicleSummonState>(vehicleSummonState.Arrived) {
      this.SetState(vehicleSummonState.Arrived);
      let evt = new SummonFinishedEvent();
      evt.state = vehicleSummonState.Arrived;
      if IsDefined(this.vehicle) {
        this.vehicle.QueueEvent(evt);
      }
    }
    this.Reset();
  }

  // A summon that never flew: the car is on the ground (or still at the spawn's height, a
  // transform spawn) so switching flight off is safe... unless it is up and hovering: release
  // (below) instead. Vanilla's PathfindingFailed is the honest native state for it.
  private func FailOnGround(component: ref<FlightComponent>, reason: String) -> Void {
    FlightLog.Warn("[Summon] giving up: " + reason);
    if IsDefined(component) && component.active {
      this.FailRelease(component, false);
    }
    if this.IsState(this.State(), vehicleSummonState.EnRoute) || this.IsState(this.State(), vehicleSummonState.Arrived) {
      this.SetState(vehicleSummonState.PathfindingFailed);
    }
    this.Reset();
  }

  // The autopilot mode ref for a component, from its (non-active) mode list
  private func FailRelease(component: ref<FlightComponent>, silent: Bool) -> Void {
    let mode = component.GetFlightMode() as FlightModeAutopilot;
    if IsDefined(mode) {
      this.ClearSummonFields(mode);
    }
    if component.active {
      if IsDefined(mode) {
        mode.Deactivate();
      }
      component.Deactivate(false);
    }
  }

  // A summon to release while the car is up (a fight vanilla would not end, a flight that took
  // too long, a pad the car would not land on): keep it up - switching flight off here would drop
  // it - drop the ownership, and let vanilla's state say PathfindingFailed. The flight keeps a
  // destination of its own, where the car is now: with none, the Autopilot reads the one the
  // player tracks and the empty car would fly off to it.
  private func ReleaseAirborne(mode: ref<FlightModeAutopilot>, reason: String) -> Void {
    FlightLog.Warn("[Summon] " + reason + ": leaving the vehicle where it is (flight stays on), summon released");
    if IsDefined(mode) {
      if IsDefined(this.vehicle) {
        mode.externalDestination = this.vehicle.GetWorldPosition();
      }
      mode.hasExternalDestination = true;
      mode.padLandingOffset = new Vector4(0.0, 0.0, 0.0, 0.0);
      mode.landingKeepClearRadius = 0.0;
    }
    if this.IsState(this.State(), vehicleSummonState.EnRoute) || this.IsState(this.State(), vehicleSummonState.Arrived) {
      this.SetState(vehicleSummonState.PathfindingFailed);
    }
    this.Reset();
  }

  private func ClearSummonFields(mode: ref<FlightModeAutopilot>) -> Void {
    if IsDefined(mode) {
      mode.externalDestination = new Vector4(0.0, 0.0, 0.0, 0.0);
      mode.hasExternalDestination = false;
      mode.padLandingOffset = new Vector4(0.0, 0.0, 0.0, 0.0);
      mode.landingKeepClearRadius = 0.0;
    }
  }

  private func Where() -> String {
    return this.pad >= 0 ? "pad " + FlightNavMap.GetHelipadId(this.pad) : "V";
  }

  // Ownership only (the summon is gone or was never ours); the vehicle keeps whatever state it has
  private func Reset() -> Void {
    let none: EntityID;
    this.summoning = false;
    this.engaged = false;
    this.vehicle = null;
    this.vehicleID = none;
    this.pad = -1;
    this.reason = "";
    this.ownSpawn = false;
    this.recordID = t""; // an empty record, the none case
    this.awaitingSpawnTicks = 0;
    this.pendingTicks = 0;
    this.tries = 0;
    this.flightTimer = 0.0;
    this.planningTimer = 0.0;
  }
}

// The wraps (QuickSlotsManager's own calls, the player flow):

// The DPADActionPerformed vanilla's summon body posts, for a summon taken from it.
@addMethod(QuickSlotsManager)
public func LTBF_SummonPerformed() -> Void {
  let dpadAction = new DPADActionPerformed();
  dpadAction.action = EHotkey.DPAD_RIGHT;
  dpadAction.state = EUIActionState.COMPLETED;
  dpadAction.successful = true;
  GameInstance.GetUISystem(this.m_Player.GetGame()).QueueEvent(dpadAction);
}

// The summon itself: the dpad tap and the wheel's summon item both run SummonActiveVehicle(force)
// (the raw quickSlotsManager.script; the decompile's one-arg SummonVehicle is stale for 2.5). At a
// landing pad (the gate) spawn the picked record at a spot out from the pad and fly it in, and
// queue the same DPADActionPerformed the vanilla body posts; otherwise vanilla, whose own
// behaviour at a pad is the safe refusal (docs/design/vehicle-summon.md).
@wrapMethod(QuickSlotsManager)
public func SummonActiveVehicle(force: Bool) -> Void {
  let game = this.m_Player.GetGame();
  let system = FlightSummonSystem.GetInstance(game);
  let current = GameInstance.GetVehicleSystem(game).GetActivePlayerVehicle(this.GetActiveVehicleType());
  if IsDefined(system) && system.ResummonOwnCar(game, current.recordID) {
    this.LTBF_SummonPerformed();
    return;
  }
  if IsDefined(system) && system.ShouldFlySummon(this.m_Player.GetWorldPosition()) {
    // the garage's active vehicle, as vanilla's summon reads it
    let padPick = GameInstance.GetVehicleSystem(game).GetActivePlayerVehicle(this.GetActiveVehicleType());
    if system.FlySummonToPad(game, padPick.recordID) {
      this.LTBF_SummonPerformed();
      return;
    }
  }
  // vanilla's own summon; the fallback watches what comes of it (vanilla's cooldown check: a press
  // on cooldown summons nothing, and that is no failure)
  let summons = force || !GameInstance.GetVehicleSystem(game).IsActivePlayerVehicleOnCooldown(this.GetActiveVehicleType());
  if IsDefined(system) && summons {
    let picked = GameInstance.GetVehicleSystem(game).GetActivePlayerVehicle(this.GetActiveVehicleType());
    if system.FlySummonAlways(game, picked.recordID) {
      this.LTBF_SummonPerformed();
      return;
    }
  }
  let state = -1;
  let id: EntityID;
  if IsDefined(system) {
    state = system.State();
    id = system.SummonedID();
  }
  wrappedMethod(force);
  if IsDefined(system) && summons {
    let active = GameInstance.GetVehicleSystem(game).GetActivePlayerVehicle(this.GetActiveVehicleType());
    system.NoteSummonPressed(active.recordID, state, id);
  }
}

// The record summon (SummonVehicle(force, type, vehicle, spawnOnlyOnValidRoad), the raw script;
// its SpawnPlayerVehicle(type, vehicle, spawnOnlyOnValidRoad) native is the one that crashes the
// game handed a record - this session's dumps, docs section 4): at a landing pad, take the
// record in the pad summon instead; otherwise vanilla runs exactly as the game wrote it.
@wrapMethod(QuickSlotsManager)
public func SummonVehicle(force: Bool, type: gamedataVehicleType, vehicle: TweakDBID, spawnOnlyOnValidRoad: Bool) -> Void {
  let game = this.m_Player.GetGame();
  let system = FlightSummonSystem.GetInstance(game);
  if IsDefined(system) && TDBID.IsValid(vehicle) && system.ResummonOwnCar(game, vehicle) {
    this.LTBF_SummonPerformed();
    return;
  }
  if IsDefined(system) && TDBID.IsValid(vehicle) && system.ShouldFlySummon(this.m_Player.GetWorldPosition()) {
    if system.FlySummonToPad(game, vehicle) {
      this.LTBF_SummonPerformed();
      return;
    }
  }
  let summons = force || !GameInstance.GetVehicleSystem(game).IsPlayerVehicleOnCooldown(type, vehicle);
  if IsDefined(system) && summons && system.FlySummonAlways(game, vehicle) {
    this.LTBF_SummonPerformed();
    return;
  }
  let state = -1;
  let id: EntityID;
  if IsDefined(system) {
    state = system.State();
    id = system.SummonedID();
  }
  wrappedMethod(force, type, vehicle, spawnOnlyOnValidRoad);
  if IsDefined(system) && summons {
    system.NoteSummonPressed(vehicle, state, id);
  }
}