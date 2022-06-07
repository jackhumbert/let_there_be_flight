@replaceMethod(VehicleComponent)
protected cb func OnVehicleWaterEvent(evt: ref<VehicleWaterEvent>) -> Bool {
  if evt.isInWater  && !this.GetPS().GetIsSubmerged() {
    if !Equals(GetMountedVehicle(FlightController.GetInstance().player), this.GetVehicle()) && FlightController.GetInstance().IsActive() {
      this.BreakAllDamageStageFX(true);
      this.DestroyVehicle();
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
  this.GetVehicle().GetFlightComponent().isDestroyed = true;
  this.GetVehicle().GetFlightComponent().hasExploded = true;
  this.GetVehicle().GetFlightComponent().hasUpdate = false;
  this.GetVehicle().GetFlightComponent().Deactivate(true);
}

// @wrapMethod(VehicleComponent) 
// protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
//   wrappedMethod(action, consumer);
//   let actionName: CName = ListenerAction.GetName(action);
//   let value: Float = ListenerAction.GetValue(action);
//   if Equals(actionName, n"Choice1") && ListenerAction.IsButtonJustReleased(action) {
//     FlightLog.Info("Attempting to repair vehicle");
//     this.RepairVehicle();
//     let player: ref<PlayerPuppet> = GetPlayer(this.GetVehicle().GetGame());
//     let uiSystem: ref<UISystem> = GameInstance.GetUISystem(this.GetVehicle().GetGame());
//     player.UnregisterInputListener(this, n"Choice1");
//     uiSystem.QueueEvent(FlightController.HideHintFromSource(n"RepairVehicle"));
//   }
// }


// requires vehicle to be off to control? also makes a sound, which is nice
// @replaceMethod(VehicleComponent)
// private final func SetupThrusterFX() -> Void {
//   let toggle: Bool = (this.GetPS() as VehicleComponentPS).GetThrusterState();
//   if toggle || (Equals(FlightController.GetInstance().GetVehicle(), this.GetVehicle()) && FlightController.GetInstance().GetThrusterState()) {
//     GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"thrusters", true);
//   } else {
//     GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"thrusters");
//   };
// }