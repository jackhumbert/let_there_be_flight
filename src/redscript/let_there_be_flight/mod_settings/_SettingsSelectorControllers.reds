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

// @addMethod(SettingsSelectorController)
// public func SetupMod(entry: ref<ConfigVar>, isPreGame: Bool) -> Void {
//   this.m_SettingsEntry = entry;
//   this.m_IsPreGame = isPreGame;
//   this.m_varGroupPath = this.m_SettingsEntry.GetGroupPath();
//   this.m_varName = this.m_SettingsEntry.GetName();
//   this.BindSettings(entry);
// }

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

@wrapMethod(SettingsSelectorControllerFloat)
public func Refresh() -> Void {
  if ModSettings.GetInstance().isActive {
    super.Refresh();
    this.UpdateValueTextLanguageResources();
    let value = this.m_SettingsEntry as ConfigVarFloat;
    let step = value.GetStepValue();
    let prec = 1;
    if (step < 0.1) {
      prec = 2;
    }
    if (step < 0.01) {
      prec = 3;
    }
    if (step < 0.001) {
      prec = 4;
    }
    if (step < 0.0001) {
      prec = 5;
    }
    inkTextRef.SetText(this.m_ValueText, FloatToStringPrec(this.m_newValue, prec));
    this.m_sliderController.ChangeValue(this.m_newValue);
  } else {
    wrappedMethod();
  }
}

@wrapMethod(SettingsSelectorControllerBool)
private func AcceptValue(forward: Bool) -> Void {
  wrappedMethod(forward);
  this.Refresh();
}

@wrapMethod(SettingsSelectorController)
protected cb func OnLeft(e: ref<inkPointerEvent>) -> Bool {
  let og = wrappedMethod(e);
  this.Refresh();
  return og;
}


@wrapMethod(SettingsSelectorController)
protected cb func OnRight(e: ref<inkPointerEvent>) -> Bool {
  let og = wrappedMethod(e);
  this.Refresh();
  return og;
}


@wrapMethod(SettingsSelectorControllerListInt)
public func Refresh() -> Void {
  if ModSettings.GetInstance().isActive {
    let index: Int32;
    let value: ref<ConfigVarListInt>;
    super.Refresh();
    value = this.m_SettingsEntry as ConfigVarListInt;
    index = value.GetIndex();
    this.UpdateValueTextLanguageResources();
    if !value.ListHasDisplayValues() {
      inkTextRef.SetText(this.m_ValueText, IntToString(value.GetValue()));
    } else {
      inkTextRef.SetText(this.m_ValueText, ToString(value.GetDisplayValue(index)));
    };
    this.SelectDot(index);
  } else {
    wrappedMethod();
  }
}

@wrapMethod(SettingsSelectorControllerListName)
public func Refresh() -> Void {
  if ModSettings.GetInstance().isActive {
    let params: ref<inkTextParams>;
    super.Refresh();
    this.UpdateValueTextLanguageResources();
    if !this.m_realValue.ListHasDisplayValues() {
      inkTextRef.SetText(this.m_ValueText, ToString(this.m_realValue.GetValueFor(this.m_currentIndex)));
    } else {
      if Equals(this.m_additionalText, n"") {
        inkTextRef.SetText(this.m_ValueText, ToString(this.m_realValue.GetDisplayValue(this.m_currentIndex)));
      } else {
        params = new inkTextParams();
        // params.AddLocalizedString("description", GetLocalizedTextByKey(this.m_realValue.GetDisplayValue(this.m_currentIndex)));
        params.AddString("description", ToString(this.m_realValue.GetDisplayValue(this.m_currentIndex)));
        params.AddLocalizedString("additional_text", ToString(this.m_additionalText));
        inkTextRef.SetLocalizedTextScript(this.m_ValueText, "LocKey#76949", params);
      };
    };
    this.SelectDot(this.m_currentIndex);
  } else {
    wrappedMethod();
  }
}