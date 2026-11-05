// Settings only Jack changes (Flight/DeveloperSettings.hpp): developer.ini in the plugin folder, or Set live
// (bridge.py call FlightDeveloperSettings.Set).
public native class FlightDeveloperSettings extends IScriptable {
  public native static func Set(name: String, value: String) -> Bool;
  public native static func Get(name: String) -> String;
  public native static func Describe() -> String;
}
