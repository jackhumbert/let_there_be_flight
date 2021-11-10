// -----------------------------------------------------------------------------
// Codeware.UI.inkCustomController
// -----------------------------------------------------------------------------
//
// public abstract class inkCustomController extends inkLogicController {
//   public func GetRootWidget() -> wref<inkWidget>
//   public func GetRootCompoundWidget() -> wref<inkCompoundWidget>
//   public func GetContainerWidget() -> wref<inkCompoundWidget>
//   public func GetGameController() -> wref<inkGameController>
//   public func GetPlayer() -> ref<PlayerPuppet>
//   public func GetGame() -> GameInstance
//   public func CallCustomCallback(eventName: CName) -> Void
//   public func RegisterToCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void
//   public func UnregisterFromCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void
//   public func RegisterToGlobalInputCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void
//   public func UnregisterFromGlobalInputCallback(eventName: CName, object: ref<IScriptable>, functionName: CName) -> Void
//   public func PlaySound(widgetName: CName, eventName: CName, opt actionKey: CName) -> Void
//   public func Reparent(newParent: wref<inkCompoundWidget>) -> Void
//   public func Reparent(newParent: wref<inkCompoundWidget>, index: Int32) -> Void
//   public func Reparent(newParent: wref<inkCompoundWidget>, gameController: ref<inkGameController>) -> Void
//   public func Reparent(newParent: wref<inkCustomController>) -> Void
//   public func Reparent(newParent: wref<inkCustomController>, index: Int32) -> Void
//   public func Mount(rootWidget: ref<inkCompoundWidget>, opt gameController: wref<inkGameController>) -> Void
//   public func Mount(rootController: ref<inkLogicController>, opt gameController: ref<inkGameController>) -> Void
//   public func Mount(rootController: ref<inkGameController>) -> Void
//   protected cb func OnCreate() -> Void
//   protected cb func OnInitialize() -> Void
//   protected cb func OnUninitialize() ->  Void
//   protected cb func OnReparent(parent: ref<inkCompoundWidget>) ->  Void
//   protected func SetRootWidget(rootWidget: ref<inkWidget>) -> Void
//   protected func SetContainerWidget(containerWidget: ref<inkCompoundWidget>) -> Void
//   protected func SetGameController(gameController: ref<inkGameController>) -> Void
//   protected func SetGameController(parentController: ref<inkCustomController>) -> Void
// }
//

module Codeware.UI

public abstract class inkCustomController extends inkLogicController {
	private let m_isCreated: Bool;

	private let m_isInitialized: Bool;

	private let m_detachedWidget: ref<inkWidget>;

	private let m_gameController: wref<inkGameController>;

	protected let m_rootWidget: wref<inkWidget>;

	protected let m_containerWidget: wref<inkCompoundWidget>;

	protected func IsInitialized() -> Bool {
		return this.m_isInitialized;
	}

	protected func SetRootWidget(rootWidget: ref<inkWidget>) -> Void {
		this.m_rootWidget = rootWidget;

		if IsDefined(this.m_rootWidget) {
			if !IsDefined(this.m_rootWidget.GetController()) {
				this.m_rootWidget.SetController(this);
			} else {
				if NotEquals(this, this.m_rootWidget.GetControllerByType(this.GetClassName())) {
					this.m_rootWidget.AddSecondaryController(this);
				}
			}

			if !inkWidgetHelper.InWindowTree(this.m_rootWidget) {
				this.m_detachedWidget = this.m_rootWidget;
			}
		} else {
			this.m_detachedWidget = null;
		}
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
				this.m_isCreated = true;
			}
		}
	}

	protected func InitializeInstance() -> Void {
		if this.m_isCreated && !this.m_isInitialized {
			if inkWidgetHelper.InWindowTree(this.m_rootWidget) {
				this.InitializeChildren(this.GetRootCompoundWidget());

				this.OnInitialize();
				this.CallCustomCallback(n"OnInitialize");

				this.m_isInitialized = true;
				this.m_detachedWidget = null;
			}
		}
	}

	protected func InitializeChildren(rootWidget: wref<inkCompoundWidget>) -> Void {
		if IsDefined(rootWidget) {
			let index: Int32 = 0;
			let numChildren: Int32 = rootWidget.GetNumChildren();
			let childWidget: wref<inkWidget>;
			let childControllers: array<wref<inkLogicController>>;
			let customController: wref<inkCustomController>;

			while index < numChildren {
				childWidget = rootWidget.GetWidgetByIndex(index);
				childControllers = childWidget.GetControllers();

				for childController in childControllers {
					customController = childController as inkCustomController;

					if IsDefined(customController) {
						customController.SetGameController(this);
						customController.InitializeInstance();
					}
				}

				if childWidget.IsA(n"inkCompoundWidget") && !IsDefined(childWidget.GetController() as inkCustomController) {
					this.InitializeChildren(childWidget as inkCompoundWidget);
				}

				index += 1;
			}
		}
	}

	protected cb func OnCreate() -> Void

	protected cb func OnInitialize() -> Void

	protected cb func OnUninitialize() ->  Void {
		//this.m_isCreated = false;
		//this.m_isInitialized = false;
		this.m_detachedWidget = null;
	}

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

	public func Reparent(newParent: wref<inkCompoundWidget>) -> Void {
		this.Reparent(newParent, -1);
	}

	public func Reparent(newParent: wref<inkCompoundWidget>, index: Int32) -> Void {
		this.CreateInstance();

		if IsDefined(this.m_rootWidget) && IsDefined(newParent) {
			this.m_rootWidget.Reparent(newParent, index);

			this.OnReparent(newParent);
			this.CallCustomCallback(n"OnReparent");

			this.InitializeInstance();
		}
	}

	public func Reparent(newParent: wref<inkCompoundWidget>, gameController: ref<inkGameController>) -> Void {
		if IsDefined(gameController) {
			this.SetGameController(gameController);
		}

		this.Reparent(newParent, -1);
	}

	public func Reparent(newParent: wref<inkCustomController>) -> Void {
		this.Reparent(newParent, -1);
	}

	public func Reparent(newParent: wref<inkCustomController>, index: Int32) -> Void {
		if IsDefined(newParent.GetGameController()) {
			this.SetGameController(newParent.GetGameController());
		}

		this.Reparent(newParent.GetContainerWidget(), index);
	}

	public func Mount(rootWidget: ref<inkCompoundWidget>, opt gameController: wref<inkGameController>) -> Void {
		if !this.m_isInitialized && IsDefined(rootWidget) {
			this.SetRootWidget(rootWidget);
			this.SetGameController(gameController);

			this.CreateInstance();
			this.InitializeInstance();
		}
	}

	public func Mount(rootController: ref<inkLogicController>, opt gameController: ref<inkGameController>) -> Void {
		this.Mount(rootController.GetRootCompoundWidget(), gameController);
	}

	public func Mount(rootController: ref<inkGameController>) -> Void {
		this.Mount(rootController.GetRootCompoundWidget(), rootController);
	}
}