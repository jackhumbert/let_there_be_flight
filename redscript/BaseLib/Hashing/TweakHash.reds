// -----------------------------------------------------------------------------
// TweakDB Hasher
// -----------------------------------------------------------------------------
//
// This is the hashing used by TweakDB.
// Hash consists of CRC32 and original string length.
//

module BaseLib.Hashing

public abstract class TweakHash {
	public static func Compute(str: String) -> Uint64 {
		return TDBID.ToNumber(TDBID.Create(str));
	}

	public static func Compute(name: CName) -> Uint64 {
		return TweakHash.Compute(NameToString(name));
	}
}
