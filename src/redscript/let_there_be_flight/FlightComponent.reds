public class FlightComponent extends ScriptableDeviceComponent {
  private let sys: ref<FlightSystem>;
  public let fx: ref<FlightFx>;
  private let helper: ref<vehicleFlightHelper>;
  private let stats: ref<FlightStats>;

  public let m_interaction: ref<InteractionComponent>;
  public let m_healthStatPoolListener: ref<VehicleHealthStatPoolListener>;
  public let m_vehicleBlackboard: wref<IBlackboard>;
  public let m_vehicleTPPCallbackID: ref<CallbackHandle>;

  public let active: Bool;
  public let hasUpdate: Bool;
  public let isPlayerMounted: Bool;

  let hoverGroundPID: ref<PID>;
  let hoverPID: ref<PID>;
  let pitchGroundPID: ref<DualPID>;
  let pitchPID: ref<PID>;
  let rollGroundPID: ref<DualPID>;
  let rollPID: ref<PID>;
  let yawPID: ref<PID>;
  let pitchAeroPID: ref<PID>;

  private let sqs: ref<SpatialQueriesSystem>;

  public let bl_tire: ref<IPlacedComponent>;
  public let br_tire: ref<IPlacedComponent>;
  public let fl_tire: ref<IPlacedComponent>;
  public let fr_tire: ref<IPlacedComponent>;
  public let hood: ref<IPlacedComponent>;
  public let trunk: ref<IPlacedComponent>;
  
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

  public let force: Vector4;
  public let torque: Vector4;

  // public let ui: wref<worlduiWidgetComponent>;
  // public let ui_info: wref<worlduiWidgetComponent>;

  private let uiBlackboard: wref<IBlackboard>;
  private let menuCallback: ref<CallbackHandle>;
  public let isInMenu: Bool;

  private let uiGameDataBlackboard: wref<IBlackboard>;
  private let popupCallback: ref<CallbackHandle>;
  public let isPopupShown: Bool;
  public let alarmIsPlaying: Bool;

  protected final const func GetVehicle() -> wref<VehicleObject> {
    return this.GetEntity() as VehicleObject;
  }

  private final func OnGameAttach() -> Void {
    //FlightLog.Info("[FlightComponent] OnGameAttach: " + this.GetVehicle().GetDisplayName());
    this.m_interaction = this.FindComponentByName(n"interaction") as InteractionComponent;
    this.m_healthStatPoolListener = new VehicleHealthStatPoolListener();
    this.m_healthStatPoolListener.m_owner = this.GetVehicle();
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestRegisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    this.m_vehicleBlackboard = this.GetVehicle().GetBlackboard();
    // QuickhackModule.RequestRefreshQuickhackMenu(this.GetVehicle().GetGame(), this.GetVehicle().GetEntityID());

    // this.hoverGroundPID = PID.Create(1.0, 0.01, 0.1);
    this.hoverGroundPID = PID.Create(1.0, 0.005, 0.5);
    this.hoverPID = PID.Create(1.0, 0.01, 0.1);
    this.pitchGroundPID = DualPID.Create(0.8, 0.2, 0.05,  0.8, 0.2, 0.05);
    // this.pitchPID = DualPID.Create(1.0, 0.5, 0.5,  1.0, 0.5, 0.5);
    this.pitchPID = PID.Create(1.0, 0.5, 0.5);
    this.rollGroundPID =  DualPID.Create(0.5, 0.2, 0.05,  2.5, 1.5, 0.5);
    // this.rollPID =  DualPID.Create(1.0, 0.5, 0.5,  1.0, 0.5, 0.5);
    this.rollPID =  PID.Create(1.0, 0.5, 0.5);
    this.yawPID = PID.Create(1.0, 0.01, 1.0);
    this.pitchAeroPID = PID.Create(1.0, 0.01, 1.0);

    this.sys = FlightSystem.GetInstance();
    this.sqs = GameInstance.GetSpatialQueriesSystem(this.GetVehicle().GetGame());
    this.fx = FlightFx.Create(this);
    
    // this.helper = this.GetVehicle().AddFlightHelper();
    // this.stats = FlightStats.Create(this.GetVehicle());

    this.collisionTimer = FlightSettings.GetFloat(n"collisionRecoveryDelay");
    this.distance = 0.0;
    this.hoverHeight = FlightSettings.GetFloat(n"defaultHoverHeight");
    
    ArrayPush(this.modes, FlightModeHoverFly.Create(this));
    ArrayPush(this.modes, FlightModeHover.Create(this));
    ArrayPush(this.modes, FlightModeAutomatic.Create(this));
    ArrayPush(this.modes, FlightModeFly.Create(this));
    ArrayPush(this.modes, FlightModeDroneAntiGravity.Create(this));
    ArrayPush(this.modes, FlightModeDrone.Create(this));

    this.audioUpdate = new FlightAudioUpdate();
  }

  private final func OnGameDetach() -> Void {
    //FlightLog.Info("[FlightComponent] OnGameDetach: " + this.GetVehicle().GetDisplayName());
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestUnregisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    this.UnregisterVehicleTPPBBListener();
    this.isDestroyed = true;
    this.hasExploded = true;
    if this.active {
      this.Deactivate(true);
    }
    this.hasUpdate = false;
    if IsDefined(this.uiBlackboard) && IsDefined(this.menuCallback) {
      this.uiBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_System.IsInMenu, this.menuCallback);
    }
    if IsDefined(this.uiGameDataBlackboard) && IsDefined(this.popupCallback) {
      this.uiGameDataBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown, this.popupCallback);
    }
  }

  protected cb func OnIsInMenu(inMenu: Bool) -> Bool {
    this.isInMenu = inMenu;
    this.UpdateAudioParams(1.0/60.0);
  }
  protected cb func OnPopupIsShown(isShown: Bool) -> Bool {
    this.isPopupShown = isShown;
    this.UpdateAudioParams(1.0/60.0);
  }
  
  // private final func RegisterInputListener() -> Void {
  //   let playerPuppet: ref<PlayerPuppet> = GameInstance.GetPlayerSystem(this.GetVehicle().GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet;
  //   playerPuppet.RegisterInputListener(this, n"VehicleInsideWheel");
  //   playerPuppet.RegisterInputListener(this, n"VehicleHorn");
  // }

  // private final func UnregisterInputListener() -> Void {
  //   let playerPuppet: ref<PlayerPuppet> = GameInstance.GetPlayerSystem(this.GetVehicle().GetGame()).GetLocalPlayerMainGameObject() as PlayerPuppet;
  //   if IsDefined(playerPuppet) {
  //     playerPuppet.UnregisterInputListener(this);
  //   };
  // }

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

  private func GetPitch() -> Float{
    return ClampF(700.0 / this.stats.s_mass + 0.5, 0.25, 2.0);
  }

  public func GetFlightModeIndex() -> Int32 {
    return this.mode;
  }

  public func GetFlightMode() -> ref<FlightMode> {
    return this.modes[this.mode];
  }

  public func GetNextFlightMode(direction: Int32) -> ref<FlightMode> {
    let mode = this.mode + direction;
    if mode < 0 {
      mode += ArraySize(this.sys.playerComponent.modes);
    } 
    mode = mode % ArraySize(this.sys.playerComponent.modes);
    return this.modes[mode];
  }

  public func GetNextFlightModeDescription() -> String {
    if ArraySize(this.modes) > 0 {
      return this.GetNextFlightMode(1).GetDescription();
    } else {
      return "None";
    }
  }

  // callbacks

  // public let uiControl: ref<FlightControllerUI>;
  
  protected cb func OnMountingEvent(evt: ref<MountingEvent>) -> Bool {
    // this.helper = this.GetVehicle().AddFlightHelper();
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if mountChild.IsPlayer() {
      // this.GetVehicle().TurnOffAirControl();
      this.SetupVehicleTPPBBListener();
      // FlightLog.Info("[FlightComponent] OnMountingEvent: " + this.GetVehicle().GetDisplayName());
      this.sys.audio.Start("windLeft", "wind_TPP");
      this.sys.audio.Start("windRight", "wind_TPP");
      // (this.GetVehicle().FindComponentByName(n"cars_sport_fx") as EffectSpawnerComponent).AddEffect();
      this.sys.playerComponent = this;
      this.isPlayerMounted = true;
      // this.uiControl = FlightControllerUI.Create(this.ui_info.GetGameController(), this.ui_info.GetGameController().GetRootCompoundWidget());
      // this.uiControl.Setup(this.stats);
    } else {
      // FlightLog.Info("[FlightComponent] OnMountingEvent for other vehicle: " + this.GetVehicle().GetDisplayName());
    }
  }
  
  protected cb func OnVehicleFinishedMountingEvent(evt: ref<VehicleFinishedMountingEvent>) -> Bool {
    // FlightLog.Info("[FlightComponent] OnVehicleFinishedMountingEvent: " + this.GetVehicle().GetDisplayName());
    if this.isPlayerMounted {
      this.sys.ctlr.Enable();
      if this.active {
        this.sys.ctlr.Activate(true);
        // this.sys.audio.Stop("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())));
        //this.sys.audio.Play("vehicle3_on");
        // this.sys.audio.StartWithPitch("playerVehicle", "vehicle3_TPP", this.GetPitch());
      }
    }
    let normal: Vector4;
    this.SetupTires();
    if !this.FindGround(normal) || this.distance > 2.0 {
      this.Activate();
    }
  }

  protected cb func OnUnmountingEvent(evt: ref<UnmountingEvent>) -> Bool {
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if IsDefined(mountChild) && mountChild.IsPlayer() {
      this.UnregisterVehicleTPPBBListener();
      this.sys.audio.Stop("windLeft");
      this.sys.audio.Stop("windRight");
      this.sys.playerComponent = null;
      this.isPlayerMounted = false;
      if this.active {
        this.sys.ctlr.Deactivate(true);
        // this.sys.audio.Stop("playerVehicle");
        // this.sys.audio.StartWithPitch("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())), "vehicle3_TPP", this.GetPitch());
      }
      this.sys.ctlr.Disable();
    }
  }

  // protected cb func OnVehicleHasExplodedEvent(evt: ref<VehicleHasExplodedEvent>) -> Bool {
  //   this.sys.audio.Stop("vehicleDestroyed" + this.GetUniqueID());
  //   this.Deactivate(true);
  // }

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
    if evt.isInWater  {
      this.audioUpdate.water = 1.0;
    } else {
      this.audioUpdate.water = 0.0;
    }
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    FlightLog.Info("[FlightComponent] OnAction: " + this.GetVehicle().GetDisplayName());
  }

  protected cb func OnVehicleFlightActivationEvent(evt: ref<VehicleFlightActivationEvent>) -> Bool {
    this.Activate();
  }

  public func Activate(opt silent: Bool) -> Void {
    // this.helper = this.GetVehicle().AddFlightHelper();
    FlightLog.Info("[FlightComponent] OnVehicleFlightActivationEvent: " + this.GetVehicle().GetDisplayName());
    if !this.active {

      this.uiBlackboard = GameInstance.GetBlackboardSystem(this.sys.ctlr.gameInstance).Get(GetAllBlackboardDefs().UI_System);
      if IsDefined(this.uiBlackboard) {
        if !IsDefined(this.menuCallback) {
          this.menuCallback = this.uiBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_System.IsInMenu, this, n"OnIsInMenu");
          this.isInMenu = this.uiBlackboard.GetBool(GetAllBlackboardDefs().UI_System.IsInMenu);
        }
      }

      this.uiGameDataBlackboard = GameInstance.GetBlackboardSystem(this.sys.ctlr.gameInstance).Get(GetAllBlackboardDefs().UIGameData);
      if IsDefined(this.uiGameDataBlackboard) {
        if !IsDefined(this.popupCallback) {
          this.popupCallback = this.uiGameDataBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown, this, n"OnPopupIsShown");
          this.isPopupShown = this.uiGameDataBlackboard.GetBool(GetAllBlackboardDefs().UIGameData.Popup_IsShown);
        }
      }

      this.stats = FlightStats.Create(this.GetVehicle());
      this.sys.ctlr.ui.Setup(this.stats);

      this.SetupTires();
      this.fx.Start();
      // these stop engine noises if they were already playing?
      this.GetVehicle().TurnEngineOn(false);
      // this.GetVehicle().TurnOn(true);
      this.GetVehicle().GetVehicleComponent().GetVehicleControllerPS().SetLightMode(vehicleELightMode.HighBeams);
      this.GetVehicle().GetVehicleComponent().GetVehicleController().ToggleLights(true);

      this.hoverGroundPID.Reset();
      this.hoverPID.Reset();
      this.pitchGroundPID.Reset();
      this.pitchPID.Reset();
      this.rollGroundPID.Reset();
      this.rollPID.Reset();
      this.yawPID.Reset();
      this.pitchAeroPID.Reset();

      this.modes[this.mode].Activate();

      if this.isPlayerMounted {
        this.mode = this.sys.ctlr.mode;
        this.sys.ctlr.Activate(silent);
        this.sys.audio.Play("vehicle3_on");
        // this.sys.audio.StartWithPitch("playerVehicle", "vehicle3_TPP", this.GetPitch());
        // this.sys.audio.Start("leftFront", "vehicle3_TPP");
        // this.sys.audio.Start("rightFront", "vehicle3_TPP");
        // this.sys.audio.Start("leftRear", "vehicle3_TPP");
        // this.sys.audio.Start("rightRear", "vehicle3_TPP");
      }
      this.sys.audio.StartWithPitch("vehicle" + this.GetUniqueID(), "vehicle3_TPP", this.GetPitch());
      this.active = true;
      this.hasUpdate = true;
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

  protected func OnUpdate(timeDelta: Float) -> Void {
    if this.GetVehicle().IsDestroyed() {
      if !this.isDestroyed {
        this.sys.audio.StartWithPitch("vehicleDestroyed" + this.GetUniqueID(), "vehicle3_destroyed", 1.0);
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
    if timeDelta <= 0.0 {
      this.stats.UpdateDynamic();
      this.UpdateAudioParams(1.0/60.0);
      return;
    }
    if this.active {
      if this.isPlayerMounted {
        this.sys.ctlr.OnUpdate(timeDelta);
        let fc = this.sys.ctlr;
        this.yaw = fc.yaw.GetValue();
        this.roll = fc.roll.GetValue();
        this.pitch = fc.pitch.GetValue();
        this.lift = fc.lift.GetValue();
        this.linearBrake = fc.linearBrake.GetValue();
        this.angularBrake = fc.angularBrake.GetValue();
        this.surge = fc.surge.GetValue();
        this.sway = fc.sway.GetValue();
      } else {
        let v = this.GetVehicle();
        this.surge = v.acceleration * 0.5 - v.deceleration * 0.1;
        this.yaw = -v.turnX4;
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

    this.stats.UpdateDynamic();

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
        force += this.modes[this.mode].force;
        torque += this.modes[this.mode].torque;
      }
    }

    // this.smoothForce = Vector4.Interpolate(this.smoothForce, force, 0.99);
    // this.smoothTorque = Vector4.Interpolate(this.smoothTorque, torque, 0.99);


    // process user-inputted force/torque in visuals/audio
    this.fx.Update(force, torque);
    this.UpdateAudioParams(timeDelta, force, torque);
    
    if this.isPlayerMounted {
      this.sys.ctlr.GetBlackboard().SetVector4(GetAllBlackboardDefs().VehicleFlight.Force, force);
      this.sys.ctlr.GetBlackboard().SetVector4(GetAllBlackboardDefs().VehicleFlight.Torque, torque);
      this.sys.ctlr.GetBlackboard().SetFloat(GetAllBlackboardDefs().VehicleFlight.Pitch, Vector4.GetAngleDegAroundAxis(this.stats.d_localUp, FlightUtils.Up(), FlightUtils.Right()));
      this.sys.ctlr.GetBlackboard().SetFloat(GetAllBlackboardDefs().VehicleFlight.Roll, Vector4.GetAngleDegAroundAxis(this.stats.d_localUp, FlightUtils.Up(), FlightUtils.Forward()));
      // this.sys.ctlr.GetBlackboard().SignalFloat(GetAllBlackboardDefs().VehicleFlight.Pitch);
      this.sys.ctlr.GetBlackboard().SetVector4(GetAllBlackboardDefs().VehicleFlight.Position, this.stats.d_position);
      // this.sys.ctlr.GetBlackboard().SignalVector4(GetAllBlackboardDefs().VehicleFlight.Position);
    }
    
    // apply physics helpers
    if this.mode < ArraySize(this.modes) {
      this.modes[this.mode].ApplyPhysics(timeDelta);
      force += this.modes[this.mode].force;
      torque += this.modes[this.mode].torque;
    }

    force *= timeDelta;
    // factor in mass
    force *= this.stats.s_mass;
    // convet to global
    force = this.stats.d_orientation * force;

    torque *= timeDelta;
    // factor in interia tensor - maybe half?
    let it = this.GetVehicle().GetInertiaTensor();
    torque.X *= it.X.X;
    // torque.X *= SqrtF(it.X.X) * 20.0;
    torque.Y *= it.Y.Y;
    // torque.Y *= SqrtF(it.Y.Y) * 20.0;
    torque.Z *= it.Z.Z;
    // torque.Z *= SqrtF(it.Z.Z) * 20.0;
    // convert to global
    torque = this.stats.d_orientation * torque;
    
    if this.collisionTimer < FlightSettings.GetFloat(n"collisionRecoveryDelay") + FlightSettings.GetFloat(n"collisionRecoveryDuration") {
      let collisionDampener = MinF(MaxF(0.0, (this.collisionTimer - FlightSettings.GetFloat(n"collisionRecoveryDelay")) / FlightSettings.GetFloat(n"collisionRecoveryDuration")), 1.0);
      torque *= collisionDampener;
      force *= collisionDampener;
      this.collisionTimer += timeDelta;
    }

    this.force += force;
    this.torque += torque;
  }

  protected cb func OnVehicleFlightDeactivationEvent(evt: ref<VehicleFlightDeactivationEvent>) -> Bool {
    FlightLog.Info("[FlightComponent] OnVehicleFlightDeactivationEvent: " + this.GetVehicle().GetDisplayName());
    this.Deactivate(evt.silent);
  }

  public func Deactivate(silent: Bool) -> Void{
    this.active = false;
    this.fx.Stop();

    if this.isDestroyed && this.hasExploded && this.alarmIsPlaying {
        this.sys.audio.Stop("vehicleDestroyed" + this.GetUniqueID());
    }

    if !silent {
      this.GetVehicle().TurnEngineOn(true);
    }

    if this.isPlayerMounted {
      this.sys.ctlr.Deactivate(silent);
      if !silent {
        this.sys.audio.Play("vehicle3_off");
      }
      // this.sys.audio.Stop("playerVehicle");
      // this.sys.audio.Stop("leftFront");
      // this.sys.audio.Stop("rightFront");
      // this.sys.audio.Stop("leftRear");
      // this.sys.audio.Stop("rightRear");
    }
    this.sys.audio.Stop("vehicle" + this.GetUniqueID());
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
      // FlightLog.Info("[FlightComponent] OnGridDestruction: " + FloatToStringPrec(biggestImpact, 2));
    if biggestImpact > 0.00 {
      this.ProcessImpact(biggestImpact);
      if this.isPlayerMounted {
        this.sys.ctlr.ProcessImpact(biggestImpact);
      } else {
        // if biggestImpact > 0.00 {
        //   if !this.active {
        //     this.Activate();
        //   } else {
        //     // this.Deactivate(true);
        //   }
        // }
        // if !this.active {
        //   this.FireVerticalImpulse(gridID);
        // }
        if biggestImpact > 0.20 {
          GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"explosion");
        }
        // let event = new vehicleDriveToPointEvent();
        // event.targetPos = new Vector3(0.0, 0.0, 0.0);
        // vehicle.QueueEvent(event);
      }
    }
  }

  // protected cb func OnHit(evt: ref<gameHitEvent>) -> Bool {
  //   let vehicle: ref<VehicleObject> = this.GetVehicle();
  //   let gameInstance: GameInstance = vehicle.GetGame();
  //   let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
  //   let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), vehicle);
  //   FlightLog.Info("[FlightComponent] OnPhysicalCollision: " + FloatToStringPrec(evt.attackData.vehicleImpactForce, 2));
  //   if isPlayerMounted {
  //       this.sys.ctlr.ProcessImpact(evt.attackData.vehicleImpactForce);
  //   } else {
  //     let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
  //     impulseEvent.radius = 1.0;
  //     impulseEvent.worldPosition = Vector4.Vector4To3(evt.hitPosition);
  //     impulseEvent.worldImpulse = new Vector3(0.0, 0.0, evt.attackData.vehicleImpactForce);
  //     this.GetVehicle().QueueEvent(impulseEvent);
  //   }
  // }

  
  protected cb func OnInteractionActivated(evt: ref<InteractionActivationEvent>) -> Bool {
    let radialRequest: ref<ResolveQuickHackRadialRequest>;
    if !IsDefined(evt.activator as PlayerPuppet) && !IsDefined(evt.activator as Muppet) {
      return false;
    };
    radialRequest = new ResolveQuickHackRadialRequest();
    this.GetVehicle().GetHudManager().QueueRequest(radialRequest);
  }

  protected cb func OnSetExposeQuickHacks(evt: ref<SetExposeQuickHacks>) -> Bool {
    let request: ref<RefreshActorRequest> = new RefreshActorRequest();
    request.ownerID = this.GetVehicle().GetEntityID();
    this.GetVehicle().GetHudManager().QueueRequest(request);
  }
  
  protected cb func OnActionEngineering(evt: ref<ActionEngineering>) -> Bool {
    FlightLog.Info("[FlightComponent] OnActionEngineering");
    // this.FireVerticalImpulse();
  }

  // public func OnQuickHackFlightMalfunction(evt: ref<QuickHackFlightMalfunction>) -> EntityNotificationType {
  //   FlightLog.Info("[FlightComponent] OnQuickHackFlightMalfunction");
  //   // let type: EntityNotificationType = this.OnQuickHackFlightMalfunction(evt);
  //   // if Equals(type, EntityNotificationType.DoNotNotifyEntity) {
  //   //   return type;
  //   // };
  //   if evt.IsStarted() {
  //     // this.ExecutePSAction(this.FireVerticalImpulse());
  //     // this.FireVerticalImpulse();
  //   };
  //   return EntityNotificationType.SendThisEventToEntity;
  // }

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
    // FlightLog.Info("[FlightComponent] FireVerticalImpulse: " + gridID);
    impulseEvent.worldPosition = Vector4.Vector4To3(this.GetVehicle().GetLocalToWorld() * offset);
    impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 10.0 * impulse * this.GetVehicle().GetTotalMass());
    this.GetVehicle().QueueEvent(impulseEvent);
  }

  // protected cb func OnPhysicalCollision(evt: ref<PhysicalCollisionEvent>) -> Bool {
  //   FlightLog.Info("[FlightComponent] OnPhysicalCollision");
  //   let vehicle = this.GetVehicle();
  //   let gameInstance: GameInstance = vehicle.GetGame();
  //   let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
  //   let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), this);
  //   if isPlayerMounted {
  //     // this.sys.ctlr.ProcessImpact(evt.attackData.vehicleImpactForce);
  //   } else {
  //     let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
  //     impulseEvent.radius = 1.0;
  //     impulseEvent.worldPosition = Vector4.Vector4To3(evt.worldPosition);
  //     impulseEvent.worldImpulse = new Vector3(0.0, 0.0, 10000.0);
  //     vehicle.QueueEvent(impulseEvent);
  //   }
  // }

  // protected cb func OnHit(evt: ref<gameHitEvent>) -> Bool {
  //   FlightLog.Info("[FlightComponent] OnHit: " + FloatToStringPrec(evt.attackData.vehicleImpactForce, 2));
  //   let vehicle = this.GetVehicle();
  //   let gameInstance: GameInstance = vehicle.GetGame();
  //   let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
  //   let isPlayerMounted = VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), this);
  //   if isPlayerMounted {
  //     this.sys.ctlr.ProcessImpact(evt.attackData.vehicleImpactForce);
  //   } else {
  //     let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
  //     impulseEvent.radius = 1.0;
  //     impulseEvent.worldPosition = Vector4.Vector4To3(evt.hitPosition);
  //     impulseEvent.worldImpulse = new Vector3(0.0, 0.0, evt.attackData.vehicleImpactForce);
  //     vehicle.QueueEvent(impulseEvent);
  //   }
  // }

  // public cb func OnAnyEvent(evt: ref<Event>) {
  //   FlightLog.Info("[FlightComponent] OnAnyEvent: " + ToString(evt.GetClassName()));
  // }

  // hook into sound somehow
  // protected cb func OnVehicleOnPartDetached(evt: ref<VehicleOnPartDetachedEvent>) -> Bool {
  //   let partName: CName = evt.partName;
  //   if Equals(partName, n"Trunk") {
  //     (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.trunk, VehicleDoorState.Detached);
  //   } else {
  //     if Equals(partName, n"Hood") {
  //       (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.hood, VehicleDoorState.Detached);
  //     } else {
  //       if Equals(partName, n"DoorFrontLeft") || Equals(partName, n"DoorFrontLeft_A") || Equals(partName, n"DoorFrontLeft_B") || Equals(partName, n"DoorFrontLeft_C") {
  //         (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.seat_front_left, VehicleDoorState.Detached);
  //       } else {
  //         if Equals(partName, n"DoorFrontRight") || Equals(partName, n"DoorFrontRight_A") || Equals(partName, n"DoorFrontRight_B") || Equals(partName, n"DoorFrontRight_C") {
  //           (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.seat_front_right, VehicleDoorState.Detached);
  //         } else {
  //           if Equals(partName, n"DoorBackLeft") {
  //             (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.seat_back_left, VehicleDoorState.Detached);
  //           } else {
  //             if Equals(partName, n"DoorBackRight") {
  //               (this.GetPS() as VehicleComponentPS).SetDoorState(EVehicleDoor.seat_back_right, VehicleDoorState.Detached);
  //             };
  //           };
  //         };
  //       };
  //     };
  //   };
  // }
  
  protected final func OnVehicleCameraChange(state: Bool) -> Void {
    this.sys.ctlr.isTPP = state;
  }

  public func ProcessImpact(impact: Float) {
    this.collisionTimer = (FlightSettings.GetFloat(n"collisionRecoveryDelay") + FlightSettings.GetFloat(n"collisionRecoveryDuration")) * (1.0 - (impact * this.GetFlightMode().collisionPenalty));
    // this.ui_info.StartGlitching(impact, FlightSettings.GetFloat(n"collisionRecoveryDuration") + impact);
  }

  public let audioUpdate: ref<FlightAudioUpdate>;

  public func UpdateAudioParams(timeDelta: Float, force: Vector4, torque: Vector4) -> Void {
    let ratio = 1.0;
    if this.collisionTimer < FlightSettings.GetFloat(n"collisionRecoveryDelay") + FlightSettings.GetFloat(n"collisionRecoveryDuration") {
      ratio = MaxF(0.0, (this.collisionTimer - FlightSettings.GetFloat(n"collisionRecoveryDelay")) / FlightSettings.GetFloat(n"collisionRecoveryDuration"));
    }
    
    let vehicleID = Cast<StatsObjectID>(this.GetVehicle().GetEntityID());
    let vehHealthPercent = GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).GetStatPoolValue(vehicleID, gamedataStatPoolType.Health);
    // this.audioUpdate.damage = 1.0 - MaxF(GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).GetStatPoolValue(Cast<StatsObjectID>(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, false) + ratio, 1.0);
    if this.isDestroyed {
      this.audioUpdate.damage = 0.25;
    } else {
      this.audioUpdate.damage = (1.0 - (vehHealthPercent / 100.0)) * 0.2;
    }
    if this.isPlayerMounted {
      // more responsive-sounding
      this.audioUpdate.surge = this.sys.ctlr.surge.GetInput() * ratio;
      this.audioUpdate.yaw = this.sys.ctlr.yaw.GetInput() * ratio;
      if IsDefined(this.GetFlightMode() as FlightModeDrone) {
        this.audioUpdate.lift = this.sys.ctlr.lift.GetInput() * ratio;
        this.audioUpdate.roll = this.sys.ctlr.roll.GetInput() * ratio;
        this.audioUpdate.pitch = this.sys.ctlr.pitch.GetInput() * ratio;
      } else {
        this.audioUpdate.lift = (Vector4.Dot(Vector4.Normalize(force), this.stats.d_localUp) * 0.1 + this.sys.ctlr.lift.GetInput()) * ratio;
        this.audioUpdate.roll = this.roll * ratio;
        this.audioUpdate.pitch = this.pitch * ratio;
      }
    } else {
      this.audioUpdate.surge = this.surge * ratio;
      this.audioUpdate.lift = (Vector4.Dot(Vector4.Normalize(force), this.stats.d_localUp) * 0.1 + this.lift) * ratio;
      this.audioUpdate.roll = this.roll * ratio;
      this.audioUpdate.pitch = this.pitch * ratio;
      this.audioUpdate.yaw = this.yaw * ratio;
    }
    // if this.mode == 3 {
    //   this.audioUpdate.surge *= 0.5;
    //   this.audioUpdate.surge += this.lift * ratio * 0.5;
    // }
    // this.audioUpdate.lift = this.lift * ratio;
    // this.audioUpdate.brake = this.brake;
    this.audioUpdate.brake = MaxF(this.sys.ctlr.linearBrake.GetInput(), this.sys.ctlr.angularBrake.GetInput());
    // this.audioUpdate.brake = Vector4.Dot(-force, this.stats.d_direction);

    this.UpdateAudioParams(timeDelta);
  }

  public func UpdateAudioParams(timeDelta: Float) -> Void {
    let engineVolume = 1.0;
    let windVolume = 0.6;
    let master = Cast<Float>((GameInstance.GetSettingsSystem(this.GetVehicle().GetGame()).GetVar(n"/audio/volume", n"MasterVolume") as ConfigVarInt).GetValue()) / 100.0;
    let sfx = Cast<Float>((GameInstance.GetSettingsSystem(this.GetVehicle().GetGame()).GetVar(n"/audio/volume", n"SfxVolume") as ConfigVarInt).GetValue()) / 100.0;
    engineVolume *= (master * sfx);
    windVolume *= (master * sfx);
    if this.isPopupShown || this.isInMenu || GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsPausedState() ||
      GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsTimeDilationActive(n"HubMenu") || 
      GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsTimeDilationActive(n"WorldMap")
      {
      engineVolume = 0.0;
      windVolume = 0.0;
      if this.isPlayerMounted {
        // this.sys.audio.Update("playerVehicle", Vector4.EmptyVector(), engineVolume);
        this.sys.audio.Update("windLeft", Vector4.EmptyVector(), windVolume, this.audioUpdate);
        this.sys.audio.Update("windRight", Vector4.EmptyVector(), windVolume, this.audioUpdate);
      }
      if this.active {
        this.sys.audio.Update("vehicle" + this.GetUniqueID(), Vector4.EmptyVector(), engineVolume, this.audioUpdate);
      }
      if this.isDestroyed && !this.GetVehicle().GetVehicleComponent().GetPS().GetHasExploded() && this.alarmIsPlaying {
        this.sys.audio.Update("vehicleDestroyed" + this.GetUniqueID(), Vector4.EmptyVector(), engineVolume, this.audioUpdate);
      }
      // this.sys.audio.Update("leftFront", Vector4.EmptyVector(), engineVolume);
      // this.sys.audio.Update("rightFront", Vector4.EmptyVector(), engineVolume);
      // this.sys.audio.Update("leftRear", Vector4.EmptyVector(), engineVolume);
      // this.sys.audio.Update("rightRear", Vector4.EmptyVector(), engineVolume);
      return;
    }

    // might need to handle just the scanning system's dilation, and the pause menu
    if GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsTimeDilationActive(n"radialMenu") {
      engineVolume *= 0.1;
      windVolume *= 0.1;
    }

    this.sys.audio.UpdateSlotProviders();

    // let leftFrontPosition = this.sys.audio.GetPosition(n"wheel_front_left") - (this.stats.d_velocity * timeDelta);
    // let rightFrontPosition = this.sys.audio.GetPosition(n"wheel_front_right") - (this.stats.d_velocity * timeDelta);
    // let leftRearPosition = this.sys.audio.GetPosition(n"wheel_back_left") - (this.stats.d_velocity * timeDelta);
    // let rightRearPosition = this.sys.audio.GetPosition(n"wheel_back_right") - (this.stats.d_velocity * timeDelta);

    let windLeftPosition = this.sys.audio.GetPosition(n"window_front_left_a"); // - (this.stats.d_velocity * timeDelta);
    let windRightPosition = this.sys.audio.GetPosition(n"window_front_right_a"); //- (this.stats.d_velocity * timeDelta);

    // let listenerMatrix = (this.sys.ctlr.player.FindComponentByName(n"soundListener") as IPlacedComponent).GetLocalToWorld();
    let listenerMatrix = this.sys.tppCamera.GetLocalToWorld();
    FlightAudio.UpdateListener(Matrix.GetTranslation(listenerMatrix), Matrix.GetAxisY(listenerMatrix), Matrix.GetAxisZ(listenerMatrix));

    this.audioUpdate.speed = this.stats.d_speed;
    this.audioUpdate.yawDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_up);
    this.audioUpdate.pitchDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_right);

    // engineVolume *= (ratio * 0.5 + 0.5);

    // this.sys.audio.Update("leftFront", leftFrontPosition, engineVolume);
    // this.sys.audio.Update("rightFront", rightFrontPosition, engineVolume);
    // this.sys.audio.Update("leftRear", leftRearPosition, engineVolume);
    // this.sys.audio.Update("rightRear", rightRearPosition, engineVolume);
    if this.isPlayerMounted {
      this.audioUpdate.inside = this.sys.ctlr.isTPP ? MaxF(0.0, this.audioUpdate.inside - timeDelta * 4.0) : MinF(1.0, this.audioUpdate.inside + timeDelta * 4.0);
      this.sys.audio.Update("windLeft", windLeftPosition, windVolume, this.audioUpdate);
      this.sys.audio.Update("windRight", windRightPosition, windVolume, this.audioUpdate);
    } else {
      this.audioUpdate.inside = 0.0;
    }
    if this.active {
      this.sys.audio.Update("vehicle" + this.GetUniqueID(), this.GetVehicle().GetWorldPosition(), engineVolume, this.audioUpdate);
    }
    if this.isDestroyed && !this.GetVehicle().GetVehicleComponent().GetPS().GetHasExploded() && this.alarmIsPlaying {
      this.sys.audio.Update("vehicleDestroyed" + this.GetUniqueID(), this.GetVehicle().GetWorldPosition(), engineVolume, this.audioUpdate);
    }
  }

  private func GetUniqueID() -> String {
    return ToString(EntityID.GetHash(this.GetVehicle().GetEntityID()));
  }
  
  public func SetupTires() -> Void {
    if this.GetVehicle() == (this.GetVehicle() as CarObject) {
      // this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_left_tire") as IPlacedComponent;
      // this.fr_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"front_right_tire") as IPlacedComponent;
      // this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_left_tire") as IPlacedComponent;
      // this.br_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"back_right_tire") as IPlacedComponent;
      this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFL") as IPlacedComponent;
      this.fr_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFR") as IPlacedComponent;
      this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBL") as IPlacedComponent;
      this.br_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBR") as IPlacedComponent;
      this.hood = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"VehicleHoodEmitter") as IPlacedComponent;
      this.trunk = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"VehicleTrunkEmitter") as IPlacedComponent;
    } else {
      this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFront") as IPlacedComponent;
      this.fr_tire = this.fl_tire;
      this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBack") as IPlacedComponent;
      this.br_tire = this.bl_tire;
    }
  }

  public func FindGround(out normal: Vector4) -> Bool {
    // let lookAhead = this.stats.d_velocity * timeDelta * this.lookAheadMax;
    // let fl_tire: Vector4 = Matrix.GetTranslation(this.fl_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    // let fr_tire: Vector4 = Matrix.GetTranslation(this.fr_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    // let bl_tire: Vector4 = Matrix.GetTranslation(this.bl_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
    // let br_tire: Vector4 = Matrix.GetTranslation(this.br_tire.GetLocalToWorld()) - this.stats.d_velocity * timeDelta;
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
    let lookDown = new Vector4(0.0, 0.0, -FlightSettings.GetFloat(n"maxHoverHeight") - 10.0, 0.0);
    this.sqs.SyncRaycastByCollisionGroup(fl_tire, fl_tire + lookDown, n"VehicleBlocker", findGround1, false, false);
    this.sqs.SyncRaycastByCollisionGroup(fr_tire, fr_tire + lookDown, n"VehicleBlocker", findGround2, false, false);
    this.sqs.SyncRaycastByCollisionGroup(bl_tire, bl_tire + lookDown, n"VehicleBlocker", findGround3, false, false);
    this.sqs.SyncRaycastByCollisionGroup(br_tire, br_tire + lookDown, n"VehicleBlocker", findGround4, false, false);
    
    let groundPoint1: Vector4;
    let groundPoint2: Vector4;
    let groundPoint3: Vector4;
    let groundPoint4: Vector4;

    if TraceResult.IsValid(findGround1) {
      groundPoint1 = Vector4.Vector3To4(findGround1.position);// - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint1);
      //   this.ui.DrawText(groundPoint1, FloatToStringPrec(Vector4.Distance(fl_tire, Cast(findGround1.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround2) {
      groundPoint2 = Vector4.Vector3To4(findGround2.position);// - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint2);
      //   this.ui.DrawText(groundPoint2, FloatToStringPrec(Vector4.Distance(fr_tire, Cast(findGround2.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround3) {
      groundPoint3 = Vector4.Vector3To4(findGround3.position);// - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint3);
      //   this.ui.DrawText(groundPoint3, FloatToStringPrec(Vector4.Distance(bl_tire, Cast(findGround3.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround4) {
      groundPoint4 = Vector4.Vector3To4(findGround4.position);// - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint4);
      //   this.ui.DrawText(groundPoint4, FloatToStringPrec(Vector4.Distance(br_tire, Cast(findGround4.position)), 2));
      // }
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
      // this.distance = distance * (1.0 - this.distanceEase) + this.distance * (this.distanceEase);
      this.distance = distance;
      
      // FromVariant(scriptInterface.GetStateVectorParameter(physicsStateValue.Radius)) maybe?
      let n = (Vector4.Normalize(Cast(findGround1.normal)) + Vector4.Normalize(Cast(findGround2.normal)) + Vector4.Normalize(Cast(findGround3.normal)) + Vector4.Normalize(Cast(findGround4.normal))) / 4.0;
      // this.normal = Vector4.Interpolate(this.normal, normal, this.normalEase);
      normal = Vector4.Normalize(n);

      return true;
    } else {
      return false;
    }   
  } 

  protected cb func OnHUDInstruction(evt: ref<HUDInstruction>) -> Bool {
    if evt.quickhackInstruction.ShouldProcess() {
      FlightLog.Info("[FlightComponent] OnHUDInstruction");
      this.GetVehicle().TryOpenQuickhackMenu(evt.quickhackInstruction.ShouldOpen());
    };
  }

  protected let m_attacksSpawned: array<ref<EffectInstance>>;

  public func OnFireWeapon(placeholderQuat: Quaternion, weaponItem:TweakDBID, attachmentSlot: TweakDBID) -> Void {    
    let weapon = TweakDBInterface.GetWeaponItemRecord(weaponItem);
    let wt: WorldTransform;
    let vehicleSlots = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"vehicle_slots") as SlotComponent;
    vehicleSlots.GetSlotTransform(StringToName(TweakDBInterface.GetAttachmentSlotRecord(attachmentSlot).EntitySlotName()), wt);
    let quat = WorldTransform.GetOrientation(wt);
    // let start = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(wt));
    // let end = Vector4.Vector3To4(tracePosition);
    // WorldTransform.SetPosition(wt, start)
    WorldTransform.SetOrientation(wt, quat * placeholderQuat);

    let effect = Cast<FxResource>(r"base\\fx\\vehicles\\av\\av_panzer\\weapons\\v_panzer_muzzle_flash.effect");
    // let effect = Cast<FxResource>(r"base\\fx\\weapons\\firearms\\_muzzle_lights\\smart\\w_s_rifles_mq_muzzle_lights_tpp.effect");
    let fxSystem = GameInstance.GetFxSystem(this.GetVehicle().GetGame());
    if IsDefined(fxSystem) {
      fxSystem.SpawnEffect(effect, wt);
    }

    
    // let attack: ref<Attack_GameEffect>;
    // let attackContext: AttackInitContext;
    // let effect: ref<EffectInstance>;
    // let position: Vector4;
    // let slotTransform: WorldTransform;
    // let statMods: array<ref<gameStatModifierData>>;
    // let slotName = StringToName(TweakDBInterface.GetAttachmentSlotRecord(attachmentSlot).EntitySlotName());
    // let validSlotPosition: Bool = vehicleSlots.GetSlotTransform(slotName, slotTransform);
    // if validSlotPosition {
    //   position = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(slotTransform));
    // } else {
    //   position = WorldPosition.ToVector4(WorldTransform.GetWorldPosition(this.GetVehicle().GetWorldTransform()));
    // };
    // attackContext.source = this.GetVehicle();
    // attackContext.record = weapon.RangedAttacks().DefaultFire().PlayerAttack();
    // attackContext.instigator = this.sys.player;
    // attack = IAttack.Create(attackContext) as Attack_GameEffect;
    // attack.GetStatModList(statMods);
    // effect = attack.PrepareAttack(this.sys.player);
    // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.position, position);
    // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.muzzlePosition, position);
    // EffectData.SetVector(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.forward, Quaternion.GetForward(placeholderQuat));
    // EffectData.SetVariant(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.attack, ToVariant(attack));
    // EffectData.SetVariant(effect.GetSharedData(), GetAllBlackboardDefs().EffectSharedData.attackStatModList, ToVariant(statMods));
    // attack.StartAttack();
    
    // effect.AttachToSlot(this.GetVehicle(), slotName, GetAllBlackboardDefs().EffectSharedData.position, GetAllBlackboardDefs().EffectSharedData.forward);
    
    // ArrayPush(this.m_attacksSpawned, effect);

    let broadcaster = this.sys.player.GetStimBroadcasterComponent();
    if IsDefined(broadcaster) {
      broadcaster.TriggerSingleBroadcast(this.sys.player, gamedataStimType.Gunshot, 25.0);
      let data: stimInvestigateData;
      data.illegalAction = true;
      data.attackInstigator = this.sys.player;
      broadcaster.TriggerSingleBroadcast(this.sys.player, gamedataStimType.Gunshot, 100.0, data, true);
    };
    
    // let tp: WorldPosition;
    // WorldPosition.SetVector4(tp, Vector4.Vector3To4(tracePosition));
    // fxi.UpdateTargetPosition(tp);
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