// The route the Autopilot follows (C++, src/red4ext/Navigation/FlightRoute.hpp): its points, the
// distance of each from the start, and the geometry the follower asks of it every frame. The
// segment the car is on stays with the Autopilot and is passed in.
public native class FlightRoute extends IScriptable {
  // the route through points (empty for fewer than two)
  public native static func Create(points: array<Vector4>) -> ref<FlightRoute>;
  // each corner replaced by an arc cut back up to cut metres and share of each leg, where the arc
  // is free on the nav map; the cut steps down by step to smallest before the corner stays sharp
  public native static func RoundCorners(points: array<Vector4>, cut: Float, share: Float, smallest: Float, step: Float) -> array<Vector4>;
  // points lo to hi within radius of point join eased towards their neighbours, passes times,
  // each only where both its segments stay free
  public native static func SmoothJoin(points: array<Vector4>, join: Int32, lo: Int32, hi: Int32, radius: Float, passes: Int32) -> array<Vector4>;

  public native func Size() -> Int32;
  public native func Point(index: Int32) -> Vector4;
  // route distance of point index from the start
  public native func Distance(index: Int32) -> Float;
  public native func Length() -> Float;
  public native func Points() -> array<Vector4>;

  // the segment nearest p from segment from over the next count
  public native func Nearest(p: Vector4, from: Int32, count: Int32) -> Int32;
  // the point on segment segment nearest p
  public native func Project(p: Vector4, segment: Int32) -> Vector4;
  // distance from q to the route from segment on, over ahead metres past the segment's start
  public native func DistanceTo(q: Vector4, segment: Int32, ahead: Float) -> Float;
  // point at route distance s (clamped to the ends)
  public native func At(s: Float) -> Vector4;
  // point along metres on from from, which lies on segment segment
  public native func Ahead(segment: Int32, from: Vector4, along: Float) -> Vector4;
  // the route from from (on segment segment) is free on the nav map for along metres
  public native func ClearAhead(segment: Int32, from: Vector4, along: Float) -> Bool;
  // signed horizontal curvature around the lookahead ahead metres on from from
  public native func Turn(segment: Int32, from: Vector4, ahead: Float) -> Float;
  // curvature over the first part of the lookahead, 3D: towards the bend's centre, 1 / radius long
  public native func Curvature(segment: Int32, from: Vector4, ahead: Float) -> Vector4;
  // direction at route distance s over span either side (zero where that is too short)
  public native func Tangent(s: Float, span: Float) -> Vector4;
  // bend at route distance s over span either side: towards the centre, 1 / radius long
  public native func Bend(s: Float, span: Float) -> Vector4;
  // route distance from from to the next bend of more than 25 degrees over span within 300 m
  public native func SharpBend(segment: Int32, from: Vector4, span: Float) -> Float;
}
