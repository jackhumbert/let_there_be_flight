public native class ModSettings extends IScriptable {
    public native static func GetInstance() -> ref<ModSettings>;
    public native static func GetMods() -> array<CName>;
    public native static func GetCategories(mod: CName) -> array<CName>;
    public native static func GetVars(mod: CName, category: CName) -> array<ref<ConfigVar>>;
    public native static func AcceptChanges() -> Void;
    public native static func RejectChanges() -> Void;
    public native static func RegisterListenerToClass(self: ref<IScriptable>) -> Void;
    public native static func UnregisterListenerToClass(self: ref<IScriptable>) -> Void;

    public native let changeMade: Bool;
    
    public let isActive: Bool;
}