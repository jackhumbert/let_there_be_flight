public class FlightUtils {
    public static func SqrtCurve(input: Float) -> Float {
        if input != 0.0 {
            return SqrtF(AbsF(input)) * AbsF(input) / input;
        } else {
            return 0.0;
        }
    }
    public static func IdentCurve(input: Float) -> Float {
        return input;
    }
}