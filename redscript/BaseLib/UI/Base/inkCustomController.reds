// -----------------------------------------------------------------------------
// inkCustomController
// -----------------------------------------------------------------------------
//
// public abstract class inkCustomController extends inkLogicController {
//   public func GetRootWidget() -> wref<inkWidget>
//   public func GetRootCompoundWidget() -> wref<inkCompoundWidget>
//   public func GetContainerWidget() -> wref<inkCompoundWidget>
//   public func GetGameController() -> wref<inkGameController>
//   public func GetPlayer() -> ref<PlayerPuppet>
//   public func GetGame() -> GameInstance
//   public func Reparent(newParent: wref<inkCompoundWidget>) -> Void
//   public func Reparent(newParent: wref<inkCustomController>) -> Void
//   public func Reparent(newParent: wref<inkLogicController>) -> Void
//   public func Reparent(newParent: wref<inkGameController>) -> Void
//   public func CallCustomCallback(eventName: CName) -> Void
//   public func RegisterToCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void
//   public func UnregisterFromCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void
//   public func RegisterToGlobalInputCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void
//   public func UnregisterFromGlobalInputCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void
//   public func PlaySound(widgetName: CName, eventName: CName, opt actionKey: CName) -> Void
//   public func Build() -> Void
//   protected cb func OnCreate() -> Void
//   protected cb func OnInitialize() -> Void
//   protected cb func OnUninitialize() ->  Void
//   protected cb func OnReparent(parent: ref<inkCompoundWidget>) ->  Void
//   protected func SetRootWidget(rootWidget: ref<inkCompoundWidget>) -> Void
//   protected func SetContainerWidget(containerWidget: ref<inkCompoundWidget>) -> Void
//   protected func SetGameController(gameController: ref<inkGameController>) -> Void
//   protected func SetGameController(parentController: ref<inkCustomController>) -> Void
// }
//

module BaseLib.UI

public abstract class inkCustomController extends inkLogicController {
	private let m_isCreated: Bool;

	private let m_isInitialized: Bool;

	private let m_gameController: wref<inkGameController>;

	protected let m_rootWidget: ref<inkWidget>;

	protected let m_containerWidget: wref<inkCompoundWidget>;

	protected func IsInitialized() -> Bool {
		return this.m_isInitialized;
	}

	protected func SetRootWidget(rootWidget: ref<inkCompoundWidget>) -> Void {
		this.m_rootWidget = rootWidget;
		this.m_rootWidget.SetController(this);
	}

	protected func SetContainerWidget(containerWidget: ref<inkCompoundWidget>) -> Void {
		this.m_containerWidget = containerWidget;
	}

	protected func SetGameController(gameController: ref<inkGameController>) -> Void {
		this.m_gameController = gameController;
	}

	protected func SetGameController(parentController: ref<inkCustomController>) -> Void {
		this.m_gameController = parentController.GetGameController();
	}

	protected func CreateInstance() -> Void {
		if !this.m_isCreated {
			this.OnCreate();
			this.CallCustomCallback(n"OnCreate");

			if IsDefined(this.m_rootWidget) {
				this.m_rootWidget.SetController(this);
			}

			this.m_isCreated = true;
		}
	}

	protected func InitializeInstance() -> Void {
		if this.m_isCreated && !this.m_isInitialized {
			this.InitializeChildren(this.GetRootCompoundWidget());

			this.OnInitialize();
			this.CallCustomCallback(n"OnInitialize");

			this.m_isInitialized = true;
		}
	}

	protected func InitializeChildren(rootWidget: wref<inkCompoundWidget>) -> Void {
		if IsDefined(rootWidget) {
			let index: Int32 = 0;
			let numChildren: Int32 = rootWidget.GetNumChildren();
			let childWidget: wref<inkWidget>;
			let childController: wref<inkLogicController>;
			let customController: wref<inkCustomController>;

			while index < numChildren {
				childWidget = rootWidget.GetWidgetByIndex(index);
				childController = childWidget.GetController();
				customController = childController as inkCustomController;

				if IsDefined(customController) {
					customController.InitializeInstance();
				} else {
					if childWidget.IsA(n"inkCompoundWidget") {
						this.InitializeChildren(childWidget as inkCompoundWidget);
					}
				}

				index += 1;
			}
		}
	}

	protected func UninitializeInstance() -> Void {
		if this.m_isCreated && this.m_isInitialized {
			this.OnUninitialize();
			this.CallCustomCallback(n"OnUninitialize");

			this.UninitializeChildren(this.GetRootCompoundWidget());

			this.m_isInitialized = false;
		}
	}

	protected func UninitializeChildren(rootWidget: wref<inkCompoundWidget>) -> Void {
		if IsDefined(rootWidget) {
			let index: Int32 = 0;
			let numChildren: Int32 = rootWidget.GetNumChildren();
			let childWidget: wref<inkWidget>;
			let childController: wref<inkLogicController>;
			let customController: wref<inkCustomController>;

			while index < numChildren {
				childWidget = rootWidget.GetWidgetByIndex(index);
				childController = childWidget.GetController();
				customController = childController as inkCustomController;

				if IsDefined(customController) {
					customController.UninitializeInstance();
				} else {
					if childWidget.IsA(n"inkCompoundWidget") {
						this.UninitializeChildren(childWidget as inkCompoundWidget);
					}
				}

				index += 1;
			}
		}
	}

	protected cb func OnCreate() -> Void

	protected cb func OnInitialize() -> Void

	protected cb func OnUninitialize() ->  Void

	protected cb func OnReparent(parent: ref<inkCompoundWidget>) ->  Void

	public func GetRootWidget() -> wref<inkWidget> {
		return this.m_rootWidget;
	}

	public func GetRootCompoundWidget() -> wref<inkCompoundWidget> {
		return this.m_rootWidget as inkCompoundWidget;
	}

	public func GetContainerWidget() -> wref<inkCompoundWidget> {
		if IsDefined(this.m_containerWidget) {
			return this.m_containerWidget;
		}

		return this.m_rootWidget as inkCompoundWidget;
	}

	public func GetGameController() -> wref<inkGameController> {
		return this.m_gameController;
	}

	public func GetPlayer() -> ref<PlayerPuppet> {
		return this.m_gameController.GetPlayerControlledObject() as PlayerPuppet;
	}

	public func GetGame() -> GameInstance {
		return this.m_gameController.GetPlayerControlledObject().GetGame();
	}

	public func Reparent(newParent: wref<inkCompoundWidget>) -> Void {
		this.CreateInstance();

		if IsDefined(this.m_rootWidget) {
			this.m_rootWidget.Reparent(newParent);
			this.OnReparent(newParent);
		}
	}

	public func Reparent(newParent: wref<inkCustomController>) -> Void {
		if IsDefined(newParent.GetGameController()) {
			this.SetGameController(newParent.GetGameController());
		}

		this.Reparent(newParent.GetContainerWidget());
	}

	public func Reparent(newParent: wref<inkLogicController>) -> Void {
		this.Reparent(newParent.GetRootCompoundWidget());
	}

	public func Reparent(newParent: wref<inkGameController>) -> Void {
		this.Reparent(newParent.GetRootCompoundWidget());
	}

	public func CallCustomCallback(eventName: CName) -> Void {
		this.m_rootWidget.CallCustomCallback(eventName);
	}

	public func RegisterToCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void {
		this.m_rootWidget.RegisterToCallback(eventName, object, functionName);
	}

	public func UnregisterFromCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void {
		this.m_rootWidget.UnregisterFromCallback(eventName, object, functionName);
	}

	public func RegisterToGlobalInputCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void {
		if IsDefined(this.m_gameController) {
			this.m_gameController.RegisterToGlobalInputCallback(eventName, object, functionName);
		}
	}

	public func UnregisterFromGlobalInputCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void {
		if IsDefined(this.m_gameController) {
			this.m_gameController.UnregisterFromGlobalInputCallback(eventName, object, functionName);
		}
	}

	public func PlaySound(widgetName: CName, eventName: CName, opt actionKey: CName) -> Void {
		if IsDefined(this.m_gameController) {
			this.m_gameController.PlaySound(widgetName, eventName, actionKey);
		}
	}

	public func Build() -> Void {
		this.CreateInstance();
	}
}