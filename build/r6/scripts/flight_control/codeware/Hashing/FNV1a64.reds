// -----------------------------------------------------------------------------
// Codeware.Hashing.FNV1a64
// -----------------------------------------------------------------------------
//
// This is a part of experimental code to understand what can be done in scripts.
// This is not really meant for production.
//

module Codeware.Hashing

public abstract class FNV1a64 {
	public static func Compute(str: String) -> Uint64 {
		let hash: Uint64 = Cast(-3750763034362895579l); // StringToUint64("14695981039346656037")
		let prime: Uint64 = Cast(1099511628211l);

		let length: Int32 = StrLen(str);
		let offset: Int32 = 0;

		// Because there is no function for getting the char code
		let chars: ref<inkStringMap> = FNV1a64.AsciiCharCodes();

		while offset < length {
			hash = hash ^ chars.Get(StrMid(str, offset, 1));
			hash *= prime;
			offset += 1;
		}

		return hash;
	}

	public static func Compute(name: CName) -> Uint64 {
		return FNV1a64.Compute(NameToString(name));
	}

	private static func AsciiCharCodes() -> ref<inkStringMap> {
		let map: ref<inkStringMap> = new inkStringMap();

		let code: Int32;
		while code <= 255 {
			map.Insert(StrChar(code), Cast(code));
			code += 1;
		}

		return map;
	}
}
