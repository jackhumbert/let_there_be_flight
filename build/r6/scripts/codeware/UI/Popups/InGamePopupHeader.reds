// -----------------------------------------------------------------------------
// Codeware.UI.InGamePopupHeader
// -----------------------------------------------------------------------------

module Codeware.UI

public class InGamePopupHeader extends inkCustomController {
	protected let m_title: wref<inkText>;

	protected let m_fluffLeft: wref<inkText>;

	protected let m_fluffRight: wref<inkText>;

	protected cb func OnCreate() -> Void {
		let header: ref<inkFlex> = new inkFlex();
		header.SetName(n"header");
		header.SetMargin(new inkMargin(76.0, 32.0, 76.0, 0.0));
		header.SetAnchor(inkEAnchor.TopFillHorizontaly);

		let bar: ref<inkFlex> = new inkFlex();
		bar.SetName(n"bar");
		bar.SetMargin(new inkMargin(24.0, 0.0, 0.0, 0.0));
		bar.SetRenderTransformPivot(new Vector2(0.0, 0.5));
		bar.Reparent(header);

		let bg: ref<inkImage> = new inkImage();
		bg.SetName(n"bg");
		bg.SetAtlasResource(r"base\\gameplay\\gui\\widgets\\notifications\\notification_assets.inkatlas");
		bg.SetTexturePart(n"Plate_main");
		bg.SetNineSliceScale(true);
		bg.SetAnchor(inkEAnchor.BottomFillHorizontaly);
		bg.SetAnchorPoint(new Vector2(1.0, 1.0));
		bg.SetOpacity(0.61);
		bg.SetTintColor(ThemeColors.BlackPearl());
		bg.Reparent(bar);

		let bgr: ref<inkImage> = new inkImage();
		bgr.SetName(n"bgr");
		bgr.SetAtlasResource(r"base\\gameplay\\gui\\widgets\\notifications\\notification_assets.inkatlas");
		bgr.SetTexturePart(n"Plate_main");
		bgr.SetNineSliceScale(true);
		bgr.SetAnchor(inkEAnchor.BottomFillHorizontaly);
		bgr.SetAnchorPoint(new Vector2(1.0, 1.0));
		bgr.SetOpacity(0.07);
		bgr.SetTintColor(ThemeColors.Bordeaux());
		bgr.SetRenderTransformPivot(new Vector2(1.0, 1.0));
		bgr.Reparent(bar);

		let frame: ref<inkImage> = new inkImage();
		frame.SetName(n"frame");
		frame.SetAtlasResource(r"base\\gameplay\\gui\\widgets\\notifications\\notification_assets.inkatlas");
		frame.SetTexturePart(n"Plate_main_Stroke");
		frame.SetNineSliceScale(true);
		frame.SetAnchor(inkEAnchor.BottomFillHorizontaly);
		frame.SetAnchorPoint(new Vector2(1.0, 1.0));
		frame.SetOpacity(0.217);
		frame.SetTintColor(ThemeColors.Bittersweet());
		frame.SetRenderTransformPivot(new Vector2(1.0, 1.0));
		frame.Reparent(bar);

		let bracketBg: ref<inkImage> = new inkImage();
		bracketBg.SetName(n"bracketBg");
		bracketBg.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		bracketBg.SetTexturePart(n"notification_bracket_bg");
		bracketBg.SetNineSliceScale(true);
		bracketBg.SetFitToContent(true);
		bracketBg.SetHAlign(inkEHorizontalAlign.Left);
		bracketBg.SetAnchor(inkEAnchor.BottomFillHorizontaly);
		bracketBg.SetAnchorPoint(new Vector2(1.0, 1.0));
		bracketBg.SetOpacity(0.61);
		bracketBg.SetTintColor(ThemeColors.BlackPearl());
		bracketBg.SetRenderTransformPivot(new Vector2(1.0, 1.0));
		bracketBg.Reparent(header);

		let bracketBgr: ref<inkImage> = new inkImage();
		bracketBgr.SetName(n"bracketBgr");
		bracketBgr.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		bracketBgr.SetTexturePart(n"notification_bracket_bg");
		bracketBgr.SetNineSliceScale(true);
		bracketBgr.SetFitToContent(true);
		bracketBgr.SetHAlign(inkEHorizontalAlign.Left);
		bracketBgr.SetAnchor(inkEAnchor.BottomFillHorizontaly);
		bracketBgr.SetAnchorPoint(new Vector2(1.0, 1.0));
		bracketBgr.SetOpacity(0.217);
		bracketBgr.SetTintColor(ThemeColors.Bittersweet());
		bracketBgr.Reparent(header);

		let bracketFg: ref<inkImage> = new inkImage();
		bracketFg.SetName(n"bracketFg");
		bracketFg.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		bracketFg.SetTexturePart(n"notification_bracket_fg");
		bracketFg.SetNineSliceScale(true);
		bracketFg.SetFitToContent(true);
		bracketFg.SetHAlign(inkEHorizontalAlign.Left);
		bracketFg.SetAnchor(inkEAnchor.BottomFillHorizontaly);
		bracketFg.SetAnchorPoint(new Vector2(1.0, 1.0));
		bracketFg.SetOpacity(0.217);
		bracketFg.SetTintColor(ThemeColors.Bittersweet());
		bracketFg.Reparent(header);

		let title: ref<inkText> = new inkText();
		title.SetName(n"title");
		title.SetFontFamily("base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
		title.SetFontStyle(n"Medium");
		title.SetFontSize(50);
		title.SetLetterCase(textLetterCase.UpperCase);
		title.SetFitToContent(true);
		title.SetMargin(new inkMargin(48.0, 8.0, 200.0, 6.0));
		title.SetHAlign(inkEHorizontalAlign.Left);
		title.SetVAlign(inkEVerticalAlign.Center);
		title.SetTintColor(ThemeColors.ElectricBlue());
		title.Reparent(header);

		let fluffTextR: ref<inkText> = new inkText();
		fluffTextR.SetName(n"fluffTextR");
		fluffTextR.SetFontFamily("base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
		fluffTextR.SetFontStyle(n"Medium");
		fluffTextR.SetFontSize(20);
		fluffTextR.SetLetterCase(textLetterCase.UpperCase);
		fluffTextR.SetHorizontalAlignment(textHorizontalAlignment.Right);
		fluffTextR.SetFitToContent(true);
		fluffTextR.SetMargin(new inkMargin(0.0, -30.0, 0.0, 0.0));
		fluffTextR.SetHAlign(inkEHorizontalAlign.Right);
		fluffTextR.SetAnchor(inkEAnchor.TopRight);
		fluffTextR.SetTintColor(ThemeColors.Bittersweet());
		fluffTextR.SetRenderTransformPivot(new Vector2(1.0, 0.5));
		fluffTextR.Reparent(header);

		let fluffTextL: ref<inkText> = new inkText();
		fluffTextL.SetName(n"fluffTextL");
		fluffTextL.SetFontFamily("base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
		fluffTextL.SetFontStyle(n"Medium");
		fluffTextL.SetFontSize(20);
		fluffTextL.SetLetterCase(textLetterCase.UpperCase);
		fluffTextL.SetText("TRN_TCLAS_800095");
		fluffTextL.SetFitToContent(true);
		fluffTextL.SetMargin(new inkMargin(0.0, -30.0, 0.0, 0.0));
		fluffTextL.SetHAlign(inkEHorizontalAlign.Left);
		fluffTextL.SetAnchor(inkEAnchor.TopRight);
		fluffTextL.SetTintColor(ThemeColors.Bittersweet());
		fluffTextL.SetRenderTransformPivot(new Vector2(0.0, 0.5));
		fluffTextL.Reparent(header);

		this.m_title = title;
		this.m_fluffLeft = fluffTextL;
		this.m_fluffRight = fluffTextR;

		this.SetRootWidget(header);
	}

	public func SetTitle(text: String) -> Void {
		this.m_title.SetText(text);
	}

	public func SetFluffLeft(text: String) -> Void {
		this.m_fluffLeft.SetText(text);
	}

	public func SetFluffRight(text: String) -> Void {
		this.m_fluffRight.SetText(text);
	}

	public static func Create() -> ref<InGamePopupHeader> {
		let self: ref<InGamePopupHeader> = new InGamePopupHeader();
		self.CreateInstance();

		return self;
	}
}
