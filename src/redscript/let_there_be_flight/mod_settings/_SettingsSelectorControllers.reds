// @wrapMethod(SettingsSelectorControllerBool)
// private func AcceptValue(forward: Bool) -> Void {
//     let ns = NativeSettings.GetInstance();
//     if ns.fromMods {
//         let ot = ns.GetOptionTable(this) as ConfigVarBool;
//         inkWidgetRef.SetVisible(this.m_onState, ot.GetValue());
//         inkWidgetRef.SetVisible(this.m_offState, !ot.GetValue());
//         // data.callback(data.state)
//     }
//     wrappedMethod(forward);
// }

// @wrapMethod(SettingsSelectorControllerInt)
// public func Refresh() -> Void {
//     let ns = NativeSettings.GetInstance();
//     if ns.fromMods {
//         let ot = ns.GetOptionTable(this) as ConfigVarInt;
//         if ot.GetValue() != this.m_newValue {
//             ot.SetValue(this.m_newValue);
//             inkTextRef.SetText(this.m_ValueText, ToString(this.m_newValue));
//             this.m_sliderController.ChangeValue(Cast<Float>(this.m_newValue));
//         }
//     } else {
//         wrappedMethod();
//     }
// }

// @wrapMethod(SettingsSelectorControllerInt)
// public func ChangeValue(forward: Bool) -> Void {
//     let ns = NativeSettings.GetInstance();
//     if ns.fromMods {
//         let ot = ns.GetOptionTable(this) as ConfigVarInt;
//         let step: Int32 = forward ? ot.GetStepValue() : -ot.GetStepValue();
//         this.m_newValue = Clamp(this.m_newValue + step, ot.GetMinValue(), ot.GetMaxValue());
//         this.Refresh();
//     } else {
//         wrappedMethod(forward);
//     }
// }

@addMethod(SettingsSelectorController)
public func SetupMod(entry: ref<ConfigVar>, isPreGame: Bool) -> Void {
  this.m_SettingsEntry = entry;
  this.m_IsPreGame = isPreGame;
  this.m_varGroupPath = this.m_SettingsEntry.GetGroupPath();
  this.m_varName = this.m_SettingsEntry.GetName();
  this.BindSettings(entry);
}

@replaceMethod(SettingsSelectorController)
public func Refresh() -> Void {
  let i: Int32;
  let languageProvider: ref<inkLanguageOverrideProvider>;
  let modifiedSymbol: String;
  let text: String;
  let updatePolicy: ConfigVarUpdatePolicy;
  let wasModified: Bool;
  let size: Int32 = this.m_SettingsEntry.GetDisplayNameKeysSize();
  if size > 0 {
    text = NameToString(this.m_SettingsEntry.GetDisplayName());
    i = 0;
    while i < size {
      text = StrReplace(text, "%", GetLocalizedTextByKey(this.m_SettingsEntry.GetDisplayNameKey(i)));
      i += 1;
    };
  } else {
    text = GetLocalizedTextByKey(this.m_SettingsEntry.GetDisplayName());
  };
  updatePolicy = this.m_SettingsEntry.GetUpdatePolicy();
  if Equals(text, "") {
    text = NameToString(this.m_SettingsEntry.GetDisplayName());
  };
  if Equals(updatePolicy, ConfigVarUpdatePolicy.ConfirmationRequired) {
    modifiedSymbol = "*";
    wasModified = this.m_SettingsEntry.HasRequestedValue();
  } else {
    if Equals(updatePolicy, ConfigVarUpdatePolicy.RestartRequired) || Equals(updatePolicy, ConfigVarUpdatePolicy.LoadLastCheckpointRequired) {
      modifiedSymbol = "!";
      wasModified = this.m_SettingsEntry.HasRequestedValue() || this.m_SettingsEntry.WasModifiedSinceLastSave();
    } else {
      modifiedSymbol = "";
      wasModified = false;
    };
  };
  languageProvider = inkWidgetRef.GetUserData(this.m_LabelText, n"inkLanguageOverrideProvider") as inkLanguageOverrideProvider;
  languageProvider.SetLanguage(scnDialogLineLanguage.Origin);
  inkTextRef.UpdateLanguageResources(this.m_LabelText, false);
  inkTextRef.SetText(this.m_LabelText, text);
  inkWidgetRef.SetVisible(this.m_ModifiedFlag, wasModified);
  inkTextRef.SetText(this.m_ModifiedFlag, modifiedSymbol);
}