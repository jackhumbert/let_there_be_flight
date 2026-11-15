
// might not be needed in 2.0

@wrapMethod(QuickhackModule)
protected func Process(out task: HUDJob, mode: ActiveMode) -> Void {
  let instruction: ref<QuickhackInstance>;
  if !IsDefined(task.actor) {
    return;
  };
  if IsDefined(this.m_hud.GetCurrentTarget()) && (Equals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.DEVICE) || Equals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.BODY_DISPOSAL_DEVICE) || Equals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.PUPPET) || Equals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.VEHICLE)) {
    if task.actor == this.m_hud.GetCurrentTarget() {
      if this.m_hud.GetCurrentTarget().GetShouldRefreshQHack() {
        this.m_calculateClose = true;
        this.m_hud.GetCurrentTarget().SetShouldRefreshQHack(false);
        instruction = task.instruction.quickhackInstruction;
        if IsDefined(instruction) && IsDefined(task.actor) {
          instruction.SetState(InstanceState.ON, this.DuplicateLastInstance(task.actor));
          instruction.SetContext(this.BaseOpenCheck());
        };
      };
    };
  } else {
    if this.m_calculateClose {
      if !IsDefined(this.m_hud.GetCurrentTarget()) || NotEquals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.DEVICE) || NotEquals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.BODY_DISPOSAL_DEVICE) || NotEquals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.PUPPET) || NotEquals(this.m_hud.GetCurrentTarget().GetType(), HUDActorType.VEHICLE) {
        this.m_calculateClose = false;
        this.m_hud.GetLastTarget().SetShouldRefreshQHack(true);
        QuickhackModule.SendRevealQuickhackMenu(this.m_hud, this.m_hud.GetPlayer().GetEntityID(), false);
      };
    };
  };
}

@wrapMethod(HUDManager)
private final func CanShowHintMessage() -> Bool {
  let attitudeCheck: Bool;
  let currentTargetObj: wref<GameObject>;
  if Equals(this.m_currentTarget.GetType(), HUDActorType.PUPPET) || Equals(this.m_currentTarget.GetType(), HUDActorType.DEVICE) || Equals(this.m_currentTarget.GetType(), HUDActorType.BODY_DISPOSAL_DEVICE) || Equals(this.m_currentTarget.GetType(), HUDActorType.VEHICLE) {
    currentTargetObj = GameInstance.FindEntityByID(this.GetGameInstance(), this.m_currentTarget.GetEntityID()) as GameObject;
    attitudeCheck = NotEquals(GameObject.GetAttitudeTowards(this.GetPlayer(), currentTargetObj), EAIAttitude.AIA_Friendly);
    return this.IsCyberdeckEquipped() && attitudeCheck;
  };
  if Equals(this.m_currentTarget.GetType(), HUDActorType.ITEM) {
    return false;
  };
  return this.IsCyberdeckEquipped();
}

@addMethod(VehicleObject)
public const func ShouldEnableRemoteLayer() -> Bool {
  return this.IsTechie() || this.IsQuickHacksExposed() && this.IsNetrunner();
}

//     // action.SetInactiveWithReason(false, "LocKey#49279");
//     if this.m_flightComponent.isQuickHackable {
//       ArrayPush(actions, ActionFlightEnable(this));
//       ArrayPush(actions, ActionFlightDisable(this));
//       ArrayPush(actions, ActionFlightMalfunction(this));
//       ArrayPush(actions, ActionDisableGravity(this));
//       ArrayPush(actions, ActionEnableGravity(this));
//       ArrayPush(actions, ActionFunhouse(this));
//     }

@replaceMethod(VehicleObject)
protected func SendQuickhackCommands(shouldOpen: Bool) {
  let ltbf_actions: array<ref<DeviceAction>>;
  let actions: array<ref<DeviceAction>>;
  let commands: array<ref<QuickhackData>>;
  let context: GetActionsContext;
  let quickSlotsManagerNotification: ref<RevealInteractionWheel> = new RevealInteractionWheel();
  quickSlotsManagerNotification.lookAtObject = this;
  if shouldOpen {
    
    context = this.GetVehiclePS().GenerateContext(gamedeviceRequestType.Remote, Device.GetInteractionClearance(), GameInstance.GetPlayerSystem(this.GetGame()).GetLocalPlayerControlledGameObject(), this.GetEntityID());
    this.GetVehiclePS().GetRemoteActions(actions, context);
    
    if this.IsFlightAvailable() && this.m_flightComponent.isQuickHackable {
      ArrayPush(ltbf_actions, ActionFlightEnable(this));
      ArrayPush(ltbf_actions, ActionFlightDisable(this));
      ArrayPush(ltbf_actions, ActionFlightMalfunction(this));
      ArrayPush(ltbf_actions, ActionDisableGravity(this));
      ArrayPush(ltbf_actions, ActionEnableGravity(this));
      ArrayPush(ltbf_actions, ActionFunhouse(this));
    }

    if this.m_isQhackUploadInProgress && !this.IsActionQueueEnabled() || this.IsActionQueueFull() {
      ScriptableDeviceComponentPS.SetActionsInactiveAll(ltbf_actions, "LocKey#7020");
      ScriptableDeviceComponentPS.SetActionsInactiveAll(actions, "LocKey#7020");
    };
    if this.IsActionQueueEnabled() {
      QuickHackableQueueHelper.CheckAndSetInactivityReasonForVehicleActions(ltbf_actions, this.m_currentlyUploadingAction);
      QuickHackableQueueHelper.CheckAndSetInactivityReasonForVehicleActions(actions, this.m_currentlyUploadingAction);
    };
    QuickHackableHelper.TranslateActionsIntoQuickSlotCommands(actions, commands, this, this.GetVehiclePS());
    this.TranslateActionsIntoQuickSlotCommands(ltbf_actions, commands);
    quickSlotsManagerNotification.commands = commands;
    quickSlotsManagerNotification.shouldReveal = ArraySize(actions) > 0 || ArraySize(ltbf_actions) > 0;
  };
  GameInstance.GetUISystem(this.GetGame()).QueueEvent(quickSlotsManagerNotification);
}

@addMethod(VehicleObject)
protected cb func OnQuickSlotCommandUsed(evt: ref<QuickSlotCommandUsed>) -> Bool {
  this.ExecuteAction(evt.action, GameInstance.GetPlayerSystem(this.GetGame()).GetLocalPlayerControlledGameObject());
}

@addMethod(VehicleObject)
protected final const func ExecuteAction(choice: InteractionChoice, executor: wref<GameObject>, layerTag: CName) -> Void {
  let action: ref<ScriptableDeviceAction>;
  let sAction: ref<ScriptableDeviceAction>;
  let i: Int32 = 0;
  while i < ArraySize(choice.data) {
    action = FromVariant<ref<ScriptableDeviceAction>>(choice.data[i]);
    if IsDefined(action) {
      if ChoiceTypeWrapper.IsType(choice.choiceMetaData.type, gameinteractionsChoiceType.CheckFailed) {
        return;
      };
      this.ExecuteAction(action, executor);
    };
    sAction = action as ScriptableDeviceAction;
    if IsDefined(sAction) {
      sAction.SetInteractionLayer(layerTag);
    };
    i += 1;
  };
}

@addMethod(VehicleObject)
protected final const func ExecuteAction(action: ref<DeviceAction>, opt executor: wref<GameObject>) -> Bool {
  let sAction: ref<ScriptableDeviceAction> = action as ScriptableDeviceAction;
  if sAction != null {
    sAction.RegisterAsRequester(this.GetEntityID());
    if executor != null {
      sAction.SetExecutor(executor);
    };
    sAction.ProcessRPGAction(this.GetGame());
    return true;
  };
  return false;
}

@addMethod(VehicleObject)
protected cb func OnScanningLookedAt(evt: ref<ScanningLookAtEvent>) -> Bool {
  super.OnScanningLookedAt(evt);
  let playerPuppet: ref<PlayerPuppet> = GameInstance.FindEntityByID(this.GetGame(), evt.ownerID) as PlayerPuppet;
  if IsDefined(playerPuppet) && evt.state {
    if this.IsDead() {
      return IsDefined(null);
    };
    this.UpdateScannerLookAtBB(true);
  } else {
    this.UpdateScannerLookAtBB(false);
  };
}

@addMethod(VehicleObject)
private final func UpdateScannerLookAtBB(b: Bool) -> Void {
  let scannerBlackboard: wref<IBlackboard> = GameInstance.GetBlackboardSystem(this.GetGame()).Get(GetAllBlackboardDefs().UI_Scanner);
  scannerBlackboard.SetBool(GetAllBlackboardDefs().UI_Scanner.ScannerLookAt, b);
}

@addMethod(VehicleObject)
private final const func GetQuickHackDuration(quickHackRecord: wref<ObjectAction_Record>, rootObject: wref<GameObject>, targetID: StatsObjectID, instigatorID: EntityID) -> Float {
  let durationMods: array<wref<ObjectActionEffect_Record>>;
  if !IsDefined(quickHackRecord) {
    return 0.00;
  };
  quickHackRecord.CompletionEffects(durationMods);
  return this.GetObjectActionEffectDurationValue(durationMods, rootObject, targetID, instigatorID);
}

@addMethod(VehicleObject)
private final const func GetQuickHackDuration(quickHackID: TweakDBID, rootObject: wref<GameObject>, targetID: StatsObjectID, instigatorID: EntityID) -> Float {
  let durationMods: array<wref<ObjectActionEffect_Record>>;
  let actionRecord: wref<ObjectAction_Record> = TweakDBInterface.GetObjectActionRecord(quickHackID);
  if !IsDefined(actionRecord) {
    return 0.00;
  };
  actionRecord.CompletionEffects(durationMods);
  return this.GetObjectActionEffectDurationValue(durationMods, rootObject, targetID, instigatorID);
}

@addMethod(VehicleObject)
   private final const func GetIgnoredDurationStats() -> array<wref<StatusEffect_Record>> {
    let result: array<wref<StatusEffect_Record>>;
    ArrayPush(result, TweakDBInterface.GetStatusEffectRecord(t"BaseStatusEffect.WasQuickHacked"));
    ArrayPush(result, TweakDBInterface.GetStatusEffectRecord(t"BaseStatusEffect.QuickHackUploaded"));
    return result;
  }

@addMethod(VehicleObject)
  private final const func GetObjectActionEffectDurationValue(durationMods: array<wref<ObjectActionEffect_Record>>, rootObject: wref<GameObject>, targetID: StatsObjectID, instigatorID: EntityID) -> Float {
    let duration: wref<StatModifierGroup_Record>;
    let durationValue: Float;
    let effectToCast: wref<StatusEffect_Record>;
    let i: Int32;
    let ignoredDurationStats: array<wref<StatusEffect_Record>>;
    let lastMatchingEffect: wref<StatusEffect_Record>;
    let statModifiers: array<wref<StatModifier_Record>>;
    if ArraySize(durationMods) > 0 {
      ignoredDurationStats = this.GetIgnoredDurationStats();
      i = 0;
      while i < ArraySize(durationMods) {
        effectToCast = durationMods[i].StatusEffect();
        if IsDefined(effectToCast) {
          if !ArrayContains(ignoredDurationStats, effectToCast) {
            lastMatchingEffect = effectToCast;
          };
        };
        i += 1;
      };
      effectToCast = lastMatchingEffect;
      duration = effectToCast.Duration();
      duration.StatModifiers(statModifiers);
      durationValue = RPGManager.CalculateStatModifiers(statModifiers, this.GetGame(), rootObject, targetID, Cast<StatsObjectID>(instigatorID));
    };
    return durationValue;
  }

@addMethod(VehicleObject)
private final const func GetICELevel() -> Float {
  let statsSystem: ref<StatsSystem> = GameInstance.GetStatsSystem(this.GetGame());
  let playerLevel: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(GetPlayer(this.GetGame()).GetEntityID()), gamedataStatType.Level);
  let targetLevel: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatType.Level);
  let resistance: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatType.HackingResistance);
  return resistance + 0.50 * (targetLevel - playerLevel);
}

@addMethod(VehicleObject)
  private final func TranslateActionsIntoQuickSlotCommands(actions: array<ref<DeviceAction>>, out commands: array<ref<QuickhackData>>) -> Void {
    let actionCompletionEffects: array<wref<ObjectActionEffect_Record>>;
    let actionRecord: wref<ObjectAction_Record>;
    let actionStartEffects: array<wref<ObjectActionEffect_Record>>;
    let i: Int32;
    let i1: Int32;
    let newCommand: ref<QuickhackData>;    
    let targetActivePrereqs: array<wref<ObjectActionPrereq_Record>>;
    let sAction: ref<FlightAction>;
    let isOngoingUpload: Bool = GameInstance.GetStatPoolsSystem(this.GetGame()).IsStatPoolAdded(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatPoolType.QuickHackUpload);
    let statModifiers: array<wref<StatModifier_Record>>;
    let playerRef: ref<PlayerPuppet> = GetPlayer(this.GetGame());
    let iceLVL: Float = this.GetICELevel();
    let actionOwnerName: CName = StringToName(this.GetDisplayName());
    if ArraySize(actions) == 0 {
      newCommand = new QuickhackData();
      newCommand.m_title = "LocKey#42171";
      newCommand.m_isLocked = true;
      newCommand.m_actionState = EActionInactivityReson.Invalid;
      newCommand.m_actionOwnerName = StringToName(this.GetDisplayName());
      newCommand.m_description = "LocKey#42172";
      ArrayPush(commands, newCommand);
    } else {
      i = 0;
      while i < ArraySize(actions) {
        newCommand = new QuickhackData();
        ArrayClear(actionStartEffects);
        sAction = actions[i] as FlightAction;
        actionRecord = sAction.GetObjectActionRecord();
          newCommand.m_uploadTime = sAction.GetActivationTime();
          newCommand.m_duration = this.GetQuickHackDuration(actionRecord, this, Cast<StatsObjectID>(this.GetEntityID()), playerRef.GetEntityID());         
          newCommand.m_actionOwnerName = actionOwnerName;
          newCommand.m_title = LocKeyToString(actionRecord.ObjectActionUI().Caption());
          newCommand.m_description = LocKeyToString(actionRecord.ObjectActionUI().Description());
          newCommand.m_icon = sAction.icon;
          newCommand.m_iconCategory = actionRecord.GameplayCategory().IconName();
          newCommand.m_type = actionRecord.ObjectActionType().Type();
          newCommand.m_actionOwner = this.GetEntityID();
          newCommand.m_isInstant = false;
          newCommand.m_ICELevel = iceLVL;
          newCommand.m_ICELevelVisible = true;
          newCommand.m_actionState = EActionInactivityReson.Locked;
          newCommand.m_costRaw = BaseScriptableAction.GetBaseCostStatic(playerRef, actionRecord);
          newCommand.m_category = actionRecord.HackCategory();
          ArrayClear(actionCompletionEffects);
          actionRecord.CompletionEffects(actionCompletionEffects);
          newCommand.m_actionCompletionEffects = actionCompletionEffects;
          actionRecord.StartEffects(actionStartEffects);
          i1 = 0;
          while i1 < ArraySize(actionStartEffects) {
            if Equals(actionStartEffects[i1].StatusEffect().StatusEffectType().Type(), gamedataStatusEffectType.PlayerCooldown) {
              actionStartEffects[i1].StatusEffect().Duration().StatModifiers(statModifiers);
              newCommand.m_cooldown = RPGManager.CalculateStatModifiers(statModifiers, this.GetGame(), playerRef, Cast<StatsObjectID>(playerRef.GetEntityID()), Cast<StatsObjectID>(playerRef.GetEntityID()));
              newCommand.m_cooldownTweak = actionStartEffects[i1].StatusEffect().GetID();
              ArrayClear(statModifiers);
            };
            if newCommand.m_cooldown != 0.00 {
            }
            i1 += 1;
          };

          newCommand.m_costRaw = sAction.GetBaseCost();
          newCommand.m_cost = sAction.GetCost();
          if !sAction.IsPossible(this) || !sAction.IsVisible(playerRef) {
            sAction.SetInactiveWithReason(false, "LocKey#7019");
          } else {
            newCommand.m_uploadTime = sAction.GetActivationTime();
            if sAction.IsInactive() {
            } else {
              if !sAction.CanPayCost(playerRef) {
                newCommand.m_actionState = EActionInactivityReson.OutOfMemory;
                sAction.SetInactiveWithReason(false, "LocKey#27398");
              };
              if actionRecord.GetTargetActivePrereqsCount() > 0 {
                ArrayClear(targetActivePrereqs);
                actionRecord.TargetActivePrereqs(targetActivePrereqs);
              };
              if isOngoingUpload {
                sAction.SetInactiveWithReason(false, "LocKey#7020");
              };
            }
          }

          if sAction.IsInactive() {
            newCommand.m_isLocked = true;
            newCommand.m_inactiveReason = sAction.GetInactiveReason();
            if this.HasActiveQuickHackUpload() {
              newCommand.m_action = sAction;
            };
          } else {
            if !sAction.CanPayCost() {
              newCommand.m_actionState = EActionInactivityReson.OutOfMemory;
              newCommand.m_isLocked = true;
              newCommand.m_inactiveReason = "LocKey#27398";
            };
            if GameInstance.GetStatPoolsSystem(this.GetGame()).HasActiveStatPool(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatPoolType.QuickHackUpload) {
              newCommand.m_isLocked = true;
              newCommand.m_inactiveReason = "LocKey#27398";
            };
            if !sAction.IsInactive() || this.HasActiveQuickHackUpload() {
              newCommand.m_action = sAction;
            };
          };
          newCommand.m_actionMatchesTarget = true;
          if !newCommand.m_isLocked {
            newCommand.m_actionState = EActionInactivityReson.Ready;
          };
          ArrayPush(commands, newCommand);
        i += 1;
      };
    };
    i = 0;
    while i < ArraySize(commands) {
      if commands[i].m_isLocked && IsDefined(commands[i].m_action) {
        (commands[i].m_action as ScriptableDeviceAction).SetInactiveWithReason(false, commands[i].m_inactiveReason);
      };
      i += 1;
    };
    QuickhackModule.SortCommandPriority(commands, this.GetGame());
  }

// */

// 2.0 method that isn't working

@wrapMethod(QuickHackDescriptionGameController)
protected cb func OnQuickHackDataChanged(value: Variant) -> Bool {
  wrappedMethod(value);
  this.m_selectedData = FromVariant<ref<QuickhackData>>(value);
  if IsDefined(this.m_selectedData) {
    let title: String = GetLocalizedText(this.m_selectedData.m_title);
    if StrLen(title) == 0 {
      title = ToString(this.m_selectedData.m_title);
    }
    inkTextRef.SetText(this.m_subHeader, title);

    let description: String = GetLocalizedText(this.m_selectedData.m_description);
    if StrLen(description) == 0 {
      description = ToString(this.m_selectedData.m_description);
    }
    inkTextRef.SetText(this.m_description, description);
  }
}