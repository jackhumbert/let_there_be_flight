// -----------------------------------------------------------------------------
// CustomPopup
// -----------------------------------------------------------------------------
//
// - Base implementation of custom popups
// - Close on "cancel" event (default ESC and C keys)
// - Transition animations
//
// -----------------------------------------------------------------------------
//
// public abstract class CustomPopup extends inkAttachableController {
//   public func GetName() -> CName
//   public func GetQueueName() -> CName
//   public func IsBlocking() -> Bool
//   public func UseCursor() -> Bool
//   public func Attach(rootWidget: ref<inkCanvas>, gameController: wref<inkGameController>, notificationData: ref<inkGameNotificationData>) -> Void
//   public func Detach() -> Void
// }
//

module BaseLib.UI

public abstract class CustomPopup extends inkAttachableController {
	protected let m_notificationData: ref<inkGameNotificationData>;

	protected let m_notificationToken: ref<inkGameNotificationToken>;

	protected let m_transitionAnimProxy: ref<inkAnimProxy>;

	protected func SetNotificationData(notificationData: ref<inkGameNotificationData>) -> Void {
		this.m_notificationData = notificationData;
		this.m_notificationToken = notificationData.token;
	}

	protected func ResetNotificationData() -> Void {
		this.m_notificationToken.TriggerCallback(this.m_notificationData);
		this.m_notificationToken = null;
		this.m_notificationData = null;
	}

	protected cb func OnInitialize() -> Void {
		super.OnInitialize();

		this.RegisterToGlobalInputCallback(n"OnPostOnRelease", this, n"OnGlobalReleaseInput");

		this.CallCustomCallback(n"OnShow");
		this.OnShow();
	}

	protected cb func OnUninitialize() -> Void {
		super.OnUninitialize();

		this.UnregisterFromGlobalInputCallback(n"OnPostOnRelease", this, n"OnGlobalReleaseInput");

		this.CallCustomCallback(n"OnHide");
		this.OnHide();
	}

	protected cb func OnShow() -> Void {
		let alphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		alphaAnim.SetStartTransparency(0.0);
		alphaAnim.SetEndTransparency(1.0);
		alphaAnim.SetType(inkanimInterpolationType.Linear);
		alphaAnim.SetMode(inkanimInterpolationMode.EasyIn);
		alphaAnim.SetDuration(0.5);

		let animDef: ref<inkAnimDef> = new inkAnimDef();
		animDef.AddInterpolator(alphaAnim);

		this.m_transitionAnimProxy = this.GetRootWidget().PlayAnimation(animDef);
		this.m_transitionAnimProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnShowFinish");
	}

	protected cb func OnShowFinish(animProxy: ref<inkAnimProxy>) -> Bool {
		this.m_transitionAnimProxy = null;

		this.OnShown();
		this.CallCustomCallback(n"OnShown");
	}

	protected cb func OnShown() -> Void

	protected cb func OnHide() -> Void {
		let alphaAnim: ref<inkAnimTransparency> = new inkAnimTransparency();
		alphaAnim.SetStartTransparency(1.0);
		alphaAnim.SetEndTransparency(0.0);
		alphaAnim.SetType(inkanimInterpolationType.Linear);
		alphaAnim.SetMode(inkanimInterpolationMode.EasyIn);
		alphaAnim.SetDuration(0.25);

		let animDef: ref<inkAnimDef> = new inkAnimDef();
		animDef.AddInterpolator(alphaAnim);

		this.m_transitionAnimProxy = this.GetRootWidget().PlayAnimation(animDef);
		this.m_transitionAnimProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnHideFinish");
	}

	protected cb func OnHideFinish(animProxy: ref<inkAnimProxy>) -> Bool {
		this.m_transitionAnimProxy = null;

		this.CallCustomCallback(n"OnHidden");
		this.OnHidden();
	}

	protected cb func OnHidden() -> Void {
		this.ResetNotificationData();
		this.SetGameController(null);
		this.SetRootWidget(null);
	}

	protected cb func OnGlobalReleaseInput(evt: ref<inkPointerEvent>) -> Bool {
		if evt.IsAction(n"cancel") {
      		this.Close();
			evt.Handle();
    	}
	}

	public func GetName() -> CName {
		return this.GetClassName();
	}

	public func GetQueueName() -> CName {
		return n"modal_popup";
	}

	public func IsBlocking() -> Bool {
		return true;
	}

	public func UseCursor() -> Bool {
		return false;
	}

	public func Attach(rootWidget: ref<inkCanvas>, gameController: wref<inkGameController>, notificationData: ref<inkGameNotificationData>) -> Void {
		if !this.IsInitialized() {
			this.SetNotificationData(notificationData);
			this.Attach(rootWidget, gameController);
		}
	}

	public func Detach() -> Void {
		if this.IsInitialized() {
			this.UninitializeInstance();
		}
	}

	public func Open(requester: wref<inkGameController>) -> Void {
		let uiSystem: ref<UISystem> = GameInstance.GetUISystem(requester.GetPlayerControlledObject().GetGame());
		let showEvent: ref<ShowCustomPopupEvent> = ShowCustomPopupEvent.Create(this);

		uiSystem.QueueEvent(showEvent);
	}

	public func Close() -> Void {
		let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetGame());
		let hideEvent: ref<HideCustomPopupEvent> = HideCustomPopupEvent.Create(this);

		uiSystem.QueueEvent(hideEvent);
	}
}
