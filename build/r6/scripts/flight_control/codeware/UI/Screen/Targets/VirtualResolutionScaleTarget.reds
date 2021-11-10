// -----------------------------------------------------------------------------
// Codeware.UI.VirtualResolutionScaleTarget
// -----------------------------------------------------------------------------

module Codeware.UI

public class VirtualResolutionScaleTarget extends VirtualResolutionTarget {
	public func ApplyState(state: ref<VirtualResolutionData>) -> Void
	{
		this.m_widget.SetScale(state.GetSmartScale());
	}

	public static func Create(widget: wref<inkWidget>) -> ref<VirtualResolutionScaleTarget> {
		let target = new VirtualResolutionScaleTarget();
		target.m_widget = widget;

		return target;
	}
}
