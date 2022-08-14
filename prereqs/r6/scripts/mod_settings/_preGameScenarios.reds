@addMethod(MenuScenario_SingleplayerMenu)
protected cb func OnSwitchToModSettings() -> Bool {
    this.CloseSubMenu();
    this.SwitchToScenario(n"MenuScenario_ModSettings");
}

@addMethod(MenuScenario_SingleplayerMenu)
protected cb func OnCloseModSettings() -> Bool {
    if Equals(this.m_currSubMenuName, n"mod_settings_main") {
        this.CloseSubMenu();
    };
}



public class MenuScenario_ModSettings extends MenuScenario_PreGameSubMenu {

  protected cb func OnEnterScenario(prevScenario: CName, userData: ref<IScriptable>) -> Bool {
    super.OnEnterScenario(prevScenario, userData);
    this.GetMenusState().OpenMenu(n"mod_settings_main", userData);
  }

  protected cb func OnLeaveScenario(nextScenario: CName) -> Bool {
    super.OnLeaveScenario(nextScenario);
    this.GetMenusState().CloseMenu(n"mod_settings_main");
  }

  protected func OnSubmenuOpen() -> Void {
    this.GetMenusState().CloseMenu(n"mod_settings_main");
  }

  protected cb func OnSettingsBack() -> Bool {
    if NotEquals(this.m_currSubMenuName, n"") {
      this.CloseSubMenu();
      this.GetMenusState().OpenMenu(n"mod_settings_main");
    } else {
      this.CloseSettings(false);
    };
  }

  protected cb func OnCloseModSettingsScreen() -> Bool {
    this.CloseSettings(true);
  }

  private final func CloseSettings(forceCloseSettings: Bool) -> Void {
    let menuState: wref<inkMenusState> = this.GetMenusState();
    if forceCloseSettings {
      menuState.CloseMenu(n"mod_settings_main");
      if NotEquals(this.m_currSubMenuName, n"") {
        if !menuState.DispatchEvent(this.m_currSubMenuName, n"OnBack") {
          this.CloseSubMenu();
        };
      } else {
        this.SwitchToScenario(this.m_prevScenario);
      };
    } else {
      menuState.DispatchEvent(n"mod_settings_main", n"OnBack");
    };
  }

  protected cb func OnMainMenuBack() -> Bool {
    this.SwitchToScenario(this.m_prevScenario);
  }
}