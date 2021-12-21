// -----------------------------------------------------------------------------
// Codeware.UI.VirtualResolutionResizeTarget
// -----------------------------------------------------------------------------

module Codeware.UI

public class VirtualResolutionResizeTarget extends VirtualResolutionTarget {
	protected let m_size: Vector2;

	public func ApplyState(state: ref<VirtualResolutionData>) -> Void
	{
		let scale: Vector2 = state.GetSmartScale();

		this.m_widget.SetSize(new Vector2(this.m_size.X * scale.X, this.m_size.Y * scale.Y));
	}

	public static func Create(widget: wref<inkWidget>) -> ref<VirtualResolutionResizeTarget> {
		let target = new VirtualResolutionResizeTarget();
		target.m_widget = widget;
		target.m_size = widget.GetSize();

		return target;
	}
}
