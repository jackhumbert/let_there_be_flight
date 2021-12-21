// -----------------------------------------------------------------------------
// Codeware.UI.TextMeasurer.Parts.TextMeasurer [WIP]
// -----------------------------------------------------------------------------
//
// We have no typography system available in scripts.
// This component extracts text metrics from rendering results.
//
// It can function in two modes:
// - Chunk Mode
//   Takes into account kerning, but comes with a slow initialization process
//   when the input widget has initial text. Since it seems that kerning is
//   not currently implemented in CP77, there is no need to actually use it.
// - Char Mode
//   Fast enough for measuting inititial text but ignores kerning.
//

module Codeware.UI.TextInput.Parts
import Codeware.UI.inkCustomController

public class TextMeasurer extends inkCustomController {
	protected let m_shadow: wref<inkText>;

	protected let m_isMeasuring: Bool;

	protected let m_isCharMode: Bool;

	protected let m_targetText: String;

	protected let m_targetLength: Int32;

	protected let m_targetPosition: Int32;

	protected let m_resultSize: Vector2;

	protected let m_tickProxy: ref<inkAnimProxy>;

	protected let m_useCharCache: Bool;

	protected let m_charCache: ref<inkStringMap>;

	protected cb func OnCreate() -> Void {
		this.InitializeProps();
		this.CreateWidgets();
	}

	protected cb func OnInitialize() -> Void {
		this.RegisterTick();
	}

	protected func InitializeProps() -> Void {
		this.m_useCharCache = true;
		this.m_charCache = new inkStringMap();
	}

	protected func CreateWidgets() -> Void {
		let shadow: ref<inkText> = new inkText();
		shadow.SetName(n"shadow");
		shadow.SetVisible(false);
		shadow.SetAffectsLayoutWhenHidden(true);
		shadow.SetFontFamily("base\\gameplay\\gui\\fonts\\raj\\raj.inkfontfamily");
		shadow.SetHorizontalAlignment(textHorizontalAlignment.Left);
		shadow.SetVerticalAlignment(textVerticalAlignment.Center);
		shadow.SetRenderTransformPivot(new Vector2(0.0, 0.0));

		this.m_shadow = shadow;

		this.SetRootWidget(shadow);
	}

	protected func RegisterTick() -> Void {
		let tickAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		tickAnim.SetStartTransparency(1.0);
		tickAnim.SetEndTransparency(1.0);
		tickAnim.SetDuration(1.0 / 60.0);

		let tickAnimDef: ref<inkAnimDef> = new inkAnimDef();
		tickAnimDef.AddInterpolator(tickAnim);

		let tickAnimOpts: inkAnimOptions;
		tickAnimOpts.loopInfinite = true;
		tickAnimOpts.loopType = inkanimLoopType.Cycle;

		this.m_tickProxy = this.m_shadow.PlayAnimationWithOptions(tickAnimDef, tickAnimOpts);
		this.m_tickProxy.RegisterToCallback(inkanimEventType.OnStartLoop, this, n"OnTick");
		this.m_tickProxy.Pause();
	}

	protected func MakeTargetChar() -> String {
		return StrMid(this.m_targetText, Min(StrLen(this.m_targetText), this.m_targetPosition) - 1, 1);
	}

	protected func MakeTargetChunk() -> String {
		return StrLeft(this.m_targetText, this.m_targetPosition);
	}

	protected func QueueMeasure(text: String, char: Bool, length: Int32, position: Int32) -> Bool {
		if !this.m_isMeasuring {
			this.m_isCharMode = char;
			this.m_targetText = text;
			this.m_targetLength = length;
			this.m_targetPosition = position;

			if this.m_isCharMode {
				if !this.MeasureWithCache() {
					this.m_isMeasuring = true;
					this.m_shadow.SetText(this.MakeTargetChar());
					this.m_tickProxy.Resume();
				}
			} else {
				this.m_isMeasuring = true;
				this.m_shadow.SetText(this.MakeTargetChunk());
				this.m_tickProxy.Resume();
			}
		}

		return false;
	}

	protected func MeasureWithCache() -> Bool {
		if !this.m_useCharCache {
			return false;
		}

		while this.m_targetPosition <= this.m_targetLength {
			let targetChar: String = this.MakeTargetChar();

			if !this.m_charCache.KeyExist(targetChar) {
				return false;
			}

			this.m_resultSize = new Vector2(
				Cast(this.m_charCache.Get(targetChar)),
				0.0
			);

			this.CallCustomCallback(n"OnCharMeasured");

			this.m_targetPosition += 1;
		}

		this.m_isMeasuring = false;
		this.m_targetPosition = this.m_targetLength;

		return true;
	}

	protected func AddResultToCache() -> Void {
		if this.m_useCharCache {
			this.m_charCache.Insert(this.m_shadow.GetText(), Cast(this.m_resultSize.X));
		}
	}

	protected cb func OnTick(anim: ref<inkAnimProxy>) -> Void {
		if !this.m_isMeasuring {
			this.m_tickProxy.Pause();
			return;
		}

		this.m_resultSize = this.m_shadow.GetDesiredSize();

		if this.m_resultSize.X < 0.01 {
			this.m_tickProxy.Resume();
			return;
		}

		if this.m_isCharMode {
			this.AddResultToCache();
			this.CallCustomCallback(n"OnCharMeasured");

			if this.m_targetPosition < this.m_targetLength {
				this.m_targetPosition += 1;

				if !this.MeasureWithCache() {
					this.m_shadow.SetText(this.MakeTargetChar());
					this.m_tickProxy.Resume();
					return;
				}
			}
		} else {
			this.CallCustomCallback(n"OnTextMeasured");

			if this.m_targetPosition < this.m_targetLength {
				this.m_targetPosition += 1;
				this.m_shadow.SetText(this.MakeTargetChunk());
				this.m_tickProxy.Resume();
				return;
			}
		}

		this.m_shadow.SetText("");
		this.m_tickProxy.Pause();

		this.m_isMeasuring = false;
	}

	public func IsMeasuring() -> Bool {
		return this.m_isMeasuring;
	}

	public func IsCharMode() -> Bool {
		return this.m_isCharMode;
	}

	public func GetTargetText() -> String {
		return this.m_targetText;
	}

	public func GetTargetLength() -> Int32 {
		return this.m_targetLength;
	}

	public func GetTargetPosition() -> Int32 {
		return this.m_targetPosition;
	}

	public func GetMeasuredSize() -> Vector2 {
		return this.m_resultSize;
	}

	public func CopyTextSettings(source: ref<inkText>) -> Void {
		if IsDefined(source) {
			this.m_shadow.SetFontStyle(source.GetFontStyle());
			this.m_shadow.SetFontSize(source.GetFontSize());
		}
	}

	public func CopyTextSettings(source: ref<inkCustomController>) -> Void {
		this.CopyTextSettings(source.GetRootWidget() as inkText);
	}

	public func MeasureChar(char: String, opt position: Int32) -> Bool {
		return this.QueueMeasure(char, true, position, position);
	}

	public func MeasureAllChars(text: String) -> Bool {
		return this.QueueMeasure(text, true, StrLen(text), 1);
	}

	public func MeasureChunk(text: String) -> Bool {
		return this.QueueMeasure(text, false, StrLen(text), 1);
	}

	public func MeasureChunk(text: String, position: Int32) -> Bool {
		return this.QueueMeasure(text, false, position, position);
	}

	public static func Create() -> ref<TextMeasurer> {
		let self: ref<TextMeasurer> = new TextMeasurer();
		self.CreateInstance();

		return self;
	}
}
