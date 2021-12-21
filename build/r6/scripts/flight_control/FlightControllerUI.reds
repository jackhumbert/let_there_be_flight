import Codeware.UI.*
// import Flexy.UI.*

// handle this somewhere
// this.m_tppBBConnectionId = this.m_activeVehicleUIBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this, n"OnCameraModeChanged");

// public static func OperatorAssignAdd(out a: array<ref<Elem>>, b: ref<Text>) -> array<ref<Elem>> {
//   ArrayPush(a, b as Elem);
//   return a;
// }



public class FlightControllerUI extends inkCanvas {
  public let controller: ref<inkGameController>;
  public let stats: ref<FlightStats>;
  private let m_rootAnim: ref<inkAnimProxy>;
  private let m_markerRadius: Float;
  private let m_lastPitchValue: Float;
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

    // let box_bg_image = Image.New(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas", n"frame_top_bg");
    // let box_text = Text.New("Howdy");    
    // // let box_text2 = Text.New("Hi there");

    // let box = Box.New([box_text as Elem]);
    // // // box.children += box_text2;
    // box.BackgroundImage(box_bg_image);
    // // RenderElem(box, new Vector2(500.0, 500.0)).Reparent(this);

    this.stats = stats;
    this.SetOpacity(0.0);
    this.RemoveAllChildren();
		this.SetAnchor(inkEAnchor.Centered);
    
    // let w: ref<inkWidget> = this.controller.SpawnFromExternal(this, r"base\\gameplay\\gui\\widgets\\turret_hud\\turret_hud.inkwidget", n"Root");
    // w.Reparent(this);
    // return;

    let arrow_forward = inkWidgetBuilder.inkImage(n"arrow_forward")
		  .Reparent(this)
		  .Atlas(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas")
		  .Part(n"arrow_right")
		  .Size(24.0, 24.0)
      .Anchor(1.0, 0.5)
		  .Opacity(1.0)
		  .Tint(ThemeColors.ElectricBlue())
      .BuildImage();

    // wheel markers

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

    // roll markers

    let rulers = inkWidgetBuilder.inkCanvas(n"rulers")
      .Reparent(this)
	    .Anchor(inkEAnchor.Centered)
      .Opacity(0.5)
      .Margin(0.0, 0.0, 0.0, 0.0)
      .Translation(0.0, 230.0)
      .BuildCanvas();
  
    let marks = [-30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30];
    this.m_markerRadius = 820.0;
    let points: array<Vector2>;
    for mark in marks {
      let roll_marker = inkWidgetBuilder.inkRectangle(StringToName("roll_marker_" + mark))
        .Tint(ThemeColors.ElectricBlue())
        .Opacity(1.0)
        .Size(3.0, (mark == 0) ? 50.0 : 30.0)
        .Anchor(0.5, 0.0)
        .Translation(CosF(Deg2Rad(mark - 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark - 90.0)) * this.m_markerRadius)
        .Rotation(mark + 180.0)
        .Reparent(rulers)
        .BuildRectangle();
    }
    let offset = 20.0;
    let i = -40;
    while i <= 40 {
      ArrayPush(points, new Vector2(CosF(Deg2Rad(i - 90.0)) * (this.m_markerRadius + offset), SinF(Deg2Rad(i - 90.0)) * (this.m_markerRadius + offset)));
      i += 1;
    }

    let arc = inkWidgetBuilder.inkShape(n"arc")
      .Reparent(rulers)
      .Size(1920.0 * 2.0, 1080.0 * 2.0)
      .UseNineSlice(true)
      .ShapeVariant(inkEShapeVariant.FillAndBorder)
      .LineThickness(3.0)
      .FillOpacity(0.0)
      .Tint(ThemeColors.ElectricBlue())
      .BorderColor(ThemeColors.ElectricBlue())
      .BorderOpacity(1.0)
      .EndCapStyle(inkEEndCapStyle.SQUARE)
      .Visible(true)
      .BuildShape();
    arc.SetVertexList(points);
    arc.SetRenderTransformPivot(0.0, 0.0);
  

    let roll_text = inkWidgetBuilder.inkText(n"roll_text")
      .Reparent(rulers)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .Tint(ThemeColors.ElectricBlue())
      .Text("0.00")
		  .Opacity(1.0)
      .Anchor(0.5, 0.0)
      .BuildText();

    let roll_marker_top = inkWidgetBuilder.inkRectangle(n"roll_marker_top")
      .Tint(ThemeColors.ElectricBlue())
      .Opacity(1.0)
      .Size(3.0, 70.0)
      .Anchor(0.5, 0.0)
      .Reparent(rulers)
      .BuildRectangle();

    this.SetupPitchDisplay();

    // info block

    let info = inkWidgetBuilder.inkCanvas(n"info")
      .Size(500.0, 200.0)
      .Reparent(this)
	    .Anchor(inkEAnchor.CenterLeft)
      .Margin(0.0, 0.0, 0.0, 0.0)
      .BuildCanvas();
    // info.SetEffectEnabled(inkEffectType.Glitch, n"Glitch_0", true);
    // info.SetEffectParamValue(inkEffectType.Glitch, n"Glitch_0", n"intensity", 1.0);
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
      .Tint(ThemeColors.PureBlack())
      .Opacity(0.1)
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
    
    let position_text = inkWidgetBuilder.inkText(n"position")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(ThemeColors.ElectricBlue())
      .Text("You shouldn't see this!")
      .Margin(20.0, 15.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

    let velocity_text = inkWidgetBuilder.inkText(n"velocity")
      .Reparent(panel)
      .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
      .FontSize(20)
      .LetterCase(textLetterCase.UpperCase)
      .Tint(ThemeColors.ElectricBlue())
      .Text("You shouldn't see this!")
      .Margin(20.0, 40.0, 0.0, 0.0)
      // .Overflow(textOverflowPolicy.AdjustToSize)
      .BuildText();

      // marks canvas

      let circle_canvas = inkWidgetBuilder.inkCanvas(n"marks")
        .Reparent(this)
	      .Anchor(inkEAnchor.Centered)
        .BuildCanvas();
  }

  private func UpdateRollSplay(factor: Float) -> Void {
    let marks = [-30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30];
    // let points: array<Vector2>;
    for mark in marks {
      let mark_scale = factor * mark;
      this.GetWidget(StringToName("rulers/roll_marker_" + mark)).SetTranslation(CosF(Deg2Rad(mark_scale - 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark_scale - 90.0)) * this.m_markerRadius);
      this.GetWidget(StringToName("rulers/roll_marker_" + mark)).SetRotation(mark_scale + 180.0);
      // ArrayPush(points, new Vector2(CosF(Deg2Rad(mark_scale - 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark_scale - 90.0)) * this.m_markerRadius));
    }

    // (this.GetWidget(n"rulers/arc") as inkShape).ChangeShape(n"Rectangle");
    // (this.GetWidget(n"rulers/arc") as inkShape).SetVertexList(points);
    // (this.GetWidget(n"rulers/arc") as inkShape).SetVisible(true);
    (this.GetWidget(n"rulers/arc") as inkShape).SetScale(new Vector2(1.0 + (factor - 1.0) * 0.05, 1.0 + (factor - 1.0) * 0.05));
  }

  private func UpdatePitchDisplayHeight(factor: Float) -> Void {
    this.GetWidget(n"pitch").SetSize(60.0, 520.0 + (factor - 1.0) * 1000.0);
  }

  private func SetupPitchDisplay() -> Void {

      let mark_scale = 20.0;
      let height = 520.0;
      let width = 60.0;

      let pitch = inkWidgetBuilder.inkCanvas(n"pitch")
        .Size(width, height)
        .Reparent(this)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.Centered)
        .Margin(0.0, 0.0, 0.0, 0.0)
        .Translation(-920.0, 230.0)
        .Opacity(0.5)
        .BuildCanvas();
      // pitch.SetChildOrder(inkEChildOrder.Backward);

      let arrow = inkWidgetBuilder.inkImage(n"arrow")
        .Reparent(pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_right_bg")
        .Size(20.0, 20.0)
        .Anchor(1.0, 0.5)
        .Anchor(inkEAnchor.CenterLeft)
        .Margin(-15.0, 0.0, 0.0, 0.0)
        .Opacity(1.0)
        .Tint(ThemeColors.ElectricBlue())
        .BuildImage();
        
      let fluff_text = inkWidgetBuilder.inkText(n"fluff_text")
        .Reparent(pitch)
        .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
        .FontSize(12)
        .Anchor(0.0, 1.0)
        .Anchor(inkEAnchor.TopLeft)
        .Tint(ThemeColors.Bittersweet())
        .Text("89V_PITCH")
        .HAlign(inkEHorizontalAlign.Left)
        .Margin(-14.0, -10.0, 0.0, 0.0)
        // .Overflow(textOverflowPolicy.AdjustToSize) pArrayType was nullptr.
        .BuildText();

      let border = inkWidgetBuilder.inkImage(n"border")
        .Reparent(pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_cell_fg")
        .Size(width + 24.0, height)
        .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.CenterFillVerticaly)
        .Translation(-2.5, 0.0)
        .Opacity(1.0)
        .Tint(ThemeColors.ElectricBlue())
        .BuildImage();

      let fill = inkWidgetBuilder.inkImage(n"fill")
        .Reparent(pitch)
        .Atlas(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas")
        .Part(n"arrow_cell_bg")
        .Size(width + 24.0, height)
        .NineSliceScale(true)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.CenterFillVerticaly)
        .Translation(-2.5, 0.0)
        .Opacity(0.1)
        .Tint(ThemeColors.PureBlack())
        .BuildImage();

      let mask = inkWidgetBuilder.inkMask(n"mask")
        .Reparent(pitch)
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

      let markers = inkWidgetBuilder.inkCanvas(n"markers")
        .Size(width, 180.0 * mark_scale)
        .Reparent(pitch)
        .Anchor(0.5, 0.5)
        .Anchor(inkEAnchor.Centered)
        .Margin(0.0, 0.0, 0.0, 0.0)
        .BuildCanvas();
      markers.CreateEffect(n"inkBoxBlurEffect", n"BoxBlur_0");
      markers.SetEffectEnabled(inkEffectType.BoxBlur, n"BoxBlur_0", true);
      markers.SetEffectParamValue(inkEffectType.BoxBlur, n"BoxBlur_0", n"intensity", 0.0);
      markers.SetBlurDimension(n"BoxBlur_0", inkEBlurDimension.Vertical);
      // markers.SetEffectEnabled(inkEffectType.Mask, n"Mask_0", true);
		  // markers.SetRenderTransformPivot(new Vector2(0.0, 0.0));

      let midbar_size = 16.0;
      let marks: array<Float> = [-100.0, -90.0, -80.0, -70.0, -60.0, -50.0, -40.0, -30.0, -20.0, -10.0, 0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0];
      let marks_inc: array<Float> = [-4.0, -3.0, -2.0, -1.0, 1.0, 2.0, 3.0, 4.0, 5.0];

      let marker_zero = inkWidgetBuilder.inkRectangle(n"m1_00000")
        .Tint(ThemeColors.ElectricBlue())
        // .Opacity(mark == 0.0 ? 1.0 : 0.5)
        .Opacity(1.0)
        .Reparent(markers)
        .Size(width, 19.0)
        .Anchor(0.0, 0.0)
        .Translation(0.0, 90.0 * mark_scale - 20.0)
        .BuildRectangle();

      let marker_zero1 = inkWidgetBuilder.inkRectangle(n"m1_00001")
        .Tint(ThemeColors.ElectricBlue())
        // .Opacity(mark == 0.0 ? 1.0 : 0.5)
        .Opacity(1.0)
        .Reparent(markers)
        .Size(width, 19.0)
        .Anchor(0.0, 1.0)
        .Translation(0.0, 90.0 * mark_scale + 20.0)
        .BuildRectangle();

      // let text = inkWidgetBuilder.inkText(n"text")
      //   .Reparent(markers)
      //   .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily", n"Heavy")
      //   .FontSize(16)
      //   .Anchor(0.5, 0.5)
      //   .Tint(ThemeColors.PureBlack())
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
          let text = inkWidgetBuilder.inkText(n"text")
            .Reparent(markers)
            .Font("base\\gameplay\\gui\\fonts\\industry\\industry.inkfontfamily")
            .FontSize(20)
            .Anchor(0.5, 0.5)
            .Tint(ThemeColors.ElectricBlue())
            .Text(FloatToStringPrec(AbsF(mark), 0))
            .HAlign(inkEHorizontalAlign.Center)
            .Margin(0.0, 0.0, 0.0, 0.0)
            .Translation(width / 2.0, (mark + 90.0) * mark_scale)
            // .Overflow(textOverflowPolicy.AdjustToSize)
            .BuildText();

          let marker1 = inkWidgetBuilder.inkRectangle(StringToName("m1_" + FloatToString(mark)))
            .Tint(ThemeColors.ElectricBlue())
            // .Opacity(mark == 0.0 ? 1.0 : 0.5)
            .Size(midbar_size, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(width - midbar_size, (mark + 90.0) * mark_scale)
            .Reparent(markers)
            .BuildRectangle();

          let marker2 = inkWidgetBuilder.inkRectangle(StringToName("m2_" + FloatToString(mark)))
            .Tint(ThemeColors.ElectricBlue())
            // .Opacity(mark == 0.0 ? 1.0 : 0.5)
            .Size(midbar_size, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(0.0, (mark + 90.0) * mark_scale)
            .Reparent(markers)
            .BuildRectangle();
        }
        for mark_inc in marks_inc {
          let marker = inkWidgetBuilder.inkRectangle(StringToName("m_" + FloatToString(mark + mark_inc)))
            .Tint(ThemeColors.ElectricBlue())
            .Opacity(mark_inc == 5.0 ? 0.25 : 0.05)
            .Size(width, 2.0)
            .Anchor(0.0, 0.5)
            .Translation(0.0, ((mark + 90.0) + mark_inc) * mark_scale)
            .Reparent(markers)
            .BuildRectangle();
        }
      } 
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
    (this.GetWidget(n"info/panel/position") as inkText).SetText("Location: < " + FloatToStringPrec(this.stats.d_position.X, 1)
      + ", " + FloatToStringPrec(this.stats.d_position.Y, 1) + ", " + FloatToStringPrec(this.stats.d_position.Z, 1) + " >");
    (this.GetWidget(n"info/panel/velocity") as inkText).SetText("Velocity: < " + FloatToStringPrec(this.stats.d_velocity.X, 1)
      + ", " + FloatToStringPrec(this.stats.d_velocity.Y, 1) + ", " + FloatToStringPrec(this.stats.d_velocity.Z, 1) + " >");
      // + "Velocity: <" + FloatToStringPrec(this.stats.d_velocity.X, 1)
      // + ", " + FloatToStringPrec(this.stats.d_velocity.Y, 1) + ", " + FloatToStringPrec(this.stats.d_velocity.Z, 1) + " >");
    this.GetWidget(n"info").SetTranslation(this.ScreenXY(this.stats.d_position - this.stats.d_velocity * timeDelta + Transform.GetRight(cameraTransform) * 2.5));

    let fl_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_left_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    let fr_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_right_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
   
    let bl_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_left_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    let br_position = Matrix.GetTranslation((this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_right_tire") as TargetingComponent).GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    // let br_position = (this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_right_tire") as TargetingComponent).GetLocalToWorld().W;

    let screen_position: Vector4 = this.stats.d_position - this.stats.d_velocity * timeDelta;
    this.GetWidget(n"arrow_forward").SetTranslation(this.ScreenXY(screen_position + this.stats.d_forward * 3.0));
    this.GetWidget(n"arrow_forward").SetRotation(this.ScreenAngle(screen_position, screen_position + this.stats.d_forward));
    this.GetWidget(n"arrow_forward").SetOpacity(this.OpacityForPosition(screen_position + this.stats.d_forward * 3.0));

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

    let pitch_mark = Vector4.GetAngleBetween(this.stats.d_forward, new Vector4(0.0, 0.0, 1.0, 0.0));
    this.GetWidget(n"pitch/markers").SetTranslation(0.0, (pitch_mark - 90.0) * 20.0);
    this.GetWidget(n"pitch/markers").SetEffectParamValue(inkEffectType.BoxBlur, n"BoxBlur_0", n"intensity", AbsF(pitch_mark - this.m_lastPitchValue) / timeDelta * 0.0001);
    this.m_lastPitchValue = pitch_mark;

    // let marker_vector = Vector4.RotateAxis(Transform.GetRight(cameraTransform), Transform.GetForward(cameraTransform), Deg2Rad(Vector4.GetAngleBetween(this.stats.d_right, new Vector4(0.0, 0.0, 1.0, 0.0))));
    // this.GetWidget(n"rulers").SetTranslation(this.ScreenXY(this.stats.d_position - this.stats.d_velocity * timeDelta));

    let splay = 1.0 + 0.2 * MaxF(0.0, FlightController.GetInstance().surge.GetValue()) - 0.2 * FlightController.GetInstance().brake.GetValue() + ((RandF() * 0.02 - 0.01) * this.stats.d_speedRatio);
    let mark = Vector4.GetAngleBetween(this.stats.d_right, new Vector4(0.0, 0.0, 1.0, 0.0)) + 90;
    let mark_effected = mark * splay;
    this.UpdateRollSplay(splay);
    this.UpdatePitchDisplayHeight(splay);

    this.GetWidget(n"rulers/roll_marker_top").SetTranslation(CosF(Deg2Rad(mark_effected + 90.0)) * this.m_markerRadius, SinF(Deg2Rad(mark_effected + 90.0)) * this.m_markerRadius);
    this.GetWidget(n"rulers/roll_marker_top").SetRotation(mark_effected);

    (this.GetWidget(n"rulers/roll_text") as inkText).SetText(((mark - 180.0) > 0.0 ? "+" : "") + FloatToStringPrec(mark - 180.0, 2) + "°");
    this.GetWidget(n"rulers/roll_text").SetRotation(mark_effected - 180.0);    
    this.GetWidget(n"rulers/roll_text").SetTranslation(CosF(Deg2Rad(mark_effected + 90.0)) * (this.m_markerRadius + 30.0), SinF(Deg2Rad(mark_effected + 90.0)) * (this.m_markerRadius + 30.0));

 
    // this.GetWidget(n"roll_marker_right").SetTranslation(this.ScreenXY(this.stats.d_position + marker_vector * 2.0));
    // this.GetWidget(n"roll_marker_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + marker_vector));
 
    // this.GetWidget(n"scaler_left").SetTranslation(this.ScreenXY(this.stats.d_position - Transform.GetRight(cameraTransform) * 4.0));
    // this.GetWidget(n"scaler_left").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position - Transform.GetRight(cameraTransform)));
 
    // this.GetWidget(n"scaler_right").SetTranslation(this.ScreenXY(this.stats.d_position + Transform.GetRight(cameraTransform) * 4.0));
    // this.GetWidget(n"scaler_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + Transform.GetRight(cameraTransform)));
 
  }

  // public func DrawSphere(position: Vector4, radius: Float) -> Void {
  //   let segments = 16;

  //   let point: Vector4;
  //   for (segment in segments) {
  //   }
  //   let normalLine = inkWidgetBuilder.inkShape(n"normalLine")
  //     .Reparent(this.ui.GetMarksWidget())
  //     .Size(1920.0 * 2.0, 1080.0 * 2.0)
  //     .UseNineSlice(true)
  //     .ShapeVariant(inkEShapeVariant.FillAndBorder)
  //     .LineThickness(3.0)
  //     .FillOpacity(0.0)
  //     .Tint(ThemeColors.ElectricBlue())
  //     .BorderColor(ThemeColors.ElectricBlue())
  //     .BorderOpacity(0.1)
  //     .Visible(true)
  //     .BuildShape();
  //   normalLine.SetVertexList([this.ui.ScreenXY(this.stats.d_visualPosition), this.ui.ScreenXY(this.stats.d_visualPosition + this.stats.d_up)]);
  //   this.ui.DrawMark(this.stats.d_visualPosition + this.stats.d_up);
  // }

  public func OpacityForPosition(position: Vector4) -> Float {
    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(FlightController.GetInstance().gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);
    let cameraForward = Transform.GetForward(cameraTransform);
    return 0.1 + MaxF(0.0, MinF(0.9, (Vector4.Dot(cameraTransform.position - position, cameraForward) - Vector4.Dot(cameraTransform.position - this.stats.d_position, cameraForward)) / 4.0 * 0.9));
  }

  public func ScreenXY(position: Vector4) -> Vector2 {
    return this.ScreenXY(position, 0.0, 0.0);
  }

  public func ScreenXY(position: Vector4, offsetX: Float, offsetY: Float) -> Vector2 {
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