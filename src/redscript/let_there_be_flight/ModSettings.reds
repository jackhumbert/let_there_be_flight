@if(ModuleExists("ModSettingsModule")) 
public func LTBF_RegisterListener(listener: ref<IScriptable>) {
  ModSettings.RegisterListenerToClass(listener);
}

@if(!ModuleExists("ModSettingsModule")) 
public func LTBF_RegisterListener(listener: ref<IScriptable>) { }

@if(ModuleExists("ModSettingsModule")) 
public func LTBF_UnregisterListener(listener: ref<IScriptable>) {
  ModSettings.UnregisterListenerToClass(listener);
}

@if(!ModuleExists("ModSettingsModule")) 
public func LTBF_UnregisterListener(listener: ref<IScriptable>) { }