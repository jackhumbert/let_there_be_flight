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
  public let title: String;
  public let description: String;
}

public class FlightMalfunction extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightMalfunction";
    this.title = "Orbitizer";
    this.description = "Rip a hole in the sky and throw this particular asshole in it";
    this.SetObjectActionID(t"DeviceAction.FlightMalfunction");
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[FlightMalfunction] CompleteAction");
    super.CompleteAction(gameInstance);
  }
}

public class DisableGravity extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"DisableGravity";
    this.title = "Disable Gravity";
    this.description = "Enables the antigrav plates on the vehicle";
    this.SetObjectActionID(t"DeviceAction.DisableGravity");
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[DisableGravity] CompleteAction");
    super.CompleteAction(gameInstance);
  }
}

public class Bouncy extends FlightAction {
  public final func SetProperties() -> Void {
    this.actionName = n"Bouncy";
    this.title = "Funhouse";
    this.description = "Make things extra bouncy";
    this.SetObjectActionID(t"DeviceAction.Bouncy");
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

public func ActionFlightMalfunction(ps: ref<PersistentState>) -> ref<FlightMalfunction> {
    let action: ref<FlightMalfunction> = new FlightMalfunction();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(ps);
    action.SetProperties();
    return action;
}

public func ActionDisableGravity(ps: ref<PersistentState>) -> ref<DisableGravity> {
    let action: ref<DisableGravity> = new DisableGravity();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(ps);
    action.SetProperties();
    return action;
}

public func ActionBouncy(ps: ref<PersistentState>) -> ref<Bouncy> {
    let action: ref<Bouncy> = new Bouncy();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(ps);
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
    ArrayPush(actions, ActionFlightMalfunction(this.GetPS()));
    ArrayPush(actions, ActionDisableGravity(this.GetPS()));
    ArrayPush(actions, ActionBouncy(this.GetPS()));

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
  private final func TranslateActionsIntoQuickSlotCommands(actions: array<ref<DeviceAction>>, out commands: array<ref<QuickhackData>>) -> Void {
    let actionCompletionEffects: array<wref<ObjectActionEffect_Record>>;
    let actionRecord: wref<ObjectAction_Record>;
    let actionStartEffects: array<wref<ObjectActionEffect_Record>>;
    let choice: InteractionChoice;
    let emptyChoice: InteractionChoice;
    let i: Int32;
    let i1: Int32;
    let newCommand: ref<QuickhackData>;
    let sAction: ref<FlightAction>;
    let statModifiers: array<wref<StatModifier_Record>>;
    let playerRef: ref<PlayerPuppet> = GetPlayer(this.GetGame());
    let iceLVL: Float = 1.0;
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
          newCommand.m_duration = 1.0;
          newCommand.m_title = sAction.title;
          newCommand.m_description = sAction.description;
          newCommand.m_actionOwnerName = actionOwnerName;
          // newCommand.m_title = LocKeyToString(actionRecord.ObjectActionUI().Caption());
          // newCommand.m_description = LocKeyToString(actionRecord.ObjectActionUI().Description());
          newCommand.m_icon = actionRecord.ObjectActionUI().CaptionIcon().TexturePartID().GetID();
          newCommand.m_iconCategory = actionRecord.GameplayCategory().IconName();
          newCommand.m_type = actionRecord.ObjectActionType().Type();
          newCommand.m_actionOwner = this.GetEntityID();
          newCommand.m_isInstant = false;
          newCommand.m_ICELevel = iceLVL;
          newCommand.m_ICELevelVisible = true;
          // newCommand.m_vulnerabilities = this.GetPS().GetActiveQuickHackVulnerabilities();
          newCommand.m_actionState = EActionInactivityReson.Locked;
          newCommand.m_quality = 1;
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
          newCommand.m_cost = sAction.GetCost();
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
  public let m_owner: wref<GameObject>;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOn");
    this.m_owner = owner;
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.m_vehicleComponent.CreateHitEventOnSelf(0.1);
      vehicle.m_flightComponent.Activate(true);
      vehicle.m_flightComponent.lift = 2.0;
    }
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOff");
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.m_flightComponent.Activate(false);
    }
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
  }
}

public class DisableGravityEffector extends Effector {
  public let m_owner: wref<GameObject>;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[DisableGravityEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[DisableGravityEffector] ActionOn");
    this.m_owner = owner;
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.EnableGravity(false);
      vehicle.m_inTrafficLane = false;
      vehicle.m_drivingTrafficPattern = n"stop";
      vehicle.m_crowdMemberComponent.ChangeMoveType(vehicle.m_drivingTrafficPattern);
      vehicle.UnsetPhysicsStates();
    }
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[DisableGravityEffector] ActionOff");
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.EnableGravity(true);
    }
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
  }
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
      vehicle.UnsetPhysicsStates();
      vehicle.m_inTrafficLane = false;
      vehicle.m_drivingTrafficPattern = n"stop";
      vehicle.m_crowdMemberComponent.ChangeMoveType(vehicle.m_drivingTrafficPattern);
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

@replaceMethod(QuickhacksListGameController)
private final func SelectData(data: ref<QuickhackData>) -> Void {
  this.m_selectedData = data;
  let description: String = GetLocalizedText(this.m_selectedData.m_description);
  if StrLen(description) == 0 {
      description = ToString(this.m_selectedData.m_description);
  };
  let title = GetLocalizedText(this.m_selectedData.m_title);
  if StrLen(title) == 0 {
      title = ToString(this.m_selectedData.m_title);
  };
  inkTextRef.SetText(this.m_subHeader, title);
  if this.m_selectedData.m_isLocked && this.m_selectedData.m_actionMatchesTarget && this.m_hasActiveUpload {
    inkWidgetRef.SetState(this.m_executeBtn, n"Disabled");
    inkWidgetRef.SetState(this.m_executeAndCloseBtn, n"Disabled");
    inkWidgetRef.SetState(this.m_description, n"Locked");
    inkWidgetRef.SetState(this.m_subHeader, n"Locked");
    if IsDefined(this.inkWarningAnimProxy) {
      this.inkWarningAnimProxy.Stop();
    };
    this.inkWarningAnimProxy = this.PlayLibraryAnimation(n"deviceOnly_hack", GetAnimOptionsInfiniteLoop(inkanimLoopType.Cycle));
    if NotEquals(this.m_lastMemoryWarningTransitionAnimName, n"memoryToWarning_transition") {
      if IsDefined(this.inkMemoryWarningTransitionAnimProxy) {
        this.inkMemoryWarningTransitionAnimProxy.Stop();
      };
      this.inkMemoryWarningTransitionAnimProxy = this.PlayLibraryAnimation(n"memoryToWarning_transition");
      this.m_lastMemoryWarningTransitionAnimName = n"memoryToWarning_transition";
    };
    this.ApplyQuickhackSelection();
    inkTextRef.SetText(this.m_warningText, GetLocalizedText(this.m_selectedData.m_inactiveReason));
  } else {
    if this.m_selectedData.m_isLocked {
      this.ResetQuickhackSelection();
      inkWidgetRef.SetState(this.m_executeBtn, n"Disabled");
      inkWidgetRef.SetState(this.m_executeAndCloseBtn, n"Disabled");
      inkWidgetRef.SetState(this.m_description, n"Locked");
      inkWidgetRef.SetState(this.m_subHeader, n"Locked");
      if IsDefined(this.inkWarningAnimProxy) {
        this.inkWarningAnimProxy.Stop();
      };
      this.inkWarningAnimProxy = this.PlayLibraryAnimation(n"deviceOnly_hack");
      if NotEquals(this.m_lastMemoryWarningTransitionAnimName, n"memoryToWarning_transition") {
        if IsDefined(this.inkMemoryWarningTransitionAnimProxy) {
          this.inkMemoryWarningTransitionAnimProxy.Stop();
        };
        this.inkMemoryWarningTransitionAnimProxy = this.PlayLibraryAnimation(n"memoryToWarning_transition");
        this.m_lastMemoryWarningTransitionAnimName = n"memoryToWarning_transition";
      };
      inkTextRef.SetText(this.m_warningText, GetLocalizedText(this.m_selectedData.m_inactiveReason));
    } else {
      inkWidgetRef.SetState(this.m_executeBtn, n"Default");
      inkWidgetRef.SetState(this.m_executeAndCloseBtn, n"Default");
      inkWidgetRef.SetState(this.m_description, n"Default");
      inkWidgetRef.SetState(this.m_subHeader, n"Default");
      this.ApplyQuickhackSelection();
      if IsDefined(this.inkWarningAnimProxy) {
        this.inkWarningAnimProxy.Stop();
      };
      this.inkWarningAnimProxy = this.PlayLibraryAnimation(n"warningOut");
      if NotEquals(this.m_lastMemoryWarningTransitionAnimName, n"warningToMemory_transition") {
        if IsDefined(this.inkMemoryWarningTransitionAnimProxy) {
          this.inkMemoryWarningTransitionAnimProxy.Stop();
        };
        this.inkMemoryWarningTransitionAnimProxy = this.PlayLibraryAnimation(n"warningToMemory_transition");
        this.m_lastMemoryWarningTransitionAnimName = n"warningToMemory_transition";
      };
    };
  };
  if !this.m_timeBetweenIntroAndDescritpionCheck {
  };
  inkTextRef.SetText(this.m_description, description);
  this.SetupTargetName();
  this.SetupTier();
  this.SetupVulnerabilities();
  this.SetupICE();
  this.SetupUploadTime();
  this.SetupDuration();
  this.SetupMaxCooldown();
  this.SetupMemoryCost();
  this.SetupMemoryCostDifferance();
  this.SetupNetworkBreach();
  if !this.IsCurrentSelectionOnStatPoolIndexes() {
    this.UpdateRecompileTime(false, 0.00);
  };
  GameInstance.GetBlackboardSystem(this.GetPlayerControlledObject().GetGame()).Get(GetAllBlackboardDefs().UI_QuickSlotsData).SetVariant(GetAllBlackboardDefs().UI_QuickSlotsData.quickHackDataSelected, ToVariant(this.m_selectedData), true);
}