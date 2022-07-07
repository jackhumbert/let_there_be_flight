enum GameControllerSwitchPosition {
  Center    = 0,
  Up        = 1,
  UpRight   = 2,
  Right     = 3,
  DownRight = 4,
  Down      = 5,
  DownLeft  = 6,
  Left      = 7,
  UpLeft    = 8
}

public native class ICustomGameController extends IScriptable {
  public native let buttons: array<Bool>;
  // public native let switches: array<GameControllerSwitchPosition>;
  public native let axes: array<Float>;
  
  // public native let buttonKeys: array<EInputKey>;
  // public native let axisKeys: array<EInputKey>;
  // public native let axisInversions: array<Bool>;

  public native func SetButton(button: Int32, key: EInputKey);
  public native func SetAxis(axis: Int32, key: EInputKey, inverted: Bool);

  public func OnSetup() -> Void {

  }

  public func OnUpdate() -> Void {

  }
}

public class CustomGameController extends ICustomGameController {
  public func OnSetup() -> Void {

  }
}