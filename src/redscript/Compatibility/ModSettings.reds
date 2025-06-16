@if(ModuleExists("ModSettingsModule")) 
public func LTBF_RegisterListener(listener: ref<IScriptable>) {
  // FlightLog.Info("Registering Listener for type: " + NameToString(listener.GetClassName()));
  // ModSettings.RegisterListenerToClass(listener);
}

@if(!ModuleExists("ModSettingsModule")) 
public func LTBF_RegisterListener(listener: ref<IScriptable>) { }

@if(ModuleExists("ModSettingsModule")) 
public func LTBF_UnregisterListener(listener: ref<IScriptable>) {
  // FlightLog.Info("Unregistering Listener for type: " + NameToString(listener.GetClassName()));
  // ModSettings.UnregisterListenerToClass(listener);
}

@if(!ModuleExists("ModSettingsModule")) 
public func LTBF_UnregisterListener(listener: ref<IScriptable>) { }