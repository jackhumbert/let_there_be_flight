@if(!ModuleExists("Codeware"))
public native class inkHudEntryInfo extends inkUserData {
  public native let size: Vector2;
  public native let offset: Vector2;

  // public native func SetSize(size: Vector2) -> Void;
}

@addMethod(InputHintManagerGameController)
public func LTBF_ResizeHudEntry() -> Void {
  let hudEntry = this.GetRootWidget().GetUserData(n"inkHudEntryInfo") as inkHudEntryInfo;
  hudEntry.size = new Vector2(1024, 1024); // 1027, 680
  hudEntry.offset = new Vector2(-724.0, -974.0); // -724, -630
}

@if(!ModuleExists("LimitedHudConfig"))
@addMethod(InputHintManagerGameController)
protected cb func OnInitialize() -> Bool {
  this.LTBF_ResizeHudEntry();
}

// @if(ModuleExists("LimitedHudConfig"))
// @wrapMethod(InputHintManagerGameController)
// protected cb func OnInitialize() -> Bool {
//  this.LTBF_ResizeHudEntry();
//  return wrappedMethod();
// }
