// Per-frame telemetry to red4ext/logs/ltbf_<name>_<time>.csv (red4ext part, Flight/Telemetry.cpp).
public native class FlightTelemetry {
  // opens a new file with this header line (comma-separated column names); false if it cannot
  public static native func Begin(name: String, header: String) -> Bool;
  public static native func IsOpen() -> Bool;
  // one line of numbers, in the header's order
  public static native func Row(values: array<Float>) -> Void;
  public static native func End() -> Void;
}
