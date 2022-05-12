
public class hudFLTController extends inkHUDGameController {

  private let m_SpeedValue: inkTextRef;

  private const let m_RPMChunks: array<inkImageRef>;

  private let m_psmBlackboard: wref<IBlackboard>;

  private let m_PSM_BBID: ref<CallbackHandle>;

  private let m_currentZoom: Float;

  private let currentTime: GameTime;

  private let m_activeVehicleUIBlackboard: wref<IBlackboard>;

  private let m_vehicleBBStateConectionId: ref<CallbackHandle>;

  private let m_speedBBConnectionId: ref<CallbackHandle>;

  private let m_gearBBConnectionId: ref<CallbackHandle>;

  private let m_tppBBConnectionId: ref<CallbackHandle>;

  private let m_rpmValueBBConnectionId: ref<CallbackHandle>;

  private let m_leanAngleBBConnectionId: ref<CallbackHandle>;

  private let m_playerStateBBConnectionId: ref<CallbackHandle>;

  private let m_activeChunks: Int32;

  private let m_activeVehicle: wref<VehicleObject>;

  private let m_driver: Bool;

  protected cb func OnInitialize() -> Bool {
    this.PlayLibraryAnimation(n"intro");
    FlightLog.Info("[hudFLTController] HIIII");
  }

  protected cb func OnUninitialize() -> Bool {

  }

  protected cb func OnPlayerAttach(playerPuppet: ref<GameObject>) -> Bool {
    this.m_psmBlackboard = this.GetPSMBlackboard(playerPuppet);
    if IsDefined(this.m_psmBlackboard) {
      this.m_PSM_BBID = this.m_psmBlackboard.RegisterDelayedListenerFloat(GetAllBlackboardDefs().PlayerStateMachine.ZoomLevel, this, n"OnZoomChange");
    };
    this.m_activeVehicle = GetMountedVehicle(this.GetPlayerControlledObject());
    if IsDefined(this.m_activeVehicle) {
      this.GetRootWidget().SetVisible(true);
      this.RegisterToVehicle(true);
      this.Reset();
    };
  }

  protected cb func OnPlayerDetach(playerPuppet: ref<GameObject>) -> Bool {
    this.m_psmBlackboard.UnregisterDelayedListener(GetAllBlackboardDefs().PlayerStateMachine.ZoomLevel, this.m_PSM_BBID);
    if IsDefined(this.m_activeVehicle) {
      this.GetRootWidget().SetVisible(false);
      this.RegisterToVehicle(false);
      this.m_activeVehicle = null;
    };
  }

  protected cb func OnMountingEvent(evt: ref<MountingEvent>) -> Bool {
    this.m_activeVehicle = GetMountedVehicle(this.GetPlayerControlledObject());
    this.m_driver = VehicleComponent.IsDriver(this.m_activeVehicle.GetGame(), this.GetPlayerControlledObject());
    this.GetRootWidget().SetVisible(false);
    this.RegisterToVehicle(true);
    this.Reset();
  }

  protected cb func OnUnmountingEvent(evt: ref<UnmountingEvent>) -> Bool {
    if !evt.request.mountData.mountEventOptions.silentUnmount {
      this.GetRootWidget().SetVisible(false);
      this.RegisterToVehicle(false);
      this.m_activeVehicle = null;
    };
  }

  private final func RegisterToVehicle(register: Bool) -> Void {
    let activeVehicleUIBlackboard: wref<IBlackboard>;
    let vehicleBlackboard: wref<IBlackboard>;
    let vehicle: ref<VehicleObject> = this.m_activeVehicle;
    if vehicle == null {
      return;
    };
    vehicleBlackboard = vehicle.GetBlackboard();
    if IsDefined(vehicleBlackboard) {
      if register {
        this.m_speedBBConnectionId = vehicleBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().Vehicle.SpeedValue, this, n"OnSpeedValueChanged");
        this.m_gearBBConnectionId = vehicleBlackboard.RegisterListenerInt(GetAllBlackboardDefs().Vehicle.GearValue, this, n"OnGearValueChanged");
        this.m_rpmValueBBConnectionId = vehicleBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().Vehicle.RPMValue, this, n"OnRpmValueChanged");
      } else {
        vehicleBlackboard.UnregisterListenerFloat(GetAllBlackboardDefs().Vehicle.SpeedValue, this.m_speedBBConnectionId);
        vehicleBlackboard.UnregisterListenerInt(GetAllBlackboardDefs().Vehicle.GearValue, this.m_gearBBConnectionId);
        vehicleBlackboard.UnregisterListenerFloat(GetAllBlackboardDefs().Vehicle.RPMValue, this.m_rpmValueBBConnectionId);
      };
    };
    activeVehicleUIBlackboard = GameInstance.GetBlackboardSystem(vehicle.GetGame()).Get(GetAllBlackboardDefs().UI_ActiveVehicleData);
    if IsDefined(activeVehicleUIBlackboard) {
      if register {
        this.m_tppBBConnectionId = activeVehicleUIBlackboard.RegisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this, n"OnCameraModeChanged", true);
      } else {
        activeVehicleUIBlackboard.UnregisterListenerBool(GetAllBlackboardDefs().UI_ActiveVehicleData.IsTPPCameraOn, this.m_tppBBConnectionId);
      };
    };
  }

  protected cb func OnZoomChange(evt: Float) -> Bool {
    this.m_currentZoom = evt;
  }

  protected cb func OnRpmMaxChanged(rpmMax: Float) -> Bool {

  }

  protected cb func OnSpeedValueChanged(speedValue: Float) -> Bool {
    speedValue = AbsF(speedValue);
    let multiplier: Float = GameInstance.GetStatsDataSystem(this.m_activeVehicle.GetGame()).GetValueFromCurve(n"vehicle_ui", speedValue, n"speed_to_multiplier");
    inkTextRef.SetText(this.m_SpeedValue, IntToString(RoundMath(speedValue * multiplier)));
  }

  protected cb func OnGearValueChanged(gearValue: Int32) -> Bool {
    if gearValue == 0 {
    };
  }

  protected cb func OnRpmValueChanged(rpmValue: Float) -> Bool {
    this.drawRPMGaugeFull(rpmValue);
  }

  private final func Reset() -> Void {
    this.OnSpeedValueChanged(0.00);
    this.OnRpmValueChanged(0.00);
  }

  public final func drawRPMGaugeFull(rpmValue: Float) -> Void {
    let rpm: Int32 = Cast<Int32>(rpmValue);
    let level: Float = Cast<Float>(rpm / (ArraySize(this.m_RPMChunks) * 20 + 1));
    let levelInt: Int32 = Cast<Int32>(level);
    this.EvaluateRPMMeterWidget(levelInt);
  }

  private final func EvaluateRPMMeterWidget(currentAmountOfChunks: Int32) -> Void {
    if currentAmountOfChunks == this.m_activeChunks {
      return;
    };
    this.m_activeChunks = currentAmountOfChunks;
    this.UpdateChunkVisibility();
  }

  private final func UpdateChunkVisibility() -> Void {
    let visible: Bool;
    let i: Int32 = 0;
    while i <= ArraySize(this.m_RPMChunks) {
      visible = i < this.m_activeChunks;
      inkWidgetRef.SetVisible(this.m_RPMChunks[i], visible);
      i += 1;
    };
  }

  protected cb func OnLeanAngleChanged(leanAngle: Float) -> Bool {

  }

  protected cb func OnCameraModeChanged(mode: Bool) -> Bool {
    if this.m_driver {
      this.GetRootWidget().SetVisible(mode);
    };
  }
}
