public native class ModSettings extends IScriptable {
    public native static func GetInstance() -> ref<ModSettings>;
    public native static func GetMods() -> array<CName>;
    public native static func GetCategories(mod: CName) -> array<CName>;
    public native static func GetVars(mod: CName, category: CName) -> array<ref<ConfigVar>>;
}