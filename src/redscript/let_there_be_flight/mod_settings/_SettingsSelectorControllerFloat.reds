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