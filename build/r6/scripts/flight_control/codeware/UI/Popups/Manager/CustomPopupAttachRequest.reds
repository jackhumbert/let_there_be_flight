// -----------------------------------------------------------------------------
// Codeware.UI.CustomPopupAttachRequest
// -----------------------------------------------------------------------------

module Codeware.UI

public class CustomPopupAttachRequest {
	public let controller: ref<CustomPopup>;

	public let notificationData: ref<inkGameNotificationData>;

	public let notificationToken: ref<inkGameNotificationToken>;

	public let initialCount: Int32;

	public func GetPopupController() -> ref<CustomPopup> {
		return this.controller;
	}

	public func GetNotificationData() -> ref<inkGameNotificationData> {
		return this.notificationData;
	}

	public func GetNotificationToken() -> ref<inkGameNotificationToken> {
		return this.notificationToken;
	}

	public func GetInitialCount() -> Int32 {
		return this.initialCount;
	}

	public static func Create(controller: ref<CustomPopup>, data: ref<inkGameNotificationData>, token: ref<inkGameNotificationToken>, initialCount: Int32) -> ref<CustomPopupAttachRequest> {
		let request: ref<CustomPopupAttachRequest> = new CustomPopupAttachRequest();
		request.controller = controller;
		request.notificationData = data;
		request.notificationToken = token;
		request.initialCount = initialCount;

		return request;
	}
}
