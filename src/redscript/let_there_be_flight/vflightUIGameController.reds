public class vflightUIGameController extends inkHUDGameController {
  private let m_vehicleBlackboard: wref<IBlackboard>;
  private let m_vehicleFlightBlackboard: wref<IBlackboard>;
  private let m_vehicle: wref<VehicleObject>;
  private let m_vehiclePS: ref<VehicleComponentPS>;
  private let m_vehicleBBStateConectionId: ref<CallbackHandle>;
  private let m_vehicleCollisionBBStateID: ref<CallbackHandle>;
  private let m_vehiclePitchID: ref<CallbackHandle>;
  private let m_vehiclePositionID: ref<CallbackHandle>;
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

  protected cb func OnInitialize() -> Bool {
    FlightLog.Info("[vflightUIGameController] OnInitialize");
    this.m_vehicle = this.GetOwnerEntity() as VehicleObject;
    this.m_vehiclePS = this.m_vehicle.GetVehiclePS();
    this.m_rootWidget = this.GetRootWidget();
    this.m_vehicleBlackboard = this.m_vehicle.GetBlackboard();
    this.m_vehicleFlightBlackboard = FlightController.GetInstance().GetBlackboard();
    if this.IsUIactive() {
      this.ActivateUI();
    };
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if !IsDefined(this.m_vehicleBBUIActivId) {
        this.m_vehicleBBUIActivId = this.m_vehicleFlightBlackboard.RegisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this, n"OnActivateUI");
      };
    };
    
    this.SetupInfoPanel();
    this.SetupPitchDisplay();
  }

  private let m_info: ref<inkCanvas>;
  private let m_position: ref<inkText>;

  protected cb func OnVehiclePositionChanged(position: Vector4) -> Bool {
    this.m_position.SetText(Vector4.ToStringPrec(position, 2));
  }

  private func SetupInfoPanel() {
    this.m_info = inkWidgetBuilder.inkCanvas(n"info")
      .Size(624.0, 200.0)
      .Reparent(this.GetRootCompoundWidget())
		  .Anchor(inkEAnchor.TopLeft)
      .Margin(380.0, 20.0, 0.0, 0.0)
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
		  .Tint(FlightUtils.Bittersweet())
      .Tint(n"Default.accent_color1")
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .BuildImage();

    inkWidgetBuilder.inkImage(n"fluff")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"fluff_01")
      .Tint(FlightUtils.Bittersweet())
      .Tint(n"Default.accent_color1")
      .Opacity(1.0)
      .Margin(10.0, 0.0, 0.0, 0.0)
		  .Size(174.0, 18.0)
      .Anchor(inkEAnchor.LeftFillVerticaly)
      .Reparent(top)
      .BuildImage();

    inkWidgetBuilder.inkImage(n"fluff2")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"p20_sq_element")
      .Tint(FlightUtils.Bittersweet())
      .Tint(n"Default.accent_color1")
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
		  .Tint(FlightUtils.ElectricBlue())
      .Tint(n"Default.main")
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
      .Tint(FlightUtils.PureBlack())
      .Opacity(1.0)
      .BuildImage();
    
    inkWidgetBuilder.inkText(n"fluff_text1")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Text("[  0.0000@0]  VDECO(LOW)   :0x09A00000 - 0x19A00000 (256MiB)")
      .Margin(30.0, 15.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkText(n"fluff_text2")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Text("[  0.0000@0]  PPMGRO(HIGH) :0x07A00000 - 0x07000000 (46MiB)")
      .Margin(30.0, 35.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkText(n"fluff_text3")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(14)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Text("[  0.0000@0]  VDIN10(LOW)  :0x19A00000 - 0x1AA00000 (16MiB)")
      .Margin(30.0, 55.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    inkWidgetBuilder.inkImage(n"position_fluff")
      .Reparent(this.m_info)
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"fluff_06_L")
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Opacity(1.0)
      .Margin(30.0, 100.0, 0.0, 0.0)
		  .Size(23.0, 24.0)
      .BuildImage();

    this.m_position = inkWidgetBuilder.inkText(n"position")
      .Reparent(this.m_info)
      .Font("base\\gameplay\\gui\\fonts\\arame\\arame.inkfontfamily")
      .FontSize(18)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
      .Text(" 1550.52,   850.68,    87.34")
      .Margin(60.0, 104.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();
    this.m_position;

  }

  private let m_pitch: ref<inkCanvas>;
  private let m_pitchMarkers: ref<inkCanvas>;
  private let m_lastPitchValue: Float;
  
  protected cb func OnVehiclePitchChanged(pitch: Float) -> Bool {
    this.m_pitchMarkers.SetTranslation(0.0, (pitch - 90.0) * 20.0);
    this.m_pitchMarkers.SetEffectParamValue(inkEffectType.BoxBlur, n"BoxBlur_0", n"intensity", AbsF(pitch - this.m_lastPitchValue) * 0.001);
    this.m_lastPitchValue = pitch;
  }

  private func SetupPitchDisplay() -> Void {

      let mark_scale = 20.0;
      let height = 520.0;
      let width = 60.0;

      this.m_pitch = inkWidgetBuilder.inkCanvas(n"m_pitch")
        .Size(width, height)
        .Reparent(this.GetRootCompoundWidget())
        .Anchor(inkEAnchor.TopLeft)
        .Anchor(0.5, 0.5)
        .Margin(0.0, 0.0, 0.0, 0.0)
        .Translation(314.0, 320.0)
        // .Opacity(0.5)
        .BuildCanvas();
      // this.m_pitch.SetChildOrder(inkEChildOrder.Backward);

      inkWidgetBuilder.inkImage(n"arrow")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_right_bg")
        .Size(20.0, 20.0)
        .Anchor(1.0, 0.5)
        .Anchor(inkEAnchor.CenterLeft)
        .Margin(-15.0, 0.0, 0.0, 0.0)
        .Opacity(1.0)
        .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
        .BuildImage();
        
      inkWidgetBuilder.inkText(n"fluff_text")
        .Reparent(this.m_pitch)
        .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
        .FontSize(12)
        .Anchor(0.0, 1.0)
        .Anchor(inkEAnchor.TopLeft)
        .Tint(FlightUtils.Bittersweet())
        .Tint(n"Default.accent_color1")
        .Text("89V_PITCH")
        .HAlign(inkEHorizontalAlign.Left)
        .Margin(-14.0, -10.0, 0.0, 0.0)
        // .Overflow(textOverflowPolicy.AdjustToSize) pArrayType was nullptr.
        .BuildText();

      inkWidgetBuilder.inkImage(n"border")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_cell_fg")
        .Size(width + 24.0, height)
        .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.CenterFillVerticaly)
        .Translation(-2.5, 0.0)
        .Opacity(1.0)
        .Tint(FlightUtils.ElectricBlue())
        .Tint(n"Default.main")
        .BuildImage();

      inkWidgetBuilder.inkImage(n"fill")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_cell_bg")
        .Size(width + 24.0, height)
        .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.CenterFillVerticaly)
        .Translation(-2.5, 0.0)
        .Opacity(0.1)
        .Tint(FlightUtils.PureBlack())
        .BuildImage();

      inkWidgetBuilder.inkMask(n"mask")
        .Reparent(this.m_pitch)
        .Atlas(r"base\\gameplay\\gui\\quests\\q000\\aerondight_rayfield\\assets\\logo_and_mask.inkatlas")
        .Part(n"Maska_textura")
        // .Size(width, height)
        // .Opacity(1.0)     
        .FitToContent(true)
        // .MaskTransparency(1.0)
        // .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        // .InvertMask(true)
        .Anchor(inkEAnchor.Centered)
        // .MaskSource(inkMaskDataSource.TextureAtlas)
        .BuildMask();
      // mask.SetDynamicTextureMask(n"fill");
      // mask.SetEffectEnabled(inkEffectType.Mask, n"Mask_0", true);
      // mask.SetEffectParamValue(inkEffectType.Mask, n"Mask_0", n"opacity", 1.0);

      // FlightLog.Info("Does arrow_cell_bg exist on the mask atlas? " + ToString(mask.IsTexturePartExist(n"arrow_cell_bg")));

     this.m_pitchMarkers = inkWidgetBuilder.inkCanvas(n"markers")
        .Size(width, 180.0 * mark_scale)
        .Reparent(this.m_pitch)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.Centered)
        .Margin(0.0, 0.0, 0.0, 0.0)
        .BuildCanvas();
      this.m_pitchMarkers.CreateEffect(n"inkBoxBlurEffect", n"BoxBlur_0");
      this.m_pitchMarkers.SetEffectEnabled(inkEffectType.BoxBlur, n"BoxBlur_0", true);
      this.m_pitchMarkers.SetEffectParamValue(inkEffectType.BoxBlur, n"BoxBlur_0", n"intensity", 0.0);
      this.m_pitchMarkers.SetBlurDimension(n"BoxBlur_0", inkEBlurDimension.Vertical);
      // this.m_pitchMarkers.CreateEffect(n"inkMaskEffect", n"Mask_0");
      // this.m_pitchMarkers.SetEffectEnabled(inkEffectType.Mask, n"Mask_0", true);
		  // markers.SetRenderTransformPivot(new Vector2(0.0, 0.0));

      let midbar_size = 16.0;
      let marks: array<Float> = [-100.0, -90.0, -80.0, -70.0, -60.0, -50.0, -40.0, -30.0, -20.0, -10.0, 0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0];
      let marks_inc: array<Float> = [-4.0, -3.0, -2.0, -1.0, 1.0, 2.0, 3.0, 4.0, 5.0];

      inkWidgetBuilder.inkRectangle(n"m1_00000")
        .Tint(FlightUtils.Bittersweet())
        .Tint(n"Default.accent_color1")
        // .Opacity(mark == 0.0 ? 1.0 : 0.5)
        .Opacity(1.0)
        .Reparent(this.m_pitchMarkers)
        .Size(width, 19.0)
        .Anchor(0.0, 0.0)
        .Translation(0.0, 90.0 * mark_scale - 20.0)
        .BuildRectangle();

      inkWidgetBuilder.inkRectangle(n"m1_00001")
        .Tint(FlightUtils.Bittersweet())
        .Tint(n"Default.accent_color1")
        // .Opacity(mark == 0.0 ? 1.0 : 0.5)
        .Opacity(1.0)
        .Reparent(this.m_pitchMarkers)
        .Size(width, 19.0)
        .Anchor(0.0, 1.0)
        .Translation(0.0, 90.0 * mark_scale + 20.0)
        .BuildRectangle();

      // let text = inkWidgetBuilder.inkText(n"text")
      //   .Reparent(this.m_pitchMarkers)
      //   .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily", n"Heavy")
      //   .FontSize(16)
      //   .Anchor(0.5, 0.5)
      //   .Tint(FlightUtils.PureBlack())
      //   .Text("LEVEL")
      //   .Opacity(1.0)
      //   .HAlign(inkEHorizontalAlign.Center)
      //   .VAlign(inkEVerticalAlign.Center)
      //   .Margin(0.0, 0.0, 0.0, 0.0)
      //   .Translation(width / 2.0, 90.0 * mark_scale)
      //   // .Overflow(textOverflowPolicy.AdjustToSize)
      //   .BuildText();

      for mark in marks {
        if mark != 0.0 {
          inkWidgetBuilder.inkText(n"text")
            .Reparent(this.m_pitchMarkers)
            .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
            .FontSize(20)
            .Anchor(0.5, 0.5)
            .Tint(FlightUtils.ElectricBlue())
            .Tint(n"Default.main")
            .Text(FloatToStringPrec(AbsF(mark), 0))
            .HAlign(inkEHorizontalAlign.Center)
            .Margin(0.0, 0.0, 0.0, 0.0)
            .Translation(width / 2.0, (mark + 90.0) * mark_scale)
            // .Overflow(textOverflowPolicy.AdjustToSize)
            .BuildText();

          inkWidgetBuilder.inkRectangle(StringToName("m1_" + FloatToString(mark)))
            .Tint(FlightUtils.ElectricBlue())
            .Tint(n"Default.main")
            // .Opacity(mark == 0.0 ? 1.0 : 0.5)
            .Size(midbar_size, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(width - midbar_size, (mark + 90.0) * mark_scale)
            .Reparent(this.m_pitchMarkers)
            .BuildRectangle();

          inkWidgetBuilder.inkRectangle(StringToName("m2_" + FloatToString(mark)))
            .Tint(FlightUtils.ElectricBlue())
            .Tint(n"Default.main")
            // .Opacity(mark == 0.0 ? 1.0 : 0.5)
            .Size(midbar_size, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(0.0, (mark + 90.0) * mark_scale)
            .Reparent(this.m_pitchMarkers)
            .BuildRectangle();
        }
        for mark_inc in marks_inc {
          inkWidgetBuilder.inkRectangle(StringToName("m_" + FloatToString(mark + mark_inc)))
            .Tint(FlightUtils.ElectricBlue())
            .Tint(n"Default.main")
            .Opacity(mark_inc == 5.0 ? 0.5 : 0.1)
            .Size(width, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(0.0, ((mark + 90.0) + mark_inc) * mark_scale)
            .Reparent(this.m_pitchMarkers)
            .BuildRectangle();
        }
      } 
  }

  protected cb func OnUninitialize() -> Bool {
    if IsDefined(this.m_vehicleBBUIActivId) {
      this.m_vehicleFlightBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive, this.m_vehicleBBUIActivId);
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
    let evt: ref<VehicleFlightUIActivationEvent> = new VehicleFlightUIActivationEvent();
    if activate {
      evt.m_activate = true;
    } else {
      evt.m_activate = false;
    };
    this.QueueEvent(evt);
  }

  protected cb func OnActivateUIEvent(evt: ref<VehicleFlightUIActivationEvent>) -> Bool {
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
      if !IsDefined(this.m_vehicleBBStateConectionId) {
        this.m_vehicleBBStateConectionId = this.m_vehicleBlackboard.RegisterListenerInt(GetAllBlackboardDefs().Vehicle.VehicleState, this, n"OnVehicleStateChanged");
      };
      if !IsDefined(this.m_vehicleCollisionBBStateID) {
        this.m_vehicleCollisionBBStateID = this.m_vehicleBlackboard.RegisterListenerBool(GetAllBlackboardDefs().Vehicle.Collision, this, n"OnVehicleCollision");
      };
    }
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if !IsDefined(this.m_vehiclePitchID) {
        this.m_vehiclePitchID = this.m_vehicleFlightBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this, n"OnVehiclePitchChanged");
      };
      if !IsDefined(this.m_vehiclePositionID) {
        this.m_vehiclePositionID = this.m_vehicleFlightBlackboard.RegisterListenerVector4(GetAllBlackboardDefs().VehicleFlight.Position, this, n"OnVehiclePositionChanged");
      };
      this.InitializeWidgetStyleSheet(this.m_vehicle);
    };
  }

  private final func UnregisterBlackBoardCallbacks() -> Void {
    if IsDefined(this.m_vehicleBlackboard) {
      if IsDefined(this.m_vehicleBBStateConectionId) {
        this.m_vehicleBlackboard.UnregisterListenerInt(GetAllBlackboardDefs().Vehicle.VehicleState, this.m_vehicleBBStateConectionId);
      };
      if IsDefined(this.m_vehicleCollisionBBStateID) {
        this.m_vehicleBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().Vehicle.Collision, this.m_vehicleCollisionBBStateID);
      };
    }
    if IsDefined(this.m_vehicleFlightBlackboard) {
      if IsDefined(this.m_vehiclePitchID) {
        this.m_vehicleFlightBlackboard.UnregisterListenerFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this.m_vehiclePitchID);
      };
      if IsDefined(this.m_vehiclePositionID) {
        this.m_vehicleFlightBlackboard.UnregisterListenerVector4(GetAllBlackboardDefs().VehicleFlight.Position, this.m_vehiclePositionID);
      };
    };
  }

  private final func IsUIactive() -> Bool {
    if IsDefined(this.m_vehicleFlightBlackboard) && this.m_vehicleFlightBlackboard.GetBool(GetAllBlackboardDefs().VehicleFlight.IsUIActive) {
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
