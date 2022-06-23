// public class FlightSettingsListener extends ConfigVarListener {

//   private let m_ctrl: wref<FlightController>;

//   public final func RegisterController(ctrl: ref<FlightController>) -> Void {
//     this.m_ctrl = ctrl;
//   }

//   public func OnVarModified(groupPath: CName, varName: CName, varType: ConfigVarType, reason: ConfigChangeReason) -> Void {
//     this.m_ctrl.OnVarModified(groupPath, varName, varType, reason);
//   }
// }

// could watch this
// this.m_callbackID = scriptInterface.localBlackboard.RegisterListenerInt(allBlackboardDef.PlayerStateMachine.Vehicle, this, n"OnVehicleStateChanged");

public static func fc() -> wref<FlightController> {
  return FlightController.GetInstance();
}

// public static func fcv() -> wref<VehicleObject> {
//   return FlightController.GetInstance().GetVehicle();
// }

public native class FlightController extends IScriptable {
  // defined on the RED4ext side
  private native let enabled: Bool;
  private native let active: Bool;
  public native let mode: Int32;
  public static native func GetInstance() -> ref<FlightController>;
  
  // redscript-only
  private let sys: ref<FlightSystem>;
  private let gameInstance: GameInstance;
  private let player: ref<PlayerPuppet>;
  private let m_callbackID: ref<CallbackHandle>;
  private let m_vehicleCollisionBBStateID: ref<CallbackHandle>;
  private let ui: ref<FlightControllerUI>;
  public final func SetUI(ui: ref<FlightControllerUI>) {
    this.ui = ui;
  }
  public final const func IsEnabled() -> Bool {
    return this.enabled;
  }
  public final const func IsActive() -> Bool {
    return this.active;
  }

  public let showOptions: Bool;
  public let showUI: Bool;
  public let linearBrake: ref<InputPID>;
  public let angularBrake: ref<InputPID>;
  public let lift: ref<InputPID>;
  public let surge: ref<InputPID>;
  public let roll: ref<InputPID>;
  public let pitch: ref<InputPID>;
  public let yaw: ref<InputPID>;
  public let sway: ref<InputPID>;

  private let secondCounter: Float;

  public let isInAnyMenu: Bool;
  public let audioEnabled: Bool;

  public let isTPP: Bool;  

  private let uiBlackboard: ref<IBlackboard>;
  private let uiSystemBB: ref<UI_SystemDef>;

  public let initialized: Bool;
  public let usingKB: Bool;

  // public let effectInstance: ref<EffectInstance>;

  // protected let m_settingsListener: ref<FlightSettingsListener>;
  // protected let m_groupPath: CName;

  private func Initialize() {
    FlightLog.Info("[FlightController] Initialize");
    this.sys = FlightSystem.GetInstance();
    // this.sys = GameInstance.GetScriptableSystemsContainer(player.GetGame()).Get(n"FlightSystem") as FlightSystem;
    this.enabled = false;
    this.active = false;
    this.showOptions = false;
    this.showUI = true;

    this.linearBrake = InputPID.Create(0.5, 0.5);
    this.angularBrake = InputPID.Create(0.5, 0.5);
    this.lift = InputPID.Create(0.05, 0.2);
    this.surge = InputPID.Create(0.2, 0.2);
    this.roll = InputPID.Create(0.25, 1.0);
    this.pitch = InputPID.Create(0.25, 1.0);
    this.yaw = InputPID.Create(0.1, 0.2);
    this.sway = InputPID.Create(0.1, 0.2);
    
    this.secondCounter = 0.0;

    this.audioEnabled = true;

    this.uiBlackboard = GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_System);
    this.uiSystemBB = GetAllBlackboardDefs().UI_System;

    this.initialized = true;

    // this.trackedMappinId = this.uiBlackboard.RegisterListenerVariant(this.uiSystemBB.TrackedMappin, this, n"OnTrackedMappinUpdated");
    // this.uiBlackboard.SignalVariant(this.uiSystemBB.TrackedMappin);

    // this.waypoint = new Vector4(313.6, 208.2, 62.3, 0.0);


    // this.m_groupPath = n"/controls/flight";
    // this.m_settingsListener = new FlightSettingsListener();
    // this.m_settingsListener.RegisterController(this);
    // this.m_settingsListener.Register(this.m_groupPath);
  }

  private func SetPlayer(player: ref<PlayerPuppet>) {
    this.sys.Setup(player);
    this.gameInstance = player.GetGame();
    this.player = player;
  }

  public const func GetBlackboard() -> ref<IBlackboard> {
    return GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().VehicleFlight);
  }
  
  public static func CreateInstance(player: ref<PlayerPuppet>) {
    // FlightLog.Info("[FlightController] CreateInstance Started");
    // let self: ref<FlightController> = new FlightController();
    let self = FlightController.GetInstance();
    if !self.initialized {
      self.Initialize();  
    }
    self.SetPlayer(player);

    // This strong reference will tie the lifetime of the singleton 
    // to the lifetime of the player entity
    // player.flightController = self;

    // This weak reference is used as a global variable 
    // to access the mod instance anywhere
    // GetAllBlackboardDefs().flightController = self;
    // FlightLog.Info("[FlightController] CreateInstance Finished");
  }
  
  // public static func GetInstance() -> wref<FlightController> {
  //   return GetAllBlackboardDefs().flightController;
  // }

  // public final func OnVarModified(groupPath: CName, varName: CName, varType: ConfigVarType, reason: ConfigChangeReason) -> Void {
  //   switch varName {
  //     case n"HoverHeight":
  //       let configVar: ref<ConfigVarFloat> = GameInstance.GetSettingsSystem(this.gameInstance).GetVar(this.m_groupPath, n"HoverHeight") as ConfigVarFloat;
  //       this.hoverHeight = configVar.GetValue();
  //       break;
  //     default:
  //   };
  // }

  public func SetupMountedToCallback(psmBB: ref<IBlackboard>) -> Void {
    this.m_callbackID = psmBB.RegisterListenerBool(GetAllBlackboardDefs().PlayerStateMachine.MountedToVehicle, this, n"OnMountedToVehicleChange");
    if psmBB.GetBool(GetAllBlackboardDefs().PlayerStateMachine.MountedToVehicle) {
      this.Enable();
    } 
  }
  
  public cb func OnMountedToVehicleChange(mounted: Bool) -> Bool {
    // FlightLog.Info("[FlightController] OnMountedToVehicleChange");
    // if (mounted) {
    //   this.Enable();
    // } else {
    //   this.Disable();
    // }
  }
  
  public func Enable() -> Void {
    this.enabled = true;
    this.SetupActions();
    FlightLog.Info("[FlightController] Enable");
  }

  public func Disable() -> Void {
    this.enabled = false;
    this.SetupActions();   
    FlightLog.Info("[FlightController] Disable");
  }
  
  private func Activate(silent: Bool) -> Void {
    this.enabled = true;
    this.active = true;
    this.SetupActions();

    this.pitch.Reset();
    this.roll.Reset();
    this.yaw.Reset();
    this.sway.Reset();
    this.surge.Reset();
    this.lift.Reset();
    this.linearBrake.Reset();
    this.angularBrake.Reset();

    // let data: InputHintGroupData;
    // data.localizedTitle = "Flight Control";
    // data.localizedDescription = "The controls used in Let There Be Flight";
    // data.sortingPriority = 0;
    // let evt: ref<AddInputGroupEvent> = new AddInputGroupEvent();
    // evt.data = data;
    // evt.groupId = n"FlightController";
    // evt.targetHintContainer = n"GameplayInputHelper";
    // GameInstance.GetUISystem(this.gameInstance).QueueEvent(evt);

    // let wheel = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"wheel_01_br_a") as MeshComponent;
    // if IsDefined(wheel) {
    //   wheel.UpdateHardTransformBinding(n"vehicle_slots", n"wheel_back_left");
    //   this.GetVehicle().AddComponent(wheel);
    // }
    
    this.SetupPositionProviders();

    // this.sys.tppCamera = GetPlayer(this.gameInstance).FindComponentByName(n"vehicleTPPCamera") as vehicleTPPCameraComponent;

    // idk what to do with this
    // let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    // uiSystem.PushGameContext(IntEnum(10));

    if (this.showUI) {
      this.ui.Show();
    }
  
    if !silent {
      this.ShowSimpleMessage("Flight Control Engaged");
    }
    
    FlightLog.Info("[FlightController] Activate");
    this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsActive, true, true);
    this.GetBlackboard().SignalBool(GetAllBlackboardDefs().VehicleFlight.IsActive);
  }

  private func Deactivate(silent: Bool) -> Void {
    this.active = false;
    this.SetupActions();

    //let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    //uiSystem.PopGameContext(IntEnum(10));

    if !silent {
      this.ShowSimpleMessage("Flight Control Disengaged");
    }
    if (this.showUI) {
      this.ui.Hide();
    }

    FlightLog.Info("[FlightController] Deactivate");
    this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsActive, false, true);
    this.GetBlackboard().SignalBool(GetAllBlackboardDefs().VehicleFlight.IsActive);

    
    // let evt: ref<DeleteInputGroupEvent> = new DeleteInputGroupEvent();
    // evt.groupId = n"FlightControl";
    // evt.targetHintContainer = n"GameplayInputHelper";
    // GameInstance.GetUISystem(this.gameInstance).QueueEvent(evt);
  }  

  private func ShowMoreInfo() -> Void {
    // very intrusive - need a prompt/confirmation that they want this popup, eg Detailed Info / About
    let shardUIevent = new NotifyShardRead();
    shardUIevent.title = "Flight Control: Now Available";
    shardUIevent.text = "Your new car is equiped with the state-of-the-art Flight Control!";
    GameInstance.GetUISystem(this.gameInstance).QueueEvent(shardUIevent);
  }

  private func SetupActions() -> Void {
    this.usingKB = this.player.PlayerLastUsedKBM();
    let evt = new UpdateInputHintMultipleEvent();
    evt.targetHintContainer = n"GameplayInputHelper";

    let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.gameInstance);
    this.player.UnregisterInputListener(this);    
    // this.player.RegisterInputListener(this, n"OpenPauseMenu");
    // uiSystem.QueueEvent(FlightController.HideHintFromSource(n"FlightController"));
    if this.enabled {
      // this.player.RegisterInputListener(this, n"Flight_Toggle");
      if this.active {
        this.player.RegisterInputListener(this, n"__DEVICE_CHANGED__");
        this.player.RegisterInputListener(this, n"Pitch");
        this.player.RegisterInputListener(this, n"Sway");
        this.player.RegisterInputListener(this, n"Roll");
        this.player.RegisterInputListener(this, n"SurgePos");
        this.player.RegisterInputListener(this, n"Lift");
        this.player.RegisterInputListener(this, n"Yaw");
        this.player.RegisterInputListener(this, n"SurgeNeg");
        this.player.RegisterInputListener(this, n"Flight_LinearBrake");
        this.player.RegisterInputListener(this, n"Flight_AngularBrake");
        this.player.RegisterInputListener(this, n"Flight_Trick");
        this.player.RegisterInputListener(this, n"Flight_Options");
        this.player.RegisterInputListener(this, n"Flight_UIToggle");
        this.player.RegisterInputListener(this, n"Flight_ModeSwitchForward");
        this.player.RegisterInputListener(this, n"Flight_ModeSwitchBackward");
      }
    }

    // if this.trick {
    //   evt.AddInputHint(FlightController.CreateInputHint("Aileron Roll", n"Yaw"), true);
    // } else {
      // evt.AddInputHint(FlightController.CreateInputHint("Tricks", n"Flight_Trick"), true);
    // }
    // we may want to look at something else besides this input so ForceBrakesUntilStoppedOrFor will work (not entirely sure it doesn't now)
    // vehicle.GetBlackboard().GetInt(GetAllBlackboardDefs().VehicleFlight.IsHandbraking)

    evt.AddInputHint(FlightController.CreateInputHint("Enable Flight", n"Flight_Toggle"),       this.enabled && !this.active);

    evt.AddInputHint(FlightController.CreateInputHint("Disable Flight", n"Flight_Toggle"),      this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint("Yaw", n"Yaw"),                           this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint("Pitch", n"Pitch"),                       this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint("Roll", n"Roll"),                         this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint("Lift", n"Lift"),                         this.active && !this.showOptions);
    evt.AddInputHint(FlightController.CreateInputHint("Linear Brake", n"Flight_LinearBrake"),   this.active && !this.showOptions && this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint("Angular Brake", n"Flight_AngularBrake"), this.active && !this.showOptions && this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint("Brake", n"Flight_LinearBrake"),          this.active && !this.showOptions && !this.usingKB);
    evt.AddInputHint(FlightController.CreateInputHint("Flight Options", n"Flight_Options"),     this.active && !this.showOptions);

    evt.AddInputHint(FlightController.CreateInputHint("Sway", n"Sway"),                         this.active && this.showOptions && this.usingKB);
    // let desc: String;
    // desc = this.sys.playerComponent.GetNextFlightModeDescription();
    evt.AddInputHint(FlightController.CreateInputHint("Next Mode", n"Flight_ModeSwitchForward"),     this.active && (this.showOptions || this.usingKB));
    evt.AddInputHint(FlightController.CreateInputHint("Prev Mode", n"Flight_ModeSwitchBackward"),     this.active && this.showOptions && !this.usingKB);
    // evt.AddInputHint(FlightController.CreateInputHint("Raise Hover Height", n"FlightOptions_Up"), true);
    // evt.AddInputHint(FlightController.CreateInputHint("Lower Hover Height", n"FlightOptions_Down"), true);
    evt.AddInputHint(FlightController.CreateInputHint("Toggle UI", n"Flight_UIToggle"),         this.active && this.showOptions);

    uiSystem.QueueEvent(evt);
  }

  private let trick: Bool;

  public func ProcessImpact(impact: Float) {
    // this.surge.Reset(this.surge.GetValue() * MaxF(0.0, 1.0 - impact * 5.0));
  }

  private func CycleMode(direction: Int32) -> Void {
    let newMode = this.sys.playerComponent.GetNextFlightMode(direction);
    this.mode = this.mode + direction;
    if this.mode < 0 {
      this.mode += ArraySize(this.sys.playerComponent.modes);
    } 
    this.mode = this.mode % ArraySize(this.sys.playerComponent.modes);
    this.GetBlackboard().SetInt(GetAllBlackboardDefs().VehicleFlight.Mode, this.mode);
    let evt = new VehicleFlightModeChangeEvent();
    evt.mode = this.mode;
    GetMountedVehicle(this.player).QueueEvent(evt);
    this.ShowSimpleMessage(newMode.GetDescription() + " Enabled");
    GameInstance.GetAudioSystem(this.gameInstance).Play(n"ui_menu_onpress");
    this.SetupActions();
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    if ListenerAction.IsAction(action, n"__DEVICE_CHANGED__") {
      if this.player.PlayerLastUsedKBM() {
        this.usingKB = true;
      } else {
        this.usingKB = false;
      } 
      this.SetupActions();
    }
    let actionType: gameinputActionType = ListenerAction.GetType(action);
    let actionName: CName = ListenerAction.GetName(action);
    let value: Float = ListenerAction.GetValue(action);
    // FlightLog.Info(ToString(actionType) + ToString(actionName) + ToString(value));
    // if Equals(actionName, n"Flight_Toggle") && ListenerAction.IsButtonJustPressed(action) {
        // this.Toggle();
        // ListenerActionConsumer.ConsumeSingleAction(consumer);
    // }
    if this.active {
      // if Equals(actionName, n"Flight_ModeSwitchForward") && ListenerAction.IsButtonJustPressed(action) {
      //   this.CycleMode(1);
      // }
      if Equals(actionName, n"Flight_Options") {
        if ListenerAction.IsButtonJustPressed(action) {
          // FlightLog.Info("Options button pressed");
          this.showOptions = true;
          // GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"summon_hologram", true);
          if (this.showUI) {
            this.ui.ShowInfo();
          }
          this.SetupActions();
        }
        if ListenerAction.IsButtonJustReleased(action) {
          // FlightLog.Info("Options button released");
          this.showOptions = false; 
          // GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"summon_hologram");
          this.SetupActions();
        }
      }

      // if Equals(actionName, n"Flight_Trick") {
      //   if ListenerAction.IsButtonJustPressed(action) {

      //     let attack: ref<Attack_GameEffect>;
      //     let attackContext: AttackInitContext;
      //     let effect: ref<EffectInstance>;
      //     let statMods: array<ref<gameStatModifierData>>;    
      //     let position: Vector4;
      //     let forward: Vector4;
      //     attackContext.source = this.sys.playerComponent.GetVehicle();
      //     attackContext.record = TweakDBInterface.GetAttackRecord(t"Attacks.Bullet_GameEffect");
      //     attackContext.instigator = attackContext.source;
      //     attack = IAttack.Create(attackContext) as Attack_GameEffect;
      //     attack.GetStatModList(statMods);
      //     effect = attack.PrepareAttack(this.sys.playerComponent.GetVehicle());
      //     GameInstance.GetTargetingSystem(this.gameInstance).GetDefaultCrosshairData(this.sys.player, position, forward);
      //     EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.position, position);
      //     EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.muzzlePosition, position);
      //     // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.forward, this.sys.playerComponent.stats.d_forward);
      //     EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.forward, forward);
      //     EffectData.SetVariant(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.attack, ToVariant(attack));
      //     EffectData.SetVariant(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.attackStatModList, ToVariant(statMods));
      //     attack.StartAttack();
      //   }
      // }

          // // FlightLog.Info("Options button pressed");
          // this.trick = true;
          // let direction = 0.0;
          // if AbsF(this.sway.GetInput()) > 0.9 {
          //   direction = this.sway.GetInput();
          // }
          // if AbsF(this.yaw.GetInput()) > 0.9 {
          //   direction = this.yaw.GetInput();
          // }
          // if this.sys.playerComponent.trick == null  && AbsF(direction) > 0.9 {
          //   this.sys.playerComponent.trick = FlightTrickAileronRoll.Create(this.sys.playerComponent, Cast<Float>(RoundF(direction)));
          // }
          // this.SetupActions();
        // }
        // if ListenerAction.IsButtonJustReleased(action) {
        //   FlightLog.Info("Options button released");
        //   this.trick = false; 
        //   this.SetupActions();
        // }
      // }
        // if Equals(actionName, n"FlightOptions_Up") && ListenerAction.IsButtonJustPressed(action) {
        //     this.hoverHeight += 0.1;
        //     GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_menu_onpress");
        //     FlightLog.Info("hoverHeight = " + ToString(this.hoverHeight));
        // }
        // if Equals(actionName, n"FlightOptions_Down") && ListenerAction.IsButtonJustPressed(action) {
        //     this.hoverHeight -= 0.1;
        //     GameInstance.GetAudioSystem(this.gameInstance).PlayFlightSound(n"ui_menu_onpress");
        //     FlightLog.Info("hoverHeight = " + ToString(this.hoverHeight));
        // }
      if Equals(actionName, n"Flight_ModeSwitchForward") && ListenerAction.IsButtonJustPressed(action) && (this.showOptions || this.player.PlayerLastUsedKBM()) {
        this.CycleMode(1);
      }
      if Equals(actionName, n"Flight_ModeSwitchBackward") && ListenerAction.IsButtonJustPressed(action) && (this.showOptions || this.player.PlayerLastUsedKBM()) {
        this.CycleMode(-1);
      }
      if this.showOptions && Equals(actionName, n"Flight_UIToggle") && ListenerAction.IsButtonJustPressed(action) {
          this.showUI = !this.showUI;
          if (this.showUI) {
            this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, true, true);
            this.ShowSimpleMessage("Flight UI Shown");
          } else {
            this.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, false, true);
            this.ShowSimpleMessage("Flight UI Hidden");
          }
          GameInstance.GetAudioSystem(this.gameInstance).Play(n"ui_menu_onpress");
      }
      
      if Equals(actionType, gameinputActionType.AXIS_CHANGE) {
        switch(actionName) {
          case n"Roll":
            this.roll.SetInput(value);
            break;
          case n"Pitch":
            this.pitch.SetInput(value);
            break;
          case n"SurgePos":
            this.surge.SetInput(value);
            break;
          case n"Yaw":
            // if this.trick {
            //   if this.sys.playerComponent.trick == null  && AbsF(value) > 0.9 {
            //     this.sys.playerComponent.trick = FlightTrickAileronRoll.Create(this.sys.playerComponent, Cast<Float>(RoundF(value)));
            //   }
            //   this.yaw.SetInput(0.0);
            //   this.sway.SetInput(0.0);
            // } else {
              if this.showOptions {
                this.sway.SetInput(value);
                this.yaw.SetInput(0.0);
              } else {
                this.sway.SetInput(0.0);
                this.yaw.SetInput(value);
              }
            // }
            break;
          case n"Sway":
            this.sway.SetInput(value);
            break;
          case n"Lift":
            if this.trick {
              this.lift.SetInput(0.0);
            } else {
              this.lift.SetInput(value);
            }
            break;
          case n"SurgeNeg":
            this.surge.SetInput(-value);
            break;
          default:
            // return false;
            break;
        }
      }
        // ListenerActionConsumer.ConsumeSingleAction(consumer);
      if Equals(actionName, n"Flight_LinearBrake") {
        if Equals(actionType, gameinputActionType.BUTTON_PRESSED) {
          this.linearBrake.SetInput(1.0);
        } else {
          this.linearBrake.SetInput(0.0);
        }
      }
      if Equals(actionName, n"Flight_AngularBrake") {
        if Equals(actionType, gameinputActionType.BUTTON_PRESSED) {
          this.angularBrake.SetInput(1.0);
        } else {
          this.angularBrake.SetInput(0.0);
        }
      }
    } else {
      this.lift.SetInput(0.0);
      this.surge.SetInput(0.0);
      this.yaw.SetInput(0.0);
      this.sway.SetInput(0.0);
      this.pitch.SetInput(0.0);
      this.roll.SetInput(0.0);
      this.linearBrake.SetInput(0.0);
      this.angularBrake.SetInput(0.0);
    }
  }

  public func UpdateInputs(timeDelta: Float) -> Void {
    this.yaw.GetValue(timeDelta);
    this.sway.GetValue(timeDelta);
    this.roll.GetValue(timeDelta);
    this.pitch.GetValue(timeDelta);
    this.lift.GetValue(timeDelta);
    this.linearBrake.GetValue(timeDelta);
    this.angularBrake.GetValue(timeDelta);
    this.surge.GetValue(timeDelta);
  }


  public func SetupPositionProviders() -> Void {
    this.sys.audio.AddSlotProviders(GetMountedVehicle(this.player));
  }

  // protected cb func PhysicsUpdate(evt: ref<vehicleFlightPhysicsUpdateEvent>) -> Bool {
    // FlightLog.Info("in the physics update!");
  // }

  // public final func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  public final func OnUpdate(timeDelta: Float) -> Void {

    // let cameraPos = this.ui.camera.GetLocalToWorld() * Vector4.EmptyVector();
    // let localCameraPos = Matrix.GetInverted(this.GetVehicle().chassis.GetLocalToWorld()) * cameraPos;
    // let idealCameraPos = new Vector4(0.0, -6.0, 1.2, 0.0);
    // let yaw = Vector4.GetAngleDegAroundAxis(localCameraPos, idealCameraPos, new Vector4(0.0, 0.0, 1.0, 0.0));
    // let pitch = Vector4.GetAngleDegAroundAxis(localCameraPos, idealCameraPos, new Vector4(1.0, 0.0, 0.0, 0.0));

    // if !IsDefined(this.uiBlackboard) {
    //   this.uiBlackboard = GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_System);
    //   this.uiSystemBB = GetAllBlackboardDefs().UI_System;
    //   this.trackedMappinId = this.uiBlackboard.RegisterListenerVariant(this.uiSystemBB.TrackedMappin, this, n"OnTrackedMappinUpdated");
    //   this.uiBlackboard.SignalVariant(this.uiSystemBB.TrackedMappin);
    // }

    // this.isInAnyMenu = this.uiBlackboard.GetBool(this.uiSystemBB.IsInMenu);
    
    // if !this.active {
    //   if (this.showUI) { 
    //     this.ui.ClearMarks();
    //   }
    //   return;
    // }
    // might need to handle just the scanning system's dilation, and the pause menu
    // if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive(n"radial") {
    //   // this might happpen?
    //   timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("radialMenu", "timeDilation");
    //   //FlightLog.Info("Radial menu dilation"); 
    // } else {
    //   if GameInstance.GetTimeSystem(this.gameInstance).IsTimeDilationActive() {
    //     // i think this is what this is called
    //     timeDelta *= TimeDilationHelper.GetFloatFromTimeSystemTweak("focusModeTimeDilation", "timeDilation");
    //     //FlightLog.Info("Other time dilation"); 
    //   }
    // }

    // let player: ref<PlayerPuppet> = GetPlayer(this.gameInstance);
    // if !IsDefined(this.GetVehicle()) { 
    //   // if IsDefined(scriptInterface.owner as VehicleObject) {
    //   //   this.stats = FlightStats.Create(scriptInterface.owner as VehicleObject);
    //   //   FlightLog.Warn("Vehicle undefined. Redefined to " + this.GetVehicle().GetDisplayName()); 
    //   // } else {
    //     FlightLog.Error("Owner not defined"); 
    //     return;
    //   // }
    // }
    // if !this.GetVehicle().IsPlayerMounted() { 
    //   FlightLog.Error("Vehicle is not player mounted"); 
    //   return; 
    // }

    // if (this.showUI) { 
    //   // this.navPath.Update();
    //   this.ui.ClearMarks();
    // }

    this.UpdateInputs(timeDelta);

    // if (this.showUI) {
      // this.ui.Update(timeDelta);
      // this.ui.DrawMark(this.stats.d_visualPosition);
      // this.ui.DrawText(this.stats.d_visualPosition, FloatToStringPrec(1.0 / this.timeDelta, 4));
    // }

    // this.timeDelta = timeDelta * 0.001 + this.timeDelta * 0.999;
  }

  public func ShowSimpleMessage(message: String) -> Void {
    let msg: SimpleScreenMessage;
    msg.isShown = true;
    msg.duration = 2.00;
    msg.message = message;
    msg.isInstant = true;
    GameInstance.GetBlackboardSystem(this.gameInstance).Get(GetAllBlackboardDefs().UI_Notifications).SetVariant(GetAllBlackboardDefs().UI_Notifications.OnscreenMessage, ToVariant(msg), true);
  }

  public static func CreateInputHint(label: String, action: CName) -> InputHintData {
    let data: InputHintData;
    data.source = n"FlightController";
    data.action = action;
    data.localizedLabel = label;
    // data.groupId = n"FlightController";
    return data;
  }

  public static func HideHintFromSource(source: CName) -> ref<DeleteInputHintBySourceEvent> {
    let evt: ref<DeleteInputHintBySourceEvent> = new DeleteInputHintBySourceEvent();
    evt.source = source;
    evt.targetHintContainer = n"GameplayInputHelper";
    return evt;
  }

  // Method to add new widget requires parent widget
  // inkCompoundWidget is a base class for all widget types that can have children
  // Usually it's inkCanvas or inkFlex containers with absolute positioning
  // And inkHorizontalPanel or inkVerticalPanel containers for auto layouts
  public static func HUDStatusSetup(parent: wref<inkCompoundWidget>) -> ref<inkText> {
    let flightControlStatus: ref<inkText> = new inkText();
    flightControlStatus.SetName(n"flightControlStatus");
    // Add widget instance to the parent
    flightControlStatus.Reparent(parent);

    // Set font
    flightControlStatus.SetFontFamily("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily");
    flightControlStatus.SetFontStyle(n"Medium");
    flightControlStatus.SetFontSize(24);
    flightControlStatus.SetLetterCase(textLetterCase.UpperCase);

    // Set color
    flightControlStatus.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));

    // Set content
    flightControlStatus.SetText("You shouldn't see this!");
    // flightControlStatus.SetHorizontalAlignment(textHorizontalAlignment.Center);
    // flightControlStatus.SetAnchor(inkEAnchor.TopCenter);


    // Set widget position relative to parent
    // Altough the position is absolute for FHD resoltuion,
    // it will be adapted for the current resoltuion
    flightControlStatus.SetMargin(100, 1802, 0, 0);

    // Set widget size
    flightControlStatus.SetSize(220.0, 50.0);
    return flightControlStatus;
  }

}

// @addMethod(AudioSystem)
// public final func PlayFlightSound(sound: CName) -> Void {
//   this.Play(sound);
// }

// @addField(PlayerPuppet)
// public let flightController: ref<FlightController>; // Must be strong reference

// @addMethod(PlayerPuppet)
// public func GetFlightController() -> ref<FlightController> {
//   return this.flightController;
// }

// @addField(AllBlackboardDefinitions)
// public let flightController: wref<FlightController>; // Must be weak reference

// Option 2 -- Get the player instance as soon as it's ready
@wrapMethod(PlayerPuppet)
protected cb func OnGameAttached() -> Bool {
  wrappedMethod();
  if !this.IsReplacer() {
    FlightController.CreateInstance(this);
  }
}

// @wrapMethod(Ground)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return wrappedMethod(stateContext, scriptInterface) || FlightController.GetInstance().IsActive();
// }

// @wrapMethod(Air)
// protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   return wrappedMethod(stateContext, scriptInterface) && !FlightController.GetInstance().IsActive();
// }

// might be good to replace this
// @wrapMethod(ReactionManagerComponent)
// private final func ShouldStimBeProcessedByCrowd(stimEvent: ref<StimuliEvent>) -> Bool {

// }


//  public const func HasDirectActionsActive() -> Bool {
//     return false;
//   }


  // protected final func MountFromParent(mountingEvent: ref<MountingEvent>, ownerEntity: ref<Entity>) -> Void {
  //   let instanceData: StateMachineInstanceData;
  //   let initData: ref<VehicleTransitionInitData> = new VehicleTransitionInitData();
  //   let relationship: MountingRelationship = mountingEvent.relationship;
  //   let otherObjectType: gameMountingObjectType = relationship.otherMountableType;
  //   let otherObject: wref<GameObject> = IMountingFacility.RelationshipGetOtherObject(relationship);
  //   switch otherObjectType {
  //     case gameMountingObjectType.Vehicle:
  //       if mountingEvent.request.mountData.mountEventOptions.silentUnmount {
  //         return;
  //       };
  //       initData.instant = mountingEvent.request.mountData.isInstant;
  //       initData.entityID = mountingEvent.request.mountData.mountEventOptions.entityID;
  //       initData.alive = mountingEvent.request.mountData.mountEventOptions.alive;
  //       initData.occupiedByNeutral = mountingEvent.request.mountData.mountEventOptions.occupiedByNeutral;
  //       instanceData.initData = initData;
  //       this.AddStateMachine(n"Vehicle", instanceData, otherObject);
