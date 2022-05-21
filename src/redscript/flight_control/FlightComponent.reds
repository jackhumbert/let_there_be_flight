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
  public let isPlayerMounted: Bool;

  let hoverGroundPID: ref<PID>;
  let hoverPID: ref<PID>;
  let pitchGroundPID: ref<DualPID>;
  let pitchPID: ref<DualPID>;
  let rollGroundPID: ref<DualPID>;
  let rollPID: ref<DualPID>;
  let yawPID: ref<PID>;

  private let sqs: ref<SpatialQueriesSystem>;

  public let fl_tire: ref<IPlacedComponent>;
  public let fr_tire: ref<IPlacedComponent>;
  public let bl_tire: ref<IPlacedComponent>;
  public let br_tire: ref<IPlacedComponent>;
  
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
  private let brake: Float;

  public let force: Vector4;
  public let torque: Vector4;

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
    
    this.hoverGroundPID = PID.Create(1.0, 0.01, 0.1);
    this.hoverPID = PID.Create(1.0, 0.01, 0.1);
    this.pitchGroundPID = DualPID.Create(0.8, 0.2, 0.05,  0.8, 0.2, 0.05);
    this.pitchPID = DualPID.Create(1.0, 0.5, 0.5,  1.0, 0.5, 0.5);
    this.rollGroundPID =  DualPID.Create(0.5, 0.2, 0.05,  2.5, 1.5, 0.5);
    this.rollPID =  DualPID.Create(1.0, 0.5, 0.5,  1.0, 0.5, 0.5);
    this.yawPID = PID.Create(1.0, 0.1, 0.0);

    this.sys = FlightSystem.GetInstance();
    this.sqs = GameInstance.GetSpatialQueriesSystem(this.GetVehicle().GetGame());
    this.fx = FlightFx.Create(this);
    
    // this.helper = this.GetVehicle().AddFlightHelper();
    // this.stats = FlightStats.Create(this.GetVehicle());

    this.collisionTimer = this.sys.settings.collisionRecoveryDelay();
    this.distance = 0.0;
    this.hoverHeight = this.sys.settings.defaultHoverHeight();
    
    ArrayPush(this.modes, FlightModeHoverFly.Create(this));
    ArrayPush(this.modes, FlightModeHover.Create(this));
    ArrayPush(this.modes, FlightModeFly.Create(this));
    ArrayPush(this.modes, FlightModeDrone.Create(this));

    // ArrayPush(this.sys.components, this);
  }

  private final func OnGameDetach() -> Void {
    //FlightLog.Info("[FlightComponent] OnGameDetach: " + this.GetVehicle().GetDisplayName());
    GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).RequestUnregisteringListener(Cast(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, this.m_healthStatPoolListener);
    this.UnregisterVehicleTPPBBListener();
    // ArrayRemove(this.sys.components, this);
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
    return 700.0 / this.stats.s_mass + 0.5;
  }

  // callbacks
  
  protected cb func OnMountingEvent(evt: ref<MountingEvent>) -> Bool {
    // this.helper = this.GetVehicle().AddFlightHelper();
    let mountChild: ref<GameObject> = GameInstance.FindEntityByID(this.GetVehicle().GetGame(), evt.request.lowLevelMountingInfo.childId) as GameObject;
    if mountChild.IsPlayer() {
      this.GetVehicle().TurnOffAirControl();
      this.SetupVehicleTPPBBListener();
      FlightLog.Info("[FlightComponent] OnMountingEvent: " + this.GetVehicle().GetDisplayName());
      this.sys.audio.Start("windLeft", "wind_TPP");
      this.sys.audio.Start("windRight", "wind_TPP");
      // (this.GetVehicle().FindComponentByName(n"cars_sport_fx") as EffectSpawnerComponent).AddEffect();
      this.sys.ctlr.ui.Setup(this.stats);
      this.sys.playerComponent = this;
      this.isPlayerMounted = true;
      if this.active {
        this.sys.ctlr.Activate();
        this.sys.audio.Stop("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())));
        this.sys.audio.Play("vehicle3_on");
        this.sys.audio.StartWithPitch("playerVehicle", "vehicle3_TPP", this.GetPitch());
      }
    } else {
      FlightLog.Info("[FlightComponent] OnMountingEvent for other vehicle: " + this.GetVehicle().GetDisplayName());
    }
  }
  
  protected cb func OnVehicleFinishedMountingEvent(evt: ref<VehicleFinishedMountingEvent>) -> Bool {
    // should put action update here
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
        this.sys.audio.Stop("playerVehicle");
        this.sys.audio.StartWithPitch("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())), "vehicle3_TPP", this.GetPitch());
      }
    }
  }

  protected cb func OnDeath(evt: ref<gameDeathEvent>) -> Bool {
    let vehicle: ref<VehicleObject> = this.GetVehicle();
    let gameInstance: GameInstance = vehicle.GetGame();
    let player: ref<PlayerPuppet> = GetPlayer(gameInstance);
    if VehicleComponent.IsMountedToProvidedVehicle(gameInstance, player.GetEntityID(), vehicle) {
      FlightLog.Info("[FlightComponent] OnDeath: " + this.GetVehicle().GetDisplayName());
      this.sys.ctlr.Disable();
    }
  }

  protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
    FlightLog.Info("[FlightComponent] OnAction: " + this.GetVehicle().GetDisplayName());
  }

  protected cb func OnVehicleFlightActivationEvent(evt: ref<VehicleFlightActivationEvent>) -> Bool {
    this.Activate();
  }

  public func Activate() -> Void {
    // this.helper = this.GetVehicle().AddFlightHelper();
    FlightLog.Info("[FlightComponent] OnVehicleFlightActivationEvent: " + this.GetVehicle().GetDisplayName());
    if !this.active {
      this.stats = FlightStats.Create(this.GetVehicle());

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

      if this.isPlayerMounted {
        this.sys.ctlr.Activate();
        this.sys.audio.Play("vehicle3_on");
        this.sys.audio.StartWithPitch("playerVehicle", "vehicle3_TPP", this.GetPitch());
        // this.sys.audio.Start("leftFront", "vehicle3_TPP");
        // this.sys.audio.Start("rightFront", "vehicle3_TPP");
        // this.sys.audio.Start("leftRear", "vehicle3_TPP");
        // this.sys.audio.Start("rightRear", "vehicle3_TPP");
      } else {
        this.sys.audio.StartWithPitch("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())), "vehicle3_TPP", this.GetPitch());
      }
      this.active = true;
    }
  }

  let smoothForce: Vector4;
  let smoothTorque: Vector4;

  protected func OnUpdate(timeDelta: Float) -> Void {
    if this.GetVehicle().IsDestroyed() {
      this.Deactivate(true);
      return;
    }
    if timeDelta > 0.0 {
    // if IsDefined(this.helper) {
      if this.isPlayerMounted {
        this.sys.ctlr.OnUpdate(timeDelta);
        let fc = this.sys.ctlr;
        this.yaw = fc.yaw.GetValue();
        this.roll = fc.roll.GetValue();
        this.pitch = fc.pitch.GetValue();
        this.lift = fc.lift.GetValue();
        this.brake = fc.brake.GetValue();
        this.surge = fc.surge.GetValue();
        this.mode = fc.mode;
      }

      this.stats.UpdateDynamic();
      this.UpdateAudioParams(timeDelta);

      let force = new Vector4(0.0, 0.0, 0.0, 0.0);
      let torque = new Vector4(0.0, 0.0, 0.0, 0.0);

      if this.mode < ArraySize(this.modes) {
        this.modes[this.mode].Update(timeDelta);
        force = this.modes[this.mode].force;
        torque = this.modes[this.mode].torque;
      }

      this.smoothForce = Vector4.Interpolate(this.smoothForce, force, 0.99);
      this.smoothTorque = Vector4.Interpolate(this.smoothTorque, torque, 0.99);

      this.fx.Update(force, torque);

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
      
      if this.collisionTimer < this.sys.settings.collisionRecoveryDelay() + this.sys.settings.collisionRecoveryDuration() {
        let collisionDampener = MinF(MaxF(0.0, (this.collisionTimer - this.sys.settings.collisionRecoveryDelay()) / this.sys.settings.collisionRecoveryDuration()), 1.0);
        torque *= collisionDampener;
        force *= collisionDampener;
        this.collisionTimer += timeDelta;
      }

      // this.helper.force = this.helper.force + force;
      // this.helper.torque = this.helper.torque + torque;
      this.force += force;
      this.torque += torque;
    // }
    } else {
      this.stats.UpdateDynamic();
      this.UpdateAudioParams(1.0/60.0);
    }
  }

  protected cb func OnVehicleFlightDeactivationEvent(evt: ref<VehicleFlightDeactivationEvent>) -> Bool {
    this.Deactivate(evt.silent);
  }

  public func Deactivate(silent: Bool) -> Void{
    this.active = false;
    FlightLog.Info("[FlightComponent] OnVehicleFlightDeactivationEvent: " + this.GetVehicle().GetDisplayName());
    this.fx.Stop();

    if !silent {
      this.GetVehicle().TurnEngineOn(true);
    }

    if this.isPlayerMounted {
      if !silent {
        this.sys.audio.Play("vehicle3_off");
      }
      this.sys.audio.Stop("playerVehicle");
      // this.sys.audio.Stop("leftFront");
      // this.sys.audio.Stop("rightFront");
      // this.sys.audio.Stop("leftRear");
      // this.sys.audio.Stop("rightRear");
    } else {
      this.sys.audio.Stop("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())));
    }
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
    if this.isPlayerMounted {
      if biggestImpact > 0.00 {
        this.ProcessImpact(biggestImpact);
        this.sys.ctlr.ProcessImpact(biggestImpact);
      }
    } else {
      if biggestImpact > 0.10 {
        if !this.active {
          this.Activate();
        } else {
          this.Deactivate(true);
        }
      }
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

  public func OnQuickHackFlightMalfunction(evt: ref<QuickHackFlightMalfunction>) -> EntityNotificationType {
    FlightLog.Info("[FlightComponent] OnQuickHackFlightMalfunction");
    // let type: EntityNotificationType = this.OnQuickHackFlightMalfunction(evt);
    // if Equals(type, EntityNotificationType.DoNotNotifyEntity) {
    //   return type;
    // };
    if evt.IsStarted() {
      // this.ExecutePSAction(this.FireVerticalImpulse());
      // this.FireVerticalImpulse();
    };
    return EntityNotificationType.SendThisEventToEntity;
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
    this.collisionTimer = this.sys.settings.collisionRecoveryDelay() - impact;
  }

  public func UpdateAudioParams(timeDelta: Float) -> Void {
    let engineVolume = 0.85;
    let windVolume = 0.5;
    // let engineVolume = (GameInstance.GetSettingsSystem(this.GetVehicle().GetGame()).GetVar(n"/audio/volume", n"MasterVolume") as ConfigVarListInt).GetValue();
    // let engineVolume *= (GameInstance.GetSettingsSystem(this.GetVehicle().GetGame()).GetVar(n"/audio/volume", n"SfxVolume") as ConfigVarListInt).GetValue();
    if GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsPausedState() ||
      GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsTimeDilationActive(n"HubMenu") || 
      GameInstance.GetTimeSystem(this.GetVehicle().GetGame()).IsTimeDilationActive(n"WorldMap")
      {
      engineVolume = 0.0;
      windVolume = 0.0;
      if this.isPlayerMounted {
        this.sys.audio.Update("playerVehicle", Vector4.EmptyVector(), engineVolume);
        this.sys.audio.Update("windLeft", Vector4.EmptyVector(), windVolume);
        this.sys.audio.Update("windRight", Vector4.EmptyVector(), windVolume);
      } else {
        this.sys.audio.Update("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())), Vector4.EmptyVector(), engineVolume);
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

    let listenerMatrix = (this.sys.ctlr.player.FindComponentByName(n"soundListener") as IPlacedComponent).GetLocalToWorld();
    this.sys.audio.listenerPosition = Matrix.GetTranslation(listenerMatrix);
    this.sys.audio.listenerForward = Matrix.GetAxisY(listenerMatrix);
    this.sys.audio.listenerUp = Matrix.GetAxisZ(listenerMatrix);

    this.sys.audio.speed = this.stats.d_speed;
    this.sys.audio.yawDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_up);
    this.sys.audio.pitchDiff = Vector4.GetAngleDegAroundAxis(this.stats.d_forward, this.stats.d_direction, this.stats.d_right);


    let ratio = 1.0;
    if this.collisionTimer < this.sys.settings.collisionRecoveryDelay() + this.sys.settings.collisionRecoveryDuration() {
      ratio = MaxF(0.0, (this.collisionTimer - this.sys.settings.collisionRecoveryDelay()) / this.sys.settings.collisionRecoveryDuration());
    }
    
    this.sys.audio.damage = 1.0 - MaxF(GameInstance.GetStatPoolsSystem(this.GetVehicle().GetGame()).GetStatPoolValue(Cast<StatsObjectID>(this.GetVehicle().GetEntityID()), gamedataStatPoolType.Health, false) + ratio, 1.0);

    if this.isPlayerMounted {
      // more responsive-sounding
      this.sys.audio.surge = this.sys.ctlr.surge.GetInput() * ratio;
    } else {
      this.sys.audio.surge = this.surge * ratio;
    }
    if this.mode == 3 {
      this.sys.audio.surge *= 0.5;
      this.sys.audio.surge += this.lift * ratio * 0.5;
    }
    this.sys.audio.yaw = this.yaw * ratio;
    this.sys.audio.lift = this.lift * ratio;
    this.sys.audio.brake = this.brake;
    // engineVolume *= (ratio * 0.5 + 0.5);

    // this.sys.audio.Update("leftFront", leftFrontPosition, engineVolume);
    // this.sys.audio.Update("rightFront", rightFrontPosition, engineVolume);
    // this.sys.audio.Update("leftRear", leftRearPosition, engineVolume);
    // this.sys.audio.Update("rightRear", rightRearPosition, engineVolume);
    if this.isPlayerMounted {
      this.sys.audio.inside = this.sys.ctlr.isTPP ? MaxF(0.0, this.sys.audio.inside - timeDelta * 4.0) : MinF(1.0, this.sys.audio.inside + timeDelta * 4.0);
      this.sys.audio.Update("playerVehicle", this.sys.audio.listenerPosition, engineVolume);
      this.sys.audio.Update("windLeft", windLeftPosition, windVolume);
      this.sys.audio.Update("windRight", windRightPosition, windVolume);
    } else {
      this.sys.audio.inside = 0.0;
      this.sys.audio.Update("otherVehicle" + ToString(EntityID.GetHash(this.GetVehicle().GetEntityID())), this.GetVehicle().GetWorldPosition(), engineVolume);
    }
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
    } else {
      this.fl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterFront") as IPlacedComponent;
      this.fr_tire = this.fl_tire;
      this.bl_tire = this.GetVehicle().GetVehicleComponent().FindComponentByName(n"WheelAudioEmitterBack") as IPlacedComponent;
      this.br_tire = this.bl_tire;
    }
  }

  public func FindGround(timeDelta: Float, out normal: Vector4) -> Bool {
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
    this.sqs.SyncRaycastByCollisionGroup(fl_tire, fl_tire + this.sys.settings.lookDown(), n"VehicleBlocker", findGround1, false, false);
    this.sqs.SyncRaycastByCollisionGroup(fr_tire, fr_tire + this.sys.settings.lookDown(), n"VehicleBlocker", findGround2, false, false);
    this.sqs.SyncRaycastByCollisionGroup(bl_tire, bl_tire + this.sys.settings.lookDown(), n"VehicleBlocker", findGround3, false, false);
    this.sqs.SyncRaycastByCollisionGroup(br_tire, br_tire + this.sys.settings.lookDown(), n"VehicleBlocker", findGround4, false, false);
    
    let groundPoint1: Vector4;
    let groundPoint2: Vector4;
    let groundPoint3: Vector4;
    let groundPoint4: Vector4;

    if TraceResult.IsValid(findGround1) {
      groundPoint1 = Vector4.Vector3To4(findGround1.position) - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint1);
      //   this.ui.DrawText(groundPoint1, FloatToStringPrec(Vector4.Distance(fl_tire, Cast(findGround1.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround2) {
      groundPoint2 = Vector4.Vector3To4(findGround2.position) - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint2);
      //   this.ui.DrawText(groundPoint2, FloatToStringPrec(Vector4.Distance(fr_tire, Cast(findGround2.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround3) {
      groundPoint3 = Vector4.Vector3To4(findGround3.position) - this.stats.d_velocity * timeDelta;
      // if this.showUI {
      //   this.ui.DrawMark(groundPoint3);
      //   this.ui.DrawText(groundPoint3, FloatToStringPrec(Vector4.Distance(bl_tire, Cast(findGround3.position)), 2));
      // }
    }
    if TraceResult.IsValid(findGround4) {
      groundPoint4 = Vector4.Vector3To4(findGround4.position) - this.stats.d_velocity * timeDelta;
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