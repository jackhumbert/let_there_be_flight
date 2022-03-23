
public abstract native class ScriptableTweak {

  protected cb func OnApply() -> Void
}

@addMethod(TweakDBInterface)
public final static native func GetFlat(path: TweakDBID) -> Variant

@addMethod(TweakDBInterface)
public final static native func GetRecord(path: TweakDBID) -> ref<TweakDBRecord>

@addMethod(TweakDBInterface)
public final static native func GetRecords(type: CName) -> array<ref<TweakDBRecord>>

@addMethod(TweakDBInterface)
public final static native func GetRecordCount(type: CName) -> Uint32

@addMethod(TweakDBInterface)
public final static native func GetRecordByIndex(type: CName, index: Uint32) -> ref<TweakDBRecord>

@addMethod(TweakDBInterface)
public final static func GetRecords(keys: array<TweakDBID>) -> array<ref<TweakDBRecord>> {
  let records: array<ref<TweakDBRecord>>;
  for key in keys {
    let record = TweakDBInterface.GetRecord(key);
    if IsDefined(record) {
      ArrayPush(records, record);
    }
  }
  return records;
}

@addMethod(TweakDBInterface)
public final static func GetRecordIDs(type: CName) -> array<TweakDBID> {
  let ids: array<TweakDBID>;
  for record in TweakDBInterface.GetRecords(type) {
    ArrayPush(ids, record.GetID());
  }
  return ids;
}

public abstract native class TweakDBManager {

  public final static native func SetFlat(path: TweakDBID, value: Variant) -> Bool

  public final static native func CreateRecord(path: TweakDBID, type: CName) -> Bool

  public final static native func CloneRecord(path: TweakDBID, base: TweakDBID) -> Bool

  public final static native func UpdateRecord(path: TweakDBID) -> Bool

  public final static native func RegisterName(name: CName) -> Bool

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
