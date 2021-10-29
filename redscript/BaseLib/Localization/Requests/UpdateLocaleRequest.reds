// -----------------------------------------------------------------------------
// UpdateLocaleRequest
// -----------------------------------------------------------------------------

module BaseLib.Localization

public class UpdateLocaleRequest extends ScriptableSystemRequest {
	private let m_type: CName;

	public func GetType() -> CName {
		return this.m_type;
	}

	public static func Create(type: CName) -> ref<UpdateLocaleRequest> {
		let self = new UpdateLocaleRequest();
		self.m_type = type;

		return self;
	}
}
