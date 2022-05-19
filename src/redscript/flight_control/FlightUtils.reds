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

    public static func Right() -> Vector4 = new Vector4(1.0, 0.0, 0.0, 0.0);
    public static func Left() -> Vector4 = new Vector4(-1.0, 0.0, 0.0, 0.0);
    public static func Forward() -> Vector4 = new Vector4(0.0, -1.0, 0.0, 0.0);
    public static func Backward() -> Vector4 = new Vector4(0.0, -1.0, 0.0, 0.0);
    public static func Up() -> Vector4 = new Vector4(0.0, 0.0, 1.0, 0.0);
    public static func Down() -> Vector4 = new Vector4(0.0, 0.0, 1.0, 0.0);
}