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
public const func IsQuickHackAble() -> Bool {
  return true;
}

@addMethod(VehicleObject)
public const func IsQuickHacksExposed() -> Bool {
  return true;
}

@addMethod(VehicleObject)
public const func ShouldEnableRemoteLayer() -> Bool {
  return this.IsTechie() || this.IsQuickHacksExposed() && this.IsNetrunner();
}

@addMethod(VehicleObject)
public const func CanRevealRemoteActionsWheel() -> Bool {
  if !this.ShouldRegisterToHUD() {
    return false;
  };
  if !this.IsQuickHackAble() {
    return false;
  };
  return true;
}

// @addMethod(VehicleObject)
// protected cb func OnQuickHackPanelStateChanged(evt: ref<QuickHackPanelStateEvent>) -> Bool {
  // this.DetermineInteractionStateByTask();
// }

@addMethod(VehicleObject)
public const func ShouldRegisterToHUD() -> Bool {
  return true;
}

// @addMethod(VehicleObject)
// protected final func DetermineInteractionStateByTask() -> Void {
//   GameInstance.GetDelaySystem(this.GetGame()).QueueTask(this, null, n"DetermineInteractionStateTask", gameScriptTaskExecutionStage.Any);
// }

// @addMethod(VehicleObject)
// protected final func DetermineInteractionStateTask(data: ref<ScriptTaskData>) -> Void {
//   this.DetermineInteractionState();
// }

// @addMethod(VehicleObject)
// protected final func DetermineInteractionState() -> Void {
//   let context: GetActionsContext;
//     context.requestorID = this.GetEntityID();
//     context.requestType = gamedeviceRequestType.Remote;
//     context.processInitiatorObject = GetPlayer(this.GetGame());
//     this.GetPS().DetermineInteractionState(this.m_interactionComponent, context, this.m_objectActionsCallbackCtrl);
// }

public class FlightAction extends ScriptableDeviceAction {
  public let m_owner: wref<VehicleObject>;
  public let title: CName;
  public let description: CName;
  public let icon: TweakDBID;
}

public class FlightEnable extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightEnable";
    this.title = n"Quickhacks-Enable-Flight";
    this.description = n"Quickhacks-Enable-Flight-Description";
    this.icon = t"UIIcon.SystemCollapse";
    this.SetObjectActionID(t"DeviceAction.FlightAction");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.m_flightComponent.active;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[FlightEnable] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.UnsetPhysicsStates();
    this.m_owner.EndActions();
    this.m_owner.m_flightComponent.Activate(true);
  }
}

public class FlightDisable extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightDisable";
    this.title = n"Quickhacks-Disable-Flight";
    this.description = n"Quickhacks-Disable-Flight-Description";
    this.icon = t"UIIcon.SystemCollapse";
    this.SetObjectActionID(t"DeviceAction.FlightAction");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && v.m_flightComponent.active;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[FlightDisable] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.UnsetPhysicsStates();
    this.m_owner.EndActions();
    this.m_owner.m_flightComponent.Deactivate(true);
  }
}

public class FlightMalfunction extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightMalfunction";
    this.title = n"Quickhacks-Initiate-Launch";
    this.description = n"Quickhacks-Initiate-Launch-Description";
    this.icon = t"UIIcon.TurretMalfunction";
    this.SetObjectActionID(t"DeviceAction.FlightMalfunction");
  }
}

public class DisableGravity extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"DisableGravity";
    this.title = n"Quickhacks-Enable-Antigrav";
    this.description = n"Quickhacks-Enable-Antigrav-Description";
    this.icon = t"UIIcon.SystemCollapse";
    this.SetObjectActionID(t"ChoiceIcons.EngineeringIcon");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && v.HasGravity();
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[DisableGravity] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.UnsetPhysicsStates();
    this.m_owner.EndActions();
    this.m_owner.EnableGravity(false);
  }
}

public class EnableGravity extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"EnableGravity";
    this.title = n"Quickhacks-Disable-Antigrav";
    this.description = n"Quickhacks-Disable-Antigrav-Description";
    this.icon = t"UIIcon.SystemCollapse";
    this.SetObjectActionID(t"ChoiceIcons.EngineeringIcon");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.HasGravity();
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[EnableGravity] CompleteAction");
    super.CompleteAction(gameInstance);
    this.m_owner.EnableGravity(true);
  }
}

public class Bouncy extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"Bouncy";
    this.title = n"Quickhacks-Funhouse";
    this.description = n"Quickhacks-Funhouse-Description";
    this.icon = t"ChoiceIcons.SabotageIcon";
    this.SetObjectActionID(t"DeviceAction.Bouncy");
  }

  public func IsPossible(target: wref<GameObject>, opt actionRecord: wref<ObjectAction_Record>, opt objectActionsCallbackController: wref<gameObjectActionsCallbackController>) -> Bool {
    let v = target as VehicleObject;
    return super.IsPossible(target, actionRecord, objectActionsCallbackController) && !v.bouncy;
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[Bouncy] CompleteAction");
    super.CompleteAction(gameInstance);
  }
}

// @addMethod(VehicleComponentPS)
// public func OnFlightMalfunction(evt: ref<FlightMalfunction>) -> EntityNotificationType {
//   FlightLog.Info("[VehicleComponentPS] OnFlightMalfunction");
//   return EntityNotificationType.SendThisEventToEntity;
// }

// @addMethod(VehicleObject)
// protected cb func OnFlightMalfunction(evt: ref<FlightMalfunction>) -> Bool {
//   FlightLog.Info("[VehicleObject] OnFlightMalfunction");
// }

// @addMethod(VehicleObject)
// protected cb func OnPerformedAction(evt: ref<PerformedAction>) -> Bool {
//     FlightLog.Info("[VehicleObject] OnPerformedAction");
//   let action: ref<ScriptableDeviceAction>;
//   // let sequenceQuickHacks: ref<ForwardAction>;
//   this.SetScannerDirty(true);
//   action = evt.m_action as ScriptableDeviceAction;
//   // this.ExecuteBaseActionOperation(evt.m_action.GetClassName());
//   if action.CanTriggerStim() {
//     // this.TriggerAreaEffectDistractionByAction(action);
//   };
//   if IsDefined(action) && action.IsIllegal() && !action.IsQuickHack() {
//     // this.ResolveIllegalAction(action.GetExecutor(), action.GetDurationValue());
//   };
//   // if this.IsConnectedToActionsSequencer() && !this.IsLockedViaSequencer() {
//     // sequenceQuickHacks = new ForwardAction();
//     // sequenceQuickHacks.requester = this.GetDevicePS().GetID();
//     // sequenceQuickHacks.actionToForward = action;
//     // GameInstance.GetPersistencySystem(this.GetGame()).QueuePSEvent(this.GetDevicePS().GetActionsSequencer().GetID(), this.GetDevicePS().GetActionsSequencer().GetClassName(), sequenceQuickHacks);
//   // };
//   // this.ResolveQuestImportanceOnPerformedAction(action);
// }

public func ActionFlightEnable(owner: ref<VehicleObject>) -> ref<FlightEnable> {
    let action: ref<FlightEnable> = new FlightEnable();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionFlightDisable(owner: ref<VehicleObject>) -> ref<FlightDisable> {
    let action: ref<FlightDisable> = new FlightDisable();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionFlightMalfunction(owner: ref<VehicleObject>) -> ref<FlightMalfunction> {
    let action: ref<FlightMalfunction> = new FlightMalfunction();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.m_owner = owner;
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionDisableGravity(owner: ref<VehicleObject>) -> ref<DisableGravity> {
    let action: ref<DisableGravity> = new DisableGravity();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionEnableGravity(owner: ref<VehicleObject>) -> ref<EnableGravity> {
    let action: ref<EnableGravity> = new EnableGravity();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    action.SetExecutor(GetPlayer(owner.GetGame()));
    return action;
}

public func ActionBouncy(owner: ref<VehicleObject>) -> ref<Bouncy> {
    let action: ref<Bouncy> = new Bouncy();
    action.m_owner = owner;
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(owner.GetPS());
    action.SetProperties();
    return action;
}

@addMethod(VehicleObject)
protected func SendQuickhackCommands(shouldOpen: Bool) {
  // FlightLog.Info("[VehicleObject] SendQuickhackCommands()");
  let actions: array<ref<DeviceAction>>;
  let commands: array<ref<QuickhackData>>;
  // let context: GetActionsContext;
  let quickSlotsManagerNotification: ref<RevealInteractionWheel> = new RevealInteractionWheel();
  quickSlotsManagerNotification.lookAtObject = this;
  quickSlotsManagerNotification.shouldReveal = shouldOpen;
  if shouldOpen {
    // FlightLog.Info("[VehicleObject] SendQuickhackCommands() shouldOpen == true");
    // context = this.GetPS().GenerateContext(gamedeviceRequestType.Remote, Device.GetInteractionClearance(), this.GetPlayerMainObject(), this.GetEntityID());
    // this.GetPS().GetRemoteActions(actions, context);
    
    // action.SetInactiveWithReason(false, "LocKey#49279");
    if this.m_flightComponent.isQuickHackable {
      ArrayPush(actions, ActionFlightEnable(this));
      ArrayPush(actions, ActionFlightDisable(this));
      ArrayPush(actions, ActionFlightMalfunction(this));
      ArrayPush(actions, ActionDisableGravity(this));
      ArrayPush(actions, ActionEnableGravity(this));
      ArrayPush(actions, ActionBouncy(this));
    }

    if this.m_isQhackUploadInProgerss {
      ScriptableDeviceComponentPS.SetActionsInactiveAll(actions, "LocKey#7020");
    };
    this.TranslateActionsIntoQuickSlotCommands(actions, commands);
    quickSlotsManagerNotification.commands = commands;
  };
  HUDManager.SetQHDescriptionVisibility(this.GetGame(), shouldOpen);
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

@addField(VehicleObject)
public let m_isQhackUploadInProgerss: Bool;

@addMethod(VehicleObject)
protected cb func OnUploadProgressStateChanged(evt: ref<UploadProgramProgressEvent>) -> Bool {
  // FlightLog.Info("[VehicleObject] OnUploadProgressStateChanged");
  if Equals(evt.progressBarContext, EProgressBarContext.QuickHack) {
    if Equals(evt.progressBarType, EProgressBarType.UPLOAD) {
      if Equals(evt.state, EUploadProgramState.STARTED) {
        this.m_isQhackUploadInProgerss = true;
      } else {
        if Equals(evt.state, EUploadProgramState.COMPLETED) {
          this.m_isQhackUploadInProgerss = false;
        };
      };
    };
  };
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
    let choice: InteractionChoice;
    let emptyChoice: InteractionChoice;
    let i: Int32;
    let i1: Int32;
    let newCommand: ref<QuickhackData>;    
    // let prereqsToCheck: array<wref<IPrereq_Record>>;
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
        if NotEquals(actionRecord.ObjectActionType().Type(), gamedataObjectActionType.DeviceQuickHack) {
        } else {
          newCommand.m_uploadTime = sAction.GetActivationTime();
          // newCommand.m_duration = sAction.GetDurationValue();
          newCommand.m_duration = this.GetQuickHackDuration(actionRecord, this, Cast<StatsObjectID>(this.GetEntityID()), playerRef.GetEntityID());         
          newCommand.m_title = GetLocalizedTextByKey(sAction.title);
          if StrLen(newCommand.m_title) == 0 {
            newCommand.m_title = ToString(sAction.title);
          };
          newCommand.m_description = GetLocalizedTextByKey(sAction.description);
          if StrLen(newCommand.m_description) == 0 {
            newCommand.m_description = ToString(sAction.title);
          };
          newCommand.m_actionOwnerName = actionOwnerName;
          // newCommand.m_title = LocKeyToString(actionRecord.ObjectActionUI().Caption());
          // newCommand.m_description = LocKeyToString(actionRecord.ObjectActionUI().Description());
          newCommand.m_icon = sAction.icon;
          newCommand.m_iconCategory = actionRecord.GameplayCategory().IconName();
          newCommand.m_type = actionRecord.ObjectActionType().Type();
          newCommand.m_actionOwner = this.GetEntityID();
          newCommand.m_isInstant = false;
          newCommand.m_ICELevel = iceLVL;
          newCommand.m_ICELevelVisible = true;
          // newCommand.m_vulnerabilities = this.GetPS().GetActiveQuickHackVulnerabilities();
          newCommand.m_actionState = EActionInactivityReson.Locked;
          // newCommand.m_quality = sAction.quality;
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
          if !IsDefined(this as GenericDevice) {
            choice = emptyChoice;
            choice = sAction.GetInteractionChoice();
            if TDBID.IsValid(choice.choiceMetaData.tweakDBID) {
              newCommand.m_titleAlternative = LocKeyToString(TweakDBInterface.GetInteractionBaseRecord(choice.choiceMetaData.tweakDBID).Caption());
            };
          };

          newCommand.m_costRaw = sAction.GetBaseCost();
          newCommand.m_cost = sAction.GetCost();
          if !sAction.IsPossible(this) || !sAction.IsVisible(playerRef) {
            sAction.SetInactiveWithReason(false, "LocKey#7019");
          } else {
            newCommand.m_uploadTime = sAction.GetActivationTime();
            let interactionChoice = sAction.GetInteractionChoice();
            let i2 = 0;
            while i2 < ArraySize(interactionChoice.captionParts.parts) {
              if IsDefined(interactionChoice.captionParts.parts[i2] as InteractionChoiceCaptionStringPart) {
                newCommand.m_title = GetLocalizedText(interactionChoice.captionParts.parts[i2] as InteractionChoiceCaptionStringPart.content);
              };
              i2 += 1;
            };
            if sAction.IsInactive() {
            } else {
              if !sAction.CanPayCost(playerRef) {
                newCommand.m_actionState = EActionInactivityReson.OutOfMemory;
                sAction.SetInactiveWithReason(false, "LocKey#27398");
              };
              if actionRecord.GetTargetActivePrereqsCount() > 0 {
                ArrayClear(targetActivePrereqs);
                actionRecord.TargetActivePrereqs(targetActivePrereqs);
                // i2 = 0;
                // while i2 < ArraySize(targetActivePrereqs) {
                //   ArrayClear(prereqsToCheck);
                //   targetActivePrereqs[i2].FailureConditionPrereq(prereqsToCheck);
                //   if !RPGManager.CheckPrereqs(prereqsToCheck, this) {
                //     sAction.SetInactiveWithReason(false, targetActivePrereqs[i2].FailureExplanation());
                //   }
                //   i2 += 1;
                // };
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
        };
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


public class FlightMalfunctionEffector extends Effector {
  public let m_owner: wref<VehicleObject>;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOn");
    this.m_owner = owner as VehicleObject;
    if IsDefined(this.m_owner) {
      this.m_owner.UnsetPhysicsStates();
      this.m_owner.EndActions();
      this.m_owner.m_flightComponent.Activate(true);
      this.m_owner.m_flightComponent.lift = 5.0;
    }
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOff");
    if IsDefined(this.m_owner) {
      this.m_owner.m_flightComponent.Deactivate(true);
    }
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] Uninitialize");
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
    this.m_owner.m_flightComponent.Deactivate(true);
  }
}

public class DisableGravityEffector extends Effector {

}

public class BouncyEffector extends Effector {
  public let m_owner: wref<GameObject>;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[BouncyEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[BouncyEffector] ActionOn");
    this.m_owner = owner;
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.bouncy = true;
      vehicle.ignoreImpulses = false;
      vehicle.UnsetPhysicsStates();
      vehicle.EndActions();
      vehicle.m_flightComponent.FireVerticalImpulse(0);
    }
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[BouncyEffector] ActionOff");
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.bouncy = false;
    }
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
  }
}

// @wrapMethod(QuickhacksListGameController)
// private final func SelectData(data: ref<QuickhackData>) -> Void {
//   wrappedMethod(data);
//   // let description: String = GetLocalizedText(this.m_selectedData.m_description);
//   // if StrLen(description) == 0 || Equals(description, "Loading") {
//       let description = ToString(this.m_selectedData.m_description);
//   // };
//   inkTextRef.SetText(this.m_description, description);
// }


// @wrapMethod(QuickHackDescriptionGameController)
// protected cb func OnQuickHackDataChanged(value: Variant) -> Bool {
//   wrappedMethod(value);
//   this.m_selectedData = FromVariant<ref<QuickhackData>>(value);
//   if IsDefined(this.m_selectedData) {
//     let title: String = GetLocalizedText(this.m_selectedData.m_title);
//     if StrLen(title) == 0 {
//       title = ToString(this.m_selectedData.m_title);
//     }
//     inkTextRef.SetText(this.m_subHeader, title);

//     let description: String = GetLocalizedText(this.m_selectedData.m_description);
//     if StrLen(description) == 0 {
//       description = ToString(this.m_selectedData.m_description);
//     }
//     inkTextRef.SetText(this.m_description, description);
//   }
// }