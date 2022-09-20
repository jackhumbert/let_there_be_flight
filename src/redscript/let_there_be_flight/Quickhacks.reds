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

public class FlightMalfunction extends ActionBool {
  public final func SetProperties() -> Void {
    this.actionName = n"FlightMalfunction";
    this.prop = DeviceActionPropertyFunctions.SetUpProperty_Bool(this.actionName, true, this.actionName, this.actionName);
  }

  public func CompleteAction(gameInstance: GameInstance) -> Void {
    FlightLog.Info("[FlightMalfunction] CompleteAction");
    super.CompleteAction(gameInstance);
  }
}



@addMethod(VehicleComponentPS)
public func OnFlightMalfunction(evt: ref<FlightMalfunction>) -> EntityNotificationType {
  FlightLog.Info("[VehicleComponentPS] OnFlightMalfunction");
  return EntityNotificationType.SendThisEventToEntity;
}

@addMethod(VehicleObject)
protected cb func OnFlightMalfunction(evt: ref<FlightMalfunction>) -> Bool {
  FlightLog.Info("[VehicleObject] OnFlightMalfunction");
}

@addMethod(VehicleObject)
protected cb func OnPerformedAction(evt: ref<PerformedAction>) -> Bool {
    FlightLog.Info("[VehicleObject] OnPerformedAction");
  let action: ref<ScriptableDeviceAction>;
  // let sequenceQuickHacks: ref<ForwardAction>;
  this.SetScannerDirty(true);
  action = evt.m_action as ScriptableDeviceAction;
  // this.ExecuteBaseActionOperation(evt.m_action.GetClassName());
  if action.CanTriggerStim() {
    // this.TriggerAreaEffectDistractionByAction(action);
  };
  if IsDefined(action) && action.IsIllegal() && !action.IsQuickHack() {
    // this.ResolveIllegalAction(action.GetExecutor(), action.GetDurationValue());
  };
  // if this.IsConnectedToActionsSequencer() && !this.IsLockedViaSequencer() {
    // sequenceQuickHacks = new ForwardAction();
    // sequenceQuickHacks.requester = this.GetDevicePS().GetID();
    // sequenceQuickHacks.actionToForward = action;
    // GameInstance.GetPersistencySystem(this.GetGame()).QueuePSEvent(this.GetDevicePS().GetActionsSequencer().GetID(), this.GetDevicePS().GetActionsSequencer().GetClassName(), sequenceQuickHacks);
  // };
  // this.ResolveQuestImportanceOnPerformedAction(action);
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
    
    let action: ref<FlightMalfunction> = new FlightMalfunction();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(this.GetPS());
    action.SetProperties();
    action.SetObjectActionID(t"DeviceAction.FlightMalfunction");
    // action.SetInactiveWithReason(false, "LocKey#49279");
    ArrayPush(actions, action);
    // if this.m_isQhackUploadInProgerss {
      // ScriptableDeviceComponentPS.SetActionsInactiveAll(actions, "LocKey#7020");
    // };
    this.TranslateActionsIntoQuickSlotCommands(actions, commands);
    quickSlotsManagerNotification.commands = commands;
  };
  HUDManager.SetQHDescriptionVisibility(this.GetGame(), shouldOpen);
  GameInstance.GetUISystem(this.GetGame()).QueueEvent(quickSlotsManagerNotification);
}

@addMethod(VehicleObject)
  private final func TranslateActionsIntoQuickSlotCommands(actions: array<ref<DeviceAction>>, out commands: array<ref<QuickhackData>>) -> Void {
    let actionCompletionEffects: array<wref<ObjectActionEffect_Record>>;
    let actionMatchDeck: Bool;
    let actionRecord: wref<ObjectAction_Record>;
    let actionStartEffects: array<wref<ObjectActionEffect_Record>>;
    let choice: InteractionChoice;
    let emptyChoice: InteractionChoice;
    let i: Int32;
    let i1: Int32;
    let newCommand: ref<QuickhackData>;
    let sAction: ref<ScriptableDeviceAction>;
    let statModifiers: array<wref<StatModifier_Record>>;
    let playerRef: ref<PlayerPuppet> = GetPlayer(this.GetGame());
    let iceLVL: Float = 1.0;
    let actionOwnerName: CName = StringToName(this.GetDisplayName());
    let playerQHacksList: array<PlayerQuickhackData> = RPGManager.GetPlayerQuickHackListWithQuality(playerRef);
    if ArraySize(playerQHacksList) == 0 {
      newCommand = new QuickhackData();
      newCommand.m_title = "LocKey#42171";
      newCommand.m_isLocked = true;
      newCommand.m_actionState = EActionInactivityReson.Invalid;
      newCommand.m_actionOwnerName = StringToName(this.GetDisplayName());
      newCommand.m_description = "LocKey#42172";
      ArrayPush(commands, newCommand);
    } else {
      i = 0;
      while i < ArraySize(playerQHacksList) {
        newCommand = new QuickhackData();
        sAction = null;
        ArrayClear(actionStartEffects);
        actionRecord = playerQHacksList[i].actionRecord;
        if NotEquals(actionRecord.ObjectActionType().Type(), gamedataObjectActionType.DeviceQuickHack) {
        } else {
          actionMatchDeck = false;
          i1 = 0;
          while i1 < ArraySize(actions) {
            sAction = actions[i1] as ScriptableDeviceAction;
            if Equals(actionRecord.ActionName(), sAction.GetObjectActionRecord().ActionName()) ||
            Equals(actionRecord.ActionName(), n"Ping") && Equals(sAction.GetObjectActionRecord().ActionName(), n"FlightMalfunction") {
              actionMatchDeck = true;
              // if actionRecord.Priority() >= sAction.GetObjectActionRecord().Priority() {
                // sAction.SetObjectActionID(playerQHacksList[i].actionRecord.GetID());
              // } else {
                actionRecord = sAction.GetObjectActionRecord();
              // };
              newCommand.m_uploadTime = sAction.GetActivationTime();
              newCommand.m_duration = 1.0;
            }
            i1 += 1;
          };
          newCommand.m_actionOwnerName = actionOwnerName;
          // newCommand.m_title = LocKeyToString(actionRecord.ObjectActionUI().Caption());
          newCommand.m_title = "Launch The Motherfucker";
          // newCommand.m_description = LocKeyToString(actionRecord.ObjectActionUI().Description());
          newCommand.m_description = "Rip a hole in the sky and throw this particular asshole in it";
          newCommand.m_icon = actionRecord.ObjectActionUI().CaptionIcon().TexturePartID().GetID();
          newCommand.m_iconCategory = actionRecord.GameplayCategory().IconName();
          newCommand.m_type = actionRecord.ObjectActionType().Type();
          newCommand.m_actionOwner = this.GetEntityID();
          newCommand.m_isInstant = false;
          newCommand.m_ICELevel = iceLVL;
          newCommand.m_ICELevelVisible = false;
          // newCommand.m_vulnerabilities = this.GetPS().GetActiveQuickHackVulnerabilities();
          newCommand.m_actionState = EActionInactivityReson.Locked;
          newCommand.m_quality = playerQHacksList[i].quality;
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
          if actionMatchDeck {
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
          } else {
            newCommand.m_isLocked = true;
            newCommand.m_inactiveReason = "LocKey#10943";
          };
          newCommand.m_actionMatchesTarget = actionMatchDeck;
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

  public let m_squadMembers: array<EntityID>;
  public let m_owner: wref<GameObject>;
  public let m_oldSquadAttitude: ref<AttitudeAgent>;
  public let m_quickhackLevel: Float;
  public let m_data: ref<FocusForcedHighlightData>;
  public let m_squadName: CName;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOn");
    this.m_owner = owner;
    if !IsDefined(owner) {
      return;
    };
    let vehicle = owner as VehicleObject;
    vehicle.m_flightComponent.Toggle(true);
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOff");
    let vehicle = owner as VehicleObject;
    vehicle.m_flightComponent.Toggle(false);
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
    // this.MarkSquad(false, this.m_owner);
  }

  public final func GetPingLevel(level: Float) -> TweakDBID {
    switch level {
      case 1.00:
        return t"BaseStatusEffect.Ping";
      case 2.00:
        return t"BaseStatusEffect.PingLevel2";
      case 3.00:
        return t"BaseStatusEffect.PingLevel3";
      case 4.00:
        return t"BaseStatusEffect.PingLevel4";
      default:
        return t"BaseStatusEffect.Ping";
    };
    return t"BaseStatusEffect.Ping";
  }
}