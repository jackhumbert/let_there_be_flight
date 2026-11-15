@replaceMethod(VehicleComponent)
protected cb func OnVehicleWaterEvent(evt: ref<VehicleWaterEvent>) -> Bool {
  if evt.isInWater  && !this.GetPS().GetIsSubmerged() {
    if !Equals(GetMountedVehicle(FlightController.GetInstance().player), this.GetVehicle()) && FlightController.GetInstance().IsActive() {
      this.BreakAllDamageStageFX(true);
      this.DestroyRandomWindow();
      this.ApplyVehicleDOT(n"high");
    }
    GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"fire");
  }
  ScriptedPuppet.ReevaluateOxygenConsumption(this.m_mountedPlayer);
  if FlightController.GetInstance().IsActive() {
    let playerPuppet = GameInstance.GetPlayerSystem(this.GetVehicle().GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet;
    let playerStateMachineBlackboard = GameInstance.GetBlackboardSystem(this.GetVehicle().GetGame()).GetLocalInstanced(playerPuppet.GetEntityID(), GetAllBlackboardDefs().PlayerStateMachine);
    playerStateMachineBlackboard.SetInt(GetAllBlackboardDefs().PlayerStateMachine.Swimming, EnumInt(gamePSMSwimming.Surface), true);
  }
}

@wrapMethod(VehicleComponent)
private final func ExplodeVehicle(instigator: wref<GameObject>) -> Void {
  wrappedMethod(instigator);
  let flight = this.GetVehicle().GetFlightComponent();
  if IsDefined(flight) {
    flight.HandleExplosion();
  }
}

