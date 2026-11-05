// FlightComponent.FindGround in C++ (Flight/FlightGround.hpp): the same rays and averaging, for the C++ modes; Find is
// for scripts and the bridge: the normal in X Y Z and the distance in W, W -1 when nothing is found.
public native class FlightGround extends IScriptable {
  public native static func Find(component: ref<IScriptable>) -> Vector4;
}
