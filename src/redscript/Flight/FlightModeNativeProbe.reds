// The C++ mode probe (Flight/Mode.hpp, docs/design/flight-modes.md, FlightMode, the native base): counts its calls, gives no force. In
// the mode list only with the developer setting nativeModeProbe.
public native class FlightModeNativeProbe extends FlightMode {
  public native func Calls() -> String;
}
