import BaseLib.UI.*

public class FlightControllerUI extends inkCanvas {
  public let controller: ref<inkGameController>;
  public let stats: ref<FlightStats>;
  private let m_rootAnim: ref<inkAnimProxy>;
  private let m_markerRadius: Float;
  public static func Create(controller: ref<inkGameController>, parent: ref<inkCompoundWidget>) -> ref<FlightControllerUI> {
    let instance = new FlightControllerUI();
    instance.controller = controller;
    instance.Reparent(parent);
    instance.SetName(n"flightControllerUI");
    FlightController.GetInstance().SetUI(instance);
    return instance;
  }
  public final const func GetVehicle() -> wref<VehicleObject> {
    if !Equals(this.stats, null) {
      return this.stats.vehicle;
    } else {
      return null;
    }
  }
  public func Setup(stats: ref<FlightStats>) -> Void {
    this.stats = stats;
    this.SetOpacity(0.0);
    this.RemoveAllChildren();
		this.SetAnchor(inkEAnchor.Centered);
    
    // let w: ref<inkWidget> = this.controller.SpawnFromExternal(this, r"base\\gameplay\\gui\\widgets\\turret_hud\\turret_hud.inkwidget", n"Root");
    // w.Reparent(this);
    // return;


    let arrow_left = inkWidgetBuilder.inkImage(n"arrow_left")
		  .Reparent(this)
		  .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		  .Part(n"arrow_right")
		  .Size(24.0, 24.0)
      .Anchor(1.0, 0.5)
		  .Opacity(1.0)
		  .Tint(ThemeColors.ElectricBlue())
      .BuildImage();

    let arrow_right = inkWidgetBuilder.inkImage(n"arrow_right")
		  .Reparent(this)
		  .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		  .Part(n"arrow_right")
		  .Size(24.0, 24.0)
      .Anchor(1.0, 0.5)
		  .Opacity(1.0)
		  .Tint(ThemeColors.ElectricBlue())
      .BuildImage();

    let arrow_leftr = inkWidgetBuilder.inkImage(n"arrow_left_rear")
		  .Reparent(this)
		  .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		  .Part(n"arrow_right")
		  .Size(24.0, 24.0)
      .Anchor(1.0, 0.5)
		  .Opacity(1.0)
		  .Tint(ThemeColors.ElectricBlue())
      .BuildImage();

    let arrow_rightr = inkWidgetBuilder.inkImage(n"arrow_right_rear")
		  .Reparent(this)
		  .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		  .Part(n"arrow_right")
		  .Size(24.0, 24.0)
      .Anchor(1.0, 0.5)
		  .Opacity(1.0)
		  .Tint(ThemeColors.ElectricBlue())
      .BuildImage();

    let rulers = inkWidgetBuilder.inkCanvas(n"rulers")
      .Reparent(this)
	    .Anchor(inkEAnchor.Centered)
      .Margin(0.0, 0.0, 0.0, 0.0)
      .BuildCanvas();
  
    let marks = [-25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 155, 160, 165, 170, 175, 180, 185, 190, 195, 200, 205];
    this.m_markerRadius = 500.0;

    for mark in marks {
      let roll_marker = inkWidgetBuilder.inkRectangle(StringToName("roll_marker_" + mark))
        .Tint(ThemeColors.ElectricBlue())
        .Opacity(0.05)
        .Size(3.0, (mark == 0 || mark == 180) ? 50.0 : 30.0)
        .Anchor(0.5, 0.0)
        .Translation(CosF(Deg2Rad(mark + 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark + 90.0)) * this.m_markerRadius)
        .Rotation(mark)
        .Reparent(rulers)
        .BuildRectangle();
    }

    let roll_text = inkWidgetBuilder.inkText(n"roll_text")
      .Reparent(rulers)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .Tint(ThemeColors.ElectricBlue())
      .Text("0.00")
		  .Opacity(0.5)
      .Anchor(0.5, 0.0)
      .BuildText();

    let roll_marker_left = inkWidgetBuilder.inkRectangle(n"roll_marker_left")
      .Tint(ThemeColors.ElectricBlue())
      .Opacity(0.5)
      .Size(3.0, 70.0)
      .Anchor(0.5, 0.0)
      .Reparent(rulers)
      .BuildRectangle();

    let roll_marker_right = inkWidgetBuilder.inkRectangle(n"roll_marker_right")
      .Tint(ThemeColors.ElectricBlue())
      .Opacity(0.5)
      .Size(3.0, 70.0)
      .Anchor(0.5, 0.0)
      .Reparent(rulers)
      .BuildRectangle();

    let info = inkWidgetBuilder.inkCanvas(n"info")
      .Size(500.0, 200.0)
      .Reparent(this)
	    .Anchor(inkEAnchor.CenterLeft)
      .Margin(0.0, 0.0, 0.0, 0.0)
      .BuildCanvas();
    info.SetEffectEnabled(inkEffectType.Glitch, n"Glitch_0", true);
    info.SetEffectParamValue(inkEffectType.Glitch, n"Glitch_0", n"intensity", 1.0);
		// altitude.SetAnchorPoint(new Vector2(0.5, 0.5));

    let top = new inkHorizontalPanel();
		top.SetName(n"top");
		top.SetSize(500.0, 18.0);
    top.Reparent(info);
		// top.SetHAlign(inkEHorizontalAlign.Left);
		// top.SetVAlign(inkEVerticalAlign.Top);
    top.SetMargin(0.0, 0.0, 0.0, 0.0);

    let manufacturer = this.stats.vehicle.GetRecord().Manufacturer().EnumName();
    let iconRecord = TweakDBInterface.GetUIIconRecord(TDBID.Create("UIIcon." + manufacturer));
    let vehicle_manufacturer = inkWidgetBuilder.inkImage(n"manufacturer")
      .Reparent(top)
      .Part(iconRecord.AtlasPartName())
      .Atlas(iconRecord.AtlasResourcePath())
		  .Size(174.0, 18.0)
      .Margin(20.0, 0.0, 0.0, 0.0)
		  .Opacity(1.0)
		  .Tint(ThemeColors.Bittersweet())
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .BuildImage();

    let fluff = inkWidgetBuilder.inkImage(n"fluff")
      .Atlas(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas")
      .Part(n"fluff_01")
      .Tint(ThemeColors.Bittersweet())
      .Opacity(1.0)
      .Margin(10.0, 0.0, 0.0, 0.0)
		  .Size(174.0, 18.0)
      .Anchor(inkEAnchor.LeftFillVerticaly)
      .Reparent(top)
      .BuildImage();

    let fluff2 = inkWidgetBuilder.inkImage(n"fluff2")
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
      .Reparent(info)
		  .Anchor(inkEAnchor.Fill)
      .BuildFlex();
    
    let altitude_background = inkWidgetBuilder.inkImage(n"background")
      .Reparent(panel)
      .Anchor(inkEAnchor.Fill)
      .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
      .Part(n"frame_top_bg")
      .NineSliceScale(true)
      .Margin(0.0, 0.0, 0.0, 0.0)
      .Tint(ThemeColors.ElectricBlue())
      .Opacity(0.005)
      .BuildImage();
    // .BindProperty(n"tintColor", n"Briefings.BackgroundColour");

    let altitude_frame = inkWidgetBuilder.inkImage(n"frame")
      .Reparent(panel)
		  .Anchor(inkEAnchor.LeftFillVerticaly)
      .Atlas(r"base\\gameplay\\gui\\widgets\\crosshair\\smart_rifle\\arghtlas_smargun.inkatlas")
		  .Part(n"smartFrameTopLeft")
		  .NineSliceScale(true)
      .Margin(new inkMargin(0.0, 0.0, 0.0, 0.0))
		  .Tint(ThemeColors.ElectricBlue())
		  .Opacity(1.0)
      .BuildImage();
    //  .BindProperty(n"tintColor", n"Briefings.BackgroundColour");
    
    let altitude_text = inkWidgetBuilder.inkText(n"text")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(ThemeColors.ElectricBlue())
      .Text("You shouldn't see this!")
      .Margin(20.0, 15.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

      let circle_canvas = inkWidgetBuilder.inkCanvas(n"marks")
        .Reparent(this)
	      .Anchor(inkEAnchor.Centered)
        .BuildCanvas();
  }

  public func ClearMarks() -> Void {
    (this.GetWidget(n"marks") as inkCanvas).RemoveAllChildren();
  }

  public func GetMarksWidget() -> ref<inkCanvas> {
    return (this.GetWidget(n"marks") as inkCanvas);
  }

  public func DrawMark(position: Vector4) -> Void {
    let circle = inkWidgetBuilder.inkImage(StringToName("marker_" + ToString(RandF())))
      .Reparent((this.GetWidget(n"marks") as inkCanvas))
      .Atlas(r"base\\gameplay\\gui\\widgets\\crosshair\\master_crosshair.inkatlas")
      .Part(n"lockon-b")
      .Tint(ThemeColors.ElectricBlue())
      .Opacity(0.5)
      .Size(10.0, 10.0)
      .Anchor(0.5, 0.5)
      .Translation(this.ScreenXY(position))
      .BuildImage();
  }

  public func DrawText(position: Vector4, text: String) -> Void {
    let circle = inkWidgetBuilder.inkText(StringToName("text_" + ToString(RandF())))
      .Reparent((this.GetWidget(n"marks") as inkCanvas))
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .Tint(ThemeColors.ElectricBlue())
      .Opacity(0.2)
      .Anchor(0.0, 0.5)
      .Text(text)
      .Translation(this.ScreenXY(position, 15.0, 0.0))
      .BuildText();
  }

  public func Show() -> Void {
    FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().FlightControllerBB.ShouldShowUI, true, true);
    FlightController.GetInstance().GetBlackboard().SignalBool(GetAllBlackboardDefs().FlightControllerBB.ShouldShowUI);
    
    let startValue = this.GetOpacity();
    let duration = 1.0 - startValue;
    if IsDefined(this.m_rootAnim) && this.m_rootAnim.IsPlaying() {
      this.m_rootAnim.Stop();
    };
    this.m_rootAnim = this.PlayAnimation(InkAnimHelper.GetDef_Transparency(startValue, 1.0, duration * 2.0, 0.0, inkanimInterpolationType.Quadratic, inkanimInterpolationMode.EasyInOut));

    let animSelect = new inkAnimDef();
    let animEffectInterp = new inkAnimEffect();
    animEffectInterp.SetStartDelay(0.00);
    animEffectInterp.SetEffectType(inkEffectType.Glitch);
    animEffectInterp.SetEffectName(n"Glitch_0");
    animEffectInterp.SetParamName(n"intensity");
    animEffectInterp.SetStartValue(1.0);
    animEffectInterp.SetEndValue(0.0);
    animEffectInterp.SetDuration(duration * 2.0);
    animSelect.AddInterpolator(animEffectInterp);
    let info_anim = (this.GetWidget(n"info") as inkCanvas).PlayAnimation(animSelect);
    info_anim.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnInfoAnimationCompleted");
  }

  public func Hide() -> Void {    
    let startValue = this.GetOpacity();
    let duration = startValue;
    if IsDefined(this.m_rootAnim) && this.m_rootAnim.IsPlaying() {
      this.m_rootAnim.Stop();
    };
    this.m_rootAnim = this.PlayAnimation(InkAnimHelper.GetDef_Transparency(startValue, 0.0, duration * 0.5, 0.0, inkanimInterpolationType.Quadratic, inkanimInterpolationMode.EasyInOut));
    this.m_rootAnim.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnHideAnimationCompleted");
  } 
   
  protected cb func OnHideAnimationCompleted(anim: ref<inkAnimProxy>) -> Bool {
    if (FlightController.GetInstance().IsActive()) {
      this.HideInfo();
    } else {
      FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().FlightControllerBB.ShouldShowUI, false, true);
      FlightController.GetInstance().GetBlackboard().SignalBool(GetAllBlackboardDefs().FlightControllerBB.ShouldShowUI);
    }
  }

  protected cb func OnInfoAnimationCompleted(anim: ref<inkAnimProxy>) -> Bool {
    this.HideInfo();
  }

  public func HideInfo() -> Void {
    (this.GetWidget(n"info") as inkCanvas).PlayAnimation(InkAnimHelper.GetDef_Transparency(1.0, 0.0, 2.0, 5.0, inkanimInterpolationType.Quadratic, inkanimInterpolationMode.EasyInOut));
  }

  public func ShowInfo() -> Void {
    let info_anim = (this.GetWidget(n"info") as inkCanvas).PlayAnimation(InkAnimHelper.GetDef_Transparency(0.0, 1.0, 1.0, 0.0, inkanimInterpolationType.Quadratic, inkanimInterpolationMode.EasyInOut));
    info_anim.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnInfoAnimationCompleted");
  }

  public func Update(timeDelta: Float) -> Void {
    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(FlightController.GetInstance().gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);
    // (this.GetWidget(n"fps") as inkText).SetText(FloatToStringPrec(1.0 / timeDelta));
    (this.GetWidget(n"info/panel/text") as inkText).SetText("Location: < " + FloatToStringPrec(this.stats.d_position.X, 1)
      + ", " + FloatToStringPrec(this.stats.d_position.Y, 1) + ", " + FloatToStringPrec(this.stats.d_position.Z, 1) + " >");
      // + "Velocity: <" + FloatToStringPrec(this.stats.d_velocity.X, 1)
      // + ", " + FloatToStringPrec(this.stats.d_velocity.Y, 1) + ", " + FloatToStringPrec(this.stats.d_velocity.Z, 1) + " >");
    this.GetWidget(n"info").SetTranslation(this.ScreenXY(this.stats.d_position - this.stats.d_velocity * timeDelta + Transform.GetRight(cameraTransform) * 2.5));

    let fl_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_left_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    let fr_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_right_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
   
    let bl_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_left_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    let br_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_right_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    // let br_position = (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_right_tire") as TargetingComponent).GetLocalToWorld().W;


    this.GetWidget(n"arrow_left").SetTranslation(this.ScreenXY(fl_position - this.stats.d_right * 0.5));
    this.GetWidget(n"arrow_left").SetRotation(this.ScreenAngle(fl_position, fl_position + this.stats.d_right * 0.5));
    this.GetWidget(n"arrow_left").SetOpacity(this.OpacityForPosition(fl_position - this.stats.d_right * 0.5));

    this.GetWidget(n"arrow_right").SetTranslation(this.ScreenXY(fr_position + this.stats.d_right * 0.5));
    this.GetWidget(n"arrow_right").SetRotation(this.ScreenAngle(fr_position, fr_position - this.stats.d_right * 0.5));
    this.GetWidget(n"arrow_right").SetOpacity(this.OpacityForPosition(fr_position + this.stats.d_right * 0.5));

    this.GetWidget(n"arrow_left_rear").SetTranslation(this.ScreenXY(bl_position - this.stats.d_right * 0.5));
    this.GetWidget(n"arrow_left_rear").SetRotation(this.ScreenAngle(bl_position, bl_position + this.stats.d_right * 0.5));
    this.GetWidget(n"arrow_left_rear").SetOpacity(this.OpacityForPosition(bl_position - this.stats.d_right * 0.5));

    this.GetWidget(n"arrow_right_rear").SetTranslation(this.ScreenXY(br_position + this.stats.d_right * 0.5));
    this.GetWidget(n"arrow_right_rear").SetRotation(this.ScreenAngle(br_position, br_position - this.stats.d_right * 0.5));
    this.GetWidget(n"arrow_right_rear").SetOpacity(this.OpacityForPosition(br_position + this.stats.d_right * 0.5));
 
 
    // this.GetWidget(n"ruler_left").SetTranslation(this.ScreenXY(this.stats.d_position - Transform.GetUp(cameraTransform) * 2.0));
    // this.GetWidget(n"ruler_left").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position - Transform.GetUp(cameraTransform) * 2.0));
 
    // this.GetWidget(n"ruler_right").SetTranslation(this.ScreenXY(this.stats.d_position + Transform.GetUp(cameraTransform) * 2.0));
    // this.GetWidget(n"ruler_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + Transform.GetUp(cameraTransform) * 2.0));


    // let marker_vector = Vector4.RotateAxis(Transform.GetRight(cameraTransform), Transform.GetForward(cameraTransform), Deg2Rad(Vector4.GetAngleBetween(this.stats.d_right, new Vector4(0.0, 0.0, 1.0, 0.0))));
    this.GetWidget(n"rulers").SetTranslation(this.ScreenXY(this.stats.d_position - this.stats.d_velocity * timeDelta));

    let mark = Vector4.GetAngleBetween(this.stats.d_right, new Vector4(0.0, 0.0, 1.0, 0.0)) + 90;

    this.GetWidget(n"rulers/roll_marker_left").SetTranslation(CosF(Deg2Rad(mark - 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark - 90.0)) * this.m_markerRadius);
    this.GetWidget(n"rulers/roll_marker_left").SetRotation(mark);

    this.GetWidget(n"rulers/roll_marker_right").SetTranslation(CosF(Deg2Rad(mark + 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark + 90.0)) * this.m_markerRadius);
    this.GetWidget(n"rulers/roll_marker_right").SetRotation(mark + 180.0);

    (this.GetWidget(n"rulers/roll_text") as inkText).SetText(((mark - 180.0) > 0.0 ? "+" : "") + FloatToStringPrec(mark - 180.0, 2) + "°");
    this.GetWidget(n"rulers/roll_text").SetRotation(mark - 180.0);    
    this.GetWidget(n"rulers/roll_text").SetTranslation(CosF(Deg2Rad(mark + 90.0)) * (this.m_markerRadius + 30.0), SinF(Deg2Rad(mark + 90.0)) * (this.m_markerRadius + 30.0));


 
    // this.GetWidget(n"roll_marker_right").SetTranslation(this.ScreenXY(this.stats.d_position + marker_vector * 2.0));
    // this.GetWidget(n"roll_marker_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + marker_vector));
 
    // this.GetWidget(n"scaler_left").SetTranslation(this.ScreenXY(this.stats.d_position - Transform.GetRight(cameraTransform) * 4.0));
    // this.GetWidget(n"scaler_left").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position - Transform.GetRight(cameraTransform)));
 
    // this.GetWidget(n"scaler_right").SetTranslation(this.ScreenXY(this.stats.d_position + Transform.GetRight(cameraTransform) * 4.0));
    // this.GetWidget(n"scaler_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + Transform.GetRight(cameraTransform)));
 
  }

  public func OpacityForPosition(position: Vector4) -> Float {
    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(FlightController.GetInstance().gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);
    let cameraForward = Transform.GetForward(cameraTransform);
    return 0.1 + MaxF(0.0, MinF(0.9, (Vector4.Dot(cameraTransform.position - position, cameraForward) - Vector4.Dot(cameraTransform.position - this.stats.d_position, cameraForward)) / 4.0 * 0.9));
  }

  public func ScreenXY(position: Vector4, opt offsetX: Float, opt offsetY: Float) -> Vector2 {
    if IsDefined(this.controller) {
      let translation = this.controller.ProjectWorldToScreen(position);
      translation.X = translation.X * 1920.0 + offsetX;
      translation.Y = translation.Y * -1080.0 + offsetY;
      return translation;
    } else {
      return new Vector2(0.0, 0.0);
    }
  }

  public func ScreenAngle(a: Vector4, b: Vector4) -> Float {
    if IsDefined(this.controller) {
      let point_a = this.controller.ProjectWorldToScreen(a);
      let point_b = this.controller.ProjectWorldToScreen(b);
      return Rad2Deg(AtanF(point_a.Y - point_b.Y, point_b.X - point_a.X));
    } else {
      return 0.0;
    }
  }

}