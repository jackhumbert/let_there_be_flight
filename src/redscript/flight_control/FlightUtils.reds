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
    public static func Forward() -> Vector4 = new Vector4(0.0, 1.0, 0.0, 0.0);
    public static func Backward() -> Vector4 = new Vector4(0.0, -1.0, 0.0, 0.0);
    public static func Up() -> Vector4 = new Vector4(0.0, 0.0, 1.0, 0.0);
    public static func Down() -> Vector4 = new Vector4(0.0, 0.0, -1.0, 0.0);
    
	public static func ElectricBlue() -> HDRColor = new HDRColor(0.368627, 0.964706, 1.0, 1.0)
	public static func Bittersweet() -> HDRColor = new HDRColor(1.1761, 0.3809, 0.3476, 1.0)
	public static func Dandelion() -> HDRColor = new HDRColor(1.1192, 0.8441, 0.2565, 1.0)
	public static func LightGreen() -> HDRColor = new HDRColor(0.113725, 0.929412, 0.513726, 1.0)
	public static func BlackPearl() -> HDRColor = new HDRColor(0.054902, 0.054902, 0.090196, 1.0)

	public static func RedOxide() -> HDRColor = new HDRColor(0.411765, 0.086275, 0.090196, 1.0)
	public static func Bordeaux() -> HDRColor = new HDRColor(0.262745, 0.086275, 0.094118, 1.0)

	public static func PureBlack() -> HDRColor = new HDRColor(0.0, 0.0, 0.0, 1.0)
	public static func PureWhite() -> HDRColor = new HDRColor(1.0, 1.0, 1.0, 1.0)
}