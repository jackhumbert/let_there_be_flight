public class ModStngsMainGameController extends gameuiSettingsMenuGameController {

  private let m_scrollPanel: inkWidgetRef;
  private let m_selectorWidget: inkWidgetRef;
  private let m_buttonHintsManagerRef: inkWidgetRef;
  private let m_settingsOptionsList: inkCompoundRef;
  private let m_applyButton: inkWidgetRef;
  private let m_resetButton: inkWidgetRef;
  private let m_defaultButton: inkWidgetRef;
  private let m_brightnessButton: inkWidgetRef;
  private let m_hdrButton: inkWidgetRef;
  private let m_controllerButton: inkWidgetRef;
  private let m_benchmarkButton: inkWidgetRef;
  private let m_descriptionText: inkTextRef;
  private let m_previousButtonHint: inkWidgetRef;
  private let m_nextButtonHint: inkWidgetRef;
  private let m_languageInstallProgressBarRoot: inkWidgetRef;
  
  private let m_languageInstallProgressBar: wref<SettingsLanguageInstallProgressBar>;
  private let m_menuEventDispatcher: wref<inkMenuEventDispatcher>;
  private let m_settingsElements: array<wref<SettingsSelectorController>>;
  private let m_buttonHintsController: wref<ButtonHints>;
  private let m_data: array<SettingsCategory>;
  private let m_menusList: array<CName>;
  private let m_eventsList: array<CName>;
  private let m_settingsListener: ref<ModSettingsVarListener>;
  private let m_settingsNotificationListener: ref<ModSettingsNotificationListener>;
  private let m_settings: ref<UserSettings>;
  private let m_isPreGame: Bool;
  private let m_benchmarkNotificationToken: ref<inkGameNotificationToken>;
  private let m_isKeybindingAlertEnabled: Bool;
  private let m_applyButtonEnabled: Bool;
  private let m_resetButtonEnabled: Bool;
  private let m_closeSettingsRequest: Bool;
  private let m_resetSettingsRequest: Bool;
  private let m_isDlcSettings: Bool;
  private let m_selectorCtrl: wref<ListController>;

  protected cb func OnInitialize() -> Void {
    ModSettings.GetInstance().isActive = true;
    inkWidgetRef.SetVisible(this.m_hdrButton, false);
    inkWidgetRef.SetVisible(this.m_controllerButton, false);
    inkWidgetRef.SetVisible(this.m_brightnessButton, false);
    inkWidgetRef.SetVisible(this.m_benchmarkButton, false);

    this.m_settings = this.GetSystemRequestsHandler().GetUserSettings();
    this.m_isPreGame = this.GetSystemRequestsHandler().IsPreGame();
    // this.m_settingsListener = new ModSettingsVarListener();
    // this.m_settingsListener.RegisterController(this);
    // this.m_settingsNotificationListener = new ModSettingsNotificationListener();
    // this.m_settingsNotificationListener.RegisterController(this);
    // this.m_settingsNotificationListener.Register();
    // this.m_languageInstallProgressBar = inkWidgetRef.GetControllerByType(this.m_languageInstallProgressBarRoot, n"SettingsLanguageInstallProgressBar") as SettingsLanguageInstallProgressBar;
    this.RegisterToGlobalInputCallback(n"OnPostOnRelease", this, n"OnButtonRelease");
    inkWidgetRef.GetControllerByType(this.m_applyButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnApplyButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_resetButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnResetButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_controllerButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnControllerButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_defaultButton, n"inkButtonController").RegisterToCallback(n"OnButtonClick", this, n"OnDefaultButtonReleased");
    this.m_selectorCtrl = inkWidgetRef.GetController(this.m_selectorWidget) as ListController;
    this.m_selectorCtrl.RegisterToCallback(n"OnItemActivated", this, n"OnMenuChanged");
    this.m_buttonHintsController = this.SpawnFromExternal(inkWidgetRef.Get(this.m_buttonHintsManagerRef), r"base\\gameplay\\gui\\common\\buttonhints.inkwidget", n"Root").GetController() as ButtonHints;
    this.PopulateHints();
    this.PopulateSettingsData();
    // this.PopulateCategories(this.m_settings.GetMenuIndex());
    this.PopulateCategories(0);
    this.CheckButtons();
    this.PlayLibraryAnimation(n"intro");
    this.m_closeSettingsRequest = false;
    this.m_resetSettingsRequest = false;

    ModSettings.RegisterListenerToModifications(this);

    //super.OnInitialize();
  }

  public func OnModSettingsChange() -> Void {
    this.CheckButtons();
    this.PopulateSettingsData();
    this.PopulateCategorySettingsOptions(-1);
    this.RefreshInputIcons();
  }

  protected cb func OnSetUserData(userData: ref<IScriptable>) -> Bool {
    let settingsUserData: ref<SettingsMenuUserData> = userData as SettingsMenuUserData;
    if IsDefined(settingsUserData) {
      this.m_isDlcSettings = settingsUserData.m_isDlcSettings;
    };
  }

  protected cb func OnUninitialize() -> Bool {
    ModSettings.GetInstance().isActive = false;
    this.UnregisterFromGlobalInputCallback(n"OnPostOnRelease", this, n"OnButtonRelease");
    this.m_menuEventDispatcher.UnregisterFromEvent(n"OnBack", this, n"OnBack");
    this.m_selectorCtrl.UnregisterFromCallback(n"OnItemActivated", this, n"OnMenuChanged");
    inkWidgetRef.GetControllerByType(this.m_applyButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnApplyButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_resetButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnResetButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_controllerButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnControllerButtonReleased");
    inkWidgetRef.GetControllerByType(this.m_defaultButton, n"inkButtonController").UnregisterFromCallback(n"OnButtonClick", this, n"OnDefaultButtonReleased");

    ModSettings.UnregisterListenerToModifications(this);

    //super.OnUninitialize();
  }

  public final func EnableApplyButton() -> Void {
    inkWidgetRef.SetVisible(this.m_applyButton, true);
    this.m_applyButtonEnabled = true;
  }

  public final func DisableApplyButton() -> Void {
    this.m_applyButtonEnabled = false;
    inkWidgetRef.SetVisible(this.m_applyButton, false);
  }

  public final func IsApplyButtonEnabled() -> Bool {
    return this.m_applyButtonEnabled;
  }

  public final func EnableResetButton() -> Void {
    this.m_resetButtonEnabled = true;
    inkWidgetRef.SetVisible(this.m_resetButton, true);
  }

  public final func DisableResetButton() -> Void {
    this.m_resetButtonEnabled = false;
    inkWidgetRef.SetVisible(this.m_resetButton, false);
  }

  public final func IsResetButtonEnabled() -> Bool {
    return this.m_resetButtonEnabled;
  }

  public final func CheckButtons() -> Void {
    // if !this.m_isDlcSettings && (this.m_settings.NeedsConfirmation() || this.m_settings.NeedsRestartToApply() || this.m_settings.NeedsLoadLastCheckpoint()) {
    let ms = ModSettings.GetInstance();
    if ms.changeMade {
      this.EnableApplyButton();
      this.EnableResetButton();
    } else {
      this.DisableApplyButton();
      this.DisableResetButton();
    }
  }

  public final func OnVarModified(groupPath: CName, varName: CName, varType: ConfigVarType, reason: ConfigChangeReason) -> Void {
    let i: Int32;
    let item: ref<SettingsSelectorController>;
    let size: Int32;
    Log("[VAR] modified groupPath: " + NameToString(groupPath) + " varName: " + NameToString(varName));
    size = ArraySize(this.m_settingsElements);
    this.CheckButtons();
    i = 0;
    while i < size {
      item = this.m_settingsElements[i];
      if Equals(item.GetGroupPath(), groupPath) && Equals(item.GetVarName(), varName) {
        item.Refresh();
      };
      i += 1;
    };
  }

  // public final func WarnAboutEmptyKeyBindingValue() -> Void {
  //   if this.m_isKeybindingAlertEnabled && this.IsAnyActionWithoutAssignedKey() {
  //     this.PushNotification();
  //     this.m_isKeybindingAlertEnabled = false;
  //   };
  // }

  public final func OnSettingsNotify(status: ConfigNotificationType) -> Void {
    // this.WarnAboutEmptyKeyBindingValue();
    switch status {
      case ConfigNotificationType.RestartRequiredConfirmed:
      case ConfigNotificationType.ChangesApplied:
      case ConfigNotificationType.Saved:
        this.CheckSettings();
        this.PopulateSettingsData();
        this.PopulateCategorySettingsOptions(-1);
        this.RefreshInputIcons();
        break;
      case ConfigNotificationType.ChangesLoadLastCheckpointApplied:
        this.CheckSettings();
        this.PopulateSettingsData();
        this.PopulateCategorySettingsOptions(-1);
        GameInstance.GetTelemetrySystem(this.GetPlayerControlledObject().GetGame()).LogLastCheckpointLoaded();
        this.GetSystemRequestsHandler().LoadLastCheckpoint(true);
        this.RefreshInputIcons();
        break;
      case ConfigNotificationType.ChangesLoadLastCheckpointRejected:
      case ConfigNotificationType.RestartRequiredRejected:
      case ConfigNotificationType.ChangesRejected:
        this.m_closeSettingsRequest = false;
        this.CheckSettings();
        this.PopulateSettingsData();
        this.PopulateCategorySettingsOptions(-1);
        this.RefreshInputIcons();
        break;
      case ConfigNotificationType.ErrorSaving:
        this.RequestClose();
        break;
      case ConfigNotificationType.Refresh:
        this.PopulateSettingsData();
        this.PopulateCategorySettingsOptions(-1);
        this.RefreshInputIcons();
    };
  }

  protected func SetLanguagePackageInstallProgress(progress: Float) -> Void {
    this.m_languageInstallProgressBar.SetProgress(progress);
  }

  protected func SetLanguagePackageInstallProgressBar(progress: Float, completed: Bool, started: Bool) -> Void {
    this.m_languageInstallProgressBar.SetProgressBarVisiblity(!completed && started);
    this.m_languageInstallProgressBar.SetProgress(progress);
  }

  // private final func AddSettingsGroup(settingsGroup: ref<ConfigGroup>) -> Void {
  //   let category: SettingsCategory;
  //   let currentSettingsGroup: ref<ConfigGroup>;
  //   let currentSubcategory: SettingsCategory;
  //   let i: Int32;
  //   let settingsGroups: array<ref<ConfigGroup>>;
  //   category.label = settingsGroup.GetDisplayName();
  //   category.groupPath = settingsGroup.GetPath();
  //   if settingsGroup.HasVars(this.m_isPreGame) {
  //     category.options = settingsGroup.GetVars(this.m_isPreGame);
  //     category.isEmpty = false;
  //   };
  //   settingsGroups = settingsGroup.GetGroups(this.m_isPreGame);
  //   i = 0;
  //   while i < ArraySize(settingsGroups) {
  //     currentSettingsGroup = settingsGroups[i];
  //     if currentSettingsGroup.IsEmpty(this.m_isPreGame) {
  //     } else {
  //       if currentSettingsGroup.HasVars(this.m_isPreGame) {
  //         currentSubcategory.label = currentSettingsGroup.GetDisplayName();
  //         currentSubcategory.options = currentSettingsGroup.GetVars(this.m_isPreGame);
  //         currentSubcategory.isEmpty = false;
  //         ArrayPush(category.subcategories, currentSubcategory);
  //         category.isEmpty = false;
  //         this.m_settingsListener.Register(currentSettingsGroup.GetPath());
  //       };
  //     };
  //     i += 1;
  //   };
  //   if Equals(category.label, n"UI-Settings-KeyBindings") && !this.IsKeyboardConnected() {
  //     category.isEmpty = true;
  //   };
  //   if !category.isEmpty {
  //     ArrayPush(this.m_data, category);
  //     this.m_settingsListener.Register(settingsGroup.GetPath());
  //   };
  // }

  private final func PopulateSettingsData() -> Void {
    // this.m_settingsListener.Register(n"/mods");
    ArrayClear(this.m_data);
    let mods = ModSettings.GetMods();
    let i = 0;
    while i < ArraySize(mods) {
    let category: SettingsCategory;
      category.label = mods[i];
      category.options = ModSettings.GetVars(mods[i], n"None");
      category.isEmpty = false;
      let categories = ModSettings.GetCategories(mods[i]);
      let j = 0;
      while j < ArraySize(categories) {
        let currentSubcategory: SettingsCategory;
        currentSubcategory.label = categories[j];
        currentSubcategory.options = ModSettings.GetVars(mods[i], categories[j]);
        currentSubcategory.isEmpty = false;
        ArrayPush(category.subcategories, currentSubcategory);
        j += 1;
      }
      // FlightLog.Info("[ModStngsMainGameController] " + NameToString(mods[i]) + ": " + i);
      // let j = 0;
      // while j < ArraySize(currentSubcategory.options) {
      //     FlightLog.Info("var: " + ToString(currentSubcategory.options[j].GetDisplayName()));
      //     j += 1;
      // }
      ArrayPush(this.m_data, category);
      i += 1;
    }
  }

  private final func PopulateCategories(idx: Int32) -> Void {
    let curCategory: SettingsCategory;
    let newData: ref<ListItemData>;
    this.m_selectorCtrl.Clear();
    let i = 0;
    while i < ArraySize(this.m_data) {
        curCategory = this.m_data[i];
        if !curCategory.isEmpty {
            newData = new ListItemData();
            newData.label = GetLocalizedTextByKey(curCategory.label);
            if StrLen(newData.label) == 0 {
                newData.label = ToString(curCategory.label);
            };
            this.m_selectorCtrl.PushData(newData);
        };
        i += 1;
    };
    this.m_selectorCtrl.Refresh();
    if idx >= 0 && idx < ArraySize(this.m_data) {
        this.m_selectorCtrl.SetToggledIndex(idx);
    } else {
        this.m_selectorCtrl.SetToggledIndex(0);
    };
  }

  protected cb func OnSetMenuEventDispatcher(menuEventDispatcher: wref<inkMenuEventDispatcher>) -> Bool {
    this.m_menuEventDispatcher = menuEventDispatcher;
    this.m_menuEventDispatcher.RegisterToEvent(n"OnBack", this, n"OnBack");
  }

  protected cb func OnBack(userData: ref<IScriptable>) -> Bool {
    // if !StatusEffectSystem.ObjectHasStatusEffectWithTag(this.GetPlayerControlledObject(), n"LockInHubMenu") {
      this.m_closeSettingsRequest = true;
      this.CheckSettings();
      ModSettings.AcceptChanges();
    // };
  }

  private final func RequestClose() -> Void {
    this.m_menuEventDispatcher.SpawnEvent(n"OnCloseSettingsScreen");
  }

  private final func RequestRestoreDefaults() -> Void {
    let index: Int32 = this.m_selectorCtrl.GetToggledIndex();
    let mod: CName = this.m_data[index].label;
    ModSettings.RestoreDefaults(mod);
    // this.m_settings.RequestRestoreDefaultDialog(this.m_isPreGame, false, groupPath);
  }

  private final func CheckSettings() -> Void {
    if this.m_resetSettingsRequest {
      this.CheckRejectSettings();
    } else {
      this.CheckAcceptSettings();
    };
  }

  private final func CheckRejectSettings() -> Void {
    ModSettings.RejectChanges();
    this.m_resetSettingsRequest = false;
    // if this.m_settings.NeedsConfirmation() {
    //   this.m_settings.RejectChanges();
    // } else {
    //   if this.m_settings.NeedsRestartToApply() {
    //     this.m_settings.RejectRestartToApply();
    //   } else {
    //     if this.m_settings.NeedsLoadLastCheckpoint() {
    //       this.m_settings.RejectLoadLastCheckpointChanges();
    //     } else {
    //       this.m_resetSettingsRequest = false;
    //       if this.m_closeSettingsRequest {
    //         this.m_closeSettingsRequest = false;
    //         this.RequestClose();
    //       };
    //     };
    //   };
    // };
  }

  private final func CheckAcceptSettings() -> Void {
    ModSettings.AcceptChanges();
    // if this.m_settings.WasModifiedSinceLastSave() {
    //   if this.m_settings.NeedsConfirmation() {
    //     this.m_settings.RequestConfirmationDialog();
    //   } else {
    //     if this.m_settings.NeedsRestartToApply() {
    //       this.m_settings.RequestNeedsRestartDialog();
    //     } else {
    //       if this.m_settings.NeedsLoadLastCheckpoint() {
    //         this.m_settings.RequestLoadLastCheckpointDialog();
    //       } else {
    //         this.GetSystemRequestsHandler().RequestSaveUserSettings();
    //         if this.m_closeSettingsRequest {
    //           this.m_closeSettingsRequest = false;
    //           this.RequestClose();
    //         };
    //       };
    //     };
    //   };
    // } else {
    //   if this.m_closeSettingsRequest {
    //     this.m_closeSettingsRequest = false;
    //     this.RequestClose();
    //   };
    // };
  }

  protected cb func OnMenuChanged(index: Int32, target: ref<ListItemController>) -> Bool {
    this.PlaySound(n"Button", n"OnPress");
    this.PopulateCategorySettingsOptions(index);
    (inkWidgetRef.GetController(this.m_scrollPanel) as inkScrollController).SetScrollPosition(0.00);
    // this.m_settings.SetMenuIndex(index);
  }

  protected cb func OnApplyButtonReleased(controller: wref<inkButtonController>) -> Bool {
    this.OnApplyButton();
  }

  protected cb func OnResetButtonReleased(controller: wref<inkButtonController>) -> Bool {
    this.OnResetButton();
  }

  protected cb func OnBenchmarkButtonReleased(controller: wref<inkButtonController>) -> Bool {
    if this.IsBenchmarkPossible() {
      this.RunGraphicsBenchmark();
    };
  }

  protected cb func OnDefaultButtonReleased(controller: wref<inkButtonController>) -> Bool {
    this.RequestRestoreDefaults();
  }

  protected cb func OnLocalizationChanged(evt: ref<inkLocalizationChangedEvent>) -> Bool {
    let idx: Int32 = this.m_selectorCtrl.GetToggledIndex();
    this.PopulateCategories(idx);
    this.PopulateCategorySettingsOptions(idx);
    this.PopulateHints();
  }

  private final func PopulateHints() -> Void {
    this.m_buttonHintsController.ClearButtonHints();
    this.m_buttonHintsController.AddButtonHint(n"select", "UI-UserActions-Select");
    this.m_buttonHintsController.AddButtonHint(n"back", "Common-Access-Close");
    if !this.m_isDlcSettings {
      this.m_buttonHintsController.AddButtonHint(n"restore_default_settings", "UI-UserActions-RestoreDefaults");
    };
  }

  private final func OnApplyButton() -> Void {
    this.m_isKeybindingAlertEnabled = this.m_settings.GetMenuIndex() == 8;
    if !this.IsApplyButtonEnabled() {
      return;
    };
    Log("OnApplyButton");
    if this.m_settings.NeedsConfirmation() {
      this.m_settings.ConfirmChanges();
    } else {
      this.CheckSettings();
    };
  }

  private final func OnResetButton() -> Void {
    if !this.IsResetButtonEnabled() {
      return;
    };
    Log("OnResetButton");
    this.m_resetSettingsRequest = true;
    this.CheckSettings();
  }

  protected cb func OnButtonRelease(evt: ref<inkPointerEvent>) -> Bool {
    let currentToggledIndex: Int32;
    let listSize: Int32 = this.m_selectorCtrl.Size();
    if evt.IsAction(n"prior_menu") {
      currentToggledIndex = this.m_selectorCtrl.GetToggledIndex();
      if currentToggledIndex < 1 {
        this.m_selectorCtrl.SetToggledIndex(listSize - 1);
      } else {
        this.m_selectorCtrl.SetToggledIndex(currentToggledIndex - 1);
      };
    } else {
      if evt.IsAction(n"next_menu") {
        currentToggledIndex = this.m_selectorCtrl.GetToggledIndex();
        if currentToggledIndex >= this.m_selectorCtrl.Size() - 1 {
          this.m_selectorCtrl.SetToggledIndex(0);
        } else {
          this.m_selectorCtrl.SetToggledIndex(currentToggledIndex + 1);
        };
      } else {
        if evt.IsAction(n"restore_default_settings") {
          this.RequestRestoreDefaults();
        } else {
            return false;
        };
      };
    };
  }

  protected cb func OnSettingHoverOver(evt: ref<inkPointerEvent>) -> Bool {
    let descriptionName: CName;
    let params: ref<inkTextParams>;
    let updatePolicy: ConfigVarUpdatePolicy;
    let currentItem: wref<SettingsSelectorController> = evt.GetCurrentTarget().GetController() as SettingsSelectorController;
    if IsDefined(currentItem) {
      descriptionName = currentItem.GetDescription();
      updatePolicy = currentItem.GetVarUpdatePolicy();
      if Equals(updatePolicy, ConfigVarUpdatePolicy.ConfirmationRequired) {
        params = new inkTextParams();
        // params.AddLocalizedName("description", descriptionName);
          if !Equals(descriptionName, n"None") {
            params.AddString("description", ToString(descriptionName));
          } else {
            params.AddString("description", "");
          }
        params.AddLocalizedString("additional_text", "LocKey#76947");
        inkTextRef.SetLocalizedTextScript(this.m_descriptionText, "LocKey#76949", params);
      } else {
        if Equals(updatePolicy, ConfigVarUpdatePolicy.RestartRequired) {
          params = new inkTextParams();
          // params.AddLocalizedName("description", descriptionName);
          if !Equals(descriptionName, n"None") {
            params.AddString("description", ToString(descriptionName));
          } else {
            params.AddString("description", "");
          }
          params.AddLocalizedString("additional_text", "LocKey#76948");
          inkTextRef.SetLocalizedTextScript(this.m_descriptionText, "LocKey#76949", params);
        } else {
          // inkTextRef.SetLocalizedTextScript(this.m_descriptionText, descriptionName);
          if !Equals(descriptionName, n"None") {
            inkTextRef.SetText(this.m_descriptionText, ToString(descriptionName), params);
          } else {
            inkTextRef.SetText(this.m_descriptionText, "", params);
          }
        };
      };
      inkWidgetRef.SetVisible(this.m_descriptionText, true);
    };
  }

  protected cb func OnSettingHoverOut(evt: ref<inkPointerEvent>) -> Bool {
    inkWidgetRef.SetVisible(this.m_descriptionText, false);
  }

  private final func PopulateOptions(options: array<ref<ConfigVar>>) -> Void {
    let currentItem: wref<SettingsSelectorController>;
    let currentSettingsItem: ref<ConfigVar>;
    let currentSettingsItemType: ConfigVarType;
    let size: Int32 = ArraySize(options);
    let i: Int32 = 0;
    while i < size {
      currentSettingsItem = options[i];
      if IsDefined(currentSettingsItem) {
        if currentSettingsItem.IsVisible() {
          currentSettingsItemType = currentSettingsItem.GetType();
          switch currentSettingsItemType {
            case ConfigVarType.Bool:
              currentItem = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsSelectorBool").GetController() as SettingsSelectorController;
              break;
            case ConfigVarType.Int:
              currentItem = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsSelectorInt").GetController() as SettingsSelectorController;
              break;
            case ConfigVarType.Float:
              currentItem = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsSelectorFloat").GetController() as SettingsSelectorController;
              break;
            case ConfigVarType.Name:
              currentItem = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsSelectorKeyBinding").GetController() as SettingsSelectorController;
              break;
            case ConfigVarType.IntList:
              currentItem = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsSelectorIntList").GetController() as SettingsSelectorController;
              break;
            case ConfigVarType.FloatList:
              currentItem = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsSelectorFloatList").GetController() as SettingsSelectorController;
              break;
            case ConfigVarType.StringList:
              currentItem = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsSelectorStringList").GetController() as SettingsSelectorController;
              break;
            case ConfigVarType.NameList:
              currentItem = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsSelectorNameList").GetController() as SettingsSelectorController;
              break;
            default:
              LogUIWarning("Cannot create UI settings drawer for " + NameToString(currentSettingsItem.GetDisplayName()));
          };
          if IsDefined(currentItem) {
            currentItem.Setup(currentSettingsItem, this.m_isPreGame);
            currentItem.RegisterToCallback(n"OnHoverOver", this, n"OnSettingHoverOver");
            currentItem.RegisterToCallback(n"OnHoverOut", this, n"OnSettingHoverOut");
            currentItem.Refresh();
            ArrayPush(this.m_settingsElements, currentItem);
          };
        };
      };
      i += 1;
    };
  }

  private final func PopulateCategorySettingsOptions(idx: Int32) -> Void {
    let categoryController: ref<SettingsCategoryController>;
    let i: Int32;
    let settingsCategory: SettingsCategory;
    let settingsSubCategory: SettingsCategory;
    ArrayClear(this.m_settingsElements);
    inkCompoundRef.RemoveAllChildren(this.m_settingsOptionsList);
    inkWidgetRef.SetVisible(this.m_descriptionText, false);
    if idx < 0 {
      idx = this.m_selectorCtrl.GetToggledIndex();
    };
    settingsCategory = this.m_data[idx];
    this.PopulateOptions(settingsCategory.options);
    i = 0;
    while i < ArraySize(settingsCategory.subcategories) {
      settingsSubCategory = settingsCategory.subcategories[i];
      categoryController = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsCategory").GetController() as SettingsCategoryController;
      if IsDefined(categoryController) {
        categoryController.Setup(settingsSubCategory.label);
      };
      this.PopulateOptions(settingsSubCategory.options);
      i += 1;
    };
    this.m_selectorCtrl.SetSelectedIndex(idx);
  }

}