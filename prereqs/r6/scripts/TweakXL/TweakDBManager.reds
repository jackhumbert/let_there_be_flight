
public abstract native class TweakDBManager {

  public final static native func SetFlat(path: TweakDBID, value: Variant) -> Bool;

  public final static native func CreateRecord(path: TweakDBID, type: CName) -> Bool;

  public final static native func CloneRecord(path: TweakDBID, base: TweakDBID) -> Bool;

  public final static native func UpdateRecord(path: TweakDBID) -> Bool;

  public final static native func RegisterName(name: CName) -> Bool;

  public final static func SetFlat(name: CName, value: Variant) -> Bool {
    TweakDBManager.RegisterName(name);
    let path = TDBID.Create(NameToString(name));
    return TweakDBManager.SetFlat(path, value);
  }

  public final static func CreateRecord(name: CName, type: CName) -> Bool {
    TweakDBManager.RegisterName(name);
    let path = TDBID.Create(NameToString(name));
    return TweakDBManager.CreateRecord(path, type);
  }

  public final static func CloneRecord(name: CName, base: TweakDBID) -> Bool {
    TweakDBManager.RegisterName(name);
    let path = TDBID.Create(NameToString(name));
    return TweakDBManager.CloneRecord(path, base);
  }
}
