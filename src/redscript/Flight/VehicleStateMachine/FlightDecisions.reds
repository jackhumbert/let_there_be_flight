public class FlightDecisions extends DriveDecisions {

  public final func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // FlightLog.Info("[FlightDecisions] EnterCondition");
    // return scriptInterface.IsActionJustPressed(n"Flight_Toggle");
    // let fc = scriptInterface.owner.FindComponentByName(n"flightComponent") as FlightComponent;
    // return IsDefined(fc) && fc.configuration.CanActivate();
    return true;
    // return scriptInterface.IsActionJustPressed(n"Flight_Toggle");
  }

  public final func ToDrive(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    // FlightLog.Info("[FlightDecisions] ToDrive");
    return scriptInterface.IsActionJustPressed(n"Flight_Toggle");
  }

  public const func ToFlightDriverCombat(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let drawnWeapon: StateResultBool;
    let equipWeaponActionTapped: Bool;
    // let notificationEvent: ref<UIInGameNotificationEvent>;
    let questForceEnableCombat: StateResultBool;
    // if this.DoesVehicleSupportFireArms(scriptInterface.owner as VehicleObject) {
      equipWeaponActionTapped = (scriptInterface.IsActionJustTapped(n"SwitchItem") || scriptInterface.IsActionJustTapped(n"HolsterWeapon") || scriptInterface.IsActionJustTapped(n"NextWeapon") || scriptInterface.IsActionJustTapped(n"PreviousWeapon")) && EquipmentSystem.GetData(scriptInterface.executionOwner).GetLastUsedOrFirstAvailableDriverCombatWeapon(this.GetDriverCombatWeaponTag(scriptInterface.owner as VehicleObject)) == ItemID.None() && scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vision) != EnumInt(gamePSMVision.Focus);
      // if !this.IsPlayerAllowedToEnterDriverCombat(stateContext, scriptInterface) {
      //   if equipWeaponActionTapped {
      //     notificationEvent = new UIInGameNotificationEvent();
      //     notificationEvent.m_notificationType = UIInGameNotificationType.ActionRestriction;
      //     scriptInterface.GetUISystem().QueueEvent(notificationEvent);
      //     return false;
      //   };
      // } else {
        if equipWeaponActionTapped || UpperBodyTransition.HasAnyWeaponEquipped(scriptInterface) {
          return true;
        };
        if Equals(this.GetPuppetVehicleSceneTransition(stateContext), PuppetVehicleState.CombatSeated) {
          return true;
        };
        drawnWeapon = stateContext.GetPermanentBoolParameter(n"drawnWeapon");
        if drawnWeapon.value && EquipmentSystem.GetData(scriptInterface.executionOwner).GetLastUsedOrFirstAvailableDriverCombatWeapon(this.GetDriverCombatWeaponTag(scriptInterface.owner as VehicleObject)) == ItemID.None() {
          return true;
        };
        questForceEnableCombat = stateContext.GetTemporaryBoolParameter(n"startVehicleCombat");
        if questForceEnableCombat.value {
          return true;
        };
      // };
    // };
    return false;
  }
}
