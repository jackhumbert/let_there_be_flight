// -----------------------------------------------------------------------------
// UpdateGenderRequest
// -----------------------------------------------------------------------------

module BaseLib.Localization

public class UpdateGenderRequest extends ScriptableSystemRequest {
	public static func Create() -> ref<UpdateGenderRequest> {
		return new UpdateGenderRequest();
	}
}
