// -----------------------------------------------------------------------------
// Codeware.UI.HubButton
// -----------------------------------------------------------------------------
//
// - [TODO] Refactor layout (original layout used by game is not flexible)
// - [TODO] Menu mode (spawn menu on click)
// - [TODO] Menu style (as on main hub screen, bigger version)
// - [TODO] Badges (used on character button only)
//

module Codeware.UI

public class HubButton extends CustomButton {
	protected let m_icon: wref<inkImage>;

	protected let m_fluff: wref<inkImage>;

	protected let m_hover: wref<inkWidget>;

	protected let m_disabledRootAnimDef: ref<inkAnimDef>;

	protected let m_disabledRootAnimProxy: ref<inkAnimProxy>;

	protected let m_hoverFillAnimDef: ref<inkAnimDef>;

	protected let m_hoverFillAnimProxy: ref<inkAnimProxy>;

	protected func CreateWidgets() -> Void {
		let root: ref<inkCanvas> = new inkCanvas();
		root.SetName(n"button");
		root.SetSize(new Vector2(500.0, 100.0)); // Big Mode = 160.0
		root.SetAnchorPoint(new Vector2(0.5, 0.5));
		root.SetInteractive(true);

		let flexContainer: ref<inkFlex> = new inkFlex();
		flexContainer.SetName(n"flexContainer");
		flexContainer.SetMargin(new inkMargin(15.0, 0.0, 0.0, 0.0));
		flexContainer.Reparent(root);
		
		let background: ref<inkImage> = new inkImage();
		background.SetName(n"background");
		background.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		background.SetTexturePart(n"button_big2_bg");
		background.SetNineSliceScale(true);
		background.SetAnchorPoint(new Vector2(0.5, 0.5));
		background.SetOpacity(0.8);
		background.SetTintColor(new HDRColor(0.054902, 0.054902, 0.090196, 1.0));
		background.SetSize(new Vector2(532.0, 345.0));
		background.Reparent(flexContainer);
		
		let frame: ref<inkImage> = new inkImage();
		frame.SetName(n"frame");
		frame.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		frame.SetTexturePart(n"button_big2_fg");
		frame.SetNineSliceScale(true);
		frame.SetAnchorPoint(new Vector2(0.5, 0.5));
		frame.SetTintColor(new HDRColor(0.411765, 0.086275, 0.090196, 1.0));
		frame.SetSize(new Vector2(532.0, 345.0));
		frame.Reparent(flexContainer);
		
		let backgroundLeftBg: ref<inkImage> = new inkImage();
		backgroundLeftBg.SetName(n"background_leftBg");
		backgroundLeftBg.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		backgroundLeftBg.SetTexturePart(n"item_side_bg");
		backgroundLeftBg.SetNineSliceScale(true);
		backgroundLeftBg.SetMargin(new inkMargin(-15.0, 0.0, 0.0, 0.0));
		backgroundLeftBg.SetHAlign(inkEHorizontalAlign.Left);
		backgroundLeftBg.SetAnchorPoint(new Vector2(0.5, 0.5));
		backgroundLeftBg.SetOpacity(0.5);
		backgroundLeftBg.SetTintColor(new HDRColor(0.411765, 0.086275, 0.090196, 1.0));
		backgroundLeftBg.SetSize(new Vector2(16.0, 345.0));
		backgroundLeftBg.Reparent(flexContainer);
		
		let backgroundLeftFrame: ref<inkImage> = new inkImage();
		backgroundLeftFrame.SetName(n"background_leftFrame");
		backgroundLeftFrame.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		backgroundLeftFrame.SetTexturePart(n"item_side_fg");
		backgroundLeftFrame.SetNineSliceScale(true);
		backgroundLeftFrame.SetMargin(new inkMargin(-15.0, 0.0, 0.0, 0.0));
		backgroundLeftFrame.SetHAlign(inkEHorizontalAlign.Left);
		backgroundLeftFrame.SetAnchorPoint(new Vector2(0.5, 0.5));
		backgroundLeftFrame.SetTintColor(new HDRColor(0.411765, 0.086275, 0.090196, 1.0));
		backgroundLeftFrame.SetSize(new Vector2(16.0, 345.0));
		backgroundLeftFrame.Reparent(flexContainer);
		
		let container: ref<inkHorizontalPanel> = new inkHorizontalPanel();
		container.SetName(n"container");
		container.SetFitToContent(true);
		container.SetMargin(new inkMargin(0.0, 5.0, 0.0, 0.0));
		container.SetHAlign(inkEHorizontalAlign.Left);
		container.SetVAlign(inkEVerticalAlign.Top);
		container.Reparent(flexContainer);
		
		let inkVerticalPanelWidget8: ref<inkVerticalPanel> = new inkVerticalPanel();
		inkVerticalPanelWidget8.SetName(n"inkVerticalPanelWidget8");
		inkVerticalPanelWidget8.SetFitToContent(true);
		inkVerticalPanelWidget8.SetMargin(new inkMargin(20.0, 0.0, -10.0, 0.0));
		inkVerticalPanelWidget8.Reparent(container);
		
		let icon: ref<inkImage> = new inkImage();
		icon.SetName(n"icon");
		icon.SetAtlasResource(r"base\\gameplay\\gui\\fullscreen\\hub_menu\\hub_atlas.inkatlas");
		icon.SetHAlign(inkEHorizontalAlign.Center);
		icon.SetVAlign(inkEVerticalAlign.Center);
		icon.SetAnchorPoint(new Vector2(0.5, 0.5));
		icon.SetSizeRule(inkESizeRule.Stretch);
		icon.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
		icon.SetSize(new Vector2(80.0, 80.0));
		icon.SetScale(new Vector2(0.8, 0.8));
		icon.Reparent(inkVerticalPanelWidget8);
		
		let fluff: ref<inkImage> = new inkImage();
		fluff.SetName(n"fluff");
		fluff.SetAtlasResource(r"base\\gameplay\\gui\\common\\icons\\atlas_common.inkatlas");
		fluff.SetTexturePart(n"fluffcc35_3");
		fluff.SetFitToContent(true);
		fluff.SetMargin(new inkMargin(0.0, 0.0, 0.0, 16.0));
		fluff.SetHAlign(inkEHorizontalAlign.Center);
		fluff.SetVAlign(inkEVerticalAlign.Center);
		fluff.SetAnchorPoint(new Vector2(0.5, 0.5));
		fluff.SetOpacity(0.4);
		fluff.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
		fluff.SetSize(new Vector2(90.0, 80.0));
		fluff.Reparent(inkVerticalPanelWidget8);
		
		let label: ref<inkText> = new inkText();
		label.SetName(n"label");
		label.SetFontFamily("base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
		label.SetFontStyle(n"Semi-Bold");
		label.SetFontSize(48);
		label.SetLetterCase(textLetterCase.UpperCase);
		label.SetVerticalAlignment(textVerticalAlignment.Center);
		label.SetContentHAlign(inkEHorizontalAlign.Center);
		label.SetContentVAlign(inkEVerticalAlign.Center);
		label.SetText("STATS");
		label.SetMargin(new inkMargin(20.0, -5.0, 0.0, 0.0));
		label.SetHAlign(inkEHorizontalAlign.Left);
		label.SetVAlign(inkEVerticalAlign.Center);
		label.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
		label.SetSize(new Vector2(360.0, 120.0));
		label.Reparent(container);

		let hoverFrames: ref<inkFlex> = new inkFlex();
		hoverFrames.SetName(n"hoverFrames");
		hoverFrames.SetOpacity(0.0);
		hoverFrames.SetSize(new Vector2(100.0, 100.0));
		hoverFrames.Reparent(flexContainer);
		
		let frameHovered: ref<inkImage> = new inkImage();
		frameHovered.SetName(n"frameHovered");
		frameHovered.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		frameHovered.SetTexturePart(n"button_big2_fg");
		frameHovered.SetNineSliceScale(true);
		frameHovered.SetAnchorPoint(new Vector2(0.5, 0.5));
		frameHovered.SetTintColor(new HDRColor(1.1761, 0.3809, 0.3476, 1.0));
		frameHovered.SetSize(new Vector2(532.0, 345.0));
		frameHovered.Reparent(hoverFrames);
		
		let frameHoveredBg: ref<inkImage> = new inkImage();
		frameHoveredBg.SetName(n"frameHoveredBg");
		frameHoveredBg.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		frameHoveredBg.SetTexturePart(n"button_big2_bg");
		frameHoveredBg.SetNineSliceScale(true);
		frameHoveredBg.SetAnchorPoint(new Vector2(0.5, 0.5));
		frameHoveredBg.SetOpacity(0.05);
		frameHoveredBg.SetTintColor(new HDRColor(1.1761, 0.3809, 0.3476, 1.0));
		frameHoveredBg.SetSize(new Vector2(532.0, 345.0));
		frameHoveredBg.Reparent(hoverFrames);
		
		let backgroundLeftHover: ref<inkImage> = new inkImage();
		backgroundLeftHover.SetName(n"background_leftHover");
		backgroundLeftHover.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		backgroundLeftHover.SetTexturePart(n"item_side_fg");
		backgroundLeftHover.SetNineSliceScale(true);
		backgroundLeftHover.SetMargin(new inkMargin(-15.0, 0.0, 0.0, 0.0));
		backgroundLeftHover.SetHAlign(inkEHorizontalAlign.Left);
		backgroundLeftHover.SetAnchorPoint(new Vector2(0.5, 0.5));
		backgroundLeftHover.SetTintColor(new HDRColor(1.1761, 0.3809, 0.3476, 1.0));
		backgroundLeftHover.SetSize(new Vector2(16.0, 345.0));
		backgroundLeftHover.Reparent(hoverFrames);
		
		let backgroundLeftBg: ref<inkImage> = new inkImage();
		backgroundLeftBg.SetName(n"background_leftBg");
		backgroundLeftBg.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		backgroundLeftBg.SetTexturePart(n"item_side_bg");
		backgroundLeftBg.SetNineSliceScale(true);
		backgroundLeftBg.SetMargin(new inkMargin(-15.0, 0.0, 0.0, 0.0));
		backgroundLeftBg.SetHAlign(inkEHorizontalAlign.Left);
		backgroundLeftBg.SetAnchorPoint(new Vector2(0.5, 0.5));
		backgroundLeftBg.SetOpacity(0.65); // Big Mode = 0.05
		backgroundLeftBg.SetTintColor(new HDRColor(1.1761, 0.3809, 0.3476, 1.0));
		backgroundLeftBg.SetSize(new Vector2(16.0, 345.0));
		backgroundLeftBg.Reparent(hoverFrames);

		let minSize: ref<inkRectangle> = new inkRectangle();
		minSize.SetName(n"minSize");
		minSize.SetVisible(false);
		minSize.SetAffectsLayoutWhenHidden(true);
		minSize.SetHAlign(inkEHorizontalAlign.Left);
		minSize.SetVAlign(inkEVerticalAlign.Center);
		minSize.SetSize(new Vector2(485.0, 100.0));
		minSize.Reparent(flexContainer);

		this.m_root = root;
		this.m_label = label;
		this.m_icon = icon;
		this.m_fluff = fluff;
		this.m_hover = hoverFrames;

		this.SetRootWidget(root);
	}

	protected func CreateAnimations() -> Void {
		let disabledRootAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		disabledRootAlphaAnim.SetStartTransparency(1.0);
		disabledRootAlphaAnim.SetEndTransparency(0.3);
		disabledRootAlphaAnim.SetDuration(this.m_useAnimations ? 0.05 : 0.0001);

		this.m_disabledRootAnimDef = new inkAnimDef();
		this.m_disabledRootAnimDef.AddInterpolator(disabledRootAlphaAnim);

		let hoverFillAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		hoverFillAlphaAnim.SetStartTransparency(0.0);
		hoverFillAlphaAnim.SetEndTransparency(1.0);
		hoverFillAlphaAnim.SetDuration(this.m_useAnimations ? 0.2 : 0.0001);

		this.m_hoverFillAnimDef = new inkAnimDef();
		this.m_hoverFillAnimDef.AddInterpolator(hoverFillAlphaAnim);
	}

	protected func ApplyDisabledState() -> Void {
		let reverseAnimOpts: inkAnimOptions;
		reverseAnimOpts.playReversed = !this.m_isDisabled;

		this.m_disabledRootAnimProxy.Stop();
		this.m_disabledRootAnimProxy = this.m_root.PlayAnimationWithOptions(this.m_disabledRootAnimDef, reverseAnimOpts);
	}

	protected func ApplyHoveredState() -> Void {
		let reverseAnimOpts: inkAnimOptions;
		reverseAnimOpts.playReversed = !this.m_isHovered || this.m_isDisabled;

		this.m_hoverFillAnimProxy.Stop();
		this.m_hoverFillAnimProxy = this.m_hover.PlayAnimationWithOptions(this.m_hoverFillAnimDef, reverseAnimOpts);
	}

	protected func ApplyPressedState() -> Void {
		
	}

	public func SetIcon(icon: CName) -> Void {
		this.m_icon.SetTexturePart(icon);

		if NotEquals(icon, n"") {
			this.m_icon.SetVisible(true);
		} else {
			this.m_icon.SetVisible(false);
			this.m_fluff.SetVisible(false);
		}
	}

	public func SetIcon(icon: CName, atlas: ResRef) -> Void {
		this.m_icon.SetAtlasResource(atlas);
		this.SetIcon(icon);
	}

//	public func SetFluff(showFluff: Bool) -> Void {
//		this.m_fluff.SetVisible(showFluff);
//	}

	public static func Create() -> ref<HubButton> {
		let self: ref<HubButton> = new HubButton();
		self.CreateInstance();

		return self;
	}
}
