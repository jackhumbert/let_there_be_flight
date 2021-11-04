public native class FlightLog {
  // defined in red4ext part
  public static native func Info(value: String) -> Void;
  public static native func Warn(value: String) -> Void;
  public static native func Error(value: String) -> Void;
}