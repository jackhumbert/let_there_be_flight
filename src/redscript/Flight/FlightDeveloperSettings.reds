// Developer settings (Flight/DeveloperSettings.hpp): developer.ini in the plugin folder, or Set live
// (FlightDeveloperSettings.Set(name, value)).
public native class FlightDeveloperSettings extends IScriptable {
  public native static func Set(name: String, value: String) -> Bool;
  public native static func Get(name: String) -> String;
  public native static func Describe() -> String;
}
