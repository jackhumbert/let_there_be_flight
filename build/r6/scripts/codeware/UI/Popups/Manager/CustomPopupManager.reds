// -----------------------------------------------------------------------------
// Codeware.UI.CustomPopupManager
// -----------------------------------------------------------------------------
//
// public class CustomPopupManager {
//   public func IsInitialized() -> Bool
//   public func Initialize(controller: ref<inkGameController>) -> Void
//   public func ShowPopup(popupController: ref<CustomPopup>) -> Void
//   public func HidePopup(popupController: ref<CustomPopup>) -> Void
//   public func AttachPopup(request: ref<CustomPopupAttachRequest>) -> Void
//   public static func GetInstance(game: GameInstance) -> ref<CustomPopupManager>
// }
//

module Codeware.UI
import Codeware.Registry.*

public class CustomPopupManager extends ICustomPopupManager {
	private let m_gameController: wref<inkGameController>;

	private let m_notificationsContainer: wref<inkCompoundWidget>;

	private let m_bracketsContainer: wref<inkCompoundWidget>;

	public func IsInitialized() -> Bool {
		return IsDefined(this.m_gameController);
	}

	public func Initialize(controller: ref<inkGameController>) -> Void {
		this.m_gameController = controller;
		this.m_notificationsContainer = this.m_gameController.GetChildWidgetByPath(n"NotificationsContainer") as inkCompoundWidget;
		this.m_bracketsContainer = this.m_gameController.GetChildWidgetByPath(n"BracketsContainer") as inkCompoundWidget;
	}

	public func ShowPopup(popupController: ref<CustomPopup>) -> Void {
		if !this.IsInitialized() || !IsDefined(popupController) {
			return;
		}

		let notificationData: ref<CustomPopupNotificationData> = new CustomPopupNotificationData();
		notificationData.controller = popupController;
		notificationData.notificationName = popupController.GetName();
		notificationData.queueName = popupController.GetQueueName();
		notificationData.isBlocking = popupController.IsBlocking();
		notificationData.useCursor = popupController.UseCursor();
	
		let notificationToken: ref<inkGameNotificationToken> = this.m_gameController.ShowGameNotification(notificationData);

		this.QueueAttachRequest(
			CustomPopupAttachRequest.Create(
				popupController,
				notificationData,
				notificationToken,
				this.m_notificationsContainer.GetNumChildren()
			)
		);
	}

	public func AttachPopup(request: ref<CustomPopupAttachRequest>) -> Void {
		let initialCount: Int32 = request.GetInitialCount();
		let currentCount: Int32 = this.m_notificationsContainer.GetNumChildren();

		if currentCount == request.GetInitialCount() {
			this.QueueAttachRequest(request);
			return;
		}

		let popupController: ref<CustomPopup> = request.GetPopupController();
		let notificationData: ref<inkGameNotificationData> = request.GetNotificationData();
		let notificationToken: ref<inkGameNotificationToken> = request.GetNotificationToken();

		let containerWidget: ref<inkCanvas> = new inkCanvas();
		containerWidget.SetName(popupController.GetName());
		containerWidget.SetAnchor(inkEAnchor.Fill);
		containerWidget.SetAnchorPoint(new Vector2(0.5, 0.5));
		containerWidget.SetSize(this.m_notificationsContainer.GetSize());
		containerWidget.Reparent(this.m_notificationsContainer, initialCount + 2);

		let rootWidget: ref<inkCanvas> = new inkCanvas();
		rootWidget.SetName(n"Root");
		rootWidget.SetAnchor(this.m_bracketsContainer.GetAnchor());
		rootWidget.SetAnchorPoint(this.m_bracketsContainer.GetAnchorPoint());
		rootWidget.SetSize(this.m_bracketsContainer.GetSize());
		rootWidget.SetScale(this.m_bracketsContainer.GetScale());
		rootWidget.Reparent(containerWidget);

		popupController.Attach(rootWidget, this.m_gameController, notificationData);

		notificationToken.RegisterListener(this, n"OnNotificationClosed");
	}

	public func HidePopup(popupController: ref<CustomPopup>) -> Void {
		if IsDefined(popupController) {
			popupController.Detach();
		}
	}

	protected func QueueAttachRequest(request: ref<CustomPopupAttachRequest>) -> Void {
		let game: GameInstance = this.m_gameController.GetPlayerControlledObject().GetGame();

		GameInstance.GetDelaySystem(game).DelayCallback(CustomPopupAttachCallback.Create(this, request), 0);
	}

	protected cb func OnNotificationClosed(data: ref<inkGameNotificationData>) -> Bool {
		let notificationData: ref<CustomPopupNotificationData> = data as CustomPopupNotificationData;
		let popupController: ref<CustomPopup> = notificationData.controller;

		if IsDefined(popupController) {
			let containerWidget: ref<inkCanvas> = this.m_notificationsContainer.GetWidgetByPathName(popupController.GetName()) as inkCanvas;
	
			if IsDefined(containerWidget) {
				this.m_notificationsContainer.RemoveChild(containerWidget);
			}
		}
	}

	public static func GetInstance(game: GameInstance) -> ref<CustomPopupManager> {
		let registry: ref<RegistrySystem> = RegistrySystem.GetInstance(game);
		let instance: ref<CustomPopupManager> = registry.Get(n"Codeware.UI.CustomPopupManager") as CustomPopupManager;

		if !IsDefined(instance) {
			instance = new CustomPopupManager();
			registry.Put(instance);
		}

		return instance;
	}
}

// -----------------------------------------------------------------------------

@wrapMethod(PopupsManager)
protected cb func OnPlayerAttach(playerPuppet: ref<GameObject>) -> Bool {
	wrappedMethod(playerPuppet);

	CustomPopupManager.GetInstance(this.GetPlayerControlledObject().GetGame()).Initialize(this);
}

@addMethod(PopupsManager)
protected cb func OnShowCustomPopup(evt: ref<ShowCustomPopupEvent>) -> Bool {
	CustomPopupManager.GetInstance(this.GetPlayerControlledObject().GetGame()).ShowPopup(evt.GetPopupController());
}

@addMethod(PopupsManager)
protected cb func OnHideCustomPopup(evt: ref<HideCustomPopupEvent>) -> Bool {
	CustomPopupManager.GetInstance(this.GetPlayerControlledObject().GetGame()).HidePopup(evt.GetPopupController());
}
