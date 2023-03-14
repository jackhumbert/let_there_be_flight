@if(!ModuleExists("ImprovedMinimapMain"))
public func IMZ_Comp_SetBlackboardValue(gameInstance: GameInstance, enabled: Bool) {
  GameInstance.GetBlackboardSystem(gameInstance).Get(GetAllBlackboardDefs().UI_ActiveVehicleData).SetBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsPlayerMounted, enabled);
}

@if(ModuleExists("ImprovedMinimapMain"))
public func IMZ_Comp_SetBlackboardValue(gameInstance: GameInstance, enabled: Bool) {
  GameInstance.GetBlackboardSystem(gameInstance).Get(GetAllBlackboardDefs().UI_System).SetBool(GetAllBlackboardDefs().UI_System.IsMounted_IMZ, enabled);
}

@if(!ModuleExists("ImprovedMinimapMain"))
@addMethod(hudCarController)
public func UpdateIMZSpeed(speed: Float, multiplier: Float) { }

@if(ModuleExists("ImprovedMinimapMain"))
@addMethod(hudCarController)
public func UpdateIMZSpeed(speed: Float, multiplier: Float) {
  let value: Float = Cast<Float>(RoundF(speed * 2.0)) / 6.0;
  let resultingValue: Float = value * multiplier;
  GameInstance.GetBlackboardSystem(this.m_activeVehicle.GetGame()).Get(GetAllBlackboardDefs().UI_System).SetFloat(GetAllBlackboardDefs().UI_System.CurrentSpeed_IMZ, resultingValue);
}
