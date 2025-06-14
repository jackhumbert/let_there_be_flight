// @addMethod(VehicleTransition)
// public func ToFlight(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
//   // if VehicleTransitiorn.CanEnterVehicleFlight() {
//     // let fc = scriptInterface.owner.FindComponentByName(n"flightComponent") as FlightComponent;
//     // if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) &&
//     //     GameInstance.GetQuestsSystem(scriptInterface.GetGame()).GetFact(n"map_blocked") == 0 &&
//     //     Equals(this.GetCurrentTier(stateContext), GameplayTier.Tier1_FullGameplay) {
//     if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) {
//       FlightLog.Info("[VehicleTransition] ToFlight");
//       return true;
//     };
//   };
//   return false;
// }

// // @wrapMethod(VehicleEventsTransition)
// // protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
// //   wrappedMethod(stateContext, scriptInterface);
// //   // let vehicleState = stateContext.GetStateMachineCurrentState(n"Vehicle");
// //   // FlightLog.Info("[PSMVehicle] Just left: " + NameToString(vehicleState));
// //   // let inputState = stateContext.GetStateMachineCurrentState(n"InputContext");
// //   // FlightLog.Info("[InputContext] Current: " + NameToString(inputState));
// // }

// // @addMethod(DriverCombatDecisions)
// // public func ToFlightDriverCombat(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
// //   if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
// //     if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) {
// //       FlightLog.Info("[DriverCombatDecisions] ToFlightDriverCombat");
// //       return true;
// //     };
// //   };
// //   return false;
// // }

// @addMethod(DriverCombatFirearmsDecisions)
// public func ToFlightDriverCombatFirearms(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
//     if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) {
//       FlightLog.Info("[DriverCombatFirearmsDecisions] ToFlightDriverCombatFirearms");
//       return true;
//     };
//   };
//   return false;
// }

// @addMethod(DriverCombatMountedWeaponsDecisions)
// public func ToFlightDriverCombatMountedWeapons(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   if this.IsPlayerAllowedToEnterVehicleFlight(scriptInterface) && VehicleTransition.CanEnterVehicleFlight() {
//     if (scriptInterface.IsActionJustPressed(n"Flight_Toggle") || (IsDefined(fs().playerComponent) && fs().playerComponent.active)) {
//       FlightLog.Info("[DriverCombatMountedWeaponsDecisions] ToFlightDriverCombatMountedWeapons");
//       return true;
//     };
//   };
//   return false;
// }

// @addMethod(VehicleEventsTransition)
// protected func OnEnterFlight(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   this.SetIsInFlight(stateContext, true);
//   if (!(scriptInterface.owner as VehicleObject).GetFlightComponent().active) {
//     // FlightLog.Info("[VehicleEventsTransition] OnEnterFlight, starting");
//     stateContext.SetTemporaryBoolParameter(n"stopVehicleFlight", false, true);
//     this.SetSide(stateContext, scriptInterface);
//     this.SetIsVehicleDriver(stateContext, true);
//     this.PlayerStateChange(scriptInterface, 1);
//     // this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Vehicle, 8);
//     this.SendAnimFeature(stateContext, scriptInterface);
//     this.SetVehFppCameraParams(stateContext, scriptInterface, false);
//     switch (scriptInterface.owner as VehicleObject).GetCameraManager().GetActivePerspective() {
//       case vehicleCameraPerspective.FPP:
//         FlightSystem.GetInstance().cameraIndex = 0;
//         break;
//       case vehicleCameraPerspective.TPPClose:
//         FlightSystem.GetInstance().cameraIndex = 2;
//         break;
//       case vehicleCameraPerspective.TPPFar:
//         FlightSystem.GetInstance().cameraIndex = 3;
//     };

//     this.PauseStateMachines(stateContext, scriptInterface.executionOwner);
    
//     // if !VehicleTransition.CanEnterDriverCombat() {
//     //   stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", true, true);
//     // };
//     // FlightController.GetInstance().Activate();
//     let evt = new VehicleFlightActivationEvent();
//     // evt.vehicle = scriptInterface.owner as VehicleObject;
//     (scriptInterface.owner as VehicleObject).QueueEvent(evt);
//   } else {
//     // FlightLog.Info("[VehicleEventsTransition] OnEnterFlight");
//   }
// }

// @addMethod(VehicleEventsTransition)
// public final func OnExitFlight(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   // FlightLog.Info("[VehicleEventsTransition] OnExitFlight");
//   let stopFlight = stateContext.GetTemporaryBoolParameter(n"stopVehicleFlight");
//   if stopFlight.value {
//     // FlightLog.Info("[VehicleEventsTransition] OnExitFlight, stopping");
//     this.ExitCustomCamera(scriptInterface);
//     this.SetIsVehicleDriver(stateContext, false);
//     this.SendAnimFeature(stateContext, scriptInterface);
//     // (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
//     // FlightController.GetInstance().Deactivate(false);
//     // stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", false, true);
//     let evt = new VehicleFlightDeactivationEvent();
//     evt.silent = false;
//     // evt.vehicle = scriptInterface.owner as VehicleObject;
//     (scriptInterface.owner as VehicleObject).QueueEvent(evt);
//     this.ResumeStateMachines(scriptInterface.executionOwner);
//   } else {
//     // FlightLog.Info("[VehicleEventsTransition] OnExitFlight");
//   }
//   this.SetIsInFlight(stateContext, false);
// }

// @addMethod(VehicleEventsTransition)
// public func OnForcedExitFlight(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   // FlightLog.Info("[VehicleEventsTransition] OnForcedExitFlight");
//   this.ExitCustomCamera(scriptInterface);
//   this.SetIsInFlight(stateContext, false);
//   // (scriptInterface.owner as VehicleObject).ToggleFlightComponent(false);
//   //FlightController.GetInstance().Deactivate(true);
//   // stateContext.SetPermanentBoolParameter(n"ForceEmptyHands", false, true);
//   // this.ResumeStateMachines(scriptInterface.executionOwner);
// }

// @addMethod(VehicleEventsTransition)
// public final func OnUpdateFlight(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   // let fc = FlightController.GetInstance();
//   // fc.OnUpdate(timeDelta, stateContext, scriptInterface);
//   // fc.sys.OnUpdate(timeDelta);
//   this.SetIsInVehicle(stateContext, true);
//   this.SetSide(stateContext, scriptInterface);
//   this.SendAnimFeature(stateContext, scriptInterface);
//   if (!FlightController.GetInstance().showOptions) {
//     this.HandleFlightCameraInput(scriptInterface);
//   }
//   this.HandleFlightExitRequest(stateContext, scriptInterface);
// }


// @addMethod(VehicleEventsTransition)
// protected final func HandleFlightCameraInput(scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   if scriptInterface.IsActionJustTapped(n"ToggleVehCamera") && !this.IsVehicleCameraChangeBlocked(scriptInterface) {
//     this.RequestToggleVehicleFlightCamera(scriptInterface);
//   };
//   if scriptInterface.IsActionJustTapped(n"VehicleCameraInverse") {
//     this.ResetVehicleCamera(scriptInterface);
//   };
// }

// @addMethod(VehicleEventsTransition)
// protected final func RequestToggleVehicleFlightCamera(scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   let camEvent: ref<vehicleRequestCameraPerspectiveEvent>;
//   if scriptInterface.localBlackboard.GetInt(GetAllBlackboardDefs().PlayerStateMachine.Vision) == EnumInt(gamePSMVision.Focus) {
//     return;
//   };
//   camEvent = new vehicleRequestCameraPerspectiveEvent();
//   switch (scriptInterface.owner as VehicleObject).GetCameraManager().GetActivePerspective() {
//     case vehicleCameraPerspective.FPP:
//       if FlightSystem.GetInstance().cameraIndex == 1 {
//         camEvent.cameraPerspective = vehicleCameraPerspective.TPPFar;
//         FlightSystem.GetInstance().cameraIndex = 2;
//       } else {
//         this.EnterCustomCamera(scriptInterface);
//         FlightSystem.GetInstance().cameraIndex = 1;
//       }
//       break;
//     case vehicleCameraPerspective.TPPClose:
//       this.ExitCustomCamera(scriptInterface);
//       camEvent.cameraPerspective = vehicleCameraPerspective.FPP;
//       FlightSystem.GetInstance().cameraIndex = 3;
//       break;
//     case vehicleCameraPerspective.TPPFar:
//       camEvent.cameraPerspective = vehicleCameraPerspective.TPPClose;
//       FlightSystem.GetInstance().cameraIndex = 0;
//   };
//   scriptInterface.executionOwner.QueueEvent(camEvent);
// }

// @addMethod(VehicleEventsTransition)
// public func EnterCustomCamera(scriptInterface: ref<StateGameScriptInterface>) {
//   let camera = (scriptInterface.executionOwner as PlayerPuppet).GetFPPCameraComponent();
//   if IsDefined(camera) {
//     let slotT: WorldTransform;
//     let OccupantSlots = (scriptInterface.owner as VehicleObject).GetVehicleComponent().FindComponentByName(n"OccupantSlots") as SlotComponent;
//     OccupantSlots.GetSlotTransform(n"seat_front_left", slotT);
//     let roof: WorldTransform;
//     let vehicle_slots = (scriptInterface.owner as VehicleObject).GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
//     vehicle_slots.GetSlotTransform(n"CustomFlightCamera", roof);
//     let vwt = Matrix.GetInverted((scriptInterface.owner as VehicleObject).GetLocalToWorld());
//     let v = (WorldPosition.ToVector4(WorldTransform.GetWorldPosition(roof)) * vwt) - (WorldPosition.ToVector4(WorldTransform.GetWorldPosition(slotT)) * vwt);
//     camera.SetLocalPosition(v + Vector4.Vector3To4(FlightSettings.GetVector3("FPVCameraOffset")));
//     camera.SetLocalOrientation(EulerAngles.ToQuat(MakeEulerAngles(FlightSettings.GetInstance().fpvCameraPitchOffset, 0.0, 0.0)));
//   } 

//   // let workspotSystem: ref<WorkspotGameSystem> = scriptInterface.GetWorkspotSystem();
//   // workspotSystem.SwitchSeatVehicle(scriptInterface.owner, scriptInterface.executionOwner, n"OccupantSlots", n"CustomFlightCamera");
// }

// @addMethod(VehicleEventsTransition)
// public func ExitCustomCamera(scriptInterface: ref<StateGameScriptInterface>) {
//   let camera = (scriptInterface.executionOwner as PlayerPuppet).GetFPPCameraComponent();
//   if IsDefined(camera) {
//     camera.SetLocalPosition(new Vector4(0.0, 0.0, 0.0, 0.0));
//     camera.SetLocalOrientation(EulerAngles.ToQuat(MakeEulerAngles(0.0, 0.0, 0.0)));
//   }
//   // let workspotSystem: ref<WorkspotGameSystem> = scriptInterface.GetWorkspotSystem();
//   // workspotSystem.SwitchSeatVehicle(scriptInterface.owner, scriptInterface.executionOwner, n"OccupantSlots", n"seat_front_left");
// }

// @addMethod(VehicleEventsTransition)
// public final func HandleFlightExitRequest(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   let isTeleportExiting: StateResultBool = stateContext.GetPermanentBoolParameter(n"teleportExitActive");
//   let isScheduledExit: StateResultBool = stateContext.GetPermanentBoolParameter(n"validExitAfterSwitchRequest");
//   let isSwitchingSeats: StateResultBool = stateContext.GetPermanentBoolParameter(n"validSwitchSeatExitRequest");
//   if isTeleportExiting.value || isScheduledExit.value || isSwitchingSeats.value {
//     return;
//   };
//   if !this.IsExitVehicleBlocked(scriptInterface) {
//     let stateTime = this.GetInStateTime();      
//     let exitActionPressCount = scriptInterface.GetActionPressCount(n"Exit");
//     let exitPressCountResult = stateContext.GetPermanentIntParameter(n"exitPressCountOnEnter");
//     let onDifferentExitPress = !exitPressCountResult.valid || exitPressCountResult.value != Cast<Int32>(exitActionPressCount);
//     if onDifferentExitPress && stateTime >= 0.30 && scriptInterface.GetActionValue(n"Exit") > 0.00 && scriptInterface.GetActionStateTime(n"Exit") > 0.30{
//       let vehicle = scriptInterface.owner as VehicleObject;
//       // let inputStateTime = scriptInterface.GetActionStateTime(n"Exit");
//       let validUnmount = vehicle.CanUnmount(true, scriptInterface.executionOwner);
//       stateContext.SetPermanentIntParameter(n"vehUnmountDir", EnumInt(validUnmount.direction), true);
//       this.ExitWithTeleport(stateContext, scriptInterface, validUnmount, false, true);
//     }
//   }
// }