// -----------------------------------------------------------------------------
// GenderNeutralEntry
// -----------------------------------------------------------------------------

module BaseLib.Localization

public class GenderNeutralEntry extends LocalizationEntry {
	private let m_value: String;

	public func GetVariant(gender: PlayerGender) -> String {
		return this.m_value;
	}

	public func SetVariant(gender: PlayerGender, value: String) -> Void {
		this.m_value = value;
	}

	public static func Create(key: String) -> ref<GenderNeutralEntry> {
		let self = new GenderNeutralEntry();
		self.m_key = key;

		return self;
	}
}
