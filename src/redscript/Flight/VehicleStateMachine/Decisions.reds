public class FlightDecisions extends VehicleTransition {

  public func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    return true;
  }

  public final func ToDrive(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let value = scriptInterface.IsActionJustPressed(n"Flight_Toggle");
    if value {
      FlightLog.Info("[FlightDecisions] ToDrive");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", true, true);
    }
    return value;
  }

  public final const func ToFlightDriverCombatFirearms(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let drawnWeapon: StateResultBool;
    let equipWeaponActionTapped: Bool;
    // let notificationEvent: ref<UIInGameNotificationEvent>;
    let questForceEnableCombat: StateResultBool;
    let value: Bool = false;
    if this.DoesVehicleSupportFireArms(scriptInterface.owner as VehicleObject) {
      equipWeaponActionTapped = (scriptInterface.IsActionJustTapped(n"SwitchItem") || scriptInterface.IsActionJustTapped(n"HolsterWeapon") || scriptInterface.IsActionJustTapped(n"NextWeapon") || scriptInterface.IsActionJustTapped(n"PreviousWeapon")) && EquipmentSystem.GetData(scriptInterface.executionOwner).GetLastUsedOrFirstAvailableDriverCombatWeapon(this.GetDriverCombatWeaponTag(scriptInterface.owner as VehicleObject)) == ItemID.None() && scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vision) != EnumInt(gamePSMVision.Focus);
      // if !this.IsPlayerAllowedToEnterDriverCombat(stateContext, scriptInterface) {
      //   if equipWeaponActionTapped {
      //     notificationEvent = new UIInGameNotificationEvent();
      //     notificationEvent.m_notificationType = UIInGameNotificationType.ActionRestriction;
      //     scriptInterface.GetUISystem().QueueEvent(notificationEvent);
      //   };
      // } else {
        if equipWeaponActionTapped || UpperBodyTransition.HasAnyWeaponEquipped(scriptInterface) {
          value = true;
        };
        if Equals(this.GetPuppetVehicleSceneTransition(stateContext), PuppetVehicleState.CombatSeated) {
          value = true;
        };
        drawnWeapon = stateContext.GetPermanentBoolParameter(n"drawnWeapon");
        if drawnWeapon.value && EquipmentSystem.GetData(scriptInterface.executionOwner).GetLastUsedOrFirstAvailableDriverCombatWeapon(this.GetDriverCombatWeaponTag(scriptInterface.owner as VehicleObject)) == ItemID.None() {
          value = true;
        };
        questForceEnableCombat = stateContext.GetTemporaryBoolParameter(n"startVehicleCombat");
        if questForceEnableCombat.value {
          value = true;
        };
      // };
    };
    if value {
      FlightLog.Info("[FlightDecisions] ToFlightDriverCombatFirearms");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", false, true);
    }
    return value;
  }

  public final const func ToFlightDriverCombatMountedWeapons(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let drawnWeapon: StateResultBool;
    let equipWeaponActionTapped: Bool;
    // let notificationEvent: ref<UIInGameNotificationEvent>;
    let questForceEnableCombat: StateResultBool;
    let value: Bool = false;
    if Equals(this.GetVehicleDriverCombatType(scriptInterface.owner as VehicleObject), gamedataDriverCombatType.MountedWeapons) {
      equipWeaponActionTapped = (scriptInterface.IsActionJustTapped(n"MountedWeapons_SwitchWeapons") || scriptInterface.IsActionJustTapped(n"MountedWeapons_HolsterWeapon") || scriptInterface.IsActionJustTapped(n"MountedWeapons_NextWeapon") || scriptInterface.IsActionJustTapped(n"MountedWeapons_PreviousWeapon") || scriptInterface.IsActionJustTapped(n"MountedWeapons_WeaponSlot1") || scriptInterface.IsActionJustTapped(n"MountedWeapons_WeaponSlot2") && (scriptInterface.owner as VehicleObject).CanSwitchWeapons()) && scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vision) != EnumInt(gamePSMVision.Focus);
      // if !this.IsPlayerAllowedToEnterDriverCombat(stateContext, scriptInterface) || (scriptInterface.owner as VehicleObject).GetVehicleComponent().IsVehicleInDecay() {
      //   if equipWeaponActionTapped {
      //     notificationEvent = new UIInGameNotificationEvent();
      //     notificationEvent.m_notificationType = UIInGameNotificationType.ActionRestriction;
      //     scriptInterface.GetUISystem().QueueEvent(notificationEvent);
      //   };
      // } else {
        if equipWeaponActionTapped {
          value = true;
        };
        if Equals(this.GetPuppetVehicleSceneTransition(stateContext), PuppetVehicleState.CombatSeated) {
          value = true;
        };
        drawnWeapon = stateContext.GetPermanentBoolParameter(n"drawnWeapon");
        if drawnWeapon.value {
          value = true;
        };
        questForceEnableCombat = stateContext.GetTemporaryBoolParameter(n"startVehicleCombat");
        if questForceEnableCombat.value {
          value = true;
        };
      // };
    };
    if value {
      FlightLog.Info("[FlightDecisions] ToFlightDriverCombatMountedWeapons");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", false, true);
    }
    return value;
  }
}

public class FlightDriverCombatDecisions extends FlightDecisions {

  // public final func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
  //   return true;
  // }
  
  public final const func ToFlightCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    if !this.IsPlayerAllowedToEnterCombat(scriptInterface) {
      FlightLog.Info("[FlightDriverCombatDecisions] ToFlightCondition");
      return true;
    };
    return false;
  }

  public final const func ToCombatExiting(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let exitRequest: StateResultBool;
    if this.GetVehClass(stateContext, scriptInterface) == 2 {
      return false;
    };
    exitRequest = stateContext.GetPermanentBoolParameter(n"validExitRequest");
    if exitRequest.value {
      FlightLog.Info("[FlightDriverCombatDecisions] ToCombatExiting");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", true, true);
    }
    return exitRequest.value;
  }

  public final func ToDriverCombat(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let value = scriptInterface.IsActionJustPressed(n"Flight_Toggle");
    if value {
      FlightLog.Info("[FlightDriverCombatDecisions] ToDriverCombat");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", true, true);
    }
    return value;
  }

  public const func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    if this.ToFlightCondition(stateContext, scriptInterface) {
      FlightLog.Info("[FlightDriverCombatDecisions] ToFlight");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", false, true);
      return true;
    };
    return false;
  }
}

public class FlightDriverCombatMountedWeaponsDecisions extends FlightDriverCombatDecisions {

  public final func ToDriverCombatMountedWeapons(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let value = scriptInterface.IsActionJustPressed(n"Flight_Toggle");
    if value {
      FlightLog.Info("[FlightDriverCombatMountedWeaponsDecisions] ToDriverCombatMountedWeapons");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", true, true);
    }
    return value;
  }

  public final const func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let driverCombatForbiddenZone: StateResultBool;
    let questForceEnableCombat: StateResultBool;
    let value: Bool = false;
    if this.ToFlightCondition(stateContext, scriptInterface) {
      value = true;
    };
    if (scriptInterface.owner as VehicleObject).GetVehicleComponent().IsVehicleInDecay() {
      value = true;
    };
    if scriptInterface.IsActionJustReleased(n"Exit") || scriptInterface.IsActionJustTapped(n"MountedWeapons_HolsterWeapon") {
      value = true;
    };
    questForceEnableCombat = stateContext.GetTemporaryBoolParameter(n"stopVehicleCombat");
    if questForceEnableCombat.value {
      value = true;
    };
    driverCombatForbiddenZone = stateContext.GetPermanentBoolParameter(n"driverCombatForbiddenZone");
    if driverCombatForbiddenZone.value {
      value = true;
    };
    if value {
      FlightLog.Info("[FlightDriverCombatMountedWeaponsDecisions] ToFlight");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", false, true);
    }
    return value;
  }
}

public class FlightDriverCombatFirearmsDecisions extends FlightDriverCombatDecisions {

  public final func ToDriverCombatFirearms(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let value = scriptInterface.IsActionJustPressed(n"Flight_Toggle");
    if value {
      FlightLog.Info("[FlightDriverCombatDecisions] ToDriverCombatFirearms");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", true, true);
    }
    return value;
  }

  public final const func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
    let value: Bool = false;
    if this.ToFlightCondition(stateContext, scriptInterface) {
      value = true;
    };
    if stateContext.IsStateActive(n"UpperBody", n"emptyHands") && this.GetInStateTime() >= 0.50 && (stateContext.IsStateActive(n"Equipment", n"selfRemoval") || !stateContext.IsStateMachineActive(n"Equipment")) {
      value = true;
    };
    if value {
      FlightLog.Info("[FlightDriverCombatFirearmsDecisions] ToFlight");
      stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", false, true);
    }
    return value;
  }
}