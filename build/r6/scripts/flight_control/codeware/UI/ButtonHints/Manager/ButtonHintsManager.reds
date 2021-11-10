// -----------------------------------------------------------------------------
// Codeware.UI.ButtonHintsManager
// -----------------------------------------------------------------------------
//
// public class ButtonHintsManager {
//   public func IsInitialized() -> Bool
//   public func Initialize(buttonHints: ref<inkWidget>) -> Void
//   public func SpawnButtonHints(parentWidget: wref<inkWidget>) -> ref<ButtonHintsEx>
//   public static func GetInstance(game: GameInstance) -> ref<ButtonHintsManager>
// }
//

module Codeware.UI
import Codeware.Registry.*

public class ButtonHintsManager extends IButtonHintsManager {
	private let m_buttonHints: ref<inkWidget>;

	private let m_inputHint: wref<inkInputDisplayController>;

	public func IsInitialized() -> Bool {
		return IsDefined(this.m_buttonHints);
	}

	public func Initialize(buttonHints: ref<inkWidget>) -> Void {
		this.m_buttonHints = buttonHints;
	}

	public func Initialize(buttonHints: ref<ButtonHints>) -> Void {
		this.m_buttonHints = buttonHints.GetRootWidget();
	}

	public func Initialize(parent: ref<inkGameController>) -> Void {
		let rootWidget: ref<inkCompoundWidget> = parent.GetRootCompoundWidget();
		let buttonHints: ref<inkWidget> = parent.SpawnFromExternal(rootWidget, r"base\\gameplay\\gui\\common\\buttonhints.inkwidget", n"Root");

		this.Initialize(buttonHints);

		rootWidget.RemoveChild(buttonHints);
	}

	public func SpawnButtonHints(parentWidget: wref<inkWidget>) -> ref<ButtonHintsEx> {
		return ButtonHintsEx.Wrap(
			this.m_buttonHints.GetController().SpawnFromLocal(parentWidget, n"Root")
		);
	}

	public func GetActionKey(action: CName) -> String {
		if !IsDefined(this.m_inputHint) {
			let buttonHints: ref<ButtonHints> = this.m_buttonHints.GetController() as ButtonHints;
			buttonHints.ClearButtonHints();
			buttonHints.AddButtonHint(action, "");

			this.m_inputHint = buttonHints.CheckForPreExisting(action).m_buttonHint;
		}

		this.m_inputHint.SetInputAction(action);

		let icon: wref<inkImage> = this.m_inputHint.GetWidget(n"inputRoot/inputIcon") as inkImage;
		let part: CName = icon.GetTexturePart();
		let key: String = NameToString(part);

		return key;
	}

	public static func GetInstance(game: GameInstance) -> ref<ButtonHintsManager> {
		let registry: ref<RegistrySystem> = RegistrySystem.GetInstance(game);
		let instance: ref<ButtonHintsManager> = registry.Get(n"Codeware.UI.ButtonHintsManager") as ButtonHintsManager;

		if !IsDefined(instance) {
			instance = new ButtonHintsManager();
			registry.Put(instance);
		}

		return instance;
	}

	public static func InitializeFromController(controller: ref<inkGameController>) -> Void {
		let game: GameInstance = controller.GetPlayerControlledObject().GetGame();
		let instance: ref<ButtonHintsManager> = ButtonHintsManager.GetInstance(game);

		instance.Initialize(controller);
	}
}

// -----------------------------------------------------------------------------

@wrapMethod(SingleplayerMenuGameController)
protected cb func OnInitialize() -> Bool {
	wrappedMethod();

	ButtonHintsManager.InitializeFromController(this);
}

@wrapMethod(DpadWheelGameController)
protected cb func OnInitialize() -> Bool {
	wrappedMethod();

	ButtonHintsManager.InitializeFromController(this);
}
