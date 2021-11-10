// -----------------------------------------------------------------------------
// Codeware.UI.ICustomPopupManager
// -----------------------------------------------------------------------------

module Codeware.UI

public abstract class ICustomPopupManager {
	public func IsInitialized() -> Bool
	public func ShowPopup(popupController: ref<CustomPopup>) -> Void
	public func HidePopup(popupController: ref<CustomPopup>) -> Void
}
