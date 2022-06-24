
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
  private let m_bbPlayerStats: wref<IBlackboard>;
  private let m_bbPlayerEventId: ref<CallbackHandle>;
  private let m_currentHealth: Int32;
  private let m_previousHealth: Int32;
  private let m_maximumHealth: Int32;
  private let m_playerObject: wref<GameObject>;
  private let m_playerPuppet: wref<GameObject>;
  private let m_gameInstance: GameInstance;
  private let m_animationProxy: ref<inkAnimProxy>;
  private let m_vehicleFlightBlackboard: wref<IBlackboard>;
  private let m_vehicleBBUIActivId: ref<CallbackHandle>;
  public let m_healthStatPoolListener: ref<FlightUIVehicleHealthStatPoolListener>;

  protected cb func OnInitialize() -> Bool {
    FlightLog.Info("[hudFlightController] OnInitialize");
    let delayInitialize: ref<DelayedHUDInitializeEvent>;
    this.currentTime = GameInstance.GetTimeSystem(FlightSystem.GetInstance().gameInstance).GetGameTime();
    inkTextRef.SetText(this.m_Date, "XX-XX-XXXX");
    inkTextRef.SetText(this.m_CameraID, FlightSystem.GetInstance().playerComponent.GetFlightMode().GetDescription());
    inkTextRef.SetText(this.m_Timer, ToString(GameTime.Hours(this.currentTime)) + ":" + ToString(GameTime.Minutes(this.currentTime)) + ":" + ToString(GameTime.Seconds(this.currentTime)));
    delayInitialize = new DelayedHUDInitializeEvent();
    GameInstance.GetDelaySystem(this.GetPlayerControlledObject().GetGame()).DelayEvent(this.GetPlayerControlledObject(), delayInitialize, 0.10);
    this.GetPlayerControlledObject().RegisterInputListener(this);
    this.offsetLeft = -838.0;
    this.offsetRight = 1495.0;
    this.GetRootWidget().SetVisible(false);
    
    this.m_vehicleFlightBlackboard = FlightController.GetInstance().GetBlackboard();
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if !IsDefined(this.m_vehicleBBUIActivId) {
        this.m_vehicleBBUIActivId = this.m_vehicleFlightBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this, n"OnActivateUI");
      };
    };
  }

  private final func IsUIactive() -> Bool {
    if IsDefined(this.m_vehicleFlightBlackboard) && this.m_vehicleFlightBlackboard.GetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive) {
      return true;
    };
    return false;
  }

  protected cb func OnActivateUI(activate: Bool) -> Bool {
    this.ActivateUI(activate);
  }

  private func ActivateUI(activate: Bool) -> Void {
    if activate {
      this.GetRootWidget().SetVisible(true);
      let optionIntro: inkAnimOptions;
      this.PlayLibraryAnimation(n"Malfunction");
      optionIntro.executionDelay = 1.50;
      // this.PlaySound(n"MiniGame", n"AccessGranted");
      this.PlayLibraryAnimation(n"intro", optionIntro);
      this.PlayAnim(n"intro2", n"OnIntroComplete");
      optionIntro.executionDelay = 0.5;
      this.PlayLibraryAnimation(n"Malfunction_off", optionIntro);
      this.PlayAnim(n"Malfunction_timed", n"OnMalfunction");
      this.UpdateJohnnyThemeOverride(true);
    } else {
      this.PlayLibraryAnimation(n"outro");
      this.UpdateJohnnyThemeOverride(false);
    }
  }

  protected cb func OnUninitialize() -> Bool {
    TakeOverControlSystem.CreateInputHint(this.GetPlayerControlledObject().GetGame(), false);
    SecurityTurret.CreateInputHint(this.GetPlayerControlledObject().GetGame(), false);
  }

  protected cb func OnPlayerAttach(playerPuppet: ref<GameObject>) -> Bool {
    FlightLog.Info("[hudFlightController] OnPlayerAttach");
    this.m_bbPlayerStats = this.GetBlackboardSystem().Get(GetAllBlackboardDefs().UI_PlayerBioMonitor);
    this.m_bbPlayerEventId = this.m_bbPlayerStats.RegisterListenerVariant(GetAllBlackboardDefs().UI_PlayerBioMonitor.PlayerStatsInfo, this, n"OnStatsChanged");    
    this.m_playerObject = playerPuppet;
    this.m_playerPuppet = playerPuppet;
    this.m_gameInstance = this.GetPlayerControlledObject().GetGame();
    this.m_healthStatPoolListener = new FlightUIVehicleHealthStatPoolListener();
    this.m_healthStatPoolListener.m_owner = this;
    this.m_healthStatPoolListener.m_vehicle = FlightSystem.GetInstance().playerComponent.GetVehicle();
    GameInstance.GetStatPoolsSystem(this.m_gameInstance).RequestRegisteringListener(Cast(this.m_healthStatPoolListener.m_vehicle.GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    if this.IsUIactive() {
      this.ActivateUI(true);
    }
  }

  protected cb func OnPlayerDetach(playerPuppet: ref<GameObject>) -> Bool {
    GameInstance.GetStatPoolsSystem(this.m_gameInstance).RequestUnregisteringListener(Cast(this.m_healthStatPoolListener.m_vehicle.GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    if IsDefined(this.m_bbPlayerStats) {
      this.m_bbPlayerStats.UnregisterListenerVariant(GetAllBlackboardDefs().UI_PlayerBioMonitor.PlayerStatsInfo, this.m_bbPlayerEventId);
    };
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    let yaw: Float = ClampF(this.m_playerPuppet.GetWorldYaw(), -300.00, 300.00);
    inkTextRef.SetText(this.m_yawFluff, ToString(yaw));
    inkTextRef.SetText(this.m_pitchFluff, ToString(yaw * 1.50));
    inkWidgetRef.SetMargin(this.m_leftPart, new inkMargin(yaw, this.offsetLeft, 0.00, 0.00));
    inkWidgetRef.SetMargin(this.m_rightPart, new inkMargin(this.offsetRight, yaw, 0.00, 0.00));
  }

  protected cb func OnMalfunction(anim: ref<inkAnimProxy>) -> Bool {
    let optionIntro: inkAnimOptions;
    let optionMalfunction: inkAnimOptions;
    if GameInstance.GetQuestsSystem(this.m_gameInstance).GetFact(n"q104_turret_broken") == 1 && GameInstance.GetQuestsSystem(this.m_gameInstance).GetFact(n"q104_turret_fixed") == 0 {
      this.PlaySound(n"MiniGame", n"AccessDenied");
      inkTextRef.SetText(this.m_MessageText, "LocKey#11338");
      optionMalfunction.fromMarker = n"intro";
      optionMalfunction.toMarker = n"loop_start";
      this.PlayAnim(n"Malfunction", n"OnMalfunctionLoop", optionMalfunction);
      optionIntro.executionDelay = 28.00;
      this.PlayLibraryAnimation(n"Malfunction_off", optionIntro);
    };
  }

  protected cb func OnMalfunctionLoop(anim: ref<inkAnimProxy>) -> Bool {
    let optionMalfunctionLoop: inkAnimOptions;
    optionMalfunctionLoop.loopInfinite = false;
    optionMalfunctionLoop.loopType = inkanimLoopType.Cycle;
    optionMalfunctionLoop.loopCounter = 65u;
    optionMalfunctionLoop.fromMarker = n"loop_start";
    optionMalfunctionLoop.toMarker = n"loop_end";
    this.PlayAnim(n"Malfunction", n"OnMalfunctionLoopEnd", optionMalfunctionLoop);
  }

  protected cb func OnMalfunctionLoopEnd(anim: ref<inkAnimProxy>) -> Bool {
    let optionMalfunctionLoopEnd: inkAnimOptions;
    optionMalfunctionLoopEnd.fromMarker = n"loop_end";
    this.PlayAnim(n"Malfunction", n"", optionMalfunctionLoopEnd);
  }

  protected cb func OnIntroComplete(anim: ref<inkAnimProxy>) -> Bool {
    GameInstance.GetAudioSystem(this.GetPlayerControlledObject().GetGame()).Play(n"ui_main_menu_cc_loading");
  }

  protected cb func OnStatsChanged(value: Variant) -> Bool {
    // let incomingData: PlayerBioMonitor = FromVariant<PlayerBioMonitor>(value);
    // this.m_previousHealth = this.m_currentHealth;
    // this.m_maximumHealth = incomingData.maximumHealth;
       
   
    // this.m_currentHealth = CeilF(GameInstance.GetStatPoolsSystem(this.m_playerObject.GetGame()).GetStatPoolValue(Cast<StatsObjectID>(GetPlayer(this.m_playerObject.GetGame()).GetEntityID()), gamedataStatPoolType.Health, false));
    // this.m_currentHealth = Clamp(this.m_currentHealth, 0, this.m_maximumHealth);
  }

  public func ReactToHPChange(value: Float) -> Void {
    inkTextRef.SetText(this.healthStatus, IntToString(RoundF(value)) + "/100");
  }

  protected cb func OnDelayedHUDInitializeEvent(evt: ref<DelayedHUDInitializeEvent>) -> Bool {
    TakeOverControlSystem.CreateInputHint(this.GetPlayerControlledObject().GetGame(), true);
    SecurityTurret.CreateInputHint(this.GetPlayerControlledObject().GetGame(), true);
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
