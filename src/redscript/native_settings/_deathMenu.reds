@replaceMethod(DeathMenuGameController)
private func PopulateMenuItemList() -> Void {
    if this.GetSystemRequestsHandler().HasLastCheckpoint() {
        this.AddMenuItem(GetLocalizedText("UI-ScriptExports-LoadLastSavegame"), PauseMenuAction.QuickLoad);
    };
    this.AddMenuItem(GetLocalizedText("UI-ScriptExports-LoadGame0"), n"OnSwitchToLoadGame");
    this.AddMenuItem(GetLocalizedText("UI-Labels-Settings"), n"OnSwitchToSettings");
    this.AddMenuItem("Mod Settings", n"OnSwitchToModSettings");
    this.AddMenuItem(GetLocalizedText("UI-Labels-ExitToMenu"), PauseMenuAction.ExitToMainMenu);
    this.m_menuListController.Refresh();
    this.SetCursorOverWidget(inkCompoundRef.GetWidgetByIndex(this.m_menuList, 0));
}