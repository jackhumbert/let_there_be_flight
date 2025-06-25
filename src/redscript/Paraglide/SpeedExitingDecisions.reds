// @replaceMethod(SpeedExitingDecisions)
// public const func EnterCondition(stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   let exitDirection: vehicleUnmountPosition;
//   let exitRequest: StateResultBool;
//   let vehicle: ref<VehicleObject>;
//   let vehClass: Int32 = stateContext.GetIntParameter(n"vehClass", true);
//   if vehClass == 2 {
//     return false;
//   };
//   if this.IsVehicleExitBlocked1Frame(stateContext, scriptInterface) {
//     return false;
//   };
//   exitRequest = stateContext.GetPermanentBoolParameter(n"validExitRequest");
//   if exitRequest.value {
//     vehicle = scriptInterface.owner as VehicleObject;
//     if vehClass == 1 {
//       exitDirection = vehicle.CanUnmount(true, scriptInterface.executionOwner, vehicleExitDirection.Left);
//       if Equals(exitDirection.direction, vehicleExitDirection.NoDirection) {
//         return false;
//       };
//     };
//     return vehicle.GetCurrentSpeed() > this.GetStaticFloatParameterDefault("highSpeedThreshold", 20.00) || FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.IsActive);
//   };
//   return false;
// }

// @replaceMethod(CoolExitingDecisions)
// public const func EnterCondition(stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   let exitRequest: StateResultBool = stateContext.GetPermanentBoolParameter(n"coolExitRequest");
//   if this.IsVehicleExitBlocked1Frame(stateContext, scriptInterface) {
//     return false;
//   };
//   if !exitRequest.value {
//     return false;
//   };
//   stateContext.SetPermanentBoolParameter(n"coolExitRequest", false, true);
//   if !this.IsCoolExitAllowed(stateContext, scriptInterface) && !FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.IsActive) {
//     return false;
//   };
//   return true;
// }

@wrapMethod(SlideFallDecisions)
protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return wrappedMethod(stateContext, scriptInterface) 
    && !FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive) 
    && !FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.IsActive);
}

@wrapMethod(SlideExitingDecisions)
public const func EnterCondition(stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  return wrappedMethod(stateContext, scriptInterface)
    && !FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive) 
    && !FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.IsActive);
}