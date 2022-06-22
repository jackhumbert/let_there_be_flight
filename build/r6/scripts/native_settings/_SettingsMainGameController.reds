// @wrapMethod(SettingsMainGameController)
// protected cb func OnInitialize() -> Void {
//     let ns = NativeSettings.GetInstance();
    
//     if ns.fromMods {
//         // ns.settingsMainController = this;
//         let root = this.GetRootCompoundWidget();
//         root.GetWidgetByPathName(n"wrapper/extra/controller_btn").SetVisible(false);
//         root.GetWidgetByPathName(n"wrapper/extra/brightness_btn").SetVisible(false);
//         root.GetWidgetByPathName(n"wrapper/extra/hdr_btn").SetVisible(false);
//         ns.isAccessingModspace = true;
//         wrappedMethod();
//         // ns.isAccessingModspace = false;
//     } else {
//         wrappedMethod();
//     }


//     // if ns.fromMods {
//     //     ns.settingsOptionsList = this.m_settingsOptionsList;
//     // }
// }

@wrapMethod(SettingsMainGameController)
private final func CheckHDRSettingVisibility() -> Void {
    if !IsDefined(this as ModStngsMainGameController) {
        wrappedMethod();
    }
}

// @wrapMethod(SettingsMainGameController)
// protected cb func OnUninitialize() -> Bool {
//     let ns = NativeSettings.GetInstance();
//     ns.isAccessingModspace = false;
// }

@wrapMethod(SettingsMainGameController)
private final func ShowBrightnessScreen() -> Void {
    if !IsDefined(this as ModStngsMainGameController) {
        wrappedMethod();
    }
}

@wrapMethod(SettingsMainGameController)
private final func ShowControllerScreen() -> Void {
    if !IsDefined(this as ModStngsMainGameController) {
        wrappedMethod();
    }
}

@wrapMethod(SettingsMainGameController)
private final func RequestClose() -> Void {
    if !IsDefined(this as ModStngsMainGameController) {
        NativeSettings.GetInstance().RequestClose();
    }
    wrappedMethod();
}

@wrapMethod(SettingsMainGameController)
private final func PopulateCategories(idx: Int32) -> Void {
    if !IsDefined(this as ModStngsMainGameController) {
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
    } else {
        wrappedMethod(idx);
    }
}

// @wrapMethod(SettingsMainGameController)
// private final func PopulateSettingsData() -> Void {
//     let ns = NativeSettings.GetInstance();
//     if ns.fromMods {
//         // ns.isAccessingModspace = true;
//         wrappedMethod();
//         // ns.isAccessingModspace = false;
//     } else {
//         wrappedMethod();
//     }
// }

// @wrapMethod(SettingsMainGameController)
// private final func PopulateCategorySettingsOptions(idx: Int32) -> Void {
//     let ns = NativeSettings.GetInstance();
//     if ns.fromMods {
//         this.PopulateSettingsData();
//         ns.SaveScrollPosition();
//         ns.OnCurrentTabClosed();

//         inkCompoundRef.RemoveAllChildren(ns.settingsOptionsList);
//         inkTextRef.SetVisible(this.m_descriptionText, false);
//         if idx < 0 {
//             idx = this.m_selectorCtrl.GetToggledIndex();
//         }

//         let settingsCategory = this.m_data[idx + 1];

//         ns.currentTabPath = settingsCategory.groupPath;
//         ns.ClearControllers();

//         ns.currentTab = settingsCategory.groupPath;
//         ns.PopulateOptions(this, ns.currentTab);
//         let i = 0;
//         while i < ArraySize(settingsCategory.subcategories) {
//             let settingsSubCategory = settingsCategory.subcategories[i];
//             let categoryController = this.SpawnFromLocal(inkWidgetRef.Get(this.m_settingsOptionsList), n"settingsCategory").GetController() as SettingsCategoryController;
//             if IsDefined(categoryController) {
//                 categoryController.Setup(settingsSubCategory.label);
//             };
//             ns.PopulateOptions(this, settingsSubCategory);
//             i += 1;
//         };
//         ns.RestoreScrollPosition();
//         this.m_selectorCtrl.SetSelectedIndex(idx);
//     } else {
//         wrappedMethod(idx);
//     }
// }

// @wrapMethod(SettingsMainGameController)
// protected cb func OnSettingHoverOver(evt: ref<inkPointerEvent>) -> Bool {
//     wrappedMethod(evt);
//     let ns = NativeSettings.GetInstance();
//     if ns.fromMods {
//         inkTextRef.SetText(this.m_descriptionText, NameToString(ns.GetOptionTable(evt.GetCurrentTarget().GetController()).GetDescription()));
//         inkTextRef.SetVisible(this.m_descriptionText, true);
//     }
// }