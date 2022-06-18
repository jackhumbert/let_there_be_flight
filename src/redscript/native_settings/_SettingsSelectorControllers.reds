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