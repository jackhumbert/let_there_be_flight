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
