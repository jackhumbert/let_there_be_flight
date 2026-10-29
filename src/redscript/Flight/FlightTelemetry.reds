// Per-frame telemetry to red4ext/logs/ltbf_<name>_<time>.csv (red4ext part, Flight/Telemetry.cpp).
public native class FlightTelemetry {
  // opens a new file with this header line (comma-separated column names); its handle, 0 if it cannot
  public static native func Begin(name: String, header: String) -> Int32;
  public static native func IsOpen(file: Int32) -> Bool;
  // one line of numbers, in the header's order
  public static native func Row(file: Int32, values: array<Float>) -> Void;
  public static native func End(file: Int32) -> Void;
  // seconds since midnight UTC, to line rows up with a recording made outside the game
  public static native func WallClock() -> Float;
}
