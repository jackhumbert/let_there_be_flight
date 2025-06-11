// Matrix

public static func OperatorMultiply(m: Matrix, v: Vector4) -> Vector4 {
  return Vector4.Transform(m, v);
}
public static func OperatorMultiply(v: Vector4, m: Matrix) -> Vector4 {
  return m * v;
}

public static func OperatorAssignMultiply(out v: Vector4, m: Matrix) -> Vector4 {
  v = m * v;
  return v;
}

// Quaternion

public static func OperatorXor(q: Quaternion, n: Int32) -> Quaternion {
  let out = q;
  while (n > 0) {
    Quaternion.SetInverse(out);
    n -= 1;
  }
  return out;
}

// EulerAngles

public static func MakeEulerAngles(Roll: Float, Yaw: Float, Pitch: Float) -> EulerAngles {
  let ea = new EulerAngles();
  ea.Roll = Roll;
  ea.Yaw = Yaw;
  ea.Pitch = Pitch;
  return ea;
}

public static func Cast(a: EulerAngles) -> Vector4 {
  let v = Vector4.EmptyVector();
  v.X = a.Pitch;
  v.Y = a.Roll;
  v.Z = a.Yaw;
  return v;
}

public static func Cast(a: EulerAngles) -> WorldTransform {
  let wt = new WorldTransform();
  WorldTransform.SetOrientation(wt, EulerAngles.ToQuat(a));
  return wt;
}

public static func OperatorMultiply(e: EulerAngles, f: Float) -> EulerAngles {
  let out = e;
  e.Roll *= f;
  e.Yaw *= f;
  e.Pitch += f;
  return out;
}

public static func OperatorAssignMultiply(out e: EulerAngles, f: Float) -> EulerAngles {
  e = e * f;
  return e;
}

public static func OperatorDivide(e: EulerAngles, f: Float) -> EulerAngles {
  let out = e;
  e.Roll /= f;
  e.Yaw /= f;
  e.Pitch /= f;
  return out;
}

public static func OperatorAdd(e: EulerAngles, v: Vector4) -> Vector4 {
  let out = v;
  v.X += e.Pitch;
  v.Y += e.Roll;
  v.Z += e.Yaw;
  return out;
}

public static func OperatorAdd(v: Vector4, e: EulerAngles) -> Vector4 {
  return e + v;
}

public static func OperatorAssignAdd(out v: Vector4, e: EulerAngles) -> Vector4 {
  v = e + v;
  return v;
}

// Vector3

public static func OperatorAdd(a: Vector4, b: Vector3) -> Vector4 {
  return a + Vector4.Vector3To4(b);
}

// FxResource

public static native func Cast(a: ResRef) -> FxResource;


public static func OperatorEqual(a: Vector3, b: Vector3) -> Bool {
  return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
}

public static func OperatorEqual(a: Vector4, b: Vector4) -> Bool {
  return a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
}

public static func OperatorNotEqual(a: Vector3, b: Vector3) -> Bool {
  return !OperatorEqual(a, b);
}

public static func OperatorNotEqual(a: Vector4, b: Vector4) -> Bool {
  return !OperatorEqual(a, b);
}