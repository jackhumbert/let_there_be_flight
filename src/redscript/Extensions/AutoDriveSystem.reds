// While flying, the autodrive hold switches the Autopilot flight mode (FlightController.
// ToggleAutopilot). Vanilla's own handler (the autodrive HUD widget) may queue the same request
// when the car still counts as on a road lane; ground autodrive must not start under a flying car.
@wrapMethod(AutoDriveSystem)
private func OnEnableAutoDriveRequest(request: ref<EnableAutoDriveRequest>) -> Void {
  if !request.isDelamain && FlightController.GetInstance().IsActive() {
    return;
  }
  wrappedMethod(request);
}

// While the Autopilot flies, UI_AutodriveData.AutoDriveEnabled is ours (FlightModeAutopilot.
// PublishAutodriveUI), and the native GetAutodriveEnabled reads it as vanilla autodrive driving.
// Vanilla's checks then stop "autodrive" when it is not available, which in flight it never is
// (no road lane): a collision's damage reached StopAutodriveIfNecessary through the car's health
// listener and took the panel, the cinematic camera and the facts away with the car still on
// Autopilot. The Autopilot ends itself (FlightModeAutopilot), so none of vanilla's stops apply.
@wrapMethod(AutoDriveSystem)
private func StopAutodriveIfNecessary(stopVehicleIfDeactivated: Bool) -> Void {
  if FlightModeAutopilot.IsEngaged() {
    return;
  }
  wrappedMethod(stopVehicleIfDeactivated);
}

@wrapMethod(AutoDriveSystem)
private func OnAutoDriveHitRequest(request: ref<AutoDriveHitRequest>) -> Void {
  if FlightModeAutopilot.IsEngaged() {
    return;
  }
  wrappedMethod(request);
}

@wrapMethod(AutoDriveSystem)
private func OnUpdateAutodriveOnDestinationChangeRequest(request: ref<UpdateAutodriveOnDestinationChangeRequest>) -> Void {
  if FlightModeAutopilot.IsEngaged() {
    return;
  }
  wrappedMethod(request);
}

@wrapMethod(AutoDriveSystem)
private func OnStopAutoDriveOnDestinationReachedRequest(request: ref<StopAutoDriveOnDestinationReachedRequest>) -> Void {
  if FlightModeAutopilot.IsEngaged() {
    return;
  }
  wrappedMethod(request);
}

@wrapMethod(AutoDriveSystem)
private func OnStopAutoDriveOnTeleportRequest(request: ref<StopAutoDriveOnTeleportRequest>) -> Void {
  if FlightModeAutopilot.IsEngaged() {
    return;
  }
  wrappedMethod(request);
}
