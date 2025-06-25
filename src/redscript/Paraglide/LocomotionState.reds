// public class ParaglideDecisions extends LocomotionAirDecisions {

//   protected const func EnterCondition(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//     let shouldFall: Bool = this.ShouldFall(stateContext, scriptInterface);
//     if shouldFall {
//       scriptInterface.GetAudioSystem().NotifyGameTone(n"StartFalling");
//     };
//     return shouldFall && scriptInterface.IsActionJustTapped(n"Flight_Toggle");
//     // return scriptInterface.IsActionJustTapped(n"Flight_Toggle");
//   }
// }

// public class ParaglideEvents extends LocomotionAirEvents {

//   public final func OnEnterFromDodge(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     if !this.IsTouchingGround(scriptInterface) {
//       stateContext.SetPermanentBoolParameter(n"enteredFallFromAirDodge", true, true);
//       this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Fall, EnumInt(gamePSMFallStates.RegularFall));
//     };
//     this.OnEnter(stateContext, scriptInterface);
//   }

//   public final func OnEnterFromDodgeAir(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     stateContext.SetPermanentBoolParameter(n"enteredFallFromAirDodge", true, true);
//     this.OnEnter(stateContext, scriptInterface);
//   }

//   public func ToFall(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//     return scriptInterface.IsActionJustTapped(n"Flight_Toggle");
//   }

//   protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     super.OnEnter(stateContext, scriptInterface);
//     this.PlaySound(n"lcm_falling_wind_loop", scriptInterface);
//     scriptInterface.PushAnimationEvent(n"Fall");
//     this.SetDetailedState(scriptInterface, gamePSMDetailedLocomotionStates.Fall);    
//     this.ShowInputHint(scriptInterface, n"Flight_Toggle", n"Paraglider", LocKeyToString(n"Input-Hint-Disable-Flight"), inkInputHintHoldIndicationType.FromInputConfig, false, 127);
//   }

//   protected func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {

//     // slow fall if exited during flight
//     // let verticalSpeed = this.GetVerticalSpeed(scriptInterface);
//     // // let threshold = stateContext.GetFloatParameter(n"HardLandingFallingSpeed", true);
//     // let threshold = this.GetFallingSpeedBasedOnHeight(scriptInterface, 15.0);
//     // if verticalSpeed < threshold {
//     //   let velocity = FromVariant<Vector4>(scriptInterface.GetStateVectorParameter(physicsStateValue.LinearVelocity));
//     //   velocity.Z = verticalSpeed;

//     //   // scriptInterface.SetStateVectorParameter(physicsStateValue.LinearVelocity, ToVariant(new Vector3(0, 0, this.GetFallingSpeedBasedOnHeight(scriptInterface, 10.0))));
//     //   scriptInterface.SetStateVectorParameter(physicsStateValue.LinearVelocity, ToVariant(velocity));

//     //   // let ev = new PSMImpulse();
//     //   // ev.id = n"impulse";
//     //   // ev.impulse =  new Vector4(0.0, 0.0, 2.0, 1.0);
//     //   // scriptInterface.owner.QueueEvent(ev);
//     // }

//     super.OnUpdate(timeDelta, stateContext, scriptInterface);
//     this.RemoveOpticalCamoEffect(stateContext, scriptInterface, EnumInt(gamePSMDetailedLocomotionStates.Fall), 1.00);
//   }

//   protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     this.RemoveInputHint(scriptInterface, n"Flight_Toggle", n"Paraglider");
//     super.OnExit(stateContext, scriptInterface);
//   }
// }