// -----------------------------------------------------------------------------
// LocalizationEntry
// -----------------------------------------------------------------------------

module BaseLib.Localization

public abstract class LocalizationEntry {
	private let m_key: String;

	public func GetKey() -> String {
		return this.m_key;
	}

	public func GetVariant(gender: PlayerGender) -> String

	public func SetVariant(gender: PlayerGender, value: String) -> Void

	public static func Hash(str: String) -> Uint64 {
		return TDBID.ToNumber(TDBID.Create(str));
	}
}
