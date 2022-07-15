@replaceMethod(SettingsSelectorControllerFloat)
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

@wrapMethod(SettingsSelectorControllerListName)
public func Refresh() -> Void {
  if IsDefined(this as ModStngsMainGameController) {
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