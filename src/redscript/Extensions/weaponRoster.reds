// @addField(weaponRosterGameController)
// let m_FlightStateBlackboardId: ref<CallbackHandle>;

// @wrapMethod(weaponRosterGameController)
// private final func RegisterBB() -> Void {
//   wrappedMethod();
//   let flightBB = FlightController.GetInstance().GetBlackboard();
//   if IsDefined(flightBB) {
//     if !IsDefined(this.m_FlightStateBlackboardId) {
//       this.m_FlightStateBlackboardId = flightBB.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this, n"OnFlightActivate");
//     }
//   }
// }

// @wrapMethod(weaponRosterGameController)
// private final func UnregisterBB() -> Void {
//   wrappedMethod();
//   let flightBB = FlightController.GetInstance().GetBlackboard();
//   if IsDefined(flightBB) {
//     if IsDefined(this.m_FlightStateBlackboardId) {
//        flightBB.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this.m_FlightStateBlackboardId);
//     }
//   }
// }

// @addMethod(weaponRosterGameController)
// private cb func OnFlightActivate() -> Void {
//   this.PlayFold();
// }