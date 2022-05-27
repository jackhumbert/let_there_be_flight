// VehicleObject

@addMethod(VehicleObject)
public const func IsQuickHackAble() -> Bool {
    // FlightLog.Info("[VehicleObject] IsQuickHackAble");
    // working
  return true;
}

@addMethod(VehicleObject)
public const func IsPotentiallyQuickHackable() -> Bool {
    FlightLog.Info("[VehicleObject] IsPotentiallyQuickHackable");
  return true;
}

@addMethod(VehicleObject)
public const func IsQuickHacksExposed() -> Bool {
    FlightLog.Info("[VehicleObject] IsQuickHacksExposed");
  return true;
}

@addMethod(VehicleObject)
protected cb func OnHUDInstruction(evt: ref<HUDInstruction>) -> Bool {
  // working
  // FlightLog.Info("[VehicleObject] OnHUDInstruction");
  super.OnHUDInstruction(evt);
  if evt.quickhackInstruction.ShouldProcess() {
    FlightLog.Info("[VehicleObject] OnHUDInstruction");
    this.TryOpenQuickhackMenu(evt.quickhackInstruction.ShouldOpen());
  };
}

@addField(VehicleObject)
public let m_isQhackUploadInProgerss: Bool;

@addMethod(VehicleObject)
protected func SendQuickhackCommands(shouldOpen: Bool) -> Void {
    FlightLog.Info("[VehicleObject] SendQuickhackCommands");
  let actions: array<ref<DeviceAction>>;
  let commands: array<ref<QuickhackData>>;
  let context: GetActionsContext;
  let quickSlotsManagerNotification: ref<RevealInteractionWheel> = new RevealInteractionWheel();
  quickSlotsManagerNotification.lookAtObject = this;
  quickSlotsManagerNotification.shouldReveal = shouldOpen;
  if shouldOpen {
    context = this.GetVehiclePS().GenerateContext(gamedeviceRequestType.Remote, Device.GetInteractionClearance(), GameInstance.GetPlayerSystem(this.GetGame()).GetLocalPlayerMainGameObject(), this.GetEntityID());
    this.GetVehiclePS().GetRemoteActions(actions, context);
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
private final const func GetICELevel() -> Float {
  let statsSystem: ref<StatsSystem> = GameInstance.GetStatsSystem(this.GetGame());
  let playerLevel: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(GetPlayer(this.GetGame()).GetEntityID()), gamedataStatType.Level);
  let targetLevel: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatType.PowerLevel);
  let resistance: Float = statsSystem.GetStatValue(Cast<StatsObjectID>(this.GetEntityID()), gamedataStatType.HackingResistance);
  return resistance + 0.50 * (targetLevel - playerLevel);
}

@addMethod(VehicleObject)
private final func TranslateActionsIntoQuickSlotCommands(actions: array<ref<DeviceAction>>, out commands: array<ref<QuickhackData>>) -> Void {
    FlightLog.Info("[VehicleObject] TranslateActionsIntoQuickSlotCommands");
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
  let iceLVL: Float = this.GetICELevel();
  // let iceLVL = 4.0;
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
          if Equals(actionRecord.ActionName(), sAction.GetObjectActionRecord().ActionName()) {
            actionMatchDeck = true;
            if actionRecord.Priority() >= sAction.GetObjectActionRecord().Priority() {
              sAction.SetObjectActionID(playerQHacksList[i].actionRecord.GetID());
            } else {
              actionRecord = sAction.GetObjectActionRecord();
            };
            newCommand.m_uploadTime = sAction.GetActivationTime();
            newCommand.m_duration = this.GetVehiclePS().GetDistractionDuration(sAction);
          } else {
            i1 += 1;
          };
        };
        newCommand.m_actionOwnerName = actionOwnerName;
        newCommand.m_title = LocKeyToString(actionRecord.ObjectActionUI().Caption());
        newCommand.m_description = LocKeyToString(actionRecord.ObjectActionUI().Description());
        newCommand.m_icon = actionRecord.ObjectActionUI().CaptionIcon().TexturePartID().GetID();
        newCommand.m_iconCategory = actionRecord.GameplayCategory().IconName();
        newCommand.m_type = actionRecord.ObjectActionType().Type();
        newCommand.m_actionOwner = this.GetEntityID();
        newCommand.m_isInstant = false;
        newCommand.m_ICELevel = iceLVL;
        newCommand.m_ICELevelVisible = false;
        newCommand.m_vulnerabilities = this.GetVehiclePS().GetActiveQuickHackVulnerabilities();
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
          } else {
            i1 += 1;
          };
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


@addMethod(VehicleObject)
public const func CanRevealRemoteActionsWheel() -> Bool {
  FlightLog.Info("[VehicleObject] CanRevealRemoteActionsWheel");
  return true;
}

@addMethod(VehicleObject)
public const func ShouldEnableRemoteLayer() -> Bool {
  FlightLog.Info("[VehicleObject] ShouldEnableRemoteLayer");
  return true;
}

@wrapMethod(VehicleObject)
public const func CompileScannerChunks() -> Bool {
  let r = wrappedMethod();
  let vulnerabilities = this.GetVehiclePS().GetAllQuickHackVulnerabilities();
  if ArraySize(vulnerabilities) > 0 {
    let vulnerabilitiesChunk = new ScannerVulnerabilities();
    let i = 0;
    let vulnerability: Vulnerability;
    while i < ArraySize(vulnerabilities) {
      let record = TweakDBInterface.GetScannableDataRecord(vulnerabilities[i]);
      if IsDefined(record) {
        vulnerability.vulnerabilityName = record.LocalizedDescription();
        vulnerability.icon = record.IconRecord().GetID();
        vulnerability.isActive = this.CanPlayerUseQuickHackVulnerability(vulnerabilities[i]);
        vulnerabilitiesChunk.PushBack(vulnerability);
      };
      i += 1;
    };
    if vulnerabilitiesChunk.IsValid() {
      let scannerBlackboard: wref<IBlackboard> = GameInstance.GetBlackboardSystem(this.GetGame()).Get(GetAllBlackboardDefs().UI_ScannerModules);
      scannerBlackboard.SetVariant(GetAllBlackboardDefs().UI_ScannerModules.ScannerVulnerabilities, ToVariant(vulnerabilitiesChunk));
    };
  }
  return r;
}

@addMethod(VehicleObject)
public const func CanPlayerUseQuickHackVulnerability(data: TweakDBID) -> Bool {
  return true;
}

@replaceMethod(VehicleObject)
public const func IsNetrunner() -> Bool {
  return true;
}

@addMethod(VehicleObject)
protected final const func HasAnyQuickHackActive() -> Bool {
  return true;
}

@addMethod(VehicleObject)
protected final const func HasAnyActiveQuickHackVulnerabilities() -> Bool {
  return true;
}

@addMethod(VehicleObject)
private final func ResolveRemoteActions(state: Bool) -> Void {
  let context: GetActionsContext = this.GetVehiclePS().GenerateContext(gamedeviceRequestType.Remote, Device.GetInteractionClearance(), GameInstance.GetPlayerSystem(this.GetGame()).GetLocalPlayerMainGameObject(), this.GetEntityID());
  if state {
    this.GetVehiclePS().AddActiveContext(gamedeviceRequestType.Remote);
    // this.NotifyConnectionHighlightSystem(true, false);
  } else {
    if !this.IsCurrentTarget() && !this.IsCurrentlyScanned() {
      this.GetVehiclePS().RemoveActiveContext(gamedeviceRequestType.Remote);
    } else {
      return;
    };
  };
  this.DetermineInteractionStateByTask(context);
}

@addMethod(VehicleObject)
protected final func RefreshInteraction() -> Void {
  let context: GetActionsContext = this.GetVehiclePS().GenerateContext(gamedeviceRequestType.Remote, Device.GetInteractionClearance(), GameInstance.GetPlayerSystem(this.GetGame()).GetLocalPlayerMainGameObject(), this.GetEntityID());
  if this.IsCurrentTarget() || this.IsCurrentlyScanned() {
    this.DetermineInteractionStateByTask(context);
  };
}


@addMethod(VehicleObject)
protected final func DetermineInteractionStateByTask(opt context: GetActionsContext) -> Void {
  let taskData: ref<DetermineInteractionStateTaskData>;
  if NotEquals(context.requestType, IntEnum<gamedeviceRequestType>(0l)) {
    taskData = new DetermineInteractionStateTaskData();
    taskData.context = context;
  };
  GameInstance.GetDelaySystem(this.GetGame()).QueueTask(this, taskData, n"DetermineInteractionStateTask", gameScriptTaskExecutionStage.Any);
}

@addMethod(VehicleObject)
protected cb func OnQuickHackPanelStateChanged(evt: ref<QuickHackPanelStateEvent>) -> Bool {
  this.DetermineInteractionStateByTask();
}

// VehicleComponentPS

@wrapMethod(VehicleComponentPS)
public final func GetTrunkActions(out actions: array<ref<DeviceAction>>, context: VehicleActionsContext) -> Void {
  wrappedMethod(actions, context);
  ArrayPush(actions, this.ActionFlightMalfunction());
}

@wrapMethod(VehicleComponentPS)
public final func GetPlayerTrunkActions(out actions: array<ref<DeviceAction>>, context: VehicleActionsContext) -> Void {
  wrappedMethod(actions, context);
  ArrayPush(actions, this.ActionFlightMalfunction());
  return;
}

@addMethod(VehicleComponentPS)
private final func ActionFlightMalfunction() -> ref<VehicleFlightMalfunction> {
  let action: ref<VehicleFlightMalfunction> = new VehicleFlightMalfunction();
  action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
  action.SetUp(this);
  action.SetProperties();
  action.AddDeviceName(this.GetDeviceName());
  action.CreateInteraction();
  return action;
}

@addMethod(VehicleComponentPS)
public final func OnVehicleFlightMalfunction(evt: ref<VehicleFlightMalfunction>) -> EntityNotificationType {
  let fc = this.GetOwnerEntity().GetFlightComponent();
  if fc.active {
    fc.Deactivate(false);
  } else {
    fc.Activate();
  }
  return EntityNotificationType.SendThisEventToEntity;
}

@replaceMethod(VehicleComponentPS)
protected const func CanCreateAnyQuickHackActions() -> Bool {
  // FlightLog.Info("[VehicleComponentPS] CanCreateAnyQuickHackActions");
  // working
  // return wrappedMethod() || true;
  return true;
}

@addMethod(VehicleComponentPS)
public func IsQuickHacksExposed() -> Bool {
  FlightLog.Info("[VehicleComponentPS] IsQuickHacksExposed");
  return true;
}

@addMethod(VehicleComponentPS)
public final const func IsPotentiallyQuickHackable() -> Bool {
  FlightLog.Info("[VehicleComponentPS] IsPotentiallyQuickHackable");
  //working
  return true;
}

// lots of trues
// @wrapMethod(ScriptableDeviceComponentPS)
// private final func UpdateQuickHackableState(isQuickHackable: Bool) -> Void {
//   wrappedMethod(isQuickHackable);
//   FlightLog.Info("[ScriptableDeviceComponentPS] UpdateQuickHackableState " + ToString(isQuickHackable));
// }

// @wrapMethod(ScriptableDeviceComponentPS)
// protected final func FinalizeGetQuickHackActions(out outActions: array<ref<DeviceAction>>, context: GetActionsContext) -> Void {
//   wrappedMethod(outActions, context);  
//   let fcAction: ref<QuickHackFlightMalfunction> = new QuickHackFlightMalfunction();
//   fcAction.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
//   fcAction.SetUp(this);
//   fcAction.SetProperties(n"hood");
//   fcAction.AddDeviceName(this.GetDeviceName());
//   fcAction.CreateInteraction();
//   fcAction.SetObjectActionID(t"DeviceAction.QuickHackFlightMalfunction");
//   ArrayPush(outActions, fcAction);
// }

@replaceMethod(VehicleComponentPS)
protected func GetQuickHackActions(out actions: array<ref<DeviceAction>>, context: GetActionsContext) -> Void {
  // FlightLog.Info("[VehicleComponentPS] GetQuickHackActions");
  // working
  let currentAction: ref<ScriptableDeviceAction>;
  let controllerPS: ref<vehicleControllerPS> = this.GetVehicleControllerPS();
  // let vehicleState: vehicleEState = controllerPS.GetState();
  let fcAction: ref<QuickHackFlightMalfunction> = new QuickHackFlightMalfunction();
  fcAction.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
  fcAction.SetUp(this);
  // fcAction.SetProperties(n"trunk");
  fcAction.AddDeviceName(this.GetDeviceName());
  fcAction.CreateInteraction();
  fcAction.SetObjectActionID(t"DeviceAction.QuickHackFlightMalfunction");
  ArrayPush(actions, fcAction);
  // if Equals(vehicleState, vehicleEState.Default) {
    if !controllerPS.IsAlarmOn() {
      currentAction = this.ActionForceCarAlarm();
      currentAction.SetObjectActionID(t"DeviceAction.MalfunctionClassHack");
      ArrayPush(actions, currentAction);
    };
  // };
  // this.MarkActionsAsQuickHacks(actions);
  this.FinalizeGetQuickHackActions(actions, context);
  // currentAction = this.ActionForceCarAlarm();
  // currentAction.SetObjectActionID(t"DeviceAction.MalfunctionClassHack");
  // currentAction.SetAsQuickHack(this.WasActionPerformed(currentAction.GetActionID(), EActionContext.QHack));
  // ArrayPush(actions, currentAction);
}

@replaceMethod(VehicleComponentPS)
public func GetActions(out outActions: array<ref<DeviceAction>>, context: GetActionsContext) -> Bool {
  FlightLog.Info("[VehicleComponentPS] GetActions");
  this.SetActionIllegality(outActions, this.m_illegalActions.regularActions);
  let fcAction: ref<QuickHackFlightMalfunction> = new QuickHackFlightMalfunction();
  fcAction.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
  fcAction.SetUp(this);
  // fcAction.SetProperties(n"trunk");
  fcAction.AddDeviceName(this.GetDeviceName());
  fcAction.CreateInteraction();
  fcAction.SetObjectActionID(t"DeviceAction.QuickHackFlightMalfunction");
  ArrayPush(outActions, fcAction);
  return true;
}

@wrapMethod(VehicleComponentPS)
public final func GetValidChoices(objectActionRecords: array<wref<ObjectAction_Record>>, context: GetActionsContext, objectActionsCallbackController: wref<gameObjectActionsCallbackController>, out choices: array<InteractionChoice>, isAutoRefresh: Bool) -> Void {
  ArrayPush(objectActionRecords, TweakDBInterface.GetObjectActionRecord(t"DeviceAction.QuickHackFlightMalfunction"));
  ArrayPush(objectActionRecords, TweakDBInterface.GetObjectActionRecord(t"DeviceAction.MalfunctionClassHack"));
  wrappedMethod(objectActionRecords, context, objectActionsCallbackController, choices, isAutoRefresh);
}

@wrapMethod(VehicleComponentPS)
protected func Initialize() -> Void {
  wrappedMethod();
  this.m_disableQuickHacks = false;
  this.m_isLockedViaSequencer = false;
  // this.m_debugExposeQuickHacks = true;
  this.UpdateQuickHackableState(true);
  this.EnableDevice();
  this.InitializeQuickHackVulnerabilities();
  this.AddQuickHackVulnerability(t"DeviceAction.QuickHackFlightMalfunction");
}

@wrapMethod(VehicleComponentPS)
public final func DetermineActionsToPush(interaction: ref<InteractionComponent>, context: VehicleActionsContext, objectActionsCallbackController: wref<gameObjectActionsCallbackController>, isAutoRefresh: Bool) -> Void {
  FlightLog.Info("[VehicleComponentPS] DetermineActionsToPush");
  wrappedMethod(interaction, context, objectActionsCallbackController, isAutoRefresh);
  
  // let choices: array<InteractionChoice>;
  // let caption: InteractionChoiceCaption;
  // InteractionChoiceCaption.AddTextPart(caption, "Fly yo!");
  // let data: array<Variant>;
  // let metadata: InteractionChoiceMetaData;
  // metadata.tweakDBID = t"DeviceAction.MalfunctionClassHack";
  // let choice = new InteractionChoice("Fly", caption, data, metadata);
  // choice.caption = "Fly";
  // ArrayPush(choices, choice);
  // this.PushActionsToInteractionComponent(interaction, choices, context);
}

@wrapMethod(RPGManager)
public final static func GetPlayerQuickHackList(player: wref<PlayerPuppet>) -> array<TweakDBID> {
  let data = wrappedMethod(player);
  ArrayPush(data, t"DeviceAction.MalfunctionClassHack");
  ArrayPush(data, t"DeviceAction.QuickHackFlightMalfunction");
  return data;
}

@wrapMethod(RPGManager)
public final static func GetPlayerQuickHackListWithQuality(player: wref<PlayerPuppet>) -> array<PlayerQuickhackData> {
  let data = wrappedMethod(player);
  let qh: PlayerQuickhackData;
  qh.actionRecord = TweakDBInterface.GetObjectActionRecord(t"DeviceAction.MalfunctionClassHack");
  qh.quality = 1;
  ArrayPush(data, qh);
  qh.actionRecord = TweakDBInterface.GetObjectActionRecord(t"DeviceAction.QuickHackFlightMalfunction");
  qh.quality = 1;
  ArrayPush(data, qh);
  return data;
}

@addMethod(VehicleComponentPS)
public func OnQuickHackFlightMalfunction(evt: ref<QuickHackFlightMalfunction>) -> EntityNotificationType {
  // let type: EntityNotificationType = this.OnQuickHackFlightMalfunction(evt);
  // if Equals(type, EntityNotificationType.DoNotNotifyEntity) {
  //   return type;
  // };
  if evt.IsStarted() {
    FlightLog.Info("[VehicleComponentPS] OnQuickHackFlightMalfunction");
    // this.ExecutePSAction(this.FireVerticalImpulse());
    let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
    impulseEvent.radius = 1.0;
    impulseEvent.worldPosition = Vector4.Vector4To3(this.GetOwnerEntity().GetWorldPosition());
    impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 10000.0);
    this.GetOwnerEntity().QueueEvent(impulseEvent);
  };
  this.UseNotifier(evt);
  return EntityNotificationType.DoNotNotifyEntity;
}

@replaceMethod(ScriptableDeviceComponentPS)
protected final func FinalizeGetQuickHackActions(out outActions: array<ref<DeviceAction>>, context: GetActionsContext) -> Void {
  let currentAction: ref<ScriptableDeviceAction>;
  if this.IsConnectedToBackdoorDevice() {
    currentAction = this.ActionRemoteBreach();
    currentAction.SetInactiveWithReason(!this.IsBreached(), "LocKey#27728");
    ArrayPush(outActions, currentAction);
    currentAction = this.ActionPing();
    currentAction.SetInactiveWithReason(!this.GetNetworkSystem().HasActivePing(this.GetMyEntityID()), "LocKey#49279");
    ArrayPush(outActions, currentAction);
  } else {
    if this.HasNetworkBackdoor() {
      currentAction = this.ActionPing();
      currentAction.SetInactiveWithReason(!this.GetNetworkSystem().HasActivePing(this.GetMyEntityID()), "LocKey#49279");
      ArrayPush(outActions, currentAction);
    };
  };
  let fcAction: ref<QuickHackFlightMalfunction> = new QuickHackFlightMalfunction();
  fcAction.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
  fcAction.SetUp(this);
  // fcAction.SetProperties(n"trunk");
  fcAction.AddDeviceName(this.GetDeviceName());
  fcAction.CreateInteraction();
  fcAction.SetObjectActionID(t"DeviceAction.QuickHackFlightMalfunction");
  ArrayPush(outActions, fcAction);
  
    let action: ref<ForceCarAlarm> = new ForceCarAlarm();
    action.clearanceLevel = DefaultActionsParametersHolder.GetInteractiveClearance();
    action.SetUp(this);
    action.SetProperties(true);
    action.AddDeviceName(this.GetDeviceName());
    action.CreateInteraction();
    action.SetObjectActionID(t"DeviceAction.MalfunctionClassHack");
    ArrayPush(outActions, action);
  // if this.IsUnpowered() {
  //   ScriptableDeviceComponentPS.SetActionsInactiveAll(outActions, "LocKey#7013");
  // };
  if !context.ignoresRPG {
    // this.EvaluateActionsRPGAvailabilty(outActions, context);
    this.SetActionIllegality(outActions, this.m_illegalActions.quickHacks);
    this.MarkActionsAsQuickHacks(outActions);
    this.SetActionsQuickHacksExecutioner(outActions);
  };
}

// custom class

// public class FlightComponentPS extends ScriptableDeviceComponentPS {
//   protected func Initialize() -> Void {
//     super.Initialize();
//     this.m_disableQuickHacks = false;
//     this.m_debugExposeQuickHacks = true;
//     this.UpdateQuickHackableState(true);
//   }

//   public const func IsQuickHackAble() -> Bool {
//     FlightLog.Info("[FlightComponentPS] IsQuickHackAble");
//     return true;
//   }

//   public func IsQuickHacksExposed() -> Bool {
//     FlightLog.Info("[FlightComponentPS] IsQuickHacksExposed");
//     return true;
//   }

//   public func OnSetExposeQuickHacks(evt: ref<SetExposeQuickHacks>) -> EntityNotificationType {
//     super.OnSetExposeQuickHacks(evt);
//     return EntityNotificationType.SendThisEventToEntity;
//   }

//   protected const func CanCreateAnyQuickHackActions() -> Bool {
//     return true;
//   }

//   protected func GetQuickHackActions(out actions: array<ref<DeviceAction>>, context: GetActionsContext) -> Void {
//     FlightLog.Info("[FlightComponentPS] GetQuickHackActions");
//     let action: ref<ScriptableDeviceAction> = this.ActionQuickHackFlightMalfunction();
//     action.SetObjectActionID(t"DeviceAction.MalfunctionClassHack");
//     action.SetDurationValue(0.1);
//     // action.SetInactiveWithReason(!this.IsDistracting(), "LocKey#7004");
//     ArrayPush(actions, action);
//     this.FinalizeGetQuickHackActions(actions, context);
//   }

//   protected func ActionQuickHackFlightMalfunction() -> ref<QuickHackFlightMalfunction> {
//     let action: ref<QuickHackFlightMalfunction> = new QuickHackFlightMalfunction();
//     action.SetUp(this);
//     action.SetProperties();
//     action.AddDeviceName(this.GetDeviceName());
//     action.CreateInteraction();
//     action.SetDurationValue(0.1);
//     return action;
//   }

//   public func OnQuickHackFlightMalfunction(evt: ref<QuickHackFlightMalfunction>) -> EntityNotificationType {
//     FlightLog.Info("[FlightComponentPS] OnQuickHackFlightMalfunction");
//     // let type: EntityNotificationType = this.OnQuickHackFlightMalfunction(evt);
//     // if Equals(type, EntityNotificationType.DoNotNotifyEntity) {
//     //   return type;
//     // };
//     if evt.IsStarted() {
//       // this.ExecutePSAction(this.FireVerticalImpulse());
//       this.FireVerticalImpulse();
//     };
//     return EntityNotificationType.SendThisEventToEntity;
//   }

//   public func GetVehicle() -> wref<VehicleObject> {
//     return GameInstance.FindEntityByID(this.GetGameInstance(), PersistentID.ExtractEntityID(this.GetID())) as VehicleObject;
//   }

//   public func FireVerticalImpulse() {
//     let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
//     impulseEvent.radius = 1.0;
//     impulseEvent.worldPosition = Vector4.Vector4To3(this.GetVehicle().GetWorldPosition());
//     impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 10000.0);
//     this.GetVehicle().QueueEvent(impulseEvent);
//   }
// }
