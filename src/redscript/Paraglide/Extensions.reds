@addField(DefaultTransition)
public let m_ltbfCanParaglide: Bool;

@addField(DefaultTransition)
public let m_ltbfCanParaglideRead: Bool;

// The flat comes from a TweakXL tweak, applied before the game loads, so it is read once per state object.
@addMethod(DefaultTransition)
public final func CanParaglide() -> Bool {
  if !this.m_ltbfCanParaglideRead {
    this.m_ltbfCanParaglide = TweakDBInterface.GetBool(TDBID.Create("player.canParaglide"), false);
    this.m_ltbfCanParaglideRead = true;
  }
  return this.m_ltbfCanParaglide;
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
      
      scriptInterface.PushAnimationEvent(n"Jump");
      this.PlaySound(n"q115_thruster_start", scriptInterface);
      this.SetDetailedState(scriptInterface, gamePSMDetailedLocomotionStates.Jump); // HoverJump, DodgeAir, AirThrusters, Jump
      DefaultTransition.PlayRumbleLoop(scriptInterface, "light");
    }
    if this.m_isParagliding && scriptInterface.IsActionJustPressed(n"PackGlider") {
      this.m_isParagliding = false;
      
      this.RemoveInputHint(scriptInterface, n"PackGlider", n"Paraglider");
      this.ShowInputHint(scriptInterface, n"DeployGlider", n"Paraglider", LocKeyToString(n"Input-Hint-Deploy-Glider"));

      scriptInterface.PushAnimationEvent(n"AirHover");
      this.PlaySound(n"q115_thruster_stop", scriptInterface);
      this.SetDetailedState(scriptInterface, gamePSMDetailedLocomotionStates.Fall);
      DefaultTransition.StopRumbleLoop(scriptInterface, "light");
    }
  }

  // // slow fall if exited during flight
  if this.m_isParagliding {
    let threshold = this.GetFallingSpeedBasedOnHeight(scriptInterface, 7.0);
    if verticalSpeed < threshold {
      let velocity = FromVariant<Vector4>(scriptInterface.GetStateVectorParameter(physicsStateValue.LinearVelocity));
      verticalSpeed = threshold;
      velocity.Z = verticalSpeed;

      scriptInterface.SetStateVectorParameter(physicsStateValue.LinearVelocity, ToVariant(velocity));

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

// ExitedWhileActive lasts from leaving a car in flight until V lands. Every air state enters through
// LocomotionAirEvents.OnEnter and every ground state (landings included) through
// LocomotionGroundEvents.OnEnter, where vanilla also resets its jump and air dodge counts. Leaving a
// car passes through a ground state before V falls, so only a ground state after an air state clears
// it. The landing type was already decided by the air state.
@wrapMethod(LocomotionAirEvents)
public func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(stateContext, scriptInterface);
  let ctlr = FlightController.GetInstance();
  if IsDefined(ctlr) && ctlr.GetBlackboard().GetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive) {
    stateContext.SetPermanentBoolParameter(n"ltbfAirborneAfterFlightExit", true, true);
  }
}

@wrapMethod(LocomotionGroundEvents)
public func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
  wrappedMethod(stateContext, scriptInterface);
  let airborne = stateContext.GetPermanentBoolParameter(n"ltbfAirborneAfterFlightExit");
  if airborne.valid && airborne.value {
    stateContext.RemovePermanentBoolParameter(n"ltbfAirborneAfterFlightExit");
    let ctlr = FlightController.GetInstance();
    if IsDefined(ctlr) {
      ctlr.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive, false, true);
    }
  }
}

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

    this.PlaySound(n"q115_thruster_stop", scriptInterface);
    DefaultTransition.StopRumbleLoop(scriptInterface, "light");
  }
  super.OnExit(stateContext, scriptInterface);
}