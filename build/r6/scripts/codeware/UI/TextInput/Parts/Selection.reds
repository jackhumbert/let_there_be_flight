// -----------------------------------------------------------------------------
// Codeware.UI.TextInput.Parts.Selection [WIP]
// -----------------------------------------------------------------------------

module Codeware.UI.TextInput.Parts
import Codeware.UI.inkCustomController
import Codeware.UI.ThemeColors

public class Selection extends inkCustomController {
	protected let m_selection: wref<inkRectangle>;

	protected let m_startPosition: Int32;

	protected let m_endPosition: Int32;

	protected let m_maxPosition: Int32;

	protected let m_fontSize: Float;

	protected let m_padSize: Float;

	protected cb func OnCreate() -> Void {
		this.InitializeProps();
		this.CreateWidgets();
	}

	protected cb func OnInitialize() -> Void {
		this.InitializeLayout();
	}

	protected func InitializeProps() -> Void {
		this.m_startPosition = -1;
		this.m_endPosition = -1;
		this.m_padSize = 6.0;
	}

	protected func CreateWidgets() -> Void {
		let selection: ref<inkRectangle> = new inkRectangle();
		selection.SetName(n"selection");
		selection.SetVisible(false);
		selection.SetOpacity(0.2);
		selection.SetTintColor(ThemeColors.ElectricBlue());
		selection.SetRenderTransformPivot(new Vector2(0.0, 0.0));

		this.m_selection = selection;

		this.SetRootWidget(this.m_selection);
	}

	protected func InitializeLayout() -> Void {
		this.m_selection.SetHeight(this.m_fontSize + this.m_padSize * 2);
	}

	public func GetFontSize() -> Int32 {
		return Cast(this.m_fontSize);
	}

	public func SetFontSize(fontSize: Int32) -> Void {
		this.m_fontSize = Cast(fontSize);

		this.InitializeLayout();
	}

	public func GetTintColor() -> HDRColor {
		return this.m_selection.GetTintColor();
	}

	public func SetTintColor(color: HDRColor) -> Void {
		this.m_selection.SetTintColor(color);
	}

	public func GetOpacity() -> Float {
		return this.m_selection.GetOpacity();
	}

	public func SetOpacity(opacity: Float) -> Void {
		this.m_selection.SetOpacity(opacity);
	}

	public func GetMaxPosition() -> Int32 {
		return this.m_maxPosition;
	}

	public func SetMaxPosition(max: Int32) -> Void {
		this.m_maxPosition = max;
	}

	public func GetStartPosition() -> Int32 {
		return this.m_startPosition;
	}

	public func SetStartPosition(value: Int32) -> Void {
		this.m_startPosition = value;
	}

	public func GetEndPosition() -> Int32 {
		return this.m_endPosition;
	}

	public func SetEndPosition(value: Int32) -> Void {
		this.m_endPosition = value;
	}

	public func SelectAll() -> Void {
		this.m_startPosition = 0;
		this.m_endPosition = this.m_maxPosition;
	}

	public func Clear() -> Void {
		this.m_startPosition = -1;
		this.m_endPosition = -1;
	}

	public func GetLeftPosition() -> Int32 {
		return Min(this.m_startPosition, this.m_endPosition);
	}

	public func GetRightPosition() -> Int32 {
		return Max(this.m_startPosition, this.m_endPosition);
	}

	public func GetRange() -> RectF {
		return new RectF(
			Cast(this.GetLeftPosition()),
			0.0,
			Cast(this.GetRightPosition()),
			0.0
		);
	}

	public func IsEmpty() -> Bool {
		return this.m_startPosition == this.m_endPosition;
	}

	public func UpdateState(isFocused: Bool, selectedBounds: RectF) -> Void {
		if isFocused && !this.IsEmpty() {
			this.m_selection.SetVisible(true);

			let selectionWidth: Float = selectedBounds.Right - selectedBounds.Left;
			let selectionPos: Vector2 = new Vector2(
				selectedBounds.Left,
				(this.m_fontSize - this.m_selection.GetHeight()) / 2.0
			);

			this.m_selection.SetWidth(selectionWidth);
			this.m_selection.SetTranslation(selectionPos);
		} else {
			this.m_selection.SetVisible(false);
		}
	}

	public static func Create() -> ref<Selection> {
		let self: ref<Selection> = new Selection();
		self.CreateInstance();

		return self;
	}
}
