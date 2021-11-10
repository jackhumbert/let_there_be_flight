// -----------------------------------------------------------------------------
// Codeware.UI.TextInput.Parts.Viewport [WIP]
// -----------------------------------------------------------------------------

module Codeware.UI.TextInput.Parts
import Codeware.UI.inkCustomController

public class Viewport extends inkCustomController {
	protected let m_viewport: wref<inkScrollArea>;

	protected let m_content: wref<inkCanvas>;

	protected let m_caretSize: Vector2;

	protected cb func OnCreate() -> Void {
		this.CreateWidgets();
	}

	protected func CreateWidgets() -> Void {
		let viewport: ref<inkScrollArea> = new inkScrollArea();
		viewport.SetName(n"viewport");
		viewport.SetAnchor(inkEAnchor.Fill);
		viewport.SetMargin(new inkMargin(8.0, 4.0, 8.0, 4.0));
		viewport.SetRenderTransformPivot(new Vector2(0.0, 0.0));
		viewport.SetFitToContentDirection(inkFitToContentDirection.Vertical);
		viewport.SetConstrainContentPosition(true);
		viewport.SetUseInternalMask(true);

		let content: ref<inkCanvas> = new inkCanvas();
		content.SetName(n"content");
		content.SetRenderTransformPivot(new Vector2(0.0, 0.0));
		content.Reparent(viewport);

		this.m_viewport = viewport;
		this.m_content = content;

		this.SetRootWidget(viewport);
		this.SetContainerWidget(content);
	}

	public func GetCaretSize() -> Vector2 {
		return this.m_caretSize;
	}

	public func SetCaretSize(caretSize: Vector2) -> Void {
		this.m_caretSize = caretSize;
	}

	public func UpdateState(contentSize: Vector2, caretOffset: Float) -> Void {
		if contentSize.X <= 0.01 {
			contentSize = this.m_caretSize;
		}

		let viewportSize: Vector2 = this.m_viewport.GetViewportSize();
		let contentOffset: Vector2 = this.m_content.GetTranslation();

		if contentSize.X <= viewportSize.X {
			contentOffset.X = 0.0;
		} else {
			let viewportBounds: inkMargin = new inkMargin(
				-contentOffset.X,
				0.0,
				-contentOffset.X + viewportSize.X,
				0.0
			);

			if caretOffset < viewportBounds.left {
				contentOffset.X = -caretOffset;
			} else {
				if caretOffset > viewportBounds.right {
					contentOffset.X = -(caretOffset - viewportSize.X + this.m_caretSize.X);
				} else {
					contentOffset.X = MaxF(contentOffset.X, -(contentSize.X - viewportSize.X + this.m_caretSize.X));
				}
			}
		}

		this.m_content.SetSize(contentSize);
		this.m_content.SetTranslation(contentOffset);
	}

	public static func Create() -> ref<Viewport> {
		let self: ref<Viewport> = new Viewport();
		self.CreateInstance();

		return self;
	}
}
