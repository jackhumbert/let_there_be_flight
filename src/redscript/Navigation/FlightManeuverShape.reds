// A manoeuvre's geometry in C++ (src/red4ext/Navigation/Aerobatics.hpp). FlightManeuver creates the shape once
// and asks it for the roll, barrel and pitch samples.
public native class FlightManeuverShape extends IScriptable {
  // the shape, as FlightManeuver's fields of the same name (kind is FlightManeuverKind's value)
  public native let kind: Int32;
  public native let side: Float;
  public native let duration: Float;
  public native let speed: Float;
  public native let radius: Float;
  public native let rollTime: Float;
  public native let origin: Vector4;
  public native let dir: Vector4;
  public native let right: Vector4;

  // outputs of SampleAt and BarrelAt
  public native let pos: Vector4;
  public native let vel: Vector4;
  public native let acc: Vector4;
  public native let fwd: Vector4;
  public native let up: Vector4;
  public native let offset: Vector4;
  public native let offsetVelocity: Vector4;

  // fills the shape fields (FlightManeuver.Create)
  public native func Create(kind: Int32, side: Float, position: Vector4, velocity: Vector4, accel: Float, rollTime: Float) -> Void;
  // pos, vel, acc, fwd and up of a pitch manoeuvre at t
  public native func SampleAt(t: Float) -> Void;
  // offset, offsetVelocity and up of a barrel roll at t
  public native func BarrelAt(t: Float) -> Void;
  // the up vector turned about the nose by the aileron roll at t
  public native func RollUpAt(t: Float, f: Vector4, u: Vector4) -> Vector4;
}
