// -----------------------------------------------------------------------------
// Codeware.UI.HubTextInput [WIP]
// -----------------------------------------------------------------------------
//
// Mimics the looks of the filters and sorting inputs in Hub menus.
//

module Codeware.UI
import Codeware.UI.TextInput.Parts.*

public class HubTextInput extends TextInput {

	protected let m_bg: wref<inkImage>;

	protected let m_fill: wref<inkImage>;

	protected let m_frame: wref<inkImage>;

	protected let m_hover: wref<inkImage>;

	protected let m_focus: wref<inkImage>;

	protected let m_useAnimations: Bool;

	protected let m_activeRootAnimDef: ref<inkAnimDef>;

	protected let m_activeRootAnimProxy: ref<inkAnimProxy>;

	protected let m_hoverFrameAnimDef: ref<inkAnimDef>;

	protected let m_hoverFrameAnimProxy: ref<inkAnimProxy>;

	protected let m_focusFillAnimDef: ref<inkAnimDef>;

	protected let m_focusFillAnimProxy: ref<inkAnimProxy>;

	protected let m_focusFrameAnimDef: ref<inkAnimDef>;

	protected let m_focusFrameAnimProxy: ref<inkAnimProxy>;

	protected func CreateWidgets() -> Void {
		super.CreateWidgets();

		let fontSize: Int32 = 36;
		let inputHeight: Float = 74.0;
		let textPadding: Vector2 = new Vector2(18.0, (inputHeight - Cast<Float>(fontSize)) / 2.0 - 1.0);

		this.m_text.SetFontSize(fontSize);
		this.m_root.SetHeight(inputHeight);
		this.m_wrapper.SetMargin(new inkMargin(textPadding.X, textPadding.Y, textPadding.X, 0.0));

		// filter3_bg / filter3_fg / 80
		// sorting_bg / sorting_fg / 74
		let fillPart: CName = n"sorting_bg";
		let framePart: CName = n"sorting_fg";

		let theme: ref<inkFlex> = new inkFlex();
		theme.SetName(n"theme");
		theme.SetAnchor(inkEAnchor.Fill);
		theme.Reparent(this.m_root, 0);

		let bg: ref<inkImage> = new inkImage();
		bg.SetName(n"bg");
		bg.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		bg.SetTexturePart(fillPart);
		bg.SetTintColor(ThemeColors.BlackPearl());
		bg.SetOpacity(0.61);
		bg.SetAnchor(inkEAnchor.Fill);
		bg.SetNineSliceScale(true);
		bg.SetNineSliceGrid(new inkMargin(50.0, 30.0, 100.0, 30.0));
		bg.Reparent(theme);

		let fill: ref<inkImage> = new inkImage();
		fill.SetName(n"fill");
		fill.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		fill.SetTexturePart(fillPart);
		fill.SetTintColor(ThemeColors.ElectricBlue());
		fill.SetAnchor(inkEAnchor.Fill);
		fill.SetNineSliceScale(true);
		fill.SetNineSliceGrid(new inkMargin(50.0, 30.0, 100.0, 30.0));
		fill.Reparent(theme);

		let frame: ref<inkImage> = new inkImage();
		frame.SetName(n"frame");
		frame.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		frame.SetTexturePart(framePart);
		frame.SetTintColor(ThemeColors.RedOxide());
		frame.SetAnchor(inkEAnchor.Fill);
		frame.SetNineSliceScale(true);
		frame.SetNineSliceGrid(new inkMargin(50.0, 30.0, 100.0, 30.0));
		frame.Reparent(theme);

		let hover: ref<inkImage> = new inkImage();
		hover.SetName(n"hover");
		hover.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		hover.SetTexturePart(framePart);
		hover.SetTintColor(ThemeColors.Bittersweet());
		hover.SetAnchor(inkEAnchor.Fill);
		hover.SetNineSliceScale(true);
		hover.SetNineSliceGrid(new inkMargin(50.0, 30.0, 100.0, 30.0));
		hover.Reparent(theme);

		let focus: ref<inkImage> = new inkImage();
		focus.SetName(n"focus");
		focus.SetAtlasResource(r"base\\gameplay\\gui\\common\\shapes\\atlas_shapes_sync.inkatlas");
		focus.SetTexturePart(framePart);
		focus.SetTintColor(ThemeColors.ElectricBlue());
		focus.SetAnchor(inkEAnchor.Fill);
		focus.SetNineSliceScale(true);
		focus.SetNineSliceGrid(new inkMargin(50.0, 30.0, 100.0, 30.0));
		focus.Reparent(theme);

		this.m_fill = fill;
		this.m_frame = frame;
		this.m_hover = hover;
		this.m_focus = focus;
	}

	protected func CreateAnimations() -> Void {
		super.CreateAnimations();

		let activeRootAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		activeRootAlphaAnim.SetStartTransparency(1.0);
		activeRootAlphaAnim.SetEndTransparency(0.3);
		activeRootAlphaAnim.SetDuration(this.m_useAnimations ? 0.05 : 0.0001);

		this.m_activeRootAnimDef = new inkAnimDef();
		this.m_activeRootAnimDef.AddInterpolator(activeRootAlphaAnim);

		let hoverFrameAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		hoverFrameAlphaAnim.SetStartTransparency(0.0);
		hoverFrameAlphaAnim.SetEndTransparency(0.6);
		hoverFrameAlphaAnim.SetDuration(this.m_useAnimations ? 0.15 : 0.0001);

		this.m_hoverFrameAnimDef = new inkAnimDef();
		this.m_hoverFrameAnimDef.AddInterpolator(hoverFrameAlphaAnim);

		let focusFillAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		focusFillAlphaAnim.SetStartTransparency(0.0);
		focusFillAlphaAnim.SetEndTransparency(0.02);
		focusFillAlphaAnim.SetDuration(this.m_useAnimations ? 0.1 : 0.0001);

		this.m_focusFillAnimDef = new inkAnimDef();
		this.m_focusFillAnimDef.AddInterpolator(focusFillAlphaAnim);

		let focusFrameAlphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		focusFrameAlphaAnim.SetStartTransparency(0.0);
		focusFrameAlphaAnim.SetEndTransparency(1.0);
		focusFrameAlphaAnim.SetDuration(this.m_useAnimations ? 0.15 : 0.0001);

		this.m_focusFrameAnimDef = new inkAnimDef();
		this.m_focusFrameAnimDef.AddInterpolator(focusFrameAlphaAnim);
	}

	protected func ApplyDisabledState() -> Void {
		let reverseAnimOpts: inkAnimOptions;
		reverseAnimOpts.playReversed = !this.m_isDisabled;

		this.m_activeRootAnimProxy.Stop();
		this.m_activeRootAnimProxy = this.m_root.PlayAnimationWithOptions(this.m_activeRootAnimDef, reverseAnimOpts);
	}

	protected func ApplyHoveredState() -> Void {
		let reverseAnimOpts: inkAnimOptions;
		reverseAnimOpts.playReversed = !this.m_isHovered || this.m_isDisabled;

		this.m_hoverFrameAnimProxy.Stop();
		this.m_hoverFrameAnimProxy = this.m_hover.PlayAnimationWithOptions(this.m_hoverFrameAnimDef, reverseAnimOpts);
	}

	protected func ApplyFocusedState() -> Void {
		let reverseAnimOpts: inkAnimOptions;
		reverseAnimOpts.playReversed = !this.m_isFocused || this.m_isDisabled;

		this.m_focusFillAnimProxy.Stop();
		this.m_focusFillAnimProxy = this.m_fill.PlayAnimationWithOptions(this.m_focusFillAnimDef, reverseAnimOpts);

		this.m_focusFrameAnimProxy.Stop();
		this.m_focusFrameAnimProxy = this.m_focus.PlayAnimationWithOptions(this.m_focusFrameAnimDef, reverseAnimOpts);
	}

	public func ToggleAnimations(useAnimations: Bool) -> Void {
		this.m_useAnimations = useAnimations;
		this.CreateAnimations();
	}

	public static func Create() -> ref<HubTextInput> {
		let self: ref<HubTextInput> = new HubTextInput();
		self.m_useAnimations = true;
		self.CreateInstance();

		return self;
	}
}
