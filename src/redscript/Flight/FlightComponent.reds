public native class FlightComponentPS extends GameComponentPS {

}

public native class FlightComponent extends GameComponent {
  @runtimeProperty("offset", "0xA8")
  public native let sys: ref<FlightSystem>;

  @runtimeProperty("offset", "0xB8")
  public native let active: Bool;

  @runtimeProperty("offset", "0xB9")
  public native let hasUpdate: Bool;

  @runtimeProperty("offset", "0xC0")
  public native let force: Vector4;

  @runtimeProperty("offset", "0xD0")
  public native let torque: Vector4;

  // @runtimeProperty("offset", "0xE0")
  // public native let thrusters: array<ref<IFlightThruster>>;

  @runtimeProperty("offset", "0xE0")
  public native let configuration: ref<IFlightConfiguration>;

  @runtimeProperty("offset", "0xF8")
  public native let linearDamp: Float;

  @runtimeProperty("offset", "0xFC")
  public native let angularDamp: Float;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-General-Flight-Settings")
  @runtimeProperty("ModSettings.category.order", "1")
  @runtimeProperty("ModSettings.order", "2")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Flight-Quickhacks-Enabled")
  @runtimeProperty("ModSettings.description", "UI-Settings-Flight-Quickhacks-Enabled-Description")
  public let isQuickHackable: Bool = true;

  // A flying vehicle V is not in explodes when a hit changes one of its damage grid's cells by more than this
  // (OnGridDestruction; a cell's change is 0 to 1, so 1 is never).
  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Crashes")
  @runtimeProperty("ModSettings.category.order", "4")
  @runtimeProperty("ModSettings.order", "0")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Explosion-Threshold")
  @runtimeProperty("ModSettings.description", "UI-Settings-Explosion-Threshold-Description")
  @runtimeProperty("ModSettings.step", "0.05")
  @runtimeProperty("ModSettings.min", "0.05")
  @runtimeProperty("ModSettings.max", "1.0")
  public let explosionThreshold: Float = 0.5;

  public native func ChaseTarget(target: wref<GameObject>) -> Void;
  // world-space angular velocity contacts gave the vehicle (PhysX flight physics only),
  // decaying over ~0.5 s; see FlightMode.ApplyPhysics
  public native func GetContactSpin() -> Vector4;
  // angular impulse (world) for a change of angular velocity (world, rad/s), with the inertia the
  // flight torque is applied with; W = 1, or W = 0 when not known (not PhysX-owned flight)
  public native func AngularImpulseFor(worldRate: Vector4) -> Vector4;
  // row 0, 1 or 2 of the world inertia AngularImpulseFor multiplies by (X, Y, Z of the matrix); W = 1, or all 0 when
  // it is not known
  public native func GetWorldInertiaRow(row: Int32) -> Vector4;

  private let stats: ref<FlightStats>;
  private let forceTail: ref<FlightForce>;

  public let m_interaction: ref<InteractionComponent>;
  public let m_healthStatPoolListener: ref<VehicleHealthStatPoolListener>;
  public let m_vehicleBlackboard: wref<IBlackboard>;
  public let m_vehicleTPPCallbackID: ref<CallbackHandle>;

  public let isPlayerMounted: Bool;

  let hoverGroundPID: ref<PID>;
  let pitchPID: ref<PID>;
  let rollPID: ref<PID>;

  private let sqs: ref<SpatialQueriesSystem>;

  public let bl_tire: ref<IPlacedComponent>;
  public let br_tire: ref<IPlacedComponent>;
  public let fl_tire: ref<IPlacedComponent>;
  public let fr_tire: ref<IPlacedComponent>;

  public let collisionTimer: Float;

  public let distance: Float;
  public let hoverHeight: Float;

  private let modes: array<ref<FlightMode>>;
  public let mode: Int32;

  private let surge: Float;
  private let lift: Float;
  private let roll: Float;
  private let pitch: Float;
  private let yaw: Float;
  private let sway: Float;
  private let linearBrake: Float;
  private let angularBrake: Float;

  public let thrusterTensor: Vector4;

  public let alarmIsPlaying: Bool;

  protected final func GetVehicle() -> wref<VehicleObject> {
    return this.GetEntity() as VehicleObject;
  }

  protected cb func OnVehicleOnPartDetached(evt: ref<VehicleOnPartDetachedEvent>) -> Bool {
    FlightLog.Info("[FlightComponent] Part detached: " + NameToString(evt.partName));
    if !IsDefined(this.configuration) {
      return false;
    }
    let oneOfOurs = false;
    for thruster in this.configuration.thrusters {
      if IsDefined(thruster.meshComponent) {
        if Equals(thruster.meshComponent.name, evt.partName) {
          thruster.Detach();
          oneOfOurs = true;
        }
      } else {
        thruster.Detach();
        oneOfOurs = true;
      }
    }

    if oneOfOurs {
      this.thrusterTensor = this.configuration.GetThrusterTensor();
      // only rebuild the thruster colliders while we are actually flying; a
      // parked or traffic vehicle sheds its thrusters when it is destroyed and
      // arming the collider bookkeeping there strips the wreck's own shapes
      if this.active {
        this.configuration.RemoveColliders();
        this.configuration.AddColliders();
      }
    }
  }

  // interaction layer stuff to add eventually

/*
  private final func CreateObjectActionsCallbackController(instigator: wref<Entity>) -> Void {
    this.m_objectActionsCallbackCtrl = gameObjectActionsCallbackController.Create(this.GetEntity(), instigator, this.GetVehicle().GetGame());
    this.m_objectActionsCallbackCtrl.RegisterSkillCheckCallbacks();
  }

  private final func DestroyObjectActionsCallbackController() -> Void {
    this.m_objectActionsCallbackCtrl.UnregisterSkillCheckCallbacks();
    this.m_objectActionsCallbackCtrl = null;
  }

  protected cb func OnObjectActionRefreshEvent(evt: ref<gameObjectActionRefreshEvent>) -> Bool {
    if IsDefined(this.m_objectActionsCallbackCtrl) {
      this.m_objectActionsCallbackCtrl.UnlockNotifications();
      this.DetermineInteractionState();
    };
  }

  protected cb func OnInteractionActivated(evt: ref<InteractionActivationEvent>) -> Bool {
    let context: VehicleActionsContext;
    if this.GetVehicle().IsCrowdVehicle() && StatusEffectSystem.ObjectHasStatusEffectWithTag(evt.activator, n"BlockTrafficInteractions") {
      return false;
    };
    if Equals(evt.eventType, gameinteractionsEInteractionEventType.EIET_activate) {
      if evt.IsInputLayerEvent() {
        this.CreateObjectActionsCallbackController(evt.activator);
      };
    } else {
      if Equals(evt.eventType, gameinteractionsEInteractionEventType.EIET_deactivate) {
        if evt.IsInputLayerEvent() {
          this.DestroyObjectActionsCallbackController();
        };
      };
    };
    context.requestorID = this.GetVehicle().GetEntityID();
    context.processInitiatorObject = evt.activator;
    context.interactionLayerTag = evt.layerData.tag;
    context.eventType = evt.eventType;
    this.GetPS().DetermineActionsToPush(this.m_interaction, context, this.m_objectActionsCallbackCtrl, false);
    if this.GetVehicle() == (this.GetVehicle() as TankObject) {
      this.EvaluatePanzerInteractions();
    };
  }

  protected cb func OnInteractionUsed(evt: ref<InteractionChoiceEvent>) -> Bool {
    this.ExecuteAction(evt.choice, evt.activator);
    this.m_interaction.ResetChoices();
  }
*/

  private final func OnGameAttach() -> Void {
    LTBF_RegisterListener(this);
    this.m_interaction = this.FindComponentByName(n"interaction") as InteractionComponent;
    this.m_healthStatPoolListener = new VehicleHealthStatPoolListener();
    this.m_healthStatPoolListener.m_owner = this.GetVehicle();
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestRegisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    this.m_vehicleBlackboard = this.GetVehicle().GetBlackboard();

    this.hoverGroundPID = PID.Create(FlightSettings.GetVector3("hoverModePID"));
    this.pitchPID = PID.Create(FlightSettings.GetVector3("inputPitchPID"));
    this.rollPID =  PID.Create(FlightSettings.GetVector3("inputRollPID"));

    this.sqs = GameInstance.GetSpatialQueriesSystem(this.GetVehicle().GetGame());

    this.collisionTimer = FlightSettings.GetFloat("collisionRecoveryDelay");
    this.distance = 0.0;
    this.hoverHeight = FlightSettings.GetFloat("defaultHoverHeight");

    // the modes, in the order the controller cycles them: the classes TweakDB's FlightModes.classes names
    // (src/tweaks/flight_modes.yaml; a mod appends its own or swaps one for its subclass), each made by name in C++
    // and kept when its Mod Settings switch is on
    let classes = FlightMode.ModeClasses();
    if Equals(FlightDeveloperSettings.Get("nativeModeProbe"), "true") {
      ArrayPush(classes, n"FlightModeNativeProbe"); // the native mode spike, last, for a developer only
    }
    for className in classes {
      let mode = FlightMode.Instantiate(className);
      if IsDefined(mode) {
        mode.Initialize(this);
        if mode.IsEnabled() {
          ArrayPush(this.modes, mode);
        } else {
          mode.Deinitialize();
        }
      } else {
        FlightLog.Warn("[FlightComponent] FlightModes.classes names " + NameToString(className) + ", which is not a FlightMode class");
      }
    }

  }

  private final func OnGameDetach() -> Void {
    LTBF_UnregisterListener(this);
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestUnregisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    this.UnregisterVehicleTPPBBListener();

    this.isDestroyed = true;
    this.hasExploded = true;
    if this.active {
      this.Deactivate(true);
    }
    this.hasUpdate = false;
    for mode in this.modes {
      mode.Deinitialize();
    }
  }

  protected final func SetupVehicleTPPBBListener() -> Void {
    let activeVehicleUIBlackboard: wref<IBlackboard>;
    let bbSys: ref<BlackboardSystem>;
    if !IsDefined(this.m_vehicleTPPCallbackID) {
      bbSys = GameInstance.GetBlackboardSystem(this.GetVehicle().GetGame());
      activeVehicleUIBlackboard = bbSys.Get(GetAllBlackboardDefs().UI_ActiveVehicleData);
      this.m_vehicleTPPCallbackID = activeVehicleUIBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this, n"OnVehicleCameraChange");
    };
  }

  protected final func UnregisterVehicleTPPBBListener() -> Void {
    let activeVehicleUIBlackboard: wref<IBlackboard>;
    let bbSys: ref<BlackboardSystem>;
    if IsDefined(this.m_vehicleTPPCallbackID) {
      bbSys = GameInstance.GetBlackboardSystem(this.GetVehicle().GetGame());
      activeVehicleUIBlackboard = bbSys.Get(GetAllBlackboardDefs().UI_ActiveVehicleData);
      activeVehicleUIBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this.m_vehicleTPPCallbackID);
    };
  }

  // the engine sound's pitch for this car, by its mass (IFlightThruster.Start)
  public func GetPitch() -> Float {
    return ClampF(700.0 / this.GetVehicle().GetTotalMass() + 0.5, 0.5, 1.5);
  }

  public func GetFlightMode() -> ref<FlightMode> {
    return this.modes[this.mode];
  }

  // The Autopilot mode in this component's list (FlightSummonSystem flies a summoned vehicle
  // without the player), -1 when it is not in the list.
  public func GetAutopilotModeIndex() -> Int32 {
    let i = 0;
    for mode in this.modes {
      if IsDefined(mode as FlightModeAutopilot) {
        return i;
      }
      i += 1;
    }
    return -1;
  }
  // Skips the Autopilot mode, which is reached through the autodrive hold (FlightController).
  public func GetNextFlightMode(direction: Int32) -> ref<FlightMode> {
    let count = ArraySize(this.modes);
    let mode = this.mode;
    let steps = 0;
    while steps < count {
      mode = (mode + direction + count) % count;
      steps += 1;
      if !IsDefined(this.modes[mode] as FlightModeAutopilot) {
        break;
      }
    }
    return this.modes[mode];
  }

  // callbacks

  protected cb func OnMountingEvent(evt: ref<MountingEvent>) -> Bool {

    this.thrusterTensor = this.configuration.GetThrusterTensor();
    FlightLog.Info("[FlightComponent] thrusterTensor: " + this.thrusterTensor.X + ", " + this.thrusterTensor.Y + ", " + this.thrusterTensor.Z + ", " + this.thrusterTensor.W);
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if mountChild.IsPlayer() {

      let vehicleFlight = new PSMAddOnDemandStateMachine();
      vehicleFlight.stateMachineName = n"VehicleFlight";
      vehicleFlight.owner = this.GetVehicle();
      // vehicleFlight.instanceData = instanceData;
      // needs to be player?
      mountChild.QueueEvent(vehicleFlight);

      FlightLog.Info("[FlightComponent] OnMountingEvent: " + this.GetVehicle().GetDisplayName() + ": " + NameToString(this.GetVehicle().GetCurrentAppearanceName()));
      FlightLog.Info("[FlightComponent] TDBID: " + TDBID.ToStringDEBUG(this.GetVehicle().GetRecordID()));
      this.sys.SetPlayerComponent(this);
      this.sys.NotifyHudVehicleChanged(this.GetVehicle());
      this.sys.ctlr.ResetInputs();
      this.isPlayerMounted = true;
      this.SetupVehicleTPPBBListener();
      // timing marker for the mount hitch
      FlightLog.Info("[FlightComponent] OnMountingEvent done");
    } else {
    }
  }

  protected cb func OnVehicleFinishedMountingEvent(evt: ref<VehicleFinishedMountingEvent>) -> Bool {
    if evt.character.IsPlayer() && evt.isMounting {
      FlightLog.Info("[FlightComponent] OnVehicleFinishedMountingEvent: " + this.GetVehicle().GetDisplayName());
      this.sys.ctlr.GetBlackboard().SetBool(GetAllBlackboardDefs().VehicleFlight.ExitedWhileActive, false, true);
      this.sys.ctlr.Enable();
      if this.active {
        this.sys.ctlr.Activate(true);
      }
    }
    let normal: Vector4;
    this.SetupTires();
    let wheeled = this.GetVehicle() as WheeledObject;
    // FindGround runs four synchronous raycasts; only pay for them when auto activation
    // (off by default) can actually use the result. IsFlightAvailable() keeps auto
    // activation on the same Flight Availability gate as the toggle and the input hint.
    if FlightSettings.GetInstance().autoActivationEnabled && evt.character.IsPlayer() && evt.isMounting && IsDefined(wheeled) && this.GetVehicle().IsFlightAvailable() {
      if !this.FindGround(normal) || this.distance > FlightSettings.GetInstance().autoActivationHeight {
        this.Activate(true);
      }
    }
    if evt.character.IsPlayer() && evt.isMounting {
      FlightLog.Info("[FlightComponent] OnVehicleFinishedMountingEvent done");
    }
  }

  protected cb func OnUnmountingEvent(evt: ref<UnmountingEvent>) -> Bool {
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if IsDefined(mountChild) && mountChild.IsPlayer() {
      FlightLog.Info("[FlightComponent] OnUnmountingEvent: " + this.GetVehicle().GetDisplayName());

      let vehicleFlight = new PSMRemoveOnDemandStateMachine();
      vehicleFlight.stateMachineIdentifier.definitionName = n"VehicleFlight";
      mountChild.QueueEvent(vehicleFlight);

      this.UnregisterVehicleTPPBBListener();
      // a direct seat swap mounts the next vehicle before this unmount finishes, so only clear our own entry
      if this.sys.playerComponent == this {
        this.sys.playerComponent = null;
      }
      this.isPlayerMounted = false;
      if this.active {
        this.sys.ctlr.Deactivate(true);
      }
      this.sys.ctlr.ResetInputs();
      this.sys.ctlr.Disable();
      // After the deactivation above, so a flight exit still plays its outro first.
      this.sys.NotifyHudVehicleChanged(null);
      FlightLog.Info("[FlightComponent] OnUnmountingEvent done");
    }
  }

  protected cb func OnVehicleFlightModeChangeEvent(evt: ref<VehicleFlightModeChangeEvent>) -> Bool {
    this.modes[this.mode].Deactivate();
    this.mode = evt.mode;
    this.modes[this.mode].Activate();
  }

  protected cb func OnDeath(evt: ref<gameDeathEvent>) -> Bool {
    let vehicle: ref<VehicleObject> = this.GetVehicle();
    let gameInstance: GameInstance = vehicle.GetGame();
    let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
    if VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), vehicle) {
      FlightLog.Info("[FlightComponent] OnDeath: " + this.GetVehicle().GetDisplayName());
      if this.active {
        this.isDestroyed = true;
        this.hasExploded = true;
        this.Deactivate(true);
      }
      this.hasUpdate = false;
    }
  }

  protected cb func OnVehicleWaterEvent(evt: ref<VehicleWaterEvent>) -> Bool {
    // the player's car only: the low-pass is the player's thrusters' (any car's event used to set it)
    if !this.isPlayerMounted {
      return false;
    }
    if evt.isInWater  {
      FlightAudio.Get().SetWater(1.0);
    } else {
      FlightAudio.Get().SetWater(0.0);
    }
  }

  protected cb func OnVehicleFlightActivationEvent(evt: ref<VehicleFlightActivationEvent>) -> Bool {
    this.Activate();
  }

  public func Activate(opt silent: Bool) -> Void {
    FlightLog.Info("[FlightComponent] Activating Flight: " + this.GetVehicle().GetDisplayName());
    FlightLog.Info("[FlightComponent] AppearanceName: " + NameToString(this.GetVehicle().GetCurrentAppearanceName()));
    if !this.active {
      this.stats = FlightStats.Create(this.GetVehicle());
      // OnMountingEvent sets it too, but a car flown with nobody in it (the pad summon) never gets
      // one: a zero tensor scales the pitch and yaw torque, damping included, to nothing
      this.thrusterTensor = this.configuration.GetThrusterTensor();

      this.SetupTires();
      for thruster in this.configuration.thrusters {
        thruster.Start();
      }
      // these stop engine noises if they were already playing?
      this.GetVehicle().TurnEngineOn(false);
      this.GetVehicle().GetVehicleComponent().GetVehicleController().ToggleLights(true);

      this.hoverGroundPID.Reset();
      this.pitchPID.Reset();
      this.rollPID.Reset();

      // the player's mode comes from the controller: pick it before activating, or the mode the
      // component last flew in (the Autopilot, after a landing) is the one activated
      if this.isPlayerMounted {
        this.mode = this.sys.ctlr.mode;
      }
      this.modes[this.mode].Activate();

      if this.isPlayerMounted {
        this.sys.ctlr.Activate(silent);
        FlightAudio.Get().PlayOneShot("vehicle3_on", this.GetVehicle());
      }
      this.active = true;
      this.hasUpdate = true;

      this.configuration.OnActivationCore();
    }
  }

  protected cb func OnInteractionUsed(evt: ref<InteractionChoiceEvent>) -> Bool {
    FlightLog.Info("[FlightComponent] OnInteractionUsed: " + ToString(evt.actionType));
  }

  public let trick: ref<FlightTrick>;

  let smoothForce: Vector4;
  let smoothTorque: Vector4;
  let isDestroyed: Bool;
  let hasExploded: Bool;

  // Called by the plugin (Flight/Component.cpp) when the game lets go of a teleported car: the
  // controllers and the modes start over at the new place.
  protected func OnTeleported() -> Void {
    let p = this.GetVehicle().GetWorldPosition();
    FlightLog.Info("[FlightComponent] teleported to " + FloatToStringPrec(p.X, 1) + ", " + FloatToStringPrec(p.Y, 1) + ", " + FloatToStringPrec(p.Z, 1));
    this.hoverGroundPID.Reset();
    this.pitchPID.Reset();
    this.rollPID.Reset();
    if this.mode >= 0 && this.mode < ArraySize(this.modes) {
      this.modes[this.mode].Teleported();
    }
  }

  // The stats' dynamic half is updated in C++ (FlightStats.UpdateNative) before OnUpdate once nativeUpdate is set; a
  // new FlightStats (Activate) starts with it clear, so its first frame is updated here.
  private final func UpdateStats() -> Void {
    if !this.stats.nativeUpdate {
      this.stats.UpdateNow();
    }
    this.stats.nativeUpdate = true;
  }

  protected func OnUpdate(timeDelta: Float) -> Void {
    if this.GetVehicle().IsDestroyed() {
      if !this.isDestroyed {
        FlightAudio.Get().StartLoop("vehicleDestroyed" + this.GetUniqueID(), "vehicle3_destroyed", this.GetVehicle());
        this.alarmIsPlaying = true;
        this.isDestroyed = true;
      }
      if this.GetVehicle().GetVehicleComponent().GetPS().GetHasExploded() {
        this.hasExploded = true;
      }
      if this.hasExploded {
        this.Deactivate(true);
        return;
      }
    }

    let vehicle: ref<VehicleObject> = this.GetVehicle();
    let gameInstance: GameInstance = vehicle.GetGame();

    let photoModeSys: ref<PhotoModeSystem> = GameInstance.GetPhotoModeSystem(gameInstance);
    if photoModeSys.IsPhotoModeActive() {
      return;
    }

    if timeDelta <= 0.0 {
      this.UpdateStats();

      for thruster in this.configuration.thrusters {
        thruster.Update(this.smoothForce, this.smoothTorque);
      }
      return;
    }
    if this.active {
      if this.isPlayerMounted {
        this.GetVehicle().ResetQuestEnforceTransform();
        this.sys.ctlr.OnUpdate(timeDelta);
        // the controller's smoothed and normalised inputs (FlightController.StepInputs, in C++)
        let fc = this.sys.ctlr;
        this.yaw = fc.outYaw;
        this.roll = fc.outRoll;
        this.pitch = fc.outPitch;
        this.lift = fc.outLift;
        this.linearBrake = fc.outLinearBrake;
        this.angularBrake = fc.outAngularBrake;
        this.surge = fc.outSurge;
        this.sway = fc.outSway;
      } else {
        let v = this.GetVehicle();
        this.surge = v.acceleration * 0.5 - v.deceleration * 0.1;
        this.yaw = -v.turnX;
        this.linearBrake = v.handbrake * 0.5;
        this.angularBrake = v.handbrake * 0.5;
      }
    } else {
      this.yaw = 0.0;
      this.roll = 0.0;
      this.pitch = 0.0;
      this.lift = 0.0;
      this.linearBrake = 0.0;
      this.angularBrake = 0.0;
      this.surge = 0.0;
      this.sway = 0.0;
    }

    this.UpdateStats();

    let force = new Vector4(0.0, 0.0, 0.0, 0.0);
    let torque = new Vector4(0.0, 0.0, 0.0, 0.0);

    let shouldModeUpdate = this.active;
    if IsDefined(this.trick) {
      if this.trick.Update(timeDelta) {
        this.trick = null;
      } else {
        force += this.trick.force;
        torque += this.trick.torque;
        shouldModeUpdate = !this.trick.suspendMode;
      }
    }

    if this.mode < ArraySize(this.modes) {
      if shouldModeUpdate {
        this.modes[this.mode].Update(timeDelta);
        force += this.modes[this.mode].force * this.configuration.percentageAttached;
        torque += this.modes[this.mode].torque;
      }
    }

    // what the thrusters get; the plugin reads smoothForce for the sound (Audio/Inputs/Gather)
    this.smoothForce = force;
    this.smoothTorque = torque;
    for thruster in this.configuration.thrusters {
      thruster.Update(force, torque);
    }

    if this.isPlayerMounted {
      this.sys.ctlr.GetBlackboard().SetFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, this.stats.d_pitchDeg);
      this.sys.ctlr.GetBlackboard().SetFloat(GetAllBlackboardDefs().VehicleFlight.Roll, this.stats.d_rollDeg, false);
    }

    this.NativeForceTail(force, torque, timeDelta);
  }

  // The force tail in C++ (FlightForce, Flight/Force.hpp): the damping, drag and aero redirect, then the scaling to
  // world force and torque. The params come from FlightSettings once a frame; the mode's force, torque and linearDamp
  // and the collision timer are written back.
  private final func NativeForceTail(force: Vector4, torque: Vector4, timeDelta: Float) -> Void {
    if !IsDefined(this.forceTail) {
      this.forceTail = new FlightForce();
    }
    let t = this.forceTail;
    let fs = FlightSettings.GetInstance();
    t.physxFlightPhysics = fs.physxFlightPhysics;
    t.generalDampFactorAngular = fs.generalDampFactorAngular;
    t.brakeFactorAngular = fs.AngularBrake();
    t.generalYawDampFactor = fs.generalYawDampFactor;
    t.dampStabilityK = FlightSettings.GetFloatN(n"dampStabilityK");
    t.dampMaxPerFrame = FlightSettings.GetFloatN(n"dampMaxPerFrame");
    t.generalDampFactorAngularMax = fs.generalDampFactorAngularMax;
    t.generalDampFactorLinear = fs.generalDampFactorLinear;
    t.brakeFactorLinear = fs.LinearBrake();
    t.generalSwayWithYaw = fs.generalSwayWithYaw;
    t.generalYawDirectionalityFactor = fs.generalYawDirectionalityFactor;
    t.generalPitchDirectionalityFactor = fs.generalPitchDirectionalityFactor;
    t.generalYawAeroFactor = fs.generalYawAeroFactor;
    t.generalPitchAeroFactor = fs.generalPitchAeroFactor;
    t.collisionRecoveryDelay = FlightSettings.GetFloatN(n"collisionRecoveryDelay");
    t.collisionRecoveryDuration = FlightSettings.GetFloatN(n"collisionRecoveryDuration");
    t.handling = EnumInt(fs.handling);

    let applyPhysics = this.mode < ArraySize(this.modes);
    t.dt = timeDelta;
    t.force = force;
    t.torque = torque;
    t.applyPhysics = applyPhysics;
    t.pitch = this.pitch;
    t.roll = this.roll;
    t.yaw = this.yaw;
    t.linearBrake = this.linearBrake;
    t.angularBrake = this.angularBrake;
    t.thrusterTensor = this.thrusterTensor;
    t.contactSpin = this.GetContactSpin();
    t.collisionTimer = this.collisionTimer;
    t.Step(this.stats);

    if applyPhysics {
      let m = this.modes[this.mode];
      m.force = t.physicsForce;
      m.torque = t.physicsTorque;
      m.linearDamp = t.linearDamp;
    }
    this.collisionTimer = t.collisionTimer;
    this.force += t.worldForce;
    this.torque += t.worldTorque;
  }

  protected cb func OnVehicleFlightDeactivationEvent(evt: ref<VehicleFlightDeactivationEvent>) -> Bool {
    FlightLog.Info("[FlightComponent] OnVehicleFlightDeactivationEvent: " + this.GetVehicle().GetDisplayName());
    this.Deactivate(evt.silent);
  }

  protected cb func OnFlightFxCleanedUp(evt: ref<FlightFxCleanedUpEvent>) -> Bool {
    if !this.active {
      this.hasUpdate = false;
    }
  }

  private final func StopDestroyedAlarm() -> Void {
    if this.isDestroyed && this.hasExploded && this.alarmIsPlaying {
      FlightAudio.Get().StopLoop("vehicleDestroyed" + this.GetUniqueID());
      this.alarmIsPlaying = false;
    }
  }

  public func Deactivate(silent: Bool) -> Void {
    // the destroyed-vehicle alarm can be playing on a component that is not
    // active, so it has to be stopped before the early return below
    this.StopDestroyedAlarm();

    if !this.active {
      return;
    }

    this.active = false;
    for thruster in this.configuration.thrusters {
      thruster.Stop();
    }

    if !silent {
      this.GetVehicle().TurnEngineOn(true);
    }

    if !FlightSettings.GetInstance().generalApplyFlightPhysicsWhenDeactivated {
      let evt = new FlightFxCleanedUpEvent();
      GameInstance.GetDelaySystem(this.GetVehicle().GetGame()).DelayEvent(this.GetVehicle(), evt, evt.delay);
    }

    if this.isPlayerMounted {
      this.sys.ctlr.Deactivate(silent);
      if !silent {
        FlightAudio.Get().PlayOneShot("vehicle3_off", this.GetVehicle());
      }
    }

    this.configuration.OnDeactivationCore();
  }

  protected cb func OnGridDestruction(evt: ref<VehicleGridDestructionEvent>) -> Bool {
    let biggestImpact: Float;
    let desiredChange: Float;
    let gridState: Float;
    let i: Int32 = 0;
    let gridID = 0;
    while i < 16 {
      gridState = evt.state[i];
      desiredChange = evt.desiredChange[i];
      if desiredChange > biggestImpact {
        biggestImpact = desiredChange;
        gridID = i;
      };
      i += 1;
    };
    if biggestImpact > 0.00 {
      this.ProcessImpact(biggestImpact);
      if this.isPlayerMounted {
        this.sys.ctlr.ProcessImpact(biggestImpact);
      } else {
        if !this.active && this.GetVehicle().bouncy {
          this.FireVerticalImpulse(gridID);
        }
        if biggestImpact > this.explosionThreshold {
          GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"explosion");
        }
      }
    }
  }

  public func HandleExplosion() -> Void {
    this.isDestroyed = true;
    this.hasExploded = true;
    this.hasUpdate = false;
    this.StopDestroyedAlarm();
    if this.active {
      this.Deactivate(true);
    }
  }

  // gridID
  // 0 rear left
  // 1 rear right
  // 2 -
  // 3 -
  // 4 door left
  // 5 door right
  // 6 front left
  // 7 front right

  public func FireVerticalImpulse(gridID: Int32, opt impulse: Float) {
    let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
    impulseEvent.radius = 1.0;
    let offset = new Vector4(0.0, 0.0, 0.0, 0.0);
    if gridID == 0 {
      offset = new Vector4(-0.5, -0.5, 0.0, 0.0);
    }
    if gridID == 1 {
      offset = new Vector4(0.5, -0.5, 0.0, 0.0);
    }
    if gridID == 4 {
      offset = new Vector4(-0.5, 0.0, 0.0, 0.0);
    }
    if gridID == 5 {
      offset = new Vector4(0.5, 0.0, 0.0, 0.0);
    }
    if gridID == 6 {
      offset = new Vector4(-0.5, 0.5, 0.0, 0.0);
    }
    if gridID == 7 {
      offset = new Vector4(0.5, 0.5, 0.0, 0.0);
    }
    if impulse == 0.0 {
      impulse = 1.0;
    }
    impulseEvent.worldPosition = Vector4.Vector4To3(this.GetVehicle().GetLocalToWorld() * offset);
    impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 20.0 * impulse * this.GetVehicle().GetTotalMass());
    this.GetVehicle().QueueEvent(impulseEvent);
  }

  protected final func OnVehicleCameraChange(state: Bool) -> Void {
    this.sys.ctlr.isTPP = state;
  }

  public func ProcessImpact(impact: Float) {
    this.collisionTimer = (FlightSettings.GetFloat("collisionRecoveryDelay") + FlightSettings.GetFloat("collisionRecoveryDuration")) * (1.0 - (impact * this.GetFlightMode().collisionPenalty));
  }

  private func GetUniqueID() -> String {
    return ToString(EntityID.GetHash(this.GetVehicle().GetEntityID()));
  }

  public func SetupTires() -> Void {
    if this.GetVehicle() == (this.GetVehicle() as CarObject) {
      this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFL") as IPlacedComponent;
      this.fr_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFR") as IPlacedComponent;
      this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBL") as IPlacedComponent;
      this.br_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBR") as IPlacedComponent;
    } else {
      this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFront") as IPlacedComponent;
      this.fr_tire = this.fl_tire;
      this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBack") as IPlacedComponent;
      this.br_tire = this.bl_tire;
    }
  }

  public func FindGround(out normal: Vector4) -> Bool {
    let fl_tire: Vector4 = Matrix.GetTranslation(this.fl_tire.GetLocalToWorld());
    let fr_tire: Vector4 = Matrix.GetTranslation(this.fr_tire.GetLocalToWorld());
    let bl_tire: Vector4 = Matrix.GetTranslation(this.bl_tire.GetLocalToWorld());
    let br_tire: Vector4 = Matrix.GetTranslation(this.br_tire.GetLocalToWorld());

    let findGround1: TraceResult;
    let findGround2: TraceResult;
    let findGround3: TraceResult;
    let findGround4: TraceResult;

    // all in engine\physics\collision_presets.json
    // VehicleBlocker? RagdollVehicle?
    let lookDown = new Vector4(0.0, 0.0, -FlightSettings.GetFloatN(n"hoverModeMaxHoverHeight") - 10.0, 0.0);

    let queryFilter: QueryFilter;
    QueryFilter.AddGroup(queryFilter, n"VehicleBlocker");
    QueryFilter.AddGroup(queryFilter, n"Water");

    this.sqs.SyncRaycastByQueryFilter(fl_tire, fl_tire + lookDown, queryFilter, findGround1, false, false);
    this.sqs.SyncRaycastByQueryFilter(fr_tire, fr_tire + lookDown, queryFilter, findGround2, false, false);
    this.sqs.SyncRaycastByQueryFilter(bl_tire, bl_tire + lookDown, queryFilter, findGround3, false, false);
    this.sqs.SyncRaycastByQueryFilter(br_tire, br_tire + lookDown, queryFilter, findGround4, false, false);

    let groundPoint1: Vector4;
    let groundPoint2: Vector4;
    let groundPoint3: Vector4;
    let groundPoint4: Vector4;

    if TraceResult.IsValid(findGround1) {
      groundPoint1 = Vector4.Vector3To4(findGround1.position);// - this.stats.d_velocity * timeDelta;
    }
    if TraceResult.IsValid(findGround2) {
      groundPoint2 = Vector4.Vector3To4(findGround2.position);// - this.stats.d_velocity * timeDelta;
    }
    if TraceResult.IsValid(findGround3) {
      groundPoint3 = Vector4.Vector3To4(findGround3.position);// - this.stats.d_velocity * timeDelta;
    }
    if TraceResult.IsValid(findGround4) {
      groundPoint4 = Vector4.Vector3To4(findGround4.position);// - this.stats.d_velocity * timeDelta;
    }

    if TraceResult.IsValid(findGround1) && TraceResult.IsValid(findGround2) && TraceResult.IsValid(findGround3) && TraceResult.IsValid(findGround4) {
      // let distance = MinF(
      //   MinF(Vector4.Distance(fl_tire, Cast(findGround1.position)),
      //   Vector4.Distance(fr_tire, Cast(findGround2.position))),
      //   MinF(Vector4.Distance(bl_tire, Cast(findGround3.position)),
      //   Vector4.Distance(br_tire, Cast(findGround4.position))));
      let distance = (Vector4.Distance(fl_tire, Vector4.Vector3To4(findGround1.position)) +
        Vector4.Distance(fr_tire, Vector4.Vector3To4(findGround2.position)) +
        Vector4.Distance(bl_tire, Vector4.Vector3To4(findGround3.position)) +
        Vector4.Distance(br_tire, Vector4.Vector3To4(findGround4.position))) / 4.0;
      this.distance = distance;

      // FromVariant(scriptInterface.GetStateVectorParameter(physicsStateValue.Radius)) maybe?
      let n = (Vector4.Normalize(Cast(findGround1.normal)) + Vector4.Normalize(Cast(findGround2.normal)) + Vector4.Normalize(Cast(findGround3.normal)) + Vector4.Normalize(Cast(findGround4.normal))) / 4.0;
      normal = Vector4.Normalize(n);

      return true;
    } else {
      return false;
    }
  }

  public func OnFireWeapon(placeholderQuat: Quaternion, weaponItem:TweakDBID, attachmentSlot: TweakDBID) -> Void {
    let wt: WorldTransform;
    let vehicleSlots = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
    vehicleSlots.GetSlotTransform(StringToName(TweakDBInterface.GetAttachmentSlotRecord(attachmentSlot).EntitySlotName()), wt);
    let quat = WorldTransform.GetOrientation(wt);
    WorldTransform.SetOrientation(wt, quat * placeholderQuat);

    let effect = ResRefToFxResource(r"base\\fx\\vehicles\\av\\av_panzer\\weapons\\v_panzer_muzzle_flash.effect");
    let fxSystem = GameInstance.GetFxSystem(this.GetVehicle().GetGame());
    if IsDefined(fxSystem) {
      fxSystem.SpawnEffect(effect, wt);
    }

    let broadcaster = this.sys.player.GetStimBroadcasterComponent();
    if IsDefined(broadcaster) {
      broadcaster.TriggerSingleBroadcast(this.sys.player, gamedataStimType.Gunshot, 25.0);
      let data: stimInvestigateData;
      data.illegalAction = true;
      data.attackInstigator = this.sys.player;
      broadcaster.TriggerSingleBroadcast(this.sys.player, gamedataStimType.Gunshot, 100.0, data, true);
    };

  }

/*  private final func RegisterToHUDManager(shouldRegister: Bool) -> Void {
    let hudManager: ref<HUDManager>;
    let registration: ref<HUDManagerRegistrationRequest>;
    if this.GetVehicle().IsCrowdVehicle() && !this.GetVehicle().ShouldForceRegisterInHUDManager() {
      return;
    };
    hudManager = GameInstance.GetScriptableSystemsContainer(this.GetVehicle().GetGame()).Get(n"HUDManager") as HUDManager;
    if IsDefined(hudManager) {
      registration = new HUDManagerRegistrationRequest();
      registration.SetProperties(this.GetVehicle(), shouldRegister);
      hudManager.QueueRequest(registration);
    };
  }
*/
}
