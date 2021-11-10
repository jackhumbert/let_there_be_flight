// -----------------------------------------------------------------------------
// Codeware.Localization.ModLocalizationProvider
// -----------------------------------------------------------------------------
//
// Purpose:
// - Resolve packages by language code
// - Define logic on locale and gender changes
//
// Supported language codes:
// - pl-pl (Polish)
// - en-us (English)
// - es-es (Spanish)
// - fr-fr (French)
// - it-it (Italian)
// - de-de (German)
// - es-mx (Latin American Spanish)
// - kr-kr (Korean)
// - zh-cn (Simplified Chinese)
// - ru-ru (Russian)
// - pt-br (Brazilian Portuguese)
// - jp-jp (Japanese)
// - zh-tw (Traditional Chinese)
// - ar-ar (Arabic)
// - cz-cz (Czech)
// - hu-hu (Hungarian)
// - tr-tr (Turkish)
// - th-th (Thai)
//
// -----------------------------------------------------------------------------
//
// public abstract class ModLocalizationProvider extends ScriptableSystem {
//   public func GetPackage(language: CName) -> ref<ModLocalizationPackage>
//   public func GetFallback() -> CName
//   public func OnLocaleChange() -> Void
//   public func OnGenderChange() -> Void
// }
//

module Codeware.Localization

public abstract class ModLocalizationProvider extends ScriptableSystem {
	protected func OnAttach() -> Void {
		GameInstance.GetScriptableSystemsContainer(this.GetGameInstance())
			.QueueRequest(RegisterProviderRequest.Create(this));
	}

	public func GetPackage(language: CName) -> ref<ModLocalizationPackage>

	public func GetFallback() -> CName

	public func OnLocaleChange() -> Void

	public func OnGenderChange() -> Void
}
