// Baked city-wide free space for the autopilot (src/red4ext/Navigation/NavMap.hpp).
// Positions are a car's centre: the bake already grew every solid by the car's size.
public native class FlightNavMap extends IScriptable {
  public native static func IsLoaded() -> Bool;
  // (bottom, top) of each free gap in the column, bottom up; top is 1e9 for open sky
  public native static func GetGaps(position: Vector4) -> array<Vector2>;
  public native static func IsFree(position: Vector4) -> Bool;
  // -1e9 when the position is not in a free gap
  public native static func GetFloor(position: Vector4) -> Float;
  public native static func GetCeiling(position: Vector4) -> Float;
  // fraction of the way from -> to that is free before the first blocked sample
  public native static func SegmentClear(from: Vector4, to: Vector4) -> Float;
  // route as car-centre corner points, empty when there is none; direct tries a straight leg
  // at cruise altitude first. Synchronous.
  public native static func Plan(start: Vector4, goal: Vector4, direct: Bool) -> array<Vector4>;
  public native static func GetLastPlanInfo() -> String;
  public native static func SetPlanParameters(preferredHeight: Float, climbCost: Float, underCost: Float, heuristicWeight: Float) -> Void;
  // the same search on a worker thread: poll IsPlanReady, then TakePlan once
  public native static func PlanAsync(start: Vector4, goal: Vector4, direct: Bool) -> Int32;
  public native static func IsPlanReady(id: Int32) -> Bool;
  public native static func TakePlan(id: Int32) -> array<Vector4>;
  // autopilot destination override (testing, other mods); wins over the GPS target
  public native static func SetTargetOverride(target: Vector4) -> Void;
  public native static func ClearTargetOverride() -> Void;
  public native static func HasTargetOverride() -> Bool;
  public native static func GetTargetOverride() -> Vector4;
}
