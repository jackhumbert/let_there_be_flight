// -----------------------------------------------------------------------------
// Codeware.UI.VirtualResolutionChangeEvent
// -----------------------------------------------------------------------------

module Codeware.UI

public class VirtualResolutionChangeEvent extends inkEvent {
	protected let m_state: ref<VirtualResolutionData>;

	public func GetState() -> wref<VirtualResolutionData> {
		return this.m_state;
	}

	public static func Create(state: ref<VirtualResolutionData>) -> ref<VirtualResolutionChangeEvent> {
		let event = new VirtualResolutionChangeEvent();
		event.m_state = state;

		return event;
	}
}
