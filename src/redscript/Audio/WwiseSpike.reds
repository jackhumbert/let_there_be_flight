// Step 0 of docs/design/wwise-audio.md (src/red4ext/Audio/Wwise/WwiseSpike.hpp): a tone through the game's Wwise, for
// tests only (WwiseSpike.Start("mod_sfx_2d", 440, 0.2) at run time).
public native class WwiseSpike extends IScriptable {
  public native static func Start(event: String, frequency: Float, gain: Float) -> String;
  public native static func LoadBank(path: String) -> String;
  public native static func LoadOurBank() -> String;
  public native static func Place(x: Float, y: Float, z: Float, fx: Float, fy: Float, fz: Float) -> String;
  public native static func Listen(ours: Bool, x: Float, y: Float, z: Float, fx: Float, fy: Float, fz: Float) -> String;
  public native static func State() -> String;
  public native static func Stop() -> String;
}
