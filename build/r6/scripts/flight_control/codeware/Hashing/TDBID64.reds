// -----------------------------------------------------------------------------
// Codeware.Hashing.TDBID64
// -----------------------------------------------------------------------------
//
// This is the hashing used by TweakDB.
// Hash consists of CRC32 and original string length.
//

module Codeware.Hashing

public abstract class TDBID64 {
	public static func Compute(str: String) -> Uint64 {
		return TDBID.ToNumber(TDBID.Create(str));
	}

	public static func Compute(name: CName) -> Uint64 {
		return TDBID64.Compute(NameToString(name));
	}
}
