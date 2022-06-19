@addMethod(MenuScenario_SingleplayerMenu)
protected cb func OnSwitchToModSettings() -> Bool {
    this.CloseSubMenu();
    this.SwitchToScenario(n"MenuScenario_ModSettings");
}