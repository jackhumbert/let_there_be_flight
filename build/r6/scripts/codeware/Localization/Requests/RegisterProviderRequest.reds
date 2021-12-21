// -----------------------------------------------------------------------------
// Codeware.Localization.RegisterProviderRequest
// -----------------------------------------------------------------------------

module Codeware.Localization

public class RegisterProviderRequest extends ScriptableSystemRequest {
	private let m_provider: ref<ModLocalizationProvider>;

	public func GetProvider() -> ref<ModLocalizationProvider> {
		return this.m_provider;
	}

	public static func Create(provider: ref<ModLocalizationProvider>) -> ref<RegisterProviderRequest> {
		let self = new RegisterProviderRequest();
		self.m_provider = provider;

		return self;
	}
}
