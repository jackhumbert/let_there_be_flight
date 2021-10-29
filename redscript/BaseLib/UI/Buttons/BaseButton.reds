// -----------------------------------------------------------------------------
// BaseButton
// -----------------------------------------------------------------------------

module BaseLib.UI

public class BaseButton extends CustomButton {
	protected let m_isFlipped: Bool;

	protected let m_bg: wref<inkImage>;

	protected let m_fill: wref<inkImage>;

	protected let m_frame: wref<inkImage>;

	protected let m_disabledRootAnimDef: ref<inkAnimDef>;

	protected let m_disabledRootAnimProxy: ref<inkAnimProxy>;

	protected let m_hoverFillAnimDef: ref<inkAnimDef>;

	protected let m_hoverFillAnimProxy: ref<inkAnimProxy>;

	protected let m_hoverFrameAnimDef: ref<inkAnimDef>;

	protected let m_hoverFrameAnimProxy: ref<inkAnimProxy>;

	protected let m_pressedFillAnimDef: ref<inkAnimDef>;

	protected let m_pressedFillAnimProxy: ref<inkAnimProxy>;

	protected cb func OnCreate() -> Void {
		super.OnCreate();

		this.ApplyFlippedState();
	}

	protected func CreateWidgets() -> Void {
		let root: ref<inkCanvas> = new inkCanvas();
		root.SetName(n"button");
		root.SetSize(400.0, 100.0);
		root.SetAnchorPoint(new Vector2(0.5, 0.5));
		root.SetInteractive(true);
		root.SetController(this);

		let bg: ref<inkImage> = new inkImage();
		bg.SetName(n"bg");
		bg.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		bg.SetTintColor(ThemeColors.BlackPearl());
		bg.SetOpacity(0.8);
		bg.SetAnchor(inkEAnchor.Fill);
		bg.SetNineSliceScale(true);
		bg.SetNineSliceGrid(new inkMargin(0.0, 0.0, 10.0, 0.0));
		bg.Reparent(root);

		let fill: ref<inkImage> = new inkImage();
		fill.SetName(n"fill");
		fill.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		fill.SetOpacity(0.0);
		fill.SetAnchor(inkEAnchor.Fill);
		fill.SetNineSliceScale(true);
		fill.SetNineSliceGrid(new inkMargin(0.0, 0.0, 10.0, 0.0));
		fill.Reparent(root);

		let frame: ref<inkImage> = new inkImage();
		frame.SetName(n"frame");
		frame.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		frame.SetOpacity(1.0);
		frame.SetAnchor(inkEAnchor.Fill);
		frame.SetNineSliceScale(true);
		frame.SetNineSliceGrid(new inkMargin(0.0, 0.0, 10.0, 0.0));
		frame.Reparent(root);

		let label: ref<inkText> = new inkText();
		label.SetName(n"label");
		label.SetFontFamily("base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
		label.SetFontStyle(n"Medium");
		label.SetFontSize(50);
		label.SetLetterCase(textLetterCase.UpperCase);
		label.SetTintColor(ThemeColors.ElectricBlue());
		label.SetAnchor(inkEAnchor.Fill);
		label.SetHorizontalAlignment(textHorizontalAlignment.Center);
		label.SetVerticalAlignment(textVerticalAlignment.Center);
		label.SetText("BUTTON");
		label.Reparent(root);

		this.m_root = root;
		this.m_label = label;
		this.m_bg = bg;
		this.m_fill = fill;
		this.m_frame = frame;

		this.SetRootWidget(root);
	}

	protected func CreateAnimations() -> Void {
		let disabledRootAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		disabledRootAlphaAnim.SetStartTransparency(1.0);
		disabledRootAlphaAnim.SetEndTransparency(0.3);
		disabledRootAlphaAnim.SetDuration(this.m_useAnimations ? 0.05 : 0.0001);

		this.m_disabledRootAnimDef = new inkAnimDef();
		this.m_disabledRootAnimDef.AddInterpolator(disabledRootAlphaAnim);

		let hoverFillColorAnim: ref<inkAnimColor> = new inkAnimColor();
		hoverFillColorAnim.SetStartColor(ThemeColors.BlackPearl());
		hoverFillColorAnim.SetEndColor(ThemeColors.Bittersweet());
		hoverFillColorAnim.SetDuration(this.m_useAnimations ? 0.2 : 0.0001);

		let hoverFillAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		hoverFillAlphaAnim.SetStartTransparency(0.0);
		hoverFillAlphaAnim.SetEndTransparency(0.3);
		hoverFillAlphaAnim.SetDuration(this.m_useAnimations ? 0.05 : 0.0001);

		this.m_hoverFillAnimDef = new inkAnimDef();
		this.m_hoverFillAnimDef.AddInterpolator(hoverFillColorAnim);
		this.m_hoverFillAnimDef.AddInterpolator(hoverFillAlphaAnim);

		let hoverFrameColorAnim: ref<inkAnimColor> = new inkAnimColor();
		hoverFrameColorAnim.SetStartColor(ThemeColors.RedOxide());
		hoverFrameColorAnim.SetEndColor(ThemeColors.ElectricBlue());
		hoverFrameColorAnim.SetDuration(this.m_useAnimations ? 0.2 : 0.0001);

		this.m_hoverFrameAnimDef = new inkAnimDef();
		this.m_hoverFrameAnimDef.AddInterpolator(hoverFrameColorAnim);

		let pressedFillAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		pressedFillAlphaAnim.SetStartTransparency(0.3);
		pressedFillAlphaAnim.SetEndTransparency(0.4);
		pressedFillAlphaAnim.SetDuration(this.m_useAnimations ? 0.05 : 0.0001);

		this.m_pressedFillAnimDef = new inkAnimDef();
		this.m_pressedFillAnimDef.AddInterpolator(pressedFillAlphaAnim);
	}

	protected func ApplyFlippedState() -> Void {
		this.m_bg.SetTexturePart(this.m_isFlipped ? n"cell_flip_bg" : n"cell_bg");
		this.m_fill.SetTexturePart(this.m_isFlipped ? n"cell_flip_bg" : n"cell_bg");
		this.m_frame.SetTexturePart(this.m_isFlipped ? n"cell_flip_fg" : n"cell_fg");
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
		this.m_hoverFillAnimProxy = this.m_fill.PlayAnimationWithOptions(this.m_hoverFillAnimDef, reverseAnimOpts);

		this.m_hoverFrameAnimProxy.Stop();
		this.m_hoverFrameAnimProxy = this.m_frame.PlayAnimationWithOptions(this.m_hoverFrameAnimDef, reverseAnimOpts);
	}

	protected func ApplyPressedState() -> Void {
		let reverseAnimOpts: inkAnimOptions;
		reverseAnimOpts.playReversed = !this.m_isPressed || this.m_isDisabled;

		this.m_pressedFillAnimProxy.Stop();
		this.m_pressedFillAnimProxy = this.m_fill.PlayAnimationWithOptions(this.m_pressedFillAnimDef, reverseAnimOpts);
	}

	public func SetFlipped(isFlipped: Bool) -> Void {
		this.m_isFlipped = isFlipped;

		this.ApplyFlippedState();
	}

	public static func Create() -> ref<BaseButton> {
		let instance: ref<BaseButton> = new BaseButton();
		instance.Build();

		return instance;
	}
}
