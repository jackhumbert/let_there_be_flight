// -----------------------------------------------------------------------------
// Codeware.UI.CustomPopupEvent
// -----------------------------------------------------------------------------

module Codeware.UI

public abstract class CustomPopupEvent extends inkCustomEvent {
	public func GetPopupController() -> ref<CustomPopup> {
		return this.controller as CustomPopup;
	}
}
