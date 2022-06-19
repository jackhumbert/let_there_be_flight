public class ModStngsMainGameController extends SettingsMainGameController {
  protected cb func OnInitialize() -> Void {
    let ns = NativeSettings.GetInstance();
    
    let root = this.GetRootCompoundWidget();
    root.GetWidgetByPathName(n"wrapper/extra/controller_btn").SetVisible(false);
    root.GetWidgetByPathName(n"wrapper/extra/brightness_btn").SetVisible(false);
    root.GetWidgetByPathName(n"wrapper/extra/hdr_btn").SetVisible(false);
    ns.isAccessingModspace += 1;
    // FlightLog.Info("In ModStngsMainGameController");
        
    this.m_settings = this.GetSystemRequestsHandler().GetUserSettings();
    this.m_isPreGame = this.GetSystemRequestsHandler().IsPreGame();
    this.m_settingsListener = new SettingsVarListener();
    this.m_settingsListener.RegisterController(this);
    this.m_settingsNotificationListener = new SettingsNotificationListener();
    this.m_settingsNotificationListener.RegisterController(this);
    this.m_settingsNotificationListener.Register();
    this.m_languageInstallProgressBar = inkWidgetRef.GetControllerByType(this.m_languageInstallProgressBarRoot, n"SettingsLanguageInstallProgressBar") as SettingsLanguageInstallProgressBar;
    if !this.m_isDlcSettings {
      this.RegisterToGlobalInputCallback(n"OnPostOnRelease", this, n"OnButtonRelease");
      inkWidgetRef.GetControllerByType(this.m_applyButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnApplyButtonReleased");
      inkWidgetRef.GetControllerByType(this.m_resetButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnResetButtonReleased");
      inkWidgetRef.GetControllerByType(this.m_brightnessButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnBrightnessButtonReleased");
      inkWidgetRef.GetControllerByType(this.m_hdrButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnHDRButtonReleased");
      inkWidgetRef.GetControllerByType(this.m_controllerButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnControllerButtonReleased");
      inkWidgetRef.GetControllerByType(this.m_benchmarkButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnBenchmarkButtonReleased");
      inkWidgetRef.GetControllerByType(this.m_defaultButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnDefaultButtonReleased");
      if !this.IsBenchmarkPossible() {
        inkWidgetRef.SetVisible(this.m_benchmarkButton, false);
      };
    } else {
      inkWidgetRef.SetVisible(this.m_defaultButton, false);
      inkWidgetRef.SetVisible(this.m_controllerButton, false);
      inkWidgetRef.SetVisible(this.m_previousButtonHint, false);
      inkWidgetRef.SetVisible(this.m_nextButtonHint, false);
    };
    this.m_selectorCtrl = inkWidgetRef.GetController(this.m_selectorWidget) as ListController;
    this.m_selectorCtrl.RegisterToCallback(n"OnItemActivated", this, n"OnMenuChanged");
    this.m_buttonHintsController = this.SpawnFromExternal(inkWidgetRef.Get(this.m_buttonHintsManagerRef), r"base\\gameplay\\gui\\common\\buttonhints.inkwidget", n"Root").GetController() as ButtonHints;
    this.PopulateHints();
    this.PopulateSettingsData();
    this.PopulateCategories(this.m_settings.GetMenuIndex());
    this.CheckButtons();
    this.PlayLibraryAnimation(n"intro");
    this.m_closeSettingsRequest = false;
    this.m_resetSettingsRequest = false;
    if this.m_isPreGame {
      this.GetSystemRequestsHandler().RequestTelemetryConsent(true);
    };
  }

  protected cb func OnUninitialize() -> Bool {
    this.UnregisterFromGlobalInputCallback(n"OnPostOnRelease", this, n"OnButtonRelease");
    this.m_menuEventDispatcher.UnregisterFromEvent(n"OnBack", this, n"OnBack");
    this.m_selectorCtrl.UnregisterFromCallback(n"OnItemActivated", this, n"OnMenuChanged");
    inkWidgetRef.GetControllerByType(this.m_applyButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnApplyButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_resetButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnResetButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_brightnessButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnBrightnessButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_controllerButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnControllerButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_benchmarkButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnBenchmarkButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_defaultButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnDefaultButtonReleased");

    let ns = NativeSettings.GetInstance();
    ns.isAccessingModspace -= 1;
  }
}