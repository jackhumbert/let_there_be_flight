public class VehicleFlightDisabledDecisions extends VehicleFlightTransition {
  
  public const func ToVehicleFlightEnabled(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    return this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight();
    // if VehicleTransitiorn.CanEnterVehicleFlight() {
      // let fc = scriptInterface.owner.FindComponentByName(n"flightComponent") as FlightComponent;
      // if (scriptInterface.IsActionJustTapped(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) &&
      //     GameInstance.GetQuestsSystem(scriptInterface.GetGame()).GetFact(n"map_blocked") == 0 &&
      //     Equals(this.GetCurrentTier(stateContext), GameplayTier.Tier1_FullGameplay) {
  }

  public const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    return !(this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight());
  }

}

public class VehicleFlightDisabledEvents extends VehicleFlightEventsTransition {

}