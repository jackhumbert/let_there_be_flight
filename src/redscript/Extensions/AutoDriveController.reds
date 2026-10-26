// Vanilla's autodrive HUD panel also shows the flying Autopilot (FlightModeAutopilot drives it
// through UI_AutodriveData). While flight is on the panel can only be ours (ground autodrive is
// blocked in flight, Extensions/AutoDriveSystem.reds), so it gets the AUTOPILOT logo, drawn in
// the AUTODRIVE logo's style by tools/icons/autopilot_logo.py; vanilla's logo comes back otherwise.
@wrapMethod(AutoDriveController)
private final func OnAutoDriveStateChanged(state: Bool) -> Void {
  wrappedMethod(state);
  let logo = this.GetRootCompoundWidget().GetWidgetByPathName(n"content/vertical/autodriveFrame/autoDriveIcon") as inkImage;
  if !IsDefined(logo) {
    return;
  }
  if state && FlightController.GetInstance().IsActive() {
    logo.SetAtlasResource(r"user\\jackhumbert\\textures\\autopilot_logo.inkatlas");
  } else {
    logo.SetAtlasResource(r"base\\gameplay\\gui\\widgets\\autodrive\\autodrive.inkatlas");
  }
  logo.SetTexturePart(n"logo");
}
