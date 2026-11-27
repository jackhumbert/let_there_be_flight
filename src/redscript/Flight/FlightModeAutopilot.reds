// The Autopilot: a C++ flight mode (src/red4ext/Flight/FlightModeAutopilot.hpp around Navigation::Autopilot,
// docs/design/autopilot.md). It flies the planned route to the tracked destination (or FlightNavMap's target override,
// or a summon's own goal), descends and lands, or flies free roam legs with nothing tracked. This file declares its
// fields for script and holds the game glue the C++ calls by name: the tracked destination, vanilla autodrive's facts
// and HUD panel, the cinematic camera, the toast and FlightController. The script mode it replaced is in git at
// c3e24ab.
//
// A mod extends it by subclassing it and overriding FlightMode's hooks (OnActivate, OnUpdate, ...).
public native class FlightModeAutopilot extends FlightMode {
  // its Mod Settings switch (Flight/Settings.hpp: registered from C++ under this class, as the script registered it)
  public native let enabled: Bool;
  // the Standard mode's lift factor flown this frame
  public native let standardModeLiftFactor: Float;

  // The tuning (Navigation/AutopilotParams.hpp, defaults and meaning there): the C++ fields themselves, so a write
  // reaches the next frame. brakeAccel, landingMode (the Mod Setting Land at Destination), padRange and attitudeStyle
  // (Autopilot Style) are Mod Settings, written back from their saved values when the mode is engaged; any field can be
  // overridden with the developer setting autopilot.<name>.
  public native let brakeAccel: Float;
  // 0 hover over the destination, 1 set down on the wheels, 2 touch down (Navigation::LandingMode)
  public native let landingMode: Int32;
  public native let padRange: Float;
  // 0 drone (the body tilts towards the thrust), 1 spinner (it banks, the nose stays level; Navigation::AttitudeStyle)
  public native let attitudeStyle: Int32;
  public native let droneBrakeTurn: Bool;
  public native let droneBodyShare: Float;
  public native let droneBrakeShare: Float;
  public native let droneTiltFloor: Float;
  public native let droneTakeoffTilt: Float;
  public native let droneTakeoffIn: Float;
  public native let droneTakeoffFull: Float;
  public native let droneTakeoffOut: Float;
  public native let droneTakeoffLength: Float;
  public native let droneMaxTilt: Float;
  public native let droneTiltTime: Float;
  public native let dronePushOver: Float;
  public native let trajectoryAttitudeLead: Float;
  public native let replanOffLine: Float;
  public native let trajectoryCorrectionMax: Float;
  public native let rejoinAhead: Float;
  public native let rejoinMin: Float;
  public native let hitAvoidRadius: Float;
  public native let hitAvoidTime: Float;
  public native let droneYawLead: Float;
  public native let droneYawFrequency: Float;
  public native let droneYawRateMax: Float;
  public native let droneYawAccelMax: Float;
  public native let droneLandSlewYaw: Float;
  public native let droneLandSlewBefore: Float;
  public native let droneLandSlewDuring: Float;
  public native let approachHeight: Float;
  public native let descentRate: Float;
  public native let touchdownRate: Float;
  public native let descentAccel: Float;
  public native let wheelClearance: Float;
  public native let landingSearch: Int32;
  public native let landingShaft: Float;
  public native let landingFlat: Float;
  public native let landingFootprint: Float;
  public native let landingOverSpot: Float;
  public native let landingEntry: Float;
  public native let landingEntrySpeed: Float;
  public native let landingRecheck: Float;
  public native let landingShiftSpan: Float;
  public native let padSnapDistance: Float;
  public native let padSnapLevel: Float;
  public native let brakeMax: Float;
  public native let speedGain: Float;
  public native let accelJerk: Float;
  public native let yawSmoothing: Float;
  public native let velocityGain: Float;
  public native let maxAccel: Float;
  public native let overrideHold: Float;
  public native let replanDistance: Float;
  public native let arriveDistance: Float;
  public native let cornerOvershoot: Float;
  public native let cornerSpan: Float;
  public native let collisionHorizon: Float;
  public native let attitudeFrequency: Float;
  public native let attitudeDamping: Float;
  public native let maxAngularAccel: Float;
  public native let torqueGainPitch: Float;
  public native let torqueGainRoll: Float;
  public native let torqueGainYaw: Float;
  public native let attitudeLead: Float;
  public native let maxHeadingRate: Float;
  public native let raceMargin: Float;
  public native let raceJerk: Float;
  public native let raceSpeedCap: Float;
  public native let raceCornerRound: Float;
  public native let raceRoundShare: Float;
  public native let raceRoundStep: Float;
  public native let raceRoundMin: Float;
  public native let raceOffLine: Float;
  public native let raceRouteHeight: Float;
  public native let raceClimbCost: Float;
  public native let raceMinPassage: Float;
  public native let raceSearchWeight: Float;
  public native let raceTurnCost: Float;
  public native let raceDriftMax: Float;
  public native let racePlanDecel: Float;
  public native let racePushOver: Float;
  public native let lineLead: Float;
  public native let lineSpring: Float;
  public native let lineVelocityGain: Float;
  public native let lineSpan: Float;
  public native let raceRollFrequency: Float;
  public native let raceRollRateMax: Float;
  public native let raceRollAccelMax: Float;
  public native let racePitchFrequency: Float;
  public native let racePitchRateMax: Float;
  public native let racePitchAccelMax: Float;
  public native let freeRoamRadius: Float;
  public native let freeRoamLegMin: Float;
  public native let freeRoamLegMax: Float;
  public native let freeRoamSpread: Float;
  public native let freeRoamMaxDepth: Float;
  public native let freeRoamMaxTurn: Float;
  public native let freeRoamMaxPitch: Float;
  public native let freeRoamGateMin: Float;
  public native let gateLead: Float;
  public native let gateStep: Float;
  public native let gateMemory: Int32;
  public native let freeRoamChainAt: Float;
  public native let freeRoamJoinAhead: Float;
  public native let freeRoamJoinMin: Float;
  public native let freeRoamSmoothSpan: Float;
  public native let freeRoamTopSpeed: Float;
  public native let freeRoamMaxRise: Float;
  public native let freeRoamRouteHeight: Float;
  public native let freeRoamHeightFree: Float;
  public native let freeRoamHeightWavelength: Float;
  public native let freeRoamOverCost: Float;
  public native let freeRoamGuideCost: Float;
  public native let freeRoamGuideLine: Bool;
  public native let positionJumpHold: Float;

  // The state (Navigation/Autopilot.hpp), under the script's names: the C++ fields themselves for numbers and flags,
  // copies taken after each call into the Autopilot for Strings and Vector4s. Writing a copy does nothing, except the
  // summon's three (externalDestination, padLandingOffset, landingKeepClear), which reach the next call.
  public native let status: String;
  public native let externalDestination: Vector4;
  public native let padLandingOffset: Vector4;
  public native let landingKeepClear: Vector4;
  public native let destination: Vector4;
  public native let holdPosition: Vector4;
  public native let routeGoal: Vector4;
  public native let landingSpot: Vector4;
  public native let landingHeading0: Vector4;
  public native let landingHeading: Vector4;
  public native let lastPosition: Vector4;
  public native let freeRoamCentre: Vector4;
  public native let joinPoint: Vector4;
  public native let gateEntry: Vector4;
  public native let routeGateEntry: Vector4;
  public native let landSlewHeading: Vector4;
  public native let thrustCmd: Vector4;
  public native let closestNow: Vector4;
  public native let pathCurvature: Vector4;
  public native let accelCmd: Vector4;
  public native let headingRef: Vector4;
  public native let attAccel: Vector4;
  public native let attRef: Vector4;
  public native let targetRight: Vector4;
  public native let targetForward: Vector4;
  public native let targetUp: Vector4;
  public native let omegaRef: Vector4;
  // (surge* are the script's surgeHold.<name>, ahead* its ahead.<name>)
  public native let request: Int32;
  public native let hasDestination: Bool;
  public native let hasExternalDestination: Bool;
  public native let landingKeepClearRadius: Float;
  public native let overrideTimer: Float;
  public native let replanTimer: Float;
  public native let logTimer: Float;
  public native let stuckTimer: Float;
  public native let positionJumpTimer: Float;
  public native let settleTimer: Float;
  public native let landing: Bool;
  public native let landingPad: Int32;
  public native let touchdownTimer: Float;
  public native let landingStartZ: Float;
  public native let touchdownLow: Float;
  public native let spotCheckTimer: Float;
  public native let requestRace: Bool;
  public native let requestRejoin: Bool;
  public native let routeLine: Bool;
  public native let onLine: Bool;
  public native let freeRoam: Bool;
  public native let freeRoamChosen: Bool;
  public native let requestJoin: Bool;
  public native let joinS: Float;
  public native let gateEndS: Float;
  public native let spliceGate: Bool;
  public native let clock: Float;
  public native let lastCollisionTimer: Float;
  public native let recovering: Float;
  public native let landSlewOn: Bool;
  public native let landSlewP: Float;
  public native let landSlewDescent: Float;
  public native let landSlewStartYaw: Float;
  public native let landSlewDelta: Float;
  public native let droneEaseLive: Bool;
  public native let attitudeError: Float;
  public native let commandSurge: Float;
  public native let commandSway: Float;
  public native let surgeMax: Float;
  public native let swayMax: Float;
  public native let telemetry: Bool;
  public native let telemetryTime: Float;
  public native let segment: Int32;
  public native let closestS: Float;
  public native let offRouteNow: Float;
  public native let remaining: Float;
  public native let aheadSmooth: Float;
  public native let curvatureAhead: Float;
  public native let blockedTimer: Float;
  public native let blockedAhead: Bool;
  public native let emergencyBrake: Bool;
  public native let targetSpeed: Float;
  public native let speedCmd: Float;
  public native let speedRate: Float;
  public native let surgeHeld: Bool;
  public native let surgeChanging: Float;
  public native let surgeWeight: Float;
  public native let droneYaw: Float;
  public native let droneYawRate: Float;
  public native let dronePitch: Float;
  public native let dronePitchRate: Float;
  public native let droneRoll: Float;
  public native let droneRollRate: Float;
  public native let bankAngle: Float;
  public native let bankRateNow: Float;
  public native let surgeDragFeedForward: Bool;
  public native let surgeFullSurge: Bool;
  public native let surgeSlack: Float;
  public native let surgeDwell: Float;
  public native let surgeRate: Float;
  public native let aheadEnabled: Bool;
  public native let aheadHorizon: Float;
  public native let aheadMinTrip: Float;
  public native let aheadMargin: Float;
  public native let aheadFirstLeg: Bool;
  public native let aheadWaiting: Bool;
  public native let aheadTrip: Int32;

  private let cinematicCallback: ref<CallbackHandle>;
  private let cinematicContext: Bool;         // UIGameContext.CinematicCamera pushed

  // The route: its points (0 without one), a point and its distance along the route, its length.
  public native func RouteSize() -> Int32;
  public native func RoutePoint(index: Int32) -> Vector4;
  public native func RouteDistance(index: Int32) -> Float;
  public native func RouteLength() -> Float;
  // The route has a trajectory, and its planned speed (m/s) at route distance s.
  public native func HasTrajectory() -> Bool;
  public native func PlannedSpeed(s: Float) -> Float;
  // A tap of the autodrive key while engaged (FlightController): free roam to the tracked destination, or back.
  public native func ToggleFreeRoam() -> Void;
  // The route's trajectory built again for the tuning as it is now (for test helpers, after changing it).
  public native func BuildTrajectory() -> Void;
  // The car's limits filled again, and their model (FlightVehicleModel), as the Autopilot plans with them.
  public native func RefreshVehicle() -> Void;
  public native func Vehicle() -> ref<FlightVehicleModel>;
  // The horizontal unit vector of v (zero if v is vertical). Vector4.Normalize2D keeps v's Z: a dot product of two of
  // those adds the Zs' product.
  public static native func Flat(v: Vector4) -> Vector4;
  // A flyby for the cinematic camera beside the route ahead (vehicleCinematicCamera.reds), timed by the route's
  // trajectory, for a shot offset `offset` (shot space) on `side` (+1 right of the travel, -1 left) if it is clear;
  // along the straight line from `position` at `velocity` without a route.
  public native func CinematicFlyby(position: Vector4, velocity: Vector4, offset: Vector4, side: Int32) -> ref<FlightCinematicFlyby>;
  // FindGroundSpot's search: the spot with W 1, all 0 when there is none.
  public native func GroundSpot(target: Vector4, keepClear: Vector4, keepRadius: Float) -> Vector4;

  // A ground spot near target with room to land, kept keepRadius (2D) from keepClear, for a caller deciding before
  // the flight whether there is room to land at all (FlightSummonSystem: next to V). The mode's own keep-clear is not
  // touched.
  public func FindGroundSpot(target: Vector4, keepClear: Vector4, keepRadius: Float, out spot: Vector4) -> Bool {
    let found = this.GroundSpot(target, keepClear, keepRadius);
    if found.W < 0.5 {
      return false;
    }
    spot = found;
    return true;
  }

  // The player is flying and the Autopilot mode is the one flying the car.
  public static func IsEngaged() -> Bool {
    let ctlr = FlightController.GetInstance();
    let pc = FlightSystem.GetInstance().playerComponent;
    return IsDefined(ctlr) && ctlr.IsActive() && IsDefined(pc) && IsDefined(pc.GetFlightMode() as FlightModeAutopilot);
  }

  // Engaged and flying free roam (no destination tracked).
  public static func IsFreeRoaming() -> Bool {
    if !FlightModeAutopilot.IsEngaged() {
      return false;
    }
    let mode = FlightSystem.GetInstance().playerComponent.GetFlightMode() as FlightModeAutopilot;
    return mode.freeRoam;
  }

  // Vanilla autodrive's toast (AutoDriveSystem.SendNotification): same style, its own strings.
  public static func Notify(game: GameInstance, locKey: String) -> Void {
    let msg: SimpleScreenMessage;
    msg.isShown = true;
    msg.duration = 1.5;
    msg.message = locKey;
    msg.type = SimpleMessageType.Autodrive;
    GameInstance.GetBlackboardSystem(game).Get(GetAllBlackboardDefs().UI_Notifications).SetVariant(GetAllBlackboardDefs().UI_Notifications.WarningMessage, ToVariant(msg), true);
  }

  // The GPS destination vanilla autodrive would use, or FlightNavMap's target override.
  public static func GetDestination(game: GameInstance, out target: Vector4) -> Bool {
    if FlightNavMap.HasTargetOverride() {
      target = FlightNavMap.GetTargetOverride();
      return true;
    }
    // a landing pad's pin tracked on the map: its deck (vanilla's destination is snapped to a road)
    let helipads = FlightHelipadSystem.GetInstance(game);
    let pad = IsDefined(helipads) ? helipads.GetTrackedPad() : -1;
    if pad >= 0 {
      target = FlightNavMap.GetHelipadPosition(pad);
      return true;
    }
    let autodrive = GameInstance.GetScriptableSystemsContainer(game).Get(n"AutoDriveSystem") as AutoDriveSystem;
    if IsDefined(autodrive) && NotEquals(autodrive.GetAutodriveDestinationType(), gameAutodriveDestinationType.None) {
      target = Vector4.Vector3To4(autodrive.GetAutodriveDestination());
      return true;
    }
    return false;
  }

  // --- glue the C++ calls by name (FlightModeAutopilot.cpp) ---

  // The destination the game tracks (GetDestination), read each frame unless a summon set the flight's own goal: W 1
  // when there is one, all 0 when not (a return value, not an out parameter, for the C++ caller).
  protected func TrackedDestination() -> Vector4 {
    let target: Vector4;
    if FlightModeAutopilot.GetDestination(this.component.GetVehicle().GetGame(), target) {
      return new Vector4(target.X, target.Y, target.Z, 1.0);
    }
    return new Vector4(0.0, 0.0, 0.0, 0.0);
  }

  // Engaged or disengaged with the player in the car: as vanilla autodrive, no combat events while it drives
  // (AutoDriveSystem.OnAutodriveToggled), and vanilla's autodrive panel. No saving either, as under vanilla
  // autodrive: the two facts are saved with the game, and a save made mid-flight would load with them set and
  // nothing flying. The lock has its own name, so vanilla's autodrive ending does not lift it.
  protected func AutodriveEngaged(engaged: Bool) -> Void {
    let game = this.component.GetVehicle().GetGame();
    SetFactValue(game, n"disable_combat_events", engaged ? 1 : 0);
    SetFactValue(game, n"player_in_autodrive", engaged ? 1 : 0);
    if engaged {
      SaveLocksManager.RequestSaveLockAdd(game, n"LTBF_Autopilot");
    } else {
      SaveLocksManager.RequestSaveLockRemove(game, n"LTBF_Autopilot");
    }
    this.PublishAutodriveUI(engaged);
  }

  // Drive vanilla's autodrive HUD panel (autodrive.script reads UI_AutodriveData): shown while
  // engaged, with the route's remaining length and time. Native ground autodrive stays off.
  private func PublishAutodriveUI(enabled: Bool) -> Void {
    let game = this.component.GetVehicle().GetGame();
    let bb = GameInstance.GetBlackboardSystem(game).Get(GetAllBlackboardDefs().UI_AutodriveData);
    bb.SetBool(GetAllBlackboardDefs().UI_AutodriveData.FreeRoamEnabled, false);
    bb.SetBool(GetAllBlackboardDefs().UI_AutodriveData.AutoDriveEnabled, enabled, true);
    if enabled {
      if !IsDefined(this.cinematicCallback) {
        this.cinematicCallback = bb.RegisterListenerBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive, this, n"OnCinematicCameraChanged");
      }
      this.OnCinematicCameraChanged(bb.GetBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive));
      return;
    }
    if IsDefined(this.cinematicCallback) {
      bb.UnregisterListenerBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive, this.cinematicCallback);
      this.cinematicCallback = null;
      FlightAudio.Get().SetCinematic(false);
    }
    this.OnCinematicCameraChanged(false);
    if bb.GetBool(GetAllBlackboardDefs().UI_AutodriveData.CinematicCameraActive) {
      // leave the cinematic camera with the autopilot, as vanilla does when autodrive ends
      GetPlayer(game).QueueEvent(new vehicleCinematicCameraToggleEvent());
    }
  }

  // Vanilla autodrive's cinematic camera hides the whole HUD (minimap, quest, hints, the panel):
  // the engine pushes UIGameContext.CinematicCamera for its own autodrive only, so with the
  // Autopilot flying the HUD stayed up. Pushed here while the cinematic camera is on.
  protected cb func OnCinematicCameraChanged(active: Bool) -> Bool {
    if Equals(active, this.cinematicContext) {
      return true;
    }
    this.cinematicContext = active;
    FlightAudio.Get().SetCinematic(active);
    let ui = GameInstance.GetUISystem(this.component.GetVehicle().GetGame());
    if active {
      ui.PushGameContext(UIGameContext.CinematicCamera);
    } else {
      ui.PopGameContext(UIGameContext.CinematicCamera);
    }
    return true;
  }

  // Vanilla's panel in its free roam state (no counters); the player is in the car.
  protected func PublishFreeRoamUI(enabled: Bool) -> Void {
    let bb = GameInstance.GetBlackboardSystem(this.component.GetVehicle().GetGame()).Get(GetAllBlackboardDefs().UI_AutodriveData);
    bb.SetBool(GetAllBlackboardDefs().UI_AutodriveData.FreeRoamEnabled, enabled, true);
  }

  // The panel's remaining length (m) and time (s) on a flight to a destination with the player in the car.
  protected func PublishProgress(length: Float, time: Float) -> Void {
    let bb = GameInstance.GetBlackboardSystem(this.component.GetVehicle().GetGame()).Get(GetAllBlackboardDefs().UI_AutodriveData);
    bb.SetFloat(GetAllBlackboardDefs().UI_AutodriveData.DestinationRemainingLength, length);
    bb.SetFloat(GetAllBlackboardDefs().UI_AutodriveData.DestinationRemainingTime, time);
  }

  // The vanilla autodrive toast with this localization key.
  protected func ShowNotification(locKey: String) -> Void {
    FlightModeAutopilot.Notify(this.component.GetVehicle().GetGame(), locKey);
  }

  // Settled and handed back, or the destination untracked on the way: back to the mode it was engaged from.
  // The controller is the player's: a car flying without V (a summon) leaves it alone, or it would take the
  // player out of the Autopilot in the car they are in.
  protected func ExitAutopilot() -> Void {
    if this.component.isPlayerMounted {
      FlightController.GetInstance().ExitAutopilot();
    }
  }

  // Landed with the player in the car: the flight state machine turns flight off as if the player had.
  protected func RequestFlightOff() -> Void {
    FlightController.GetInstance().autopilotRequestsFlightOff = true;
  }
}
