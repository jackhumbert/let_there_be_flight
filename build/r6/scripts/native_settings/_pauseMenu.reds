@replaceMethod(PauseMenuGameController)
private func PopulateMenuItemList() -> Void {
    this.AddMenuItem(GetLocalizedText("UI-Labels-Resume"), n"OnClosePauseMenu");
    if !IsFinal() || UseProfiler() {
        this.AddMenuItem("OPEN DEBUG MENU", n"OnOpenDebugHubMenu");
    };
    this.AddMenuItem(GetLocalizedText("UI-ResourceExports-SaveGame"), PauseMenuAction.Save);
    if this.m_savesCount > 0 {
        this.AddMenuItem(GetLocalizedText("UI-ScriptExports-LoadGame0"), n"OnSwitchToLoadGame");
    };
    this.AddMenuItem(GetLocalizedText("UI-Labels-Settings"), n"OnSwitchToSettings");
    this.AddMenuItem("Mod Settings", n"OnSwitchToModSettings");
    this.AddMenuItem(GetLocalizedText("UI-DLC-MenuTitle"), n"OnSwitchToDlc");
    this.AddMenuItem(GetLocalizedText("UI-Labels-Credits"), n"OnSwitchToCredits");
    if TrialHelper.IsInPS5TrialMode() {
        this.AddMenuItem(GetLocalizedText("UI-Notifications-Ps5TrialBuyMenuItem"), n"OnBuyGame");
    };
    this.AddMenuItem(GetLocalizedText("UI-Labels-ExitToMenu"), PauseMenuAction.ExitToMainMenu);
    this.m_menuListController.Refresh();
    this.SetCursorOverWidget(inkCompoundRef.GetWidgetByIndex(this.m_menuList, 0));
}