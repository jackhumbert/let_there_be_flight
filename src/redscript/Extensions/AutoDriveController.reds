// Vanilla's autodrive HUD panel also shows the flying Autopilot (FlightModeAutopilot drives it
// through UI_AutodriveData). While flight is on the panel can only be ours (ground autodrive is
// blocked in flight, Extensions/AutoDriveSystem.reds), so it gets the AUTOPILOT logo, drawn in
// the AUTODRIVE logo's style by tools/packaging/icons/autopilot_logo.py; vanilla's logo comes back otherwise.
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

// The panel's distance and time counters show only for vanilla's own destination
// (AutoDriveSystem.GetAutodriveDestinationType); the Autopilot publishes its route's in
// UI_AutodriveData (FlightModeAutopilot.PublishProgress) whatever it flies to. In free roam
// there are none, as in vanilla's.
@wrapMethod(AutoDriveController)
private func UpdateUIData() -> Void {
  if !FlightModeAutopilot.IsEngaged() {
    wrappedMethod();
    return;
  }
  if FlightModeAutopilot.IsFreeRoaming() {
    inkWidgetRef.SetVisible(this.m_remainingDistanceCounterContainer, false);
    inkWidgetRef.SetVisible(this.m_remainingTimeCounterContainer, false);
    return;
  }
  inkWidgetRef.SetVisible(this.m_remainingDistanceCounterContainer, true);
  this.m_remainingDistanceCounterTextParams.UpdateMeasurement("DISTANCE", Cast<Float>(FloorF(this.m_autodriveBB.GetFloat(this.m_autodriveBBDef.DestinationRemainingLength))), EMeasurementUnit.Meter);
  inkWidgetRef.SetVisible(this.m_remainingTimeCounterContainer, true);
  this.m_remainingTimeCounterTextParams.UpdateTime("TIME", FloorF(this.m_autodriveBB.GetFloat(this.m_autodriveBBDef.DestinationRemainingTime)));
}

// The panel's input hint greys out ("Unavailable") when vanilla says autodrive is not available,
// which a damaged car (or no road lane) makes it. In flight the hold toggles the Autopilot
// instead (FlightController.ToggleAutopilot), which neither stops, so the hint stays lit.
// FlightController.Activate and Deactivate signal AutoDriveAvailable to bring this back round.
@wrapMethod(AutoDriveController)
private func UpdateInputHintState() -> Void {
  wrappedMethod();
  if FlightController.GetInstance().IsActive() {
    inkWidgetRef.Get(this.m_inputHintContainer).SetState(n"Default");
  }
}
