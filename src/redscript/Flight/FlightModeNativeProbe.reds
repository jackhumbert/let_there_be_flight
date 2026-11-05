// The C++ mode spike (Flight/Mode.hpp, docs/design/hover-fly-native.md step 1): counts its calls, gives no force. In
// the mode list only with the developer setting nativeModeProbe.
public native class FlightModeNativeProbe extends FlightMode {
  public native func Calls() -> String;
}
