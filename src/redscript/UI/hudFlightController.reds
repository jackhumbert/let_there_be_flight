
public class FlightUIVehicleHealthStatPoolListener extends CustomValueStatPoolsListener {

  public let m_owner: wref<hudFlightController>;
  public let m_vehicle: wref<VehicleObject>;

  public func OnStatPoolValueChanged(oldValue: Float, newValue: Float, percToPoints: Float) -> Void {
    if IsDefined(this.m_owner) {
      this.m_owner.ReactToHPChange(newValue);
    };
  }
}

public class hudFlightController extends inkHUDGameController {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-UI-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Enabled")
  public let enabled: Bool = true;

  private let m_Date: inkTextRef;
  private let m_Timer: inkTextRef;
  private let m_CameraID: inkTextRef;
  private let healthStatus: inkTextRef;
  private let m_MessageText: inkTextRef;
  private let m_pitchFluff: inkTextRef;
  private let m_yawFluff: inkTextRef;
  private let m_leftPart: inkWidgetRef;
  private let m_rightPart: inkWidgetRef;
  // @default(hudFlightController, -838.0f)
  private let offsetLeft: Float;
  // @default(hudFlightController, 1495.0f)
  private let offsetRight: Float;
  private let currentTime: GameTime;
  private let m_currentHealth: Int32;
  private let m_previousHealth: Int32;
  private let m_maximumHealth: Int32;
  private let m_playerObject: wref<GameObject>;
  private let m_playerPuppet: wref<GameObject>;
  private let m_gameInstance: GameInstance;
  private let m_animationProxy: ref<inkAnimProxy>;

  private let m_bbPlayerStats: wref<IBlackboard>;
  private let m_scannerBlackboard: wref<IBlackboard>;
  private let m_vehicleBlackboard: wref<IBlackboard>;
  private let m_vehicleFlightBlackboard: wref<IBlackboard>;
  private let m_psmBlackboard: wref<IBlackboard>;

  private let m_uiScannerVisibleCallbackID: ref<CallbackHandle>;
  private let m_bbPlayerEventId: ref<CallbackHandle>;
  private let m_PSM_BBID: ref<CallbackHandle>;
  private let m_playerStateBBConnectionId: ref<CallbackHandle>;
  private let m_vehicleBBUIActivId: ref<CallbackHandle>;
  private let m_vehicleBBActivId: ref<CallbackHandle>;
  private let m_vehicleBBModeId: ref<CallbackHandle>;
  private let m_vehicleRollID: ref<CallbackHandle>;
  private let m_vehiclePitchID: ref<CallbackHandle>;
  private let m_tppBBConnectionId: ref<CallbackHandle>;

  public let m_healthStatPoolListener: ref<FlightUIVehicleHealthStatPoolListener>;
  private let m_hp_mask: inkWidgetRef;
  private let m_hp_condition_text: inkTextRef;
  private let m_currentZoom: Float;

  protected cb func OnInitialize() -> Bool {
    FlightLog.Info("[hudFlightController] OnInitialize");
    let delayInitialize: ref<DelayedHUDInitializeEvent>;
    // inkTextRef.SetText(this.m_Date, "XX-XX-XXXX");
    delayInitialize = new DelayedHUDInitializeEvent();
    GameInstance.GetDelaySystem(this.GetPlayerControlledObject().GetGame()).DelayEvent(this.GetPlayerControlledObject(), delayInitialize, 0.10);
    // this.GetPlayerControlledObject().RegisterInputListener(this);
    this.offsetLeft = -838.0;
    this.offsetRight = 1495.0;
    this.GetRootWidget().SetVisible(false);
    // this.PlayLibraryAnimation(n"outro");

    let vehicle = FlightSystem.GetInstance().playerComponent.GetVehicle();
    this.m_healthStatPoolListener = new FlightUIVehicleHealthStatPoolListener();
    this.m_healthStatPoolListener.m_owner = this;
    this.m_healthStatPoolListener.m_vehicle = vehicle;
    let stats = GameInstance.GetStatPoolsSystem(vehicle.GetGame());
    if IsDefined(stats) {  
      stats.RequestRegisteringListener(Cast<StatsObjectID>(vehicle.GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    }
    IMZ_Comp_SetBlackboardValue(this.m_gameInstance, true); 
  }

  protected cb func OnUninitialize() -> Bool {
    FlightLog.Info("[hudFlightController] OnUninitialize");
    // TakeOverControlSystem.CreateInputHint(this.GetPlayerControlledObject().GetGame(), false);
    // SecurityTurret.CreateInputHint(this.GetPlayerControlledObject().GetGame(), false);
    if IsDefined(this.m_healthStatPoolListener) {
      GameInstance.GetStatPoolsSystem(this.m_gameInstance).RequestUnregisteringListener(Cast(this.m_healthStatPoolListener.m_vehicle.GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    }
    IMZ_Comp_SetBlackboardValue(this.m_gameInstance, false); 
  }

  private func UpdateTime() -> Void {
    this.currentTime = GameInstance.GetTimeSystem(this.m_gameInstance).GetGameTime();
    inkTextRef.SetText(this.m_Timer, ToString(GameTime.Hours(this.currentTime)) + ":" + ToString(GameTime.Minutes(this.currentTime)) + ":" + ToString(GameTime.Seconds(this.currentTime)));
  }

  private final func IsUIactive() -> Bool {
    if IsDefined(this.m_vehicleFlightBlackboard) && this.m_vehicleFlightBlackboard.GetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive) {
      return true;
    };
    return false;
  }

  private final func IsActive() -> Bool {
    if IsDefined(this.m_vehicleFlightBlackboard) && this.m_vehicleFlightBlackboard.GetBool(GetAllBlackboardDefs().VehicleFlight.IsActive) {
      return true;
    };
    return false;
  }

  protected cb func OnActivateUI(activate: Bool) -> Bool {
    this.ActivateUI(activate);
  }

  protected cb func OnActivate(activate: Bool) -> Bool {
    if this.IsUIactive() {
      this.ActivateUI(activate);
    }
  }

  protected cb func OnModeChange(mode: Int32) -> Bool {
    inkTextRef.SetText(this.m_CameraID, FlightSystem.GetInstance().playerComponent.GetFlightMode().GetDescription());
  }

  protected cb func OnCameraModeChanged(tpp: Bool) -> Bool {
    let hp_gauge = this.GetRootCompoundWidget().GetWidget(n"hp_gauge");
    if IsDefined(hp_gauge) {
      // doesn't seem to work
      // if tpp {
      // } else {
      //   hp_gauge.SetMargin(new inkMargin(2150.0, 32.0 - 100.0, 0.0, 0.0));
      // }

      // right of LB hud stuff
      // hp_gauge.SetMargin(new inkMargin(2150.0, 32.0, 0.0, 0.0));
      // above of LB hud stuff
      hp_gauge.SetMargin(new inkMargin(1520.0, -290.0, 0.0, 0.0));
    }
  }

  protected cb func OnVehicleRollChanged(roll: Float) -> Bool {
    // if FlightSystem.GetInstance().playerComponent.GetFlightMode().usesRightStickInput && !FlightSystem.GetInstance().ctlr.isTPP {
    if !FlightSystem.GetInstance().ctlr.isTPP { // FPP
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/rulers").SetRotation(0);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/rulers/h").SetRotation(-roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/rotation").SetRotation(-roll);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetVisible(false);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetRotation(0);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch_mask").SetVisible(true);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetRotation(0);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetScale(new Vector2(1.0, 1.0));
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/crosshair").SetRotation(0);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/crosshair/reticle").SetRotation(-roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/RADIUS").SetRotation(-roll);
    } else { // TPP
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/rulers").SetRotation(roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/rulers/h").SetRotation(-roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/rotation").SetRotation(0);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetVisible(true);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetRotation(roll);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch_mask").SetVisible(false);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch_mask").SetRotation(roll);
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/mask").SetScale(new Vector2(1.0, 1.0));
      // this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/crosshair").SetRotation(roll);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/crosshair/reticle").SetRotation(0);
      this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/RADIUS").SetRotation(roll);
    }
  }

  protected cb func OnVehiclePitchChanged(pitch: Float) -> Bool {
    this.GetRootCompoundWidget().GetWidget(n"crosshairContainer/pitch/rotation/translation").SetTranslation(new Vector2(0.0, pitch/90.0 * 900.0));
  }

  private let m_introAnimationProxy: ref<inkAnimProxy>;
  private let m_outroAnimationProxy: ref<inkAnimProxy>;

  private func ActivateUI(activate: Bool) -> Void {

    FlightLog.Info("[hudFlightController] ActivateUI");
    if activate {
      if this.enabled {
        let vehicle = FlightSystem.GetInstance().playerComponent.GetVehicle();
        if IsDefined(vehicle) {
          inkTextRef.SetText(this.m_hp_condition_text, vehicle.GetDisplayName());
          let stats = GameInstance.GetStatPoolsSystem(this.m_gameInstance);
          this.ReactToHPChange(stats.GetStatPoolValue(Cast<StatsObjectID>(vehicle.GetEntityID()), gamedataStatPoolType.Health, true));  
        }
        inkTextRef.SetText(this.m_CameraID, FlightSystem.GetInstance().playerComponent.GetFlightMode().GetDescription());

        let hp_gauge = this.GetRootCompoundWidget().GetWidget(n"hp_gauge");
        if IsDefined(hp_gauge) {
          // right of LB hud stuff
          // hp_gauge.SetMargin(new inkMargin(2150.0, 32.0, 0.0, 0.0));
          // above of LB hud stuff
          hp_gauge.SetMargin(new inkMargin(1520.0, -290.0, 0.0, 0.0));
        }
        
        this.GetRootWidget().SetVisible(true);
        let options: inkAnimOptions;
        options.executionDelay = 0.50;
        if IsDefined(this.m_outroAnimationProxy) && this.m_outroAnimationProxy.IsPlaying() {
          this.m_outroAnimationProxy.Stop();
        }
        this.m_introAnimationProxy = this.PlayLibraryAnimation(n"intro", options);
        // this.PlayAnim(n"intro", n"OnIntroComplete");
        // optionIntro.executionDelay = 0.25;
        // this.PlayLibraryAnimation(n"Malfunction_off", optionIntro);
        // this.PlayAnim(n"Malfunction_timed", n"OnMalfunction");
        // this.UpdateJohnnyThemeOverride(true);
      }
    } else {
      // this.GetRootWidget().SetVisible(false);
      // this.PlayLibraryAnimation(n"outro");
      // this.PlayLibraryAnimation(n"Malfunction");
      let options: inkAnimOptions;
      if IsDefined(this.m_introAnimationProxy) && this.m_introAnimationProxy.IsPlaying() {
        this.m_introAnimationProxy.Stop();
      }
      this.m_outroAnimationProxy = this.PlayLibraryAnimation(n"outro", options);

      // this.PlayAnim(n"outro", n"OnOutroComplete");
      // this.UpdateJohnnyThemeOverride(false);
    }
  }

  protected cb func OnPlayerAttach(playerPuppet: ref<GameObject>) -> Bool {
    FlightLog.Info("[hudFlightController] OnPlayerAttach");
    this.m_playerObject = playerPuppet;
    this.m_playerPuppet = playerPuppet;
    this.m_gameInstance = playerPuppet.GetGame();
    // this.UpdateTime();
    this.m_vehicleBlackboard = FlightSystem.GetInstance().playerComponent.GetVehicle().GetBlackboard();
    this.m_vehicleFlightBlackboard = FlightController.GetInstance().GetBlackboard();
    this.m_scannerBlackboard = GameInstance.GetBlackboardSystem(this.m_gameInstance).Get(GetAllBlackboardDefs().UI_Scanner);
    this.RegisterBB();
    this.ActivateUI(this.IsUIactive() && this.IsActive());
  }

  protected func RegisterBB() {
    // this.m_bbPlayerStats = this.GetBlackboardSystem().Get(GetAllBlackboardDefs().UI_PlayerBioMonitor);
    // this.m_bbPlayerEventId = this.m_bbPlayerStats.RegisterListenerVariant(GetAllBlackboardDefs().UI_PlayerBioMonitor.PlayerStatsInfo, this, n"OnStatsChanged");    
    // this.m_psmBlackboard = this.GetPSMBlackboard(this.m_playerPuppet);
    // if IsDefined(this.m_psmBlackboard) {
    //   this.m_PSM_BBID = this.m_psmBlackboard.RegisterDelayedListenerFloat(GetAllBlackboardDefs().PlayerStateMachine.ZoomLevel, this, n"OnZoomChange");
    // };

    if IsDefined(this.m_vehicleFlightBlackboard) {
      if !IsDefined(this.m_vehicleBBUIActivId) {
        this.m_vehicleBBUIActivId = this.m_vehicleFlightBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this, n"OnActivateUI");
      }
      if !IsDefined(this.m_vehicleBBActivId) {
        this.m_vehicleBBActivId = this.m_vehicleFlightBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this, n"OnActivate");
      };
      if !IsDefined(this.m_vehicleBBModeId) {
        this.m_vehicleBBModeId = this.m_vehicleFlightBlackboard.RegisterListenerInt(GetAllBlackboardDefs().VehicleFlight.Mode, this, n"OnModeChange");
      };
      if !IsDefined(this.m_vehicleRollID) {
        this.m_vehicleRollID = this.m_vehicleFlightBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Roll, this, n"OnVehicleRollChanged");
      };
      if !IsDefined(this.m_vehiclePitchID) {
        this.m_vehiclePitchID = this.m_vehicleFlightBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this, n"OnVehiclePitchChanged");
      };
    };
    if IsDefined(this.m_vehicleBlackboard) {
      this.m_tppBBConnectionId = this.m_vehicleBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this, n"OnCameraModeChanged");
    }
    if IsDefined(this.m_scannerBlackboard) && !IsDefined(this.m_uiScannerVisibleCallbackID) {
      this.m_uiScannerVisibleCallbackID = this.m_scannerBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_Scanner.UIVisible, this, n"OnScannerUIVisibleChanged");
    };
  }

  protected func UnregisterBB() {
    // if IsDefined(this.m_bbPlayerStats) {
      // this.m_bbPlayerStats.UnregisterListenerVariant(GetAllBlackboardDefs().UI_PlayerBioMonitor.PlayerStatsInfo, this.m_bbPlayerEventId);
    // };
    // if IsDefined(this.m_psmBlackboard) && IsDefined(this.m_PSM_BBID) {
    //   this.m_psmBlackboard.UnregisterDelayedListener(GetAllBlackboardDefs().PlayerStateMachine.ZoomLevel, this.m_PSM_BBID);
    // };
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if IsDefined(this.m_vehicleBBUIActivId) {
        this.m_vehicleFlightBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this.m_vehicleBBUIActivId);
      }
      if IsDefined(this.m_vehicleBBActivId) {
        this.m_vehicleFlightBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsActive, this.m_vehicleBBActivId);
      };
      if IsDefined(this.m_vehicleBBModeId) {
        this.m_vehicleFlightBlackboard.UnregisterListenerInt(GetAllBlackboardDefs().VehicleFlight.Mode, this.m_vehicleBBModeId);
      };
      if IsDefined(this.m_vehicleRollID) {
        this.m_vehicleFlightBlackboard.UnregisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Roll, this.m_vehicleRollID);
      };
      if IsDefined(this.m_vehiclePitchID) {
        this.m_vehicleFlightBlackboard.UnregisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this.m_vehiclePitchID);
      };
    }
    if IsDefined(this.m_vehicleBlackboard) {
      if IsDefined(this.m_tppBBConnectionId) {
        this.m_vehicleBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this.m_tppBBConnectionId);
      }
    }
    if IsDefined(this.m_scannerBlackboard) {
      if IsDefined(this.m_uiScannerVisibleCallbackID) {
        this.m_scannerBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_Scanner.UIVisible, this.m_uiScannerVisibleCallbackID);
      }
    }
  }

  protected cb func OnPlayerDetach(playerPuppet: ref<GameObject>) -> Bool {
    FlightLog.Info("[hudFlightController] OnPlayerDetach");
    this.UnregisterBB();
  }

  // protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
  //   let yaw: Float = ClampF(this.m_playerPuppet.GetWorldYaw(), -300.00, 300.00);
  //   inkTextRef.SetText(this.m_yawFluff, ToString(yaw));
  //   inkTextRef.SetText(this.m_pitchFluff, ToString(yaw * 1.50));
  //   inkWidgetRef.SetMargin(this.m_leftPart, new inkMargin(yaw, this.offsetLeft, 0.00, 0.00));
  //   inkWidgetRef.SetMargin(this.m_rightPart, new inkMargin(this.offsetRight, yaw, 0.00, 0.00));
  //   this.UpdateTime();
  // }
  
  protected cb func OnZoomChange(evt: Float) -> Bool {
    // if evt > this.m_currentZoom {
    //     this.PlayLibraryAnimation(n"zoomUp");
    // } else {
    //     this.PlayLibraryAnimation(n"zoomDown");
    // }
    // this.m_currentZoom = evt;
  }

  protected cb func OnIntroComplete(anim: ref<inkAnimProxy>) -> Bool {
    // GameInstance.GetAudioSystem(this.GetPlayerControlledObject().GetGame()).Play(n"ui_main_menu_cc_loading");
  }
  protected cb func OnOutroComplete(anim: ref<inkAnimProxy>) -> Bool {
      // this.GetRootWidget().SetVisible(false);
  }

  protected cb func OnStatsChanged(value: Variant) -> Bool {
    // let incomingData: PlayerBioMonitor = FromVariant<PlayerBioMonitor>(value);
    // this.m_previousHealth = this.m_currentHealth;
    // this.m_maximumHealth = incomingData.maximumHealth;
       
   
    // this.m_currentHealth = CeilF(GameInstance.GetStatPoolsSystem(this.m_playerObject.GetGame()).GetStatPoolValue(Cast<StatsObjectID>(GetPlayer(this.m_playerObject.GetGame()).GetEntityID()), gamedataStatPoolType.Health, false));
    // this.m_currentHealth = Clamp(this.m_currentHealth, 0, this.m_maximumHealth);
  }

  protected cb func OnScannerUIVisibleChanged(visible: Bool) -> Bool {
    if this.IsUIactive() && this.IsActive() {
      this.ActivateUI(!visible);
    }
  }

  public func ReactToHPChange(value: Float) -> Void {
    inkTextRef.SetText(this.healthStatus, IntToString(RoundF(value)) + "/100");
    inkWidgetRef.SetMargin(this.m_hp_mask, new inkMargin(-1720.0 - ((100.0 - value) * 9.0), 826.66638183, 0, 0));
  }

  protected cb func OnDelayedHUDInitializeEvent(evt: ref<DelayedHUDInitializeEvent>) -> Bool {
    // TakeOverControlSystem.CreateInputHint(this.GetPlayerControlledObject().GetGame(), true);
    // SecurityTurret.CreateInputHint(this.GetPlayerControlledObject().GetGame(), true);
  }

  public final func PlayAnim(animName: CName, opt callBack: CName, opt animOptions: inkAnimOptions) -> Void {
    if IsDefined(this.m_animationProxy) && this.m_animationProxy.IsPlaying() {
      this.m_animationProxy.Stop(true);
    };
    this.m_animationProxy = this.PlayLibraryAnimation(animName, animOptions);
    if NotEquals(callBack, n"") {
      this.m_animationProxy.RegisterToCallback(inkanimEventType.OnFinish, this, callBack);
    };
  }

  private final func UpdateJohnnyThemeOverride(value: Bool) -> Void {
    let uiSystem: ref<UISystem>;
    let controlledPuppet: wref<gamePuppetBase> = GetPlayer(this.m_gameInstance);
    if IsDefined(controlledPuppet) && controlledPuppet.IsJohnnyReplacer() {
      uiSystem = GameInstance.GetUISystem(this.m_gameInstance);
      if IsDefined(uiSystem) {
        if value {
          uiSystem.SetGlobalThemeOverride(n"Johnny");
        } else {
          uiSystem.ClearGlobalThemeOverride();
        };
      };
    };
  }
}
