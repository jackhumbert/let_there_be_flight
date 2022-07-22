public native class ModSettings extends IScriptable {
    public native static func GetInstance() -> ref<ModSettings>;
    public native static func GetMods() -> array<CName>;
    public native static func GetCategories(mod: CName) -> array<CName>;
    public native static func GetVars(mod: CName, category: CName) -> array<ref<ConfigVar>>;
    public native static func AcceptChanges() -> Void;
    public native static func RejectChanges() -> Void;
    public native static func RestoreDefaults(mod: CName) -> Void;
    public native static func RegisterListenerToClass(self: ref<IScriptable>) -> Void;
    public native static func UnregisterListenerToClass(self: ref<IScriptable>) -> Void;
    public native static func RegisterListenerToModifications(self: ref<IScriptable>) -> Void;
    public native static func UnregisterListenerToModifications(self: ref<IScriptable>) -> Void;

    public native let changeMade: Bool;
    
    public let isActive: Bool;
}

public native class ModConfigVarBool extends ConfigVar {
  public native func SetValue(value: Bool) -> Void;
  public native func GetValue() -> Bool;
  public native func GetDefaultValue() -> Bool;
  public func Toggle() -> Void {
    this.SetValue(!this.GetValue());
  }
}

public native class ModConfigVarFloat extends ConfigVar {
  public native func SetValue(value: Float) -> Void;
  public native func GetValue() -> Float;
  public native func GetDefaultValue() -> Float;
  public native func GetMinValue() -> Float;
  public native func GetMaxValue() -> Float;
  public native func GetStepValue() -> Float;
}

public native class ModConfigVarInt32 extends ConfigVar {
  public native func SetValue(value: Int32) -> Void;
  public native func GetValue() -> Int32;
  public native func GetDefaultValue() -> Int32;
  public native func GetMinValue() -> Int32;
  public native func GetMaxValue() -> Int32;
  public native func GetStepValue() -> Int32;
}

public native class ModConfigVarEnum extends ConfigVar {
  public native func GetValueFor(index: Int32) -> Int32;
  public native func GetValue() -> Int32;
  public native func GetDefaultValue() -> Int32;
  public native func GetValues() -> array<Int32>;
  public native func GetIndexFor(value: Int32) -> Int32;
  public native func GetIndex() -> Int32;
  public native func GetDefaultIndex() -> Int32;
  public native func SetIndex(index: Int32) -> Void;
  public native func GetDisplayValue(index: Int32) -> CName;
}