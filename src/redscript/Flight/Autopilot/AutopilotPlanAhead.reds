// Plan ahead (docs/design/trajectory.md step 7; the planning is Navigation/PlanAhead.hpp): a trip
// from rest is asked for as FlightNavMap.PlanTripAsync, whose first leg comes in tens of
// milliseconds and is flown at once, while the rest is planned on the planning thread from where
// the car will be on that leg by then. The rest is spliced on at its join
// (FlightModeAutopilot.SpliceRoute); late (the car already past the join), the trip is planned again
// from where the car is. Owned by FlightModeAutopilot.
public class AutopilotPlanAhead extends IScriptable {
  public let enabled: Bool = true;
  public let horizon: Float = 500.0;   // m along the planner's doorway route the first leg goes (tools/navplan/ahead_sweep.py)
  public let minTrip: Float = 600.0;   // m: a trip shorter than this is planned in one go
  public let margin: Float = 2.0;      // s added to the rest's expected plan time before the join (1 s: late on 1 to 3 of 25 routes)

  public let firstLeg: Bool;           // the route is a first leg: it ends short of the goal
  public let waiting: Bool;            // its rest is being planned (trip)
  public let trip: Int32;              // the PlanTripAsync request the rest comes from

  public func Reset() -> Void {
    this.firstLeg = false;
    this.waiting = false;
    this.trip = 0;
  }

  // Asks for the trip planned ahead when the car has no route to keep; false: plan it in one go.
  public func Request(ap: ref<FlightModeAutopilot>, from: Vector4, velocity: Vector4) -> Bool {
    this.Reset();
    if !this.enabled || ArraySize(ap.route) >= 2 {
      return false;
    }
    this.trip = FlightNavMap.PlanTripAsync(from, ap.routeGoal, ap.raceRouteHeight, ap.raceClimbCost, ap.raceMinPassage,
      ap.raceSearchWeight, ap.raceTurnCost, ap.Vehicle(), ap.hits, velocity, this.horizon, this.margin, this.minTrip);
    ap.request = this.trip;
    return true;
  }

  // The plan just taken is the route now: a first leg waits for its rest, joining it joinS along.
  public func Taken(ap: ref<FlightModeAutopilot>) -> Void {
    if this.trip == 0 || !FlightNavMap.GetLastPlanPartial() {
      this.Reset();
      return;
    }
    this.firstLeg = true;
    this.waiting = true;
    ap.joinS = FlightNavMap.GetLastPlanJoin();
    FlightLog.Info("[Autopilot] plan ahead: flying a first leg, the rest joins " + FloatToStringPrec(ap.joinS, 0) + " m along");
  }

  // The rest once it has come: the line from the join on (empty while it has not), and whether it
  // can be spliced on (the car has not passed the join).
  public func TakeRest(ap: ref<FlightModeAutopilot>, out points: array<Vector4>) -> Bool {
    if !this.waiting || !FlightNavMap.IsRestReady(this.trip) {
      return false;
    }
    points = FlightNavMap.TakeRest(this.trip);
    this.waiting = false;
    this.trip = 0;
    let inTime = ArraySize(points) >= 2 && ap.joinS > ap.closestS + 20.0;
    FlightLog.Info("[Autopilot] plan ahead: the rest came " + (inTime ? FloatToStringPrec(ap.joinS - ap.closestS, 0) + " m before the join"
      : (ArraySize(points) >= 2 ? "too late" : "with no route")) + ": " + FlightNavMap.GetLastPlanInfo());
    if inTime {
      ap.joinPoint = points[0];
      this.firstLeg = false;
    }
    return true;
  }
}
