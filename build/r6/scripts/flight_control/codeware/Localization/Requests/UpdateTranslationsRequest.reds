// -----------------------------------------------------------------------------
// Codeware.Localization.UpdateTranslationsRequest
// -----------------------------------------------------------------------------

module Codeware.Localization

public class UpdateTranslationsRequest extends ScriptableSystemRequest {
	private let m_force: Bool;

	public func IsForced() -> Bool {
		return this.m_force;
	}

	public static func Create(opt force: Bool) -> ref<UpdateTranslationsRequest> {
		let self = new UpdateTranslationsRequest();
		self.m_force = force;

		return self;
	}
}
