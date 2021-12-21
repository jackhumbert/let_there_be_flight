// -----------------------------------------------------------------------------
// Codeware.Localization.GenderSensitiveEntry
// -----------------------------------------------------------------------------

module Codeware.Localization

public class GenderSensitiveEntry extends LocalizationEntry {
	private let m_variants: array<String>;

	public func GetVariant(gender: PlayerGender) -> String {
		return this.m_variants[EnumInt(gender)];
	}

	public func SetVariant(gender: PlayerGender, value: String) -> Void {
		this.m_variants[EnumInt(gender)] = value;
	}

	public static func Create(key: String) -> ref<GenderSensitiveEntry> {
		let self = new GenderSensitiveEntry();
		self.m_key = key;

		ArrayResize(self.m_variants, 2);

		return self;
	}
}
