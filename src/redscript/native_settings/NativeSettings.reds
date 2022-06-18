public native class NativeSettings extends IScriptable {
    public native static func GetInstance() -> ref<NativeSettings>;
    public native let isAccessingModspace: Bool;

    public let fromMods: Bool;
    public let settingsOptionsList: inkCompoundRef;
    public let settingsMainController: ref<SettingsMainGameController>;
    public let currentTab: CName;
    public let currentTabPath: CName;
    public let data: array<ref<ConfigGroup>>;

    public func RequestClose() -> Void {
        // this.OnCurrentTabClosed();
        // this.currentTabPath = n"";
        this.fromMods = false;
        // this.settingsMainController = null;
        // this.ClearControllers();
    }

    public func OnCurrentTabClosed() -> Void {

    }

    public func ClearControllers() -> Void {

    }

    public func PopulateOptions(controller: ref<SettingsMainGameController>, path: CName) -> Void {

    }

    public func PopulateOptions(controller: ref<SettingsMainGameController>, path: SettingsCategory) -> Void {

    }

    public func AddTab(path: CName, label: String, opt requestClose: CallbackHandle) -> Void {

    }

    public func SaveScrollPosition() {

    }

    public func RestoreScrollPosition() {

    }

    public func GetOptionTable(controller: ref<inkLogicController>) -> ref<ConfigVar> {
        return new ConfigVar();
    }
}