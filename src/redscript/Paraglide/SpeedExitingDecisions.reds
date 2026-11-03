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