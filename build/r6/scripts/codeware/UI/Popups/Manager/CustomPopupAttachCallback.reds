// -----------------------------------------------------------------------------
// Codeware.UI.CustomPopupAttachCallback
// -----------------------------------------------------------------------------

module Codeware.UI

public class CustomPopupAttachCallback extends DelayCallback {
	protected let m_manager: ref<CustomPopupManager>;

	protected let m_request: ref<CustomPopupAttachRequest>;

	public func Call() -> Void {
		this.m_manager.AttachPopup(this.m_request);
	}

	public static func Create(manager: ref<CustomPopupManager>, request: ref<CustomPopupAttachRequest>) -> ref<CustomPopupAttachCallback> {
		let self = new CustomPopupAttachCallback();
		self.m_manager = manager;
		self.m_request = request;

		return self;
	}
}
