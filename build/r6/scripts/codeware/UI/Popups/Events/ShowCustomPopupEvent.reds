// -----------------------------------------------------------------------------
// Codeware.UI.ShowCustomPopupEvent
// -----------------------------------------------------------------------------

module Codeware.UI

public class ShowCustomPopupEvent extends CustomPopupEvent {
	public static func Create(controller: ref<CustomPopup>) -> ref<ShowCustomPopupEvent> {
		let event: ref<ShowCustomPopupEvent> = new ShowCustomPopupEvent();
		event.controller = controller;

		return event;
	}
}
