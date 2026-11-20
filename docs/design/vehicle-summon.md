# Pad summon: the summoned vehicle flies to the landing pad

When V stands on or near a landing pad and calls a vehicle, the summoned car appears some way out
from the pad, takes off and flies to the pad with the Autopilot, then lands on it, in place of
vanilla's road summon. Away from pads vanilla's summon runs, and when it cannot bring the car to V
the car flies the rest of the way and lands next to V (section 5).

The feature is script-first (`src/redscript/Summon/FlightSummonSystem.reds`). The one native piece
is the spawn itself (`FlightCarSummon`, `src/red4ext/Summon/FlightCarSummon.cpp`).

## 1. Vanilla summon, in short

- The summon key and the quickslot end in `QuickSlotsManager.SummonVehicle(force)`, which checks
  the vehicle system's cooldown and calls the native `VehicleSystem.SpawnPlayerVehicle`.
- The work happens in the garage (`vehicle::GarageComponent`). It searches for a road-side summon
  spot near V, spawns the car there and has it drive to V on traffic lanes. The per-vehicle
  blackboard `VehicleSummonData` {`GarageState`, `SummonState`, `SummonedVehicleEntityID`} follows
  it, with `vehicleSummonState` {Idle 0, EnRoute 1, AlreadySummoned 2, PathfindingFailed 3,
  Arrived 4}. `SummonStartedEvent` and `SummonFinishedEvent` reach every VehicleComponent listener;
  `VehicleComponent.OnSummonFinishedEvent` plays the arrival effects (mappin, honk and flash).
- The garage never spawns a car on a pad deck: its spot search wants a road, and its drive needs
  traffic lanes, which pads do not have.

## 2. LTBF's pad summon

`FlightSummonSystem` is a `ScriptableSystem` with a 2 s tick (paused with the game), plus wraps on
the player's summon calls:

- **The wraps.** `QuickSlotsManager.SetActiveVehicle(vehicleData)` remembers the record the player
  picked. Both summon entries, `SummonActiveVehicle(force)` (the key and the wheel) and
  `SummonVehicle(force, type, vehicle, spawnOnlyOnValidRoad)` (the record summon), take over at the
  pad gate and queue the same `DPADActionPerformed` the vanilla body posts. Elsewhere
  `wrappedMethod` runs vanilla as the game wrote it.
- **The gate** (`ShouldFlySummon`): the Mod Setting on (`padSummonEnabled`), the nav map loaded,
  and V on or near a pad (`NearPad`: 2D distance to the deck within `GetHelipadRadius +
  padSummonRange`, and the deck no more than `deckMaxDrop`, 15 m, below V). Land at Destination (not Hover) and
  the flight component are checked at the engage.
- **The spawn** (`FlySummonToPad(game, record)`): `PickSpawnSpot` picks a spot `spawnDistance`
  (120 m) out from the pad, `spawnHeight` (20 m) over its deck, facing it. It tries five directions
  (away from V and V's facing, then 25 and 50 degrees either side), three heights (20, 8, 32 m) and
  three distances (120, 80, 50 m), each checked with `FlightNavMap.IsFree`. Then
  `FlightCarSummon.SpawnVehicleAt(record, x, y, z, yaw)` (section 3). The spawned entity's id comes
  back through `FlightCarSummon.LastSpawnEntity()` once the game materializes the stub. The
  blackboard gets the vanilla mirror (`SummonedVehicleEntityID` and `EnRoute`), so listeners see a
  vanilla-shaped summon. The engage follows one tick later.
- **The engage** (`TryEngage`): the vehicle alive and unmounted, with a `FlightComponent`, an
  Autopilot mode (`GetAutopilotModeIndex`) and Land at Destination not Hover (`landingMode`); otherwise the summon falls
  back to vanilla. Then `ForceEnablePhysics`, `EndActions`, the mode set and the component
  activated, and the mode's additive fields: `externalDestination` (the pad deck),
  `padLandingOffset` (`LandingOffset(pad)`: away from V, at least 4 m, at most the radius less 5 m,
  zero on a 6 m pad) and `hasExternalDestination`. `mode.freeRoamChosen` is cleared, since the
  mode decides on free roam when it activates, before these fields are set, and free roam never
  looks at the destination again.
- **The flight.** The tick watches `mode.status` go through planning, following and descending to
  landed. `ReleaseAirborne` leaves the car hovering and writes `PathfindingFailed` when the flight
  takes longer than `flightTime` (180 s) or the descent is held up over the pad. `FailOnGround`
  (flight off, `PathfindingFailed`) is for a spawn that never shows up or no route within
  `planningTime` (30 s). The Autopilot's player-input override is off while
  `hasExternalDestination` is set: an unmounted vehicle's drive inputs read as player input every
  frame and would hold the flight in the override.
- **The complete** (`Complete`, on `mode.status == "landed"`): the mode and the component are
  deactivated in FlightController's order (mode first), the additive fields cleared, and the
  summon closed (`Arrived` and a `SummonFinishedEvent` to the vehicle, so its handler plays
  vanilla's arrival effects) unless the game has already done so.
- **A vanilla summon near a pad** that this system did not spawn (`OnForeignCandidate`, on
  `EnRoute`): the same engagement, one tick later.

## 3. The spawn

The garage's own summon to a given transform (the one quest nodes use) feeds the same garage
machine, which drives on traffic lanes and binds the car to its own callback. LTBF spawns through
the vehicle system's record spawn instead, `game::VehicleSystem::RequestNewVehicle` (virtual):

`RequestNewVehicle(WorldTransform const&, TweakDBID, red::FixedSizeFunction<void(game::EntityStub*),64,8> const&) -> bool`

- It is not in the address database, so `FlightCarSummon` finds it by scanning the live vtable for
  its image offset. No match returns false and the summon stays vanilla.
- `WorldTransform` is three int32s, the fixed-point position (the float times 2^17), then the
  quaternion. It is not the 32-byte `Transform` of the quest nodes.
- The callback must be real. When the stub materializes, the game calls the callback's dispatcher
  with no null check, so an empty callback crashes the game. `FlightCarSummon.cpp` provides its own
  dispatcher, whose invoke also records the spawned stub's entity id.
- Records come from the player's unlocked list (`GetPlayerUnlockedVehicles`: plain fields), and
  any other TweakDB work is done in C++ (`RED4ext::TweakDB`). The script natives of the TweakDB
  family (`TDBID.*`, `TweakDBInterface.*`, `GarageVehicleID.Resolve`, the record summon native)
  can reach a native wrapper that is not bound in some launches and crash there.
- `GarageVehicleID` is {TweakDBID `recordID`; CName `name`}. Built from the garage's own
  `PlayerVehicle` entry, it never needs `Resolve`, which leaves `recordID` zero for a record it
  cannot find.

## 4. Settings

On the system, category `UI-Settings-Landing-Pads`:

- `UI-Settings-Pad-Summon`: Bool, default on.
- `UI-Settings-Pad-Summon-Range`: Float 10 to 200 m, step 5, default 60: how close to a pad counts
  as near.
- `UI-Settings-Summon-Fallback`: Bool, default on. Off, vanilla's summon is left alone everywhere
  but at pads.
- `UI-Settings-Summon-Always-Fly` (`alwaysFlySummon`): Bool, default off. On, a summon away from
  a pad never goes to vanilla (`FlySummonAlways`, from the summon wraps, for a press vanilla's
  cooldown check would act on): the summoned car already out is taken over where it is and flown
  to V as in the fallback (left to vanilla when it is already next to V or cannot fly), and with
  none out the record is spawned out from V and flown in as in the fallback's case 1. A press
  while a summon flight is under way is dropped.

The keys are in `let_there_be_flight.en-us.json.json` (en-us only). The summon reuses vanilla's
notification widget, which reads the same blackboard state.

## 5. The fallback: a summon that cannot drive to V

Away from pads vanilla's summon runs as written, and the tick watches it (the summon blackboard,
the summoned car against V). When vanilla cannot bring the car to V, the car flies the rest of the
way with the Autopilot and lands next to V: the destination is where V stood, the mode's landing
spot search (`FindLandingSpot`, as for a GPS pin) finds the nearest flat open column on V's level,
and the additive `landingKeepClear` and `landingKeepClearRadius` keep the car off V. Near a pad it
flies to the pad instead. The four cases, each from what the game does:

1. **Nothing at all.** Where the garage finds no road-side spot (a high roof), the summon key does
   nothing: no state, no car, no message, and the native never returns its result to script. So
   the summon wraps snapshot the blackboard before vanilla's call (only for a press vanilla acts
   on, after its own cooldown check) and note the press. When the state, the car's id and the
   car's place are all unchanged `noSpawnTime` (4 s) later, the record (`GetActivePlayerVehicle`,
   or the record summon's own) is spawned out from V as at a pad (`FlySummonToGround`, the same
   spawn) and flown in. After such a refusal later vanilla summons in the same launch do nothing
   either, so the fallback's own spawn is what makes the next summon work.
2. **PathfindingFailed.** The garage spawned the car but made no traffic slot for it. Taken on the
   change to that state. Player summons always set a target, so this comes from quests and other
   summons.
3. **Stalled.** `EnRoute`, and the car has not got `stallProgress` (5 m) closer to V in
   `stallTime` (12 s).
4. **Arrived far.** `Arrived` (on the change, or a re-summon that moved the car without one) with
   the car more than `arrivedLevel` (6 m) above or below V, or more than `arrivedFar` (80 m) away.
   On a street vanilla's first summon spawns about 40 m from V and is `Arrived` a second later
   without driving (the game sets the drive's target to the spawn point), so 40 m is normal and
   only the height or a long way off counts.

A `PathfindingFailed` or `Arrived` left over from an earlier summon is never acted on: only a state
that changed since the last tick. Arrival is judged on the tick after it, so V running off right
then counts as case 4. The takeover is the pad flow's (`TryEngage`), and its landing writes
`Arrived` and the `SummonFinishedEvent` as at a pad.

**Room next to V.** The keep-clear is 4 m (a car's centre that far from V leaves about a metre).
Before the car leaves the ground (`GroundSpot`, through the mode's `FindGroundSpot`) there has to
be a spot: with none, the Autopilot would fly to V's own point and hover there. A car vanilla
placed (cases 2 to 4) is then left where it is. A car of LTBF's own spawn (case 1) tries each floor
under V, nearest first, and only with none at all is it left as spawned.

A vanilla drive (`DriveSummoning`) ends, and the game writes `Arrived`, when its lane or slot goes,
it is blocked, it reaches its target, it is within 10 m of V, within 20 m with V standing and the
car on screen for 5 s, or past V along the path. It has no timeout, which is why case 3 watches
progress.
