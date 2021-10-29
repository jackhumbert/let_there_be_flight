import BaseLib.UI.*

public class FlightControlUI extends inkCanvas {
  public let controller: ref<inkGameController>;
  public let stats: ref<VehicleStats>;
  public static func Create(controller: ref<inkGameController>, parent: ref<inkCompoundWidget>) -> ref<FlightControlUI> {
    let instance = new FlightControlUI();
    instance.controller = controller;
    instance.Reparent(parent);
    instance.SetName(n"flightControlUI");
    FlightControl.GetInstance().SetUI(instance);
    return instance;
  }
  public func Setup(stats: ref<VehicleStats>) -> Void {
    this.stats = stats;
    this.SetOpacity(0.0);
    this.RemoveAllChildren();
		this.SetAnchor(inkEAnchor.Centered);

    let arrow_left = new inkImage();
		arrow_left.SetName(n"arrow_left");
		arrow_left.Reparent(this);
		arrow_left.SetAtlasResource(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas");
		arrow_left.SetTexturePart(n"arrow_right");
		arrow_left.SetSize(24.0, 24.0);
    arrow_left.SetMargin(new inkMargin(0.0, 12.0, 24.0, 12.0));
		arrow_left.SetOpacity(1.0);
		arrow_left.SetTintColor(ThemeColors.ElectricBlue());
    arrow_left.SetEffectEnabled(inkEffectType.Glitch, n"Glitch_0", true);

    let arrow_right = new inkImage();
		arrow_right.SetName(n"arrow_right");
		arrow_right.Reparent(this);
		arrow_right.SetAtlasResource(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas");
		arrow_right.SetTexturePart(n"arrow_right");
		arrow_right.SetSize(24.0, 24.0);
    arrow_right.SetMargin(new inkMargin(0.0, 12.0, 24.0, 12.0));
		arrow_right.SetOpacity(1.0);
		arrow_right.SetTintColor(ThemeColors.ElectricBlue());
    arrow_right.SetEffectEnabled(inkEffectType.Glitch, n"Glitch_0", true);

    // let fps = new inkText();
    // fps.SetName(n"fps");
    // fps.Reparent(this);
    // fps.SetFontFamily("base\\gameplay\\gui\\fonts\\orbitron\\orbitron.inkfontfamily");
    // fps.SetFontStyle(n"Medium");
    // fps.SetFontSize(24);
    // fps.SetLetterCase(textLetterCase.UpperCase);
    // fps.SetTintColor(ThemeColors.ElectricBlue());
    // fps.SetText("You shouldn't see this!");
    // fps.SetMargin(new inkMargin(0.0, 0.0, 0.0, 0.0));
    // fps.SetSize(220.0, 24.0);

    let info = new inkCanvas();
		info.SetName(n"info");
		info.SetSize(500.0, 200.0);
    info.Reparent(this);
		info.SetAnchor(inkEAnchor.CenterLeft);
    info.SetMargin(new inkMargin(0.0, 0.0, 0.0, 0.0));
		// altitude.SetAnchorPoint(new Vector2(0.5, 0.5));

    let top = new inkHorizontalPanel();
		top.SetName(n"top");
		top.SetSize(500.0, 18.0);
    top.Reparent(info);
		// top.SetHAlign(inkEHorizontalAlign.Left);
		// top.SetVAlign(inkEVerticalAlign.Top);
    top.SetMargin(new inkMargin(0.0, 0.0, 0.0, 0.0));

    let vehicle_manufacturer = new inkImage();
		vehicle_manufacturer.SetName(n"manufacturer");
    vehicle_manufacturer.Reparent(top);
    let manufacturer = this.stats.vehicle.GetRecord().Manufacturer().EnumName();
    let iconRecord = TweakDBInterface.GetUIIconRecord(TDBID.Create("UIIcon." + manufacturer));
    vehicle_manufacturer.SetTexturePart(iconRecord.AtlasPartName());
    vehicle_manufacturer.SetAtlasResource(iconRecord.AtlasResourcePath());
		vehicle_manufacturer.SetSize(174.0, 18.0);
    vehicle_manufacturer.SetMargin(new inkMargin(0.0, 0.0, 0.0, 0.0));
		vehicle_manufacturer.SetOpacity(1.0);
		vehicle_manufacturer.SetTintColor(ThemeColors.ElectricBlue());
		vehicle_manufacturer.SetAnchor(inkEAnchor.LeftFillVerticaly);

    let fluff = new inkImage();
		fluff.SetName(n"fluff");
    fluff.Reparent(top);
    fluff.SetAtlasResource(r"base\\gameplay\\gui\\fullscreen\\common\\general_fluff.inkatlas");
    fluff.SetTexturePart(n"fluff_01");
		fluff.SetSize(174.0, 18.0);
    fluff.SetMargin(new inkMargin(10.0, 0.0, 0.0, 0.0));
		fluff.SetOpacity(1.0);
		fluff.SetTintColor(ThemeColors.ElectricBlue());
		fluff.SetAnchor(inkEAnchor.LeftFillVerticaly);

    let panel = new inkFlex();
		panel.SetName(n"panel");
    panel.SetMargin(new inkMargin(0.0, 24.0, 0.0, 0.0));
    panel.SetPadding(10.0, 10.0, 10.0, 10.0);
		panel.SetFitToContent(true);
    panel.Reparent(info);
		panel.SetAnchor(inkEAnchor.Fill);
    
    let altitude_background = new inkImage();
		altitude_background.SetName(n"background");
    altitude_background.Reparent(panel);
		altitude_background.SetAnchor(inkEAnchor.Fill);
    altitude_background.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		altitude_background.SetTexturePart(n"frame_top_bg");
		altitude_background.SetNineSliceScale(true);
    altitude_background.SetMargin(new inkMargin(0.0, 0.0, 0.0, 0.0));
		altitude_background.SetTintColor(ThemeColors.ElectricBlue());
		altitude_background.SetOpacity(0.1);
    // altitude_background.BindProperty(n"tintColor", n"Briefings.BackgroundColour");

    let altitude_frame = new inkImage();
		altitude_frame.SetName(n"frame");
    altitude_frame.Reparent(panel);
		altitude_frame.SetAnchor(inkEAnchor.Fill);
    altitude_frame.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		altitude_frame.SetTexturePart(n"frame_top_fg");
		altitude_frame.SetNineSliceScale(true);
    altitude_frame.SetMargin(new inkMargin(0.0, 0.0, 0.0, 0.0));
		altitude_frame.SetTintColor(ThemeColors.ElectricBlue());
		altitude_frame.SetOpacity(1.0);
    // altitude_frame.BindProperty(n"tintColor", n"Briefings.BackgroundColour");
		// altitude_frame.SetOpacity(0.8);


    // let altitude_lines = new inkImage();
		// altitude_lines.SetName(n"lines");
		// altitude_lines.Reparent(panel);
		// altitude_lines.SetAtlasResource(r"base\\gameplay\\gui\\widgets\\crosshair\\smart_rifle\\arghtlas_smargun.inkatlas");
		// altitude_lines.SetTexturePart(n"smartFluffVentHori");
		// altitude_lines.SetSize(24.0, 18.0);
    // altitude_lines.SetAnchor(inkEAnchor.TopLeft);
    // // altitude_lines.SetMargin(new inkMargin(25.0, 18.0, 0.0, 0.0));
		// altitude_lines.SetOpacity(1.0);
		// altitude_lines.SetTintColor(ThemeColors.ElectricBlue());
    
    let altitude_text = new inkText();
    altitude_text.SetName(n"text");
    altitude_text.Reparent(panel);
    altitude_text.SetFontFamily("base\\gameplay\\gui\\fonts\\orbitron\\orbitron.inkfontfamily");
    altitude_text.SetFontStyle(n"Medium");
    altitude_text.SetFontSize(20);
    altitude_text.SetLetterCase(textLetterCase.UpperCase);
    altitude_text.SetTintColor(ThemeColors.ElectricBlue());
    altitude_text.SetText("You shouldn't see this!");
    altitude_text.SetMargin(new inkMargin(30.0, 0.0, 0.0, 0.0));
    // altitude_text.SetSize(220.0, 24.0);
  }

  public func Update(timeDelta: Float) -> Void {
    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(FlightControl.GetInstance().gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);
    // (this.flightControlUI.GetWidget(n"fps") as inkText).SetText(FloatToStringPrec(1.0 / timeDelta));
    (this.GetWidget(n"info/panel/text") as inkText).SetText("< " + FloatToStringPrec(this.stats.d_position.X, 1) + ", " + FloatToStringPrec(this.stats.d_position.Y, 1) + ", " + FloatToStringPrec(this.stats.d_position.Z, 1) + " >");
    this.GetWidget(n"info").SetTranslation(this.ScreenXY(this.stats.d_position + Transform.GetRight(cameraTransform) * 2.5));

    this.GetWidget(n"arrow_left").SetTranslation(this.ScreenXY(this.stats.d_position - this.stats.d_right * 2));
    this.GetWidget(n"arrow_left").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position - this.stats.d_right * 2));
    this.GetWidget(n"arrow_left").SetOpacity(this.OpacityForPosition(this.stats.d_position - this.stats.d_right * 2));
    this.GetWidget(n"arrow_left").SetEffectParamValue(inkEffectType.Glitch, n"Glitch_0", n"intensity", 1.0 - this.OpacityForPosition(this.stats.d_position - this.stats.d_right * 2));

    this.GetWidget(n"arrow_right").SetTranslation(this.ScreenXY(this.stats.d_position + this.stats.d_right * 2));
    this.GetWidget(n"arrow_right").SetRotation(this.ScreenAngle(this.stats.d_position, this.stats.d_position + this.stats.d_right * 2));
    this.GetWidget(n"arrow_right").SetOpacity(this.OpacityForPosition(this.stats.d_position + this.stats.d_right * 2));
    this.GetWidget(n"arrow_right").SetEffectParamValue(inkEffectType.Glitch, n"Glitch_0", n"intensity", 1.0 - this.OpacityForPosition(this.stats.d_position + this.stats.d_right * 2));
  }

  public func OpacityForPosition(position: Vector4) -> Float {
    let cameraTransform: Transform;
    let cameraSys: ref<CameraSystem> = GameInstance.GetCameraSystem(FlightControl.GetInstance().gameInstance);
    cameraSys.GetActiveCameraWorldTransform(cameraTransform);
    let cameraForward = Transform.GetForward(cameraTransform);
    return 0.1 + MaxF(0.0, MinF(0.9, (Vector4.Dot(cameraTransform.position - position, cameraForward) - Vector4.Dot(cameraTransform.position - this.stats.d_position, cameraForward)) / 2.0 * 0.9));
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