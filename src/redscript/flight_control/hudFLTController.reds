import Codeware.UI.*

public class vflightUIGameController extends inkHUDGameController {

  private let m_vehicleBlackboard: wref<IBlackboard>;

  private let m_vehicle: wref<VehicleObject>;

  private let m_vehiclePS: ref<VehicleComponentPS>;

  private let m_vehicleBBStateConectionId: ref<CallbackHandle>;

  private let m_vehicleCollisionBBStateID: ref<CallbackHandle>;

  private let m_vehicleBBUIActivId: ref<CallbackHandle>;

  private let m_rootWidget: wref<inkWidget>;

  private let m_UIEnabled: Bool;

  private let m_startAnimProxy: ref<inkAnimProxy>;

  private let m_loopAnimProxy: ref<inkAnimProxy>;

  private let m_endAnimProxy: ref<inkAnimProxy>;

  private let m_loopingBootProxy: ref<inkAnimProxy>;

  private let m_speedometerWidget: inkWidgetRef;

  private let m_tachometerWidget: inkWidgetRef;

  private let m_timeWidget: inkWidgetRef;

  private let m_instruments: inkWidgetRef;

  private let m_gearBox: inkWidgetRef;

  private let m_radio: inkWidgetRef;

  private let m_analogTachWidget: inkWidgetRef;

  private let m_analogSpeedWidget: inkWidgetRef;

  private let m_isVehicleReady: Bool;

  private final func SetupModule(widget: inkWidgetRef, vehicle: wref<VehicleObject>, vehBB: wref<IBlackboard>) -> Void {
    FlightLog.Info("[vflightUIGameController] SetupModule");
    let moduleController: wref<IVehicleModuleController>;
    if !inkWidgetRef.IsValid(widget) {
      return;
    };
    moduleController = inkWidgetRef.GetController(widget) as IVehicleModuleController;
    if moduleController == null {
      return;
    };
    // moduleController.RegisterCallbacks(vehicle, vehBB, this);
  }

  private final func UnregisterModule(widget: inkWidgetRef) -> Void {
    let moduleController: wref<IVehicleModuleController>;
    if !inkWidgetRef.IsValid(widget) {
      return;
    };
    moduleController = inkWidgetRef.GetController(widget) as IVehicleModuleController;
    if moduleController == null {
      return;
    };
    moduleController.UnregisterCallbacks();
  }

  let m_info: ref<inkCanvas>;

  protected cb func OnInitialize() -> Bool {
    FlightLog.Info("[vflightUIGameController] OnInitialize");
    this.m_vehicle = this.GetOwnerEntity() as VehicleObject;
    this.m_vehiclePS = this.m_vehicle.GetVehiclePS();
    this.m_rootWidget = this.GetRootWidget();
    this.m_vehicleBlackboard = this.m_vehicle.GetBlackboard();
    if this.IsUIactive() {
      this.ActivateUI();
    };
    if IsDefined(this.m_vehicleBlackboard) {
      if !IsDefined(this.m_vehicleBBUIActivId) {
        this.m_vehicleBBUIActivId = this.m_vehicleBlackboard.RegisterListenerBool(GetAllBlackboardDefs().Vehicle.IsUIActive, this, n"OnActivateUI");
      };
    };

    this.m_info = inkWidgetBuilder.inkCanvas(n"info")
      .Size(600.0, 200.0)
      .Reparent(this.GetRootCompoundWidget())
		  .Anchor(inkEAnchor.CenterLeft)
      .Margin(400.0, 0.0, 0.0, 0.0)
      .BuildCanvas();

    let top = new inkHorizontalPanel();
		top.SetName(n"top");
		top.SetSize(500.0, 18.0);
    top.Reparent(this.m_info);
		// top.SetHAlign(inkEHorizontalAlign.Left);
		// top.SetVAlign(inkEVerticalAlign.Top);
    top.SetMargin(0.0, 0.0, 0.0, 0.0);

    let manufacturer = this.m_vehicle.GetRecord().Manufacturer().EnumName();
    let iconRecord = TweakDBInterface.GetUIIconRecord(TDBID.Create("UIIcon." + manufacturer));
    inkWidgetBuilder.inkImage(n"manufacturer")
      .Reparent(top)
      .Part(iconRecord.AtlasPartName())
      .Atlas(iconRecord.AtlasResourcePath())
		  .Size(174.0, 18.0)
      .Margin(20.0, 0.0, 0.0, 0.0)
		  .Opacity(1.0)
		  .Tint(ThemeColors.Bittersweet())
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .BuildImage();

    inkWidgetBuilder.inkImage(n"fluff")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"fluff_01")
      .Tint(ThemeColors.Bittersweet())
      .Opacity(1.0)
      .Margin(10.0, 0.0, 0.0, 0.0)
		  .Size(174.0, 18.0)
      .Anchor(inkEAnchor.LeftFillVerticaly)
      .Reparent(top)
      .BuildImage();

    inkWidgetBuilder.inkImage(n"fluff2")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"p20_sq_element")
      .Tint(ThemeColors.Bittersweet())
      .Opacity(1.0)
      .Margin(10.0, 0.0, 0.0, 0.0)
		  .Size(18.0, 18.0)
      .Anchor(inkEAnchor.LeftFillVerticaly)
      .Reparent(top)
      .BuildImage();

    let panel = inkWidgetBuilder.inkFlex(n"panel")
      .Margin(0.0, 24.0, 0.0, 0.0)
      .Padding(10.0, 10.0, 10.0, 10.0)
		  .FitToContent(true)
      .Reparent(this.m_info)
		  .Anchor(inkEAnchor.Fill)
      .BuildFlex();

    inkWidgetBuilder.inkImage(n"frame")
      .Reparent(panel)
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
		  .Part(n"tooltip_side_gap_flip")
		  .NineSliceScale(true)
      .Margin(0.0, 0.0, 0.0, 20.0)
		  .Tint(ThemeColors.ElectricBlue())
		  .Opacity(1.0)
      .BuildImage();
    //  .BindProperty(n"tintColor", n"Briefings.BackgroundColour");
      
   inkWidgetBuilder.inkImage(n"background")
      .Reparent(panel)
      .Anchor(inkEAnchor.Fill)
      .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
      .Part(n"frame_small_bg")
      .NineSliceScale(true)
      .Margin(20.0, 5.0, 10.0, 30.0)
      .Tint(ThemeColors.PureBlack())
      .Opacity(1.0)
      .BuildImage();
    
    inkWidgetBuilder.inkText(n"fluff_text1")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(ThemeColors.ElectricBlue())
      .Text("[  0.0000@0]  VDECO(LOW)   :0x09A00000 - 0x19A00000 (256MiB)")
      .Margin(30.0, 15.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkText(n"fluff_text2")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(ThemeColors.ElectricBlue())
      .Text("[  0.0000@0]  PPMGRO(HIGH) :0x07A00000 - 0x07000000 (46MiB)")
      .Margin(30.0, 35.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkText(n"fluff_text3")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(ThemeColors.ElectricBlue())
      .Text("[  0.0000@0]  VDIN10(LOW)  :0x19A00000 - 0x1AA00000 (16MiB)")
      .Margin(30.0, 55.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkImage(n"position_fluff")
      .Reparent(this.m_info)
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"fluff_06_L")
      .Tint(ThemeColors.ElectricBlue())
      .Opacity(1.0)
      .Margin(30.0, 100.0, 0.0, 0.0)
		  .Size(23.0, 24.0)
      .BuildImage();

    inkWidgetBuilder.inkText(n"position")
      .Reparent(this.m_info)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(18)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(ThemeColors.ElectricBlue())
      .Text(" 1550.52,   850.68,    87.34")
      .Margin(60.0, 104.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();


  }

  protected cb func OnUninitialize() -> Bool {
    if IsDefined(this.m_vehicleBBUIActivId) {
      this.m_vehicleBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().Vehicle.IsUIActive, this.m_vehicleBBUIActivId);
    };
    this.UnregisterBlackBoardCallbacks();
  }

  private final func ActivateUI() -> Void {
    this.m_rootWidget.SetVisible(true);
    this.RegisterBlackBoardCallbacks();
    this.CheckIfVehicleShouldTurnOn();
  }

  private final func DeactivateUI() -> Void {
    this.UnregisterBlackBoardCallbacks();
    this.m_rootWidget.SetVisible(false);
  }

  protected cb func OnActivateUI(activate: Bool) -> Bool {
    let evt: ref<VehicleUIactivateEvent> = new VehicleUIactivateEvent();
    if activate {
      evt.m_activate = true;
    } else {
      evt.m_activate = false;
    };
    this.QueueEvent(evt);
  }

  protected cb func OnActivateUIEvent(evt: ref<VehicleUIactivateEvent>) -> Bool {
    if evt.m_activate {
      this.ActivateUI();
    } else {
      this.DeactivateUI();
    };
  }

  protected cb func OnVehicleReady(ready: Bool) -> Bool {
    if ready {
      this.m_rootWidget.SetVisible(true);
    } else {
      if !ready {
        this.m_rootWidget.SetVisible(false);
      };
    };
    this.m_isVehicleReady = ready;
  }

  private final func RegisterBlackBoardCallbacks() -> Void {
    if IsDefined(this.m_vehicleBlackboard) {
      // this.SetupModule(this.m_speedometerWidget, this.m_vehicle, this.m_vehicleBlackboard);
      // this.SetupModule(this.m_tachometerWidget, this.m_vehicle, this.m_vehicleBlackboard);
      // this.SetupModule(this.m_timeWidget, this.m_vehicle, this.m_vehicleBlackboard);
      // this.SetupModule(this.m_instruments, this.m_vehicle, this.m_vehicleBlackboard);
      // this.SetupModule(this.m_gearBox, this.m_vehicle, this.m_vehicleBlackboard);
      // this.SetupModule(this.m_radio, this.m_vehicle, this.m_vehicleBlackboard);
      // this.SetupModule(this.m_analogTachWidget, this.m_vehicle, this.m_vehicleBlackboard);
      // this.SetupModule(this.m_analogSpeedWidget, this.m_vehicle, this.m_vehicleBlackboard);
      if !IsDefined(this.m_vehicleBBStateConectionId) {
        this.m_vehicleBBStateConectionId = this.m_vehicleBlackboard.RegisterListenerInt(GetAllBlackboardDefs().Vehicle.VehicleState, this, n"OnVehicleStateChanged");
      };
      if !IsDefined(this.m_vehicleCollisionBBStateID) {
        this.m_vehicleCollisionBBStateID = this.m_vehicleBlackboard.RegisterListenerBool(GetAllBlackboardDefs().Vehicle.Collision, this, n"OnVehicleCollision");
      };
      this.InitializeWidgetStyleSheet(this.m_vehicle);
    };
  }

  private final func UnregisterBlackBoardCallbacks() -> Void {
    if IsDefined(this.m_vehicleBlackboard) {
      // this.UnregisterModule(this.m_speedometerWidget);
      // this.UnregisterModule(this.m_tachometerWidget);
      // this.UnregisterModule(this.m_timeWidget);
      // this.UnregisterModule(this.m_instruments);
      // this.UnregisterModule(this.m_gearBox);
      // this.UnregisterModule(this.m_radio);
      // this.UnregisterModule(this.m_analogTachWidget);
      // this.UnregisterModule(this.m_analogSpeedWidget);
      if IsDefined(this.m_vehicleBBStateConectionId) {
        this.m_vehicleBlackboard.UnregisterListenerInt(GetAllBlackboardDefs().Vehicle.VehicleState, this.m_vehicleBBStateConectionId);
      };
      if IsDefined(this.m_vehicleCollisionBBStateID) {
        this.m_vehicleBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().Vehicle.Collision, this.m_vehicleCollisionBBStateID);
      };
    };
  }

  private final func IsUIactive() -> Bool {
    if IsDefined(this.m_vehicleBlackboard) && this.m_vehicleBlackboard.GetBool(GetAllBlackboardDefs().Vehicle.IsUIActive) {
      return true;
    };
    return false;
  }

  private final func InitializeWidgetStyleSheet(veh: wref<VehicleObject>) -> Void {
    let record: wref<Vehicle_Record> = veh.GetRecord();
    let styleSheetPath: ResRef = record.WidgetStyleSheetPath();
    this.m_rootWidget.SetStyle(styleSheetPath);
  }

  private final func CheckIfVehicleShouldTurnOn() -> Void {
    if this.m_vehiclePS.GetIsUiQuestModified() {
      if this.m_vehiclePS.GetUiQuestState() {
        this.TurnOn();
      };
      return;
    };
    if this.m_vehicleBlackboard.GetInt(GetAllBlackboardDefs().Vehicle.VehicleState) == EnumInt(vehicleEState.On) {
      this.TurnOn();
    };
  }

  protected cb func OnVehicleStateChanged(state: Int32) -> Bool {
    if this.m_vehiclePS.GetIsUiQuestModified() {
      return false;
    };
    if state == EnumInt(vehicleEState.On) {
      this.TurnOn();
    };
    if state == EnumInt(vehicleEState.Default) {
      this.TurnOff();
    };
    if state == EnumInt(vehicleEState.Disabled) {
      this.TurnOff();
    };
    if state == EnumInt(vehicleEState.Destroyed) {
      this.TurnOff();
    };
  }

  private final func TurnOn() -> Void {
    this.KillBootupProxy();
    if this.m_UIEnabled {
      this.PlayIdleLoop();
    } else {
      this.m_UIEnabled = true;
      this.m_startAnimProxy = this.PlayLibraryAnimation(n"start");
      this.m_startAnimProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnStartAnimFinished");
      this.EvaluateWidgetStyle(GameInstance.GetTimeSystem(this.m_vehicle.GetGame()).GetGameTime());
    };
  }

  private final func TurnOff() -> Void {
    this.m_UIEnabled = false;
    this.KillBootupProxy();
    if IsDefined(this.m_startAnimProxy) {
      this.m_startAnimProxy.Stop();
    };
    if IsDefined(this.m_loopAnimProxy) {
      this.m_loopAnimProxy.Stop();
    };
    this.m_endAnimProxy = this.PlayLibraryAnimation(n"end");
    this.m_endAnimProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnEndAnimFinished");
  }

  protected cb func OnStartAnimFinished(anim: ref<inkAnimProxy>) -> Bool {
    this.PlayIdleLoop();
  }

  private final func PlayIdleLoop() -> Void {
    let animOptions: inkAnimOptions;
    animOptions.loopType = inkanimLoopType.Cycle;
    animOptions.loopInfinite = true;
    this.m_loopAnimProxy = this.PlayLibraryAnimation(n"loop", animOptions);
  }

  protected cb func OnEndAnimFinished(anim: ref<inkAnimProxy>) -> Bool {
    this.m_rootWidget.SetState(n"inactive");
  }

  private final func PlayLibraryAnim(animName: CName) -> Void {
    this.PlayLibraryAnimation(animName);
  }

  public final func EvaluateWidgetStyle(time: GameTime) -> Void {
    let currTime: GameTime;
    let sunRise: GameTime;
    let sunSet: GameTime;
    if this.m_UIEnabled {
      sunSet = GameTime.MakeGameTime(0, 20, 0, 0);
      sunRise = GameTime.MakeGameTime(0, 5, 0, 0);
      currTime = GameTime.MakeGameTime(0, GameTime.Hours(time), GameTime.Minutes(time), GameTime.Seconds(time));
      if currTime <= sunSet && currTime >= sunRise {
        if NotEquals(this.m_rootWidget.GetState(), n"day") {
          this.m_rootWidget.SetState(n"day");
        };
      } else {
        if NotEquals(this.m_rootWidget.GetState(), n"night") {
          this.m_rootWidget.SetState(n"night");
        };
      };
    };
  }

  protected cb func OnVehicleCollision(collision: Bool) -> Bool {
    this.PlayLibraryAnimation(n"glitch");
  }

  protected cb func OnForwardVehicleQuestEnableUIEvent(evt: ref<ForwardVehicleQuestEnableUIEvent>) -> Bool {
    switch evt.mode {
      case vehicleQuestUIEnable.Gameplay:
        this.CheckIfVehicleShouldTurnOn();
        break;
      case vehicleQuestUIEnable.ForceEnable:
        this.TurnOn();
        break;
      case vehicleQuestUIEnable.ForceDisable:
        this.TurnOff();
    };
  }

  protected cb func OnVehiclePanzerBootupUIQuestEvent(evt: ref<VehiclePanzerBootupUIQuestEvent>) -> Bool {
    let animOptions: inkAnimOptions;
    this.m_UIEnabled = true;
    this.m_rootWidget.SetVisible(true);
    animOptions.loopType = inkanimLoopType.Cycle;
    animOptions.loopInfinite = true;
    switch evt.mode {
      case panzerBootupUI.UnbootedIdle:
        this.KillBootupProxy();
        this.m_loopingBootProxy = this.PlayLibraryAnimation(n"1_unbooted_idle", animOptions);
        break;
      case panzerBootupUI.BootingAttempt:
        this.KillBootupProxy();
        this.m_loopingBootProxy = this.PlayLibraryAnimation(n"2_booting_attempt", animOptions);
        break;
      case panzerBootupUI.BootingSuccess:
        this.KillBootupProxy();
        this.m_loopingBootProxy = this.PlayLibraryAnimation(n"3_booting_success");
        break;
      case panzerBootupUI.Loop:
        this.KillBootupProxy();
        this.m_loopingBootProxy = this.PlayLibraryAnimation(n"loop", animOptions);
    };
  }

  private final func KillBootupProxy() -> Void {
    if IsDefined(this.m_loopingBootProxy) {
      this.m_loopingBootProxy.Stop();
    };
  }

  protected cb func OnForwardVehicleQuestUIEffectEvent(evt: ref<ForwardVehicleQuestUIEffectEvent>) -> Bool {
    if evt.glitch {
      this.PlayLibraryAnimation(n"glitch");
    };
    if evt.panamVehicleStartup {
      this.PlayLibraryAnimation(n"start_panam");
    };
    if evt.panamScreenType1 {
      this.PlayLibraryAnimation(n"panam_screen_type1");
    };
    if evt.panamScreenType2 {
      this.PlayLibraryAnimation(n"panam_screen_type2");
    };
    if evt.panamScreenType3 {
      this.PlayLibraryAnimation(n"panam_screen_type3");
    };
    if evt.panamScreenType4 {
      this.PlayLibraryAnimation(n"panam_screen_type4");
    };
  }
}
