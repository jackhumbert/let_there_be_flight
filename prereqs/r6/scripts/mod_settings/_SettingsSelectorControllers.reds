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

// @wrapMethod(SettingsSelectorControllerBool)
// private func AcceptValue(forward: Bool) -> Void {
//   wrappedMethod(forward);
//   this.Refresh();
// }

// @wrapMethod(SettingsSelectorController)
// protected cb func OnLeft(e: ref<inkPointerEvent>) -> Bool {
//   let og = wrappedMethod(e);
//   this.Refresh();
//   return og;
// }

// @wrapMethod(SettingsSelectorController)
// protected cb func OnRight(e: ref<inkPointerEvent>) -> Bool {
//   let og = wrappedMethod(e);
//   this.Refresh();
//   return og;
// }

// @wrapMethod(SettingsSelectorControllerListName)
// public func Refresh() -> Void {
//   if ModSettings.GetInstance().isActive {
//     let params: ref<inkTextParams>;
//     super.Refresh();
//     this.UpdateValueTextLanguageResources();
//     if !this.m_realValue.ListHasDisplayValues() {
//       inkTextRef.SetText(this.m_ValueText, ToString(this.m_realValue.GetValueFor(this.m_currentIndex)));
//     } else {
//       if Equals(this.m_additionalText, n"") {
//         inkTextRef.SetText(this.m_ValueText, ToString(this.m_realValue.GetDisplayValue(this.m_currentIndex)));
//       } else {
//         params = new inkTextParams();
//         // params.AddLocalizedString("description", GetLocalizedTextByKey(this.m_realValue.GetDisplayValue(this.m_currentIndex)));
//         params.AddString("description", ToString(this.m_realValue.GetDisplayValue(this.m_currentIndex)));
//         params.AddLocalizedString("additional_text", ToString(this.m_additionalText));
//         inkTextRef.SetLocalizedTextScript(this.m_ValueText, "LocKey#76949", params);
//       };
//     };
//     this.SelectDot(this.m_currentIndex);
//   } else {
//     wrappedMethod();
//   }
// }






public class ModStngsSelectorControllerInt extends SettingsSelectorControllerRange {
  private let m_newValue: Int32;
  private let m_sliderWidget: inkWidgetRef;
  private let m_sliderController: wref<inkSliderController>;

  public func Setup(entry: ref<ConfigVar>, isPreGame: Bool) -> Void {
    let value: ref<ModConfigVarInt32>;
    super.Setup(entry, isPreGame);
    value = this.m_SettingsEntry as ModConfigVarInt32;
    this.m_sliderController = inkWidgetRef.GetControllerByType(this.m_sliderWidget, n"inkSliderController") as inkSliderController;
    this.m_sliderController.Setup(Cast<Float>(value.GetMinValue()), Cast<Float>(value.GetMaxValue()), Cast<Float>(this.m_newValue), Cast<Float>(value.GetStepValue()));
    this.m_sliderController.RegisterToCallback(n"OnSliderValueChanged", this, n"OnSliderValueChanged");
    this.m_sliderController.RegisterToCallback(n"OnSliderHandleReleased", this, n"OnHandleReleased");
  }

  protected cb func OnSliderValueChanged(sliderController: wref<inkSliderController>, progress: Float, value: Float) -> Bool {
    this.m_newValue = Cast<Int32>(value);
    this.Refresh();
  }

  protected cb func OnHandleReleased() -> Bool {
    let value: ref<ModConfigVarInt32> = this.m_SettingsEntry as ModConfigVarInt32;
    value.SetValue(this.m_newValue);
  }

  private func RegisterShortcutCallbacks() -> Void {
    super.RegisterShortcutCallbacks();
    this.RegisterToCallback(n"OnRepeat", this, n"OnShortcutRepeat");
  }

  private func ChangeValue(forward: Bool) -> Void {
    let value: ref<ModConfigVarInt32> = this.m_SettingsEntry as ModConfigVarInt32;
    let step: Int32 = forward ? value.GetStepValue() : -value.GetStepValue();
    this.m_newValue = Clamp(this.m_newValue + step, value.GetMinValue(), value.GetMaxValue());
    this.Refresh();
  }

  private func AcceptValue(forward: Bool) -> Void {
    let value: ref<ModConfigVarInt32> = this.m_SettingsEntry as ModConfigVarInt32;
    if value.GetValue() == this.m_newValue {
      this.ChangeValue(forward);
    };
    value.SetValue(this.m_newValue);
  }

  public func Refresh() -> Void {
    super.Refresh();
    this.UpdateValueTextLanguageResources();
    inkTextRef.SetText(this.m_ValueText, IntToString(this.m_newValue));
    this.m_sliderController.ChangeValue(Cast<Float>(this.m_newValue));
  }

  protected cb func OnUpdateValue() -> Bool {
    let value: ref<ModConfigVarInt32> = this.m_SettingsEntry as ModConfigVarInt32;
    this.m_newValue = value.GetValue();
    super.OnUpdateValue();
  }
}

public class ModStngsSelectorControllerBool extends SettingsSelectorController {
  protected let m_onState: inkWidgetRef;
  protected let m_offState: inkWidgetRef;
  protected let m_onStateBody: inkWidgetRef;
  protected let m_offStateBody: inkWidgetRef;

  public func Setup(entry: ref<ConfigVar>, isPreGame: Bool) -> Void {
    super.Setup(entry, isPreGame);
  }

  public func Refresh() -> Void {
    let buttonLogic: ref<inkButtonController>;
    let value: Bool;
    let realValue: ref<ModConfigVarBool> = this.m_SettingsEntry as ModConfigVarBool;
    super.Refresh();
    value = realValue.GetValue();
    inkWidgetRef.SetVisible(this.m_onState, value);
    inkWidgetRef.SetVisible(this.m_offState, !value);
    buttonLogic = inkWidgetRef.GetControllerByType(this.m_onState, n"inkButtonController") as inkButtonController;
    if IsDefined(buttonLogic) {
      buttonLogic.SetEnabled(!this.m_SettingsEntry.IsDisabled());
    };
    buttonLogic = inkWidgetRef.GetControllerByType(this.m_offState, n"inkButtonController") as inkButtonController;
    if IsDefined(buttonLogic) {
      buttonLogic.SetEnabled(!this.m_SettingsEntry.IsDisabled());
    };
  }

  protected cb func OnInitialize() -> Bool {
    super.OnInitialize();
    if inkWidgetRef.IsValid(this.m_offStateBody) {
      inkWidgetRef.RegisterToCallback(this.m_offStateBody, n"OnRelease", this, n"OnLeft");
    };
    if inkWidgetRef.IsValid(this.m_onStateBody) {
      inkWidgetRef.RegisterToCallback(this.m_onStateBody, n"OnRelease", this, n"OnRight");
    };
    if inkWidgetRef.IsValid(this.m_Raycaster) {
      this.RegisterToCallback(n"OnRelease", this, n"OnShortcutPress");
    };
  }

  private func AcceptValue(forward: Bool) -> Void {
    let boolValue: ref<ModConfigVarBool> = this.m_SettingsEntry as ModConfigVarBool;
    boolValue.Toggle();
    this.Refresh();
  }
}

public class ModStngsSelectorControllerFloat extends SettingsSelectorControllerRange {
  public let m_newValue: Float;
  private let m_sliderWidget: inkWidgetRef;
  private let m_sliderController: wref<inkSliderController>;

  public func Setup(entry: ref<ConfigVar>, isPreGame: Bool) -> Void {
    let value: ref<ModConfigVarFloat>;
    super.Setup(entry, isPreGame);
    value = this.m_SettingsEntry as ModConfigVarFloat;
    this.m_sliderController = inkWidgetRef.GetControllerByType(this.m_sliderWidget, n"inkSliderController") as inkSliderController;
    this.m_sliderController.Setup(value.GetMinValue(), value.GetMaxValue(), this.m_newValue, value.GetStepValue());
    this.m_sliderController.RegisterToCallback(n"OnSliderValueChanged", this, n"OnSliderValueChanged");
    this.m_sliderController.RegisterToCallback(n"OnSliderHandleReleased", this, n"OnHandleReleased");
  }

  protected cb func OnSliderValueChanged(sliderController: wref<inkSliderController>, progress: Float, value: Float) -> Bool {
    this.m_newValue = value;
    this.Refresh();
  }

  protected cb func OnHandleReleased() -> Bool {
    let value: ref<ModConfigVarFloat> = this.m_SettingsEntry as ModConfigVarFloat;
    value.SetValue(this.m_newValue);
  }

  private func RegisterShortcutCallbacks() -> Void {
    super.RegisterShortcutCallbacks();
    this.RegisterToCallback(n"OnRepeat", this, n"OnShortcutRepeat");
  }

  private func ChangeValue(forward: Bool) -> Void {
    let value: ref<ModConfigVarFloat> = this.m_SettingsEntry as ModConfigVarFloat;
    let step: Float = forward ? value.GetStepValue() : -value.GetStepValue();
    this.m_newValue = ClampF(this.m_newValue + step, value.GetMinValue(), value.GetMaxValue());
    this.Refresh();
  }

  private func AcceptValue(forward: Bool) -> Void {
    let value: ref<ModConfigVarFloat> = this.m_SettingsEntry as ModConfigVarFloat;
    if value.GetValue() == this.m_newValue {
      this.ChangeValue(forward);
    };
    value.SetValue(this.m_newValue);
    this.Refresh();
  }

  public func Refresh() -> Void {
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
  }

  protected cb func OnUpdateValue() -> Bool {
    let value: ref<ModConfigVarFloat> = this.m_SettingsEntry as ModConfigVarFloat;
    this.m_newValue = value.GetValue();
    super.OnUpdateValue();
  }
}

public class ModStngsSelectorControllerListInt extends SettingsSelectorControllerList {
  public func Setup(entry: ref<ConfigVar>, isPreGame: Bool) -> Void {
    let data: array<Int32>;
    let value: ref<ModConfigVarEnum>;
    super.Setup(entry, isPreGame);
    value = this.m_SettingsEntry as ModConfigVarEnum;
    data = value.GetValues();
    this.PopulateDots(ArraySize(data));
    this.SelectDot(value.GetIndex());
  }

  private func ChangeValue(forward: Bool) -> Void {
    let value: ref<ModConfigVarEnum> = this.m_SettingsEntry as ModConfigVarEnum;
    let listElements: array<Int32> = value.GetValues();
    let index: Int32 = value.GetIndex();
    let newIndex: Int32 = index + (forward ? 1 : -1);
    if newIndex < 0 {
      newIndex = ArraySize(listElements) - 1;
    } else {
      if newIndex >= ArraySize(listElements) {
        newIndex = 0;
      };
    };
    if index != newIndex {
      value.SetIndex(newIndex);
    };
  }

  public func Refresh() -> Void {
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
  }
}