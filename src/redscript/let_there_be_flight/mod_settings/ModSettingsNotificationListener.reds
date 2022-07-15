
public class ModSettingsNotificationListener extends ConfigNotificationListener {

  private let m_ctrl: wref<ModStngsMainGameController>;

  public final func RegisterController(ctrl: ref<ModStngsMainGameController>) -> Void {
    this.m_ctrl = ctrl;
  }

  public func OnNotify(status: ConfigNotificationType) -> Void {
    Log("SettingsNotificationListener::OnNotify");
    this.m_ctrl.OnSettingsNotify(status);
  }
}

public class ModSettingsVarListener extends ConfigVarListener {

  private let m_ctrl: wref<ModStngsMainGameController>;

  public final func RegisterController(ctrl: ref<ModStngsMainGameController>) -> Void {
    this.m_ctrl = ctrl;
  }

  public func OnVarModified(groupPath: CName, varName: CName, varType: ConfigVarType, reason: ConfigChangeReason) -> Void {
    Log("ModSettingsVarListener::OnVarModified");
    this.m_ctrl.OnVarModified(groupPath, varName, varType, reason);
  }
}