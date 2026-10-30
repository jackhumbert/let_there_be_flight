// Baked city-wide free space for the autopilot (src/red4ext/Navigation/NavMap.hpp).
// Positions are a car's centre: the bake already grew every solid by the car's size.
public native class FlightNavMap extends IScriptable {
  public native static func IsLoaded() -> Bool;
  // half height of the car the bake grew solids by: a gap's bottom sits this far above the surface
  public native static func GetCarHalfHeight() -> Float;
  // (bottom, top) of each free gap in the column, bottom up; top is 1e9 for open sky
  public native static func GetGaps(position: Vector4) -> array<Vector2>;
  // the blocked space's outlines within radius of centre, sliced at each height: point pairs
  // (a, b, a, b, ...), every edge between a blocked and a free cell; for drawing the map in game
  public native static func GetOutlines(centre: Vector4, radius: Float, heights: array<Float>) -> array<Vector4>;
  // the free space's wireframe within radius of centre, heights zMin to zMax: point pairs on every
  // face between a free and a blocked column, and vertical lines at its corners; columns sampled
  // coarser farther out; cached
  public native static func GetNavEdges(centre: Vector4, radius: Float, zMin: Float, zMax: Float) -> array<Vector4>;
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
  // PlanAsync with the route spread into a racing line of wide bends (the Autopilot's Race option)
  public native static func PlanRaceAsync(start: Vector4, goal: Vector4, preferredHeight: Float, climbCost: Float, minPassage: Float, heuristicWeight: Float, turnCost: Float, vehicle: ref<FlightVehicleModel>, detour: Bool) -> Int32;
  public native static func IsPlanReady(id: Int32) -> Bool;
  public native static func TakePlan(id: Int32) -> array<Vector4>;
  // autopilot destination override (testing, other mods); wins over the GPS target
  public native static func SetTargetOverride(target: Vector4) -> Void;
  public native static func ClearTargetOverride() -> Void;
  public native static func HasTargetOverride() -> Bool;
  public native static func GetTargetOverride() -> Vector4;
  // AV landing pads found in the world (helipads.tsv, docs/archive/helipads.md), index 0 .. count - 1.
  // Position is the deck centre a car rests on (w = 1); an index out of range gives zeros / "".
  public native static func GetHelipadCount() -> Int32;
  public native static func GetHelipadPosition(index: Int32) -> Vector4;
  // degrees, from the pad prefab or its painted marking
  public native static func GetHelipadYaw(index: Int32) -> Float;
  public native static func GetHelipadRadius(index: Int32) -> Float;
  // free height above the deck; 1e9 for open sky
  public native static func GetHelipadClearance(index: Int32) -> Float;
  public native static func GetHelipadId(index: Int32) -> String;
  // empty unless named in tools/pads/helipads_overrides.json
  public native static func GetHelipadName(index: Int32) -> String;
  // nearest pad within maxDistance metres (horizontal), or -1
  public native static func FindNearestHelipad(position: Vector4, maxDistance: Float) -> Int32;
  // pads within radius metres (horizontal), nearest first
  public native static func FindHelipadsInRange(position: Vector4, radius: Float) -> array<Int32>;
  // free roam gates: openings to fly through (gates.tsv, tools/navmap_bake/gates.py), index 0 .. count - 1.
  // Position is the opening's centre at the height to fly it (w = 1); an index out of range gives zeros.
  public native static func GetGateCount() -> Int32;
  public native static func GetGatePosition(index: Int32) -> Vector4;
  // degrees, the way through (either direction)
  public native static func GetGateYaw(index: Int32) -> Float;
  // gates within radius metres (horizontal), nearest first
  public native static func FindGatesInRange(position: Vector4, radius: Float) -> array<Int32>;
}
