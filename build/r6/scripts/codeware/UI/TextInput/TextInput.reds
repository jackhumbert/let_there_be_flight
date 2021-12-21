// -----------------------------------------------------------------------------
// Codeware.UI.TextInput [WIP]
// -----------------------------------------------------------------------------
//
// Controls:
// - Supports most of the standard controls for text inputs
// - `Alt + Left` / `Alt + Right` to jump to the start and end
// - `Caps Lock` is currently not working as expected
//
// Events:
// - OnInput
//
// TODO:
// - Switch keyboard layouts
// - Navigate with `Ctrl + Left` / `Ctrl + Right`
// - Select text using mouse cursor
//

module Codeware.UI
import Codeware.UI.TextInput.Parts.*

public class TextInput extends inkCustomController {
	protected let m_root: wref<inkCompoundWidget>;

	protected let m_wrapper: wref<inkWidget>;

	protected let m_measurer: ref<TextMeasurer>;

	protected let m_viewport: ref<Viewport>;

	protected let m_selection: ref<Selection>;

	protected let m_text: ref<TextFlow>;

	protected let m_caret: ref<Caret>;

	protected let m_isDisabled: Bool;

	protected let m_isHovered: Bool;

	protected let m_isFocused: Bool;

	protected let m_inputEvents: ref<inkHashMap>;

	protected let m_lastInputEvent: ref<inkCharacterEvent>;

	protected let m_isHoldComplete: Bool;

	protected let m_holdTickCounter: Int32;

	protected let m_holdTickProxy: ref<inkAnimProxy>;

	protected cb func OnCreate() -> Void {
		this.InitializeProps();
		this.CreateWidgets();
		this.CreateAnimations();
	}

	protected cb func OnInitialize() -> Void {
		this.RegisterListeners();
		this.RegisterHoldTick();

		this.InitializeLayout();
		this.UpdateLayout();

		this.ApplyDisabledState();
		this.ApplyHoveredState();
		this.ApplyFocusedState();
	}

	protected func InitializeProps() -> Void {
		this.m_inputEvents = new inkHashMap();
	}

	protected func CreateWidgets() -> Void {
		let root: ref<inkCanvas> = new inkCanvas();
		root.SetName(n"input");
		root.SetSize(600.0, 64.0);
		root.SetAnchor(inkEAnchor.TopLeft);
		root.SetInteractive(true);
		root.SetSupportFocus(true);

		this.m_root = root;

		this.m_measurer = TextMeasurer.Create();
		this.m_measurer.Reparent(this.m_root);

		this.m_viewport = Viewport.Create();
		this.m_viewport.Reparent(this.m_root);

		this.m_selection = Selection.Create();
		this.m_selection.Reparent(this.m_viewport);

		this.m_text = TextFlow.Create();
		this.m_text.Reparent(this.m_viewport);

		this.m_caret = Caret.Create();
		this.m_caret.Reparent(this.m_viewport);

		this.m_wrapper = this.m_viewport.GetRootWidget();

		this.SetRootWidget(this.m_root);
	}

	protected func CreateAnimations() -> Void {

	}

	protected func InitializeLayout() -> Void {
		this.m_caret.SetFontSize(this.m_text.GetFontSize());
		this.m_selection.SetFontSize(this.m_text.GetFontSize());
		this.m_viewport.SetCaretSize(this.m_caret.GetSize());
		this.m_measurer.CopyTextSettings(this.m_text);
	}

	protected func UpdateLayout() -> Void {
		let contentSize: Vector2 = this.m_text.GetDesiredSize();
		let selectedBounds: RectF = this.m_text.GetCharRange(this.m_selection.GetRange());
		let caretOffset: Float = this.m_text.GetCharOffset(this.m_caret.GetPosition());

		this.m_viewport.UpdateState(contentSize, caretOffset);
		this.m_selection.UpdateState(this.m_isFocused, selectedBounds);
		this.m_caret.UpdateState(this.m_isFocused, caretOffset);
	}

	protected func ApplyDisabledState() -> Void {

	}

	protected func ApplyHoveredState() -> Void {

	}

	protected func ApplyFocusedState() -> Void {
		
	}

	protected func SetDisabledState(isDisabled: Bool) -> Void {
		if !Equals(this.m_isDisabled, isDisabled) {
			this.m_isDisabled = isDisabled;

			if this.m_isDisabled {
				this.m_isHovered = false;
				this.m_isFocused = false;
			}

			this.ApplyDisabledState();
			this.ApplyHoveredState();
			this.ApplyFocusedState();

			this.UpdateLayout();
		}
	}

	protected func SetHoveredState(isHovered: Bool) -> Void {
		if !Equals(this.m_isHovered, isHovered) {
			this.m_isHovered = isHovered;

			if !this.m_isDisabled {
				this.ApplyHoveredState();
			}
		}
	}

	protected func SetFocusedState(isFocused: Bool) -> Void {
		if !Equals(this.m_isFocused, isFocused) {
			this.m_isFocused = isFocused;

			if !this.m_isDisabled {
				this.ApplyFocusedState();

				this.UpdateLayout();
			}
		}
	}

	protected func RegisterListeners() -> Void {
		this.RegisterToCallback(n"OnEnter", this, n"OnHoverOver");
		this.RegisterToCallback(n"OnLeave", this, n"OnHoverOut");
		this.RegisterToCallback(n"OnFocusReceived", this, n"OnFocusReceived");
		this.RegisterToCallback(n"OnFocusLost", this, n"OnFocusLost");

		//this.RegisterToCallback(n"OnPress", this, n"OnPressKey");
		this.RegisterToCallback(n"OnRelease", this, n"OnReleaseKey");
		this.RegisterToCallback(n"OnCharacterKey", this, n"OnCharacterKey");

		this.m_measurer.RegisterToCallback(n"OnTextMeasured", this, n"OnTextMeasured");
		this.m_measurer.RegisterToCallback(n"OnCharMeasured", this, n"OnTextMeasured");
	}

	protected func RegisterHoldTick() -> Void {
		let tickAnim: ref<inkAnimTextValueProgress> = new inkAnimTextValueProgress();
		tickAnim.SetStartProgress(0.0);
		tickAnim.SetEndProgress(0.0);
		tickAnim.SetDuration(1.0 / 60.0);

		let tickAnimDef: ref<inkAnimDef> = new inkAnimDef();
		tickAnimDef.AddInterpolator(tickAnim);

		let tickAnimOpts: inkAnimOptions;
		tickAnimOpts.loopInfinite = true;
		tickAnimOpts.loopType = inkanimLoopType.Cycle;

		this.m_holdTickProxy = this.m_root.PlayAnimationWithOptions(tickAnimDef, tickAnimOpts);
		this.m_holdTickProxy.RegisterToCallback(inkanimEventType.OnStartLoop, this, n"OnHoldTick");
	}

	protected func TranslateChar(code: Int32, opt isCapsLocked: Bool) -> String {
		let char: String = StrChar(code);

		switch this.m_text.GetLetterCase() {
			case textLetterCase.UpperCase:
				char = StrUpper(char);
				break;
			case textLetterCase.LowerCase:
				char = StrLower(char);
				break;
			default:
				if isCapsLocked {
					char = StrUpper(char);
				}
		}

		return char;
	}

	protected func ProcessInputEvent(event: ref<inkCharacterEvent>) -> Void {
		switch event.GetType() {
			case inkCharacterEventType.CharInput:
				if this.m_text.IsFull() {
					break;
				}

				if this.m_measurer.IsMeasuring() {
					break;
				}

				if event.IsAltDown() {
					// TODO: Alt + ... -- Switch layouts
					break;
				}

				if event.IsControlDown() {
					let code: Int32 = Cast(event.GetCharacter());

					// Ctrl + A
					if code == 64 || code == 97 {
						this.m_selection.SelectAll();
						this.m_caret.MoveToStart();
						this.UpdateLayout();
					}
					break;
				}

				// NOTE: Caps Lock modifier returns holding state instead of the lock on / off state.

				let code: Int32 = Cast(event.GetCharacter());
				let char: String = this.TranslateChar(code, event.IsCapsLocked());

				if !this.m_selection.IsEmpty() {
					this.m_text.DeleteCharRange(
						this.m_selection.GetLeftPosition(),
						this.m_selection.GetRightPosition()
					);
				}

				this.m_text.InsertCharAt(this.m_caret.GetPosition(), char);

				this.m_caret.SetMaxPosition(this.m_text.GetLength());
				this.m_caret.MoveToNextChar();

				this.m_selection.SetMaxPosition(this.m_text.GetLength());
				this.m_selection.Clear();

				this.m_measurer.MeasureChar(char, this.m_caret.GetPosition());
				//this.m_measurer.MeasureChunk(this.m_text.GetText(), this.m_caret.GetPosition());
				break;

			case inkCharacterEventType.Delete:
				if this.m_measurer.IsMeasuring() {
					break;
				}

				if !this.m_selection.IsEmpty() {
					this.m_caret.SetPosition(this.m_selection.GetLeftPosition());
					this.m_text.DeleteCharRange(
						this.m_selection.GetLeftPosition(),
						this.m_selection.GetRightPosition()
					);
				} else {
					if !this.m_caret.IsAtEnd() {
						this.m_text.DeleteCharAt(this.m_caret.GetPosition());
					} else {
						return;
					}
				}

				this.m_caret.SetMaxPosition(this.m_text.GetLength());

				this.m_selection.SetMaxPosition(this.m_text.GetLength());
				this.m_selection.Clear();

				this.UpdateLayout();
				this.TriggerChangeCallback();
				break;

			case inkCharacterEventType.Backspace:
				if this.m_measurer.IsMeasuring() {
					break;
				}

				if !this.m_selection.IsEmpty() {
					this.m_caret.SetPosition(this.m_selection.GetLeftPosition());
					this.m_text.DeleteCharRange(
						this.m_selection.GetLeftPosition(),
						this.m_selection.GetRightPosition()
					);
				} else {
					if !this.m_caret.IsAtStart() {
						this.m_caret.MoveToPrevChar();
						this.m_text.DeleteCharAt(this.m_caret.GetPosition());
					} else {
						return;
					}
				}

				this.m_caret.SetMaxPosition(this.m_text.GetLength());

				this.m_selection.SetMaxPosition(this.m_text.GetLength());
				this.m_selection.Clear();

				this.UpdateLayout();
				this.TriggerChangeCallback();
				break;

			case inkCharacterEventType.MoveCaretForward:
				if this.m_measurer.IsMeasuring() {
					break;
				}

				if this.m_caret.IsAtEnd() {
					break;
				}

				if event.IsShiftDown() && this.m_selection.IsEmpty() {
					this.m_selection.SetStartPosition(
						this.m_caret.GetPosition()
					);
				}

				if event.IsAltDown() {
					this.m_caret.MoveToEnd();
				} else {
					if event.IsControlDown() {
						let stop: Int32 = this.m_text.GetNextStop(this.m_caret.GetPosition());
						if stop >= 0 {
							this.m_caret.SetPosition(stop);
						} else {
							this.m_caret.MoveToEnd();
						}
					} else {
						this.m_caret.MoveToNextChar();
					}
				}

				if event.IsShiftDown() {
					this.m_selection.SetEndPosition(
						this.m_caret.GetPosition()
					);
				} else {
					this.m_selection.Clear();
				}

				this.UpdateLayout();
				break;

			case inkCharacterEventType.MoveCaretBackward:
				if this.m_measurer.IsMeasuring() {
					break;
				}

				if this.m_caret.IsAtStart() {
					break;
				}

				if event.IsShiftDown() && this.m_selection.IsEmpty() {
					this.m_selection.SetStartPosition(
						this.m_caret.GetPosition()
					);
				}

				if event.IsAltDown() {
					this.m_caret.MoveToStart();
				} else {
					if event.IsControlDown() {
						let stop: Int32 = this.m_text.GetPrevStop(this.m_caret.GetPosition());
						if stop >= 0 {
							this.m_caret.SetPosition(stop);
						} else {
							this.m_caret.MoveToStart();
						}
					} else {
						this.m_caret.MoveToPrevChar();
					}
				}

				if event.IsShiftDown() {
					this.m_selection.SetEndPosition(
						this.m_caret.GetPosition()
					);
				} else {
					this.m_selection.Clear();
				}

				this.UpdateLayout();
				break;
		}
	}

	protected func TriggerChangeCallback() -> Void {
		this.CallCustomCallback(n"OnInput");
	}

	protected func GetEventHash(event: ref<inkCharacterEvent>) -> Uint64 {
		return Cast<Uint64>(1000 * EnumInt(event.GetType())) + Cast<Uint64>(event.GetCharacter());
	}

	protected cb func OnCharacterKey(event: ref<inkCharacterEvent>) -> Void {
		// NOTE: inkCharacterEvent currently has no PRESS / RELEASE indication.
		// We use a hash map to detect if the exact same event was sent before.
		// If event is not in the map, then it's a PRESS, otherwise it's a RELEASE.

		let eventHash: Uint64 = this.GetEventHash(event);

		if !this.m_inputEvents.KeyExist(eventHash) {
			this.ProcessInputEvent(event);

			this.m_lastInputEvent = event;
			this.m_inputEvents.Insert(eventHash, event);
		} else {
			this.m_inputEvents.Remove(eventHash);

			if eventHash == this.GetEventHash(this.m_lastInputEvent) {
				this.m_lastInputEvent = null;
			}
		}

		this.m_isHoldComplete = false;
		this.m_holdTickCounter = 0;
	}

	protected cb func OnHoldTick(anim: ref<inkAnimProxy>) -> Void {
		if IsDefined(this.m_lastInputEvent) {
			this.m_holdTickCounter += 1;

			if this.m_holdTickCounter == (this.m_isHoldComplete ? 2 : 30) {
				this.ProcessInputEvent(this.m_lastInputEvent);

				this.m_isHoldComplete = true;
				this.m_holdTickCounter = 0;
			}
		}
	}

	protected cb func OnReleaseKey(event: ref<inkPointerEvent>) -> Void {
		if this.m_isFocused && !this.m_measurer.IsMeasuring() {
			if event.IsAction(n"mouse_left") {
				let clickPoint: Vector2 = WidgetUtils.GlobalToLocal(this.m_text.GetRootWidget(), event.GetScreenSpacePosition());
				let clickPosition: Int32 = this.m_text.GetCharPosition(clickPoint.X);

				this.m_caret.SetPosition(clickPosition);
				this.m_selection.Clear();

				this.UpdateLayout();
			}
		}
	}

	protected cb func OnTextMeasured(widget: ref<inkWidget>) -> Void {
		let measuredPosition: Int32 = this.m_measurer.GetTargetPosition();
		let measuredSize: Vector2 = this.m_measurer.GetMeasuredSize();

		if this.m_measurer.IsCharMode() {
			this.m_text.SetCharWidth(measuredPosition, measuredSize.X);
		} else {
			this.m_text.SetCharOffset(measuredPosition, measuredSize.X);
		}

		// Character insertion
		if this.m_caret.IsAt(measuredPosition) {
			this.UpdateLayout();
			this.TriggerChangeCallback();
		}
	}

	protected cb func OnHoverOver(event: ref<inkPointerEvent>) -> Bool {
		this.SetHoveredState(true);
	}

	protected cb func OnHoverOut(event: ref<inkPointerEvent>) -> Bool {
		this.SetHoveredState(false);
	}

	protected cb func OnFocusReceived(event: ref<inkEvent>) -> Void {
		this.SetFocusedState(true);
	}

	protected cb func OnFocusLost(event: ref<inkEvent>) -> Void {
		this.SetFocusedState(false);
	}

	public func GetName() -> CName {
		return this.m_root.GetName();
	}

	public func SetName(name: CName) -> Void {
		this.m_root.SetName(name);
	}

	public func GetText() -> String {
		return this.m_text.GetText();
	}

	public func SetText(text: String) -> Void {
		this.m_text.SetText(text);

		this.m_measurer.MeasureAllChars(text);
		//this.m_measurer.MeasureChunk(text);

		this.m_selection.SetMaxPosition(this.m_text.GetLength());
		this.m_selection.Clear();

		this.m_caret.SetMaxPosition(this.m_text.GetLength());
		this.m_caret.MoveToStart();

		this.UpdateLayout();
	}

	public func GetMaxLength() -> Int32 {
		return this.m_text.GetMaxLength();
	}

	public func SetMaxLength(max: Int32) -> Void {
		this.m_text.SetMaxLength(max);
	}

	public func GetLetterCase() -> textLetterCase {
		return this.m_text.GetLetterCase();
	}

	public func SetLetterCase(case: textLetterCase) -> Void {
		this.m_text.SetLetterCase(case);
	}

	public func GetWidth() -> Float {
		return this.m_root.GetWidth();
	}

	public func SetWidth(width: Float) -> Void {
		this.m_root.SetWidth(width);

		this.UpdateLayout();
	}

	public func GetCaretPosition() -> Int32 {
		return this.m_caret.GetPosition();
	}

	public func SetCaretPosition(position: Int32) -> Void {
		this.m_caret.SetPosition(position);

		this.UpdateLayout();
	}

	public func IsHovered() -> Bool {
		return this.m_isHovered && !this.m_isDisabled;
	}

	public func IsFocused() -> Bool {
		return this.m_isFocused && !this.m_isDisabled;
	}

	public func IsDisabled() -> Bool {
		return this.m_isDisabled;
	}

	public func IsEnabled() -> Bool {
		return !this.m_isDisabled;
	}

	public func SetDisabled(isDisabled: Bool) -> Void {
		this.SetDisabledState(isDisabled);

		this.UpdateLayout();
	}

	// TODO: General positioning and transformations

	public static func Create() -> ref<TextInput> {
		let self: ref<TextInput> = new TextInput();
		self.CreateInstance();

		return self;
	}
}
