// @wrapMethod(LocomotionAirDecisions)
// protected const func ToDeathLand(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   let landingType: Int32;
//   if !this.IsTouchingGround(scriptInterface) {
//     return false;
//   };
//   landingType = this.GetLandingType(stateContext);
//   if landingType == EnumInt(LandingType.Death) {
//     return true;
//   };
//   return false;
// }

// @wrapMethod(LocomotionAirDecisions)
// protected const func ToDeathLand(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Bool {
//   let deathLandingHeight: StateResultFloat;
//   let playerPosition: Vector4;
//   if !this.IsTouchingGround(scriptInterface) {
//     return false;
//   };
//   deathLandingHeight = stateContext.GetPermanentFloatParameter(n"DeathLandingHeight");
//   playerPosition = DefaultTransition.GetPlayerPosition(scriptInterface);
//   if deathLandingHeight.valid {
//     if deathLandingHeight.value > playerPosition.Z {
//       return true;
//     };
//   } else {
//     return true;
//   };
//   return false;
// }

// func GetFallingSpeedBasedOnHeight(const scriptInterface: ref<StateGameScriptInterface>, height: Float) -> Float {
//   let acc: Float;
//   let speed: Float;
//   if height <= 0.00 {
//     return 0.00;
//   };
//   acc = AbsF(this.GetStaticFloatParameterDefault("upwardsGravity", this.GetStaticFloatParameterDefault("defaultGravity", -16.00)));
//   speed = 0.00;
//   if acc != 0.00 {
//     speed = acc * SqrtF((2.00 * height) / acc);
//   };
//   return speed * -1.00;
// }

@addMethod(DefaultTransition)
public final func CanParaglide() -> Bool {
  return TweakDBInterface.GetBool(TDBID.Create("player.canParaglide"), false);
}

@addField(LocomotionAirEvents)
public let m_isParagliding: Bool;

@wrapMethod(LocomotionAirEvents)
protected func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  if !this.CanParaglide() {
    wrappedMethod(timeDelta, stateContext, scriptInterface);
    return;
  }
  let deathLandingFallingSpeed: Float;
  let hardLandingFallingSpeed: Float;
  let horizontalSpeed: Float;
  let isInSuperheroFall: Bool;
  let isLeaping: Bool;
  let landingAnimFeature: ref<AnimFeature_Landing>;
  let landingType: LandingType;
  let maxAllowedDistanceToGround: Float;
  let playerVelocity: Vector4;
  let regularLandingFallingSpeed: Float;
  let safeLandingFallingSpeed: Float;
  let verticalSpeed: Float;
  let veryHardLandingFallingSpeed: Float;
  super.OnUpdate(timeDelta, stateContext, scriptInterface);

  if this.IsTouchingGround(scriptInterface) {
    this.m_resetFallingParametersOnExit = true;
    return;
  };
  this.m_resetFallingParametersOnExit = false;
  verticalSpeed = this.GetVerticalSpeed(scriptInterface);

  if FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive) {
    if !this.m_isParagliding && scriptInterface.IsActionJustPressed(n"DeployGlider") {
      this.m_isParagliding = true;
      
      this.RemoveInputHint(scriptInterface, n"DeployGlider", n"Paraglider");
      this.ShowInputHint(scriptInterface, n"PackGlider", n"Paraglider", LocKeyToString(n"Input-Hint-Pack-Glider"));
      // scriptInterface.GetAudioSystem().NotifyGameTone(n"EnterCrouch");
      // this.StartEffect(scriptInterface, n"skif_buff");
      // GameObjectEffectHelper.StartEffectEvent(scriptInterface.executionOwner, n"camera_mask");
      
      // scriptInterface.PushAnimationEvent(n"StaggerHit");

      scriptInterface.PushAnimationEvent(n"Jump");
      this.PlaySound(n"q115_thruster_start", scriptInterface);
      this.SetDetailedState(scriptInterface, gamePSMDetailedLocomotionStates.Jump); // HoverJump, DodgeAir, AirThrusters, Jump
      DefaultTransition.PlayRumbleLoop(scriptInterface, "light");
    }
    if this.m_isParagliding && scriptInterface.IsActionJustPressed(n"PackGlider") {
      this.m_isParagliding = false;
      
      this.RemoveInputHint(scriptInterface, n"PackGlider", n"Paraglider");
      this.ShowInputHint(scriptInterface, n"DeployGlider", n"Paraglider", LocKeyToString(n"Input-Hint-Deploy-Glider"));

      // GameObjectEffectHelper.StopEffectEvent(scriptInterface.executionOwner, n"camera_mask");
      // scriptInterface.GetAudioSystem().NotifyGameTone(n"LeaveCrouch");
      scriptInterface.PushAnimationEvent(n"AirHover");
      this.PlaySound(n"q115_thruster_stop", scriptInterface);
      this.SetDetailedState(scriptInterface, gamePSMDetailedLocomotionStates.Fall);
      DefaultTransition.StopRumbleLoop(scriptInterface, "light");
    }
  }

  FlightLog.Info("[LocomotionAirEvents] " + NameToString(this.GetStateName()) + ": " + ToString(verticalSpeed));

  // // slow fall if exited during flight
  if this.m_isParagliding {
    // let threshold = stateContext.GetFloatParameter(n"HardLandingFallingSpeed", true);
    let threshold = this.GetFallingSpeedBasedOnHeight(scriptInterface, 7.0);
    if verticalSpeed < threshold {
      let velocity = FromVariant<Vector4>(scriptInterface.GetStateVectorParameter(physicsStateValue.LinearVelocity));
      verticalSpeed = threshold;
      velocity.Z = verticalSpeed;

      // scriptInterface.SetStateVectorParameter(physicsStateValue.LinearVelocity, ToVariant(new Vector3(0, 0, this.GetFallingSpeedBasedOnHeight(scriptInterface, 10.0))));
      scriptInterface.SetStateVectorParameter(physicsStateValue.LinearVelocity, ToVariant(velocity));

      // let ev = new PSMImpulse();
      // ev.id = n"impulse";
      // ev.impulse =  new Vector4(0.0, 0.0, 2.0, 1.0);
      // scriptInterface.owner.QueueEvent(ev);
    }
  }



  if this.m_updateInputToggles && verticalSpeed < this.GetFallingSpeedBasedOnHeight(scriptInterface, this.GetStaticFloatParameterDefault("minFallHeightToConsiderInputToggles", 0.00)) {
    this.UpdateInputToggles(stateContext, scriptInterface);
  };
  if scriptInterface.IsActionJustPressed(n"Jump") {
    stateContext.SetConditionBoolParameter(n"CrouchToggled", false, true);
    return;
  };
  if StatusEffectSystem.ObjectHasStatusEffect(scriptInterface.executionOwner, t"BaseStatusEffect.BerserkPlayerBuff") && verticalSpeed < this.GetFallingSpeedBasedOnHeight(scriptInterface, this.GetStaticFloatParameterDefault("minFallHeightToEnterSuperheroFall", 0.00)) {
    stateContext.SetTemporaryBoolParameter(n"requestSuperheroLandActivation", true, true);
  };
  regularLandingFallingSpeed = stateContext.GetFloatParameter(n"RegularLandingFallingSpeed", true);
  safeLandingFallingSpeed = stateContext.GetFloatParameter(n"SafeLandingFallingSpeed", true);
  hardLandingFallingSpeed = stateContext.GetFloatParameter(n"HardLandingFallingSpeed", true);
  veryHardLandingFallingSpeed = stateContext.GetFloatParameter(n"VeryHardLandingFallingSpeed", true);
  deathLandingFallingSpeed = stateContext.GetFloatParameter(n"DeathLandingFallingSpeed", true);
  isLeaping = scriptInterface.localBlackboard.GetBool(GetAllBlackboardDefs().PlayerStateMachine.MeleeLeap) || StatusEffectSystem.ObjectHasStatusEffect(scriptInterface.executionOwner, t"BaseStatusEffect.PlayerMadDashLocomotionBuffer");
  isInSuperheroFall = stateContext.IsStateActive(n"Locomotion", n"superheroFall");
  maxAllowedDistanceToGround = this.GetStaticFloatParameterDefault("maxDistToGroundFromSuperheroFall", 20.00);
  if isInSuperheroFall && !this.m_maxSuperheroFallHeight {
    this.StartEffect(scriptInterface, n"falling");
    this.PlaySound(n"lcm_falling_wind_loop", scriptInterface);
    if DefaultTransition.GetDistanceToGround(scriptInterface) >= maxAllowedDistanceToGround {
      this.m_maxSuperheroFallHeight = true;
      return;
    };
    landingType = LandingType.Superhero;
  } else {
    if isLeaping {
      return;
    };
    // check to see if we exited while flight was active
    if verticalSpeed <= deathLandingFallingSpeed && !FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive) {
      landingType = LandingType.Death;
      this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Fall, EnumInt(gamePSMFallStates.DeathFall));
    } else {
      if verticalSpeed <= veryHardLandingFallingSpeed {
        landingType = LandingType.VeryHard;
        this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Fall, EnumInt(gamePSMFallStates.VeryFastFall));
      } else {
        if verticalSpeed <= hardLandingFallingSpeed {
          landingType = LandingType.Hard;
          if this.GetLandingType(stateContext) != EnumInt(LandingType.Hard) {
            this.StartEffect(scriptInterface, n"falling");
          };
          this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Fall, EnumInt(gamePSMFallStates.FastFall));
        } else {
          if verticalSpeed <= safeLandingFallingSpeed {
            landingType = LandingType.Regular;
            this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Fall, EnumInt(gamePSMFallStates.RegularFall));
            playerVelocity = DefaultTransition.GetLinearVelocity(scriptInterface);
            horizontalSpeed = Vector4.Length2D(playerVelocity);
            if horizontalSpeed <= this.GetStaticFloatParameterDefault("maxHorizontalSpeedToAerialTakedown", 0.00) {
              this.SetBlackboardIntVariable(scriptInterface, GetAllBlackboardDefs().PlayerStateMachine.Fall, EnumInt(gamePSMFallStates.SafeFall));
            };
          } else {
            if verticalSpeed <= regularLandingFallingSpeed {
              if this.GetLandingType(stateContext) != EnumInt(LandingType.Regular) {
                this.PlaySound(n"lcm_falling_wind_loop", scriptInterface);
              };
              landingType = LandingType.Regular;
            } else {
              landingType = LandingType.Off;
            };
          };
        };
      };
    };
  };

  if this.m_isParagliding {
    landingType = LandingType.Regular;
    // this.PlaySound(n"lcm_falling_wind_loop", scriptInterface);
  }


  stateContext.SetPermanentIntParameter(n"LandingType", EnumInt(landingType), true);
  stateContext.SetPermanentFloatParameter(n"ImpactSpeed", verticalSpeed, true);
  stateContext.SetPermanentFloatParameter(n"InAirDuration", this.GetInStateTime(), true);
  landingAnimFeature = new AnimFeature_Landing();
  landingAnimFeature.impactSpeed = verticalSpeed;
  landingAnimFeature.type = EnumInt(landingType);
  scriptInterface.SetAnimationParameterFeature(n"Landing", landingAnimFeature);
  this.SetAudioParameter(n"RTPC_Vertical_Velocity", verticalSpeed, scriptInterface);
  this.SetAudioParameter(n"RTPC_Landing_Type", Cast<Float>(EnumInt(landingType)), scriptInterface);
}

// eventually should be triggered by putting away parachute
// @wrapMethod(StandEvents)
// public func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//   wrappedMethod(stateContext, scriptInterface);
//   // reset exited-while-active once we're on the ground
//   FlightController.GetInstance().GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive, false, true);
// }

@wrapMethod(FallEvents)
protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  this.m_isParagliding = false;
  wrappedMethod(stateContext, scriptInterface);
  if this.CanParaglide() && FlightController.GetInstance().GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive) {
    this.ShowInputHint(scriptInterface, n"DeployGlider", n"Paraglider", LocKeyToString(n"Input-Hint-Deploy-Glider"));
  }
}

@addMethod(FallEvents)
protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  if this.CanParaglide() {
    if this.m_isParagliding {
      this.RemoveInputHint(scriptInterface, n"PackGlider", n"Paraglider");
    } else {
      this.RemoveInputHint(scriptInterface, n"DeployGlider", n"Paraglider");
    }

    // scriptInterface.PushAnimationEvent(n"AirHover");
    this.PlaySound(n"q115_thruster_stop", scriptInterface);
    // this.SetDetailedState(scriptInterface, gamePSMDetailedLocomotionStates.Fall);
    DefaultTransition.StopRumbleLoop(scriptInterface, "light");
  }
  // GameObjectEffectHelper.StopEffectEvent(scriptInterface.executionOwner, n"camera_mask");
  super.OnExit(stateContext, scriptInterface);
}