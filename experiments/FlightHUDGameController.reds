// public class FlightHUDGameController extends inkHUDGameController {

//   private let m_playerObject: wref<GameObject>;

//   private let m_root: wref<inkCompoundWidget>;
//   private let m_vehicle: wref<VehicleObject>;
//   private let m_vehiclePS: ref<VehicleComponentPS>;
//   private let m_vehicleBlackboard: wref<IBlackboard>;
//   private let m_activeVehicleUIBlackboard: wref<IBlackboard>;
  
//   private let m_playerObject: wref<GameObject>;
//   private let m_psmBlackboard: wref<IBlackboard>;
//   private let m_PSM_BBID: ref<CallbackHandle>;
  
//   protected cb func OnInitialize() -> Bool {
//     let ownerObject: ref<GameObject> = this.GetOwnerEntity() as GameObject;
//     let playerPuppet: wref<GameObject> = this.GetOwnerEntity() as PlayerPuppet;
//     let bbSys: ref<BlackboardSystem> = GameInstance.GetBlackboardSystem(playerPuppet.GetGame());
//     this.m_psmBlackboard = this.GetPSMBlackboard(playerPuppet);
//     this.m_playerObject = playerPuppet;
//     let playerControlledObject: ref<GameObject> = this.GetPlayerControlledObject();
//     playerControlledObject.RegisterInputListener(this, n"right_stick_x");
//     playerControlledObject.RegisterInputListener(this, n"right_stick_y");
//     this.m_scanBlackboard = bbSys.Get(GetAllBlackboardDefs().UI_Scanner);
//     this.m_root = this.GetRootWidget() as inkCompoundWidget;
//     this.m_root.SetVisible(false);
//     this.m_vehicle = GetMountedVehicle(playerPuppet);
//     this.m_vehiclePS = this.m_vehicle.GetVehiclePS();
//     this.m_vehicleBlackboard = this.m_vehicle.GetBlackboard();
//     this.m_activeVehicleUIBlackboard = bbSys.Get(GetAllBlackboardDefs().UI_ActiveVehicleData);
//     this.m_bbPlayerStats = this.GetBlackboardSystem().Get(GetAllBlackboardDefs().UI_PlayerBioMonitor);
//     this.m_bbPlayerEventId = this.m_bbPlayerStats.RegisterListenerVariant(GetAllBlackboardDefs().UI_PlayerBioMonitor.PlayerStatsInfo, this, n"OnStatsChanged");
//     if IsDefined(this.m_psmBlackboard) {
//       this.m_PSM_BBID = this.m_psmBlackboard.RegisterDelayedListenerFloat(GetAllBlackboardDefs().PlayerStateMachine.ZoomLevel, this, n"OnZoomChange");
//       this.m_playerStateBBConnectionId = this.m_psmBlackboard.RegisterListenerInt(GetAllBlackboardDefs().PlayerStateMachine.Vehicle, this, n"OnPlayerVehicleStateChange", true);
//     };
//     if IsDefined(this.m_vehicleBlackboard) {
//       this.m_vehicleBBStateConectionId = this.m_vehicleBlackboard.RegisterListenerInt(GetAllBlackboardDefs().Vehicle.VehicleState, this, n"OnVehicleStateChanged");
//       this.m_speedBBConnectionId = this.m_vehicleBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().Vehicle.SpeedValue, this, n"OnSpeedValueChanged");
//       this.m_gearBBConnectionId = this.m_vehicleBlackboard.RegisterListenerInt(GetAllBlackboardDefs().Vehicle.GearValue, this, n"OnGearValueChanged");
//       this.m_rpmValueBBConnectionId = this.m_vehicleBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().Vehicle.RPMValue, this, n"OnRpmValueChanged");
//       this.m_leanAngleBBConnectionId = this.m_vehicleBlackboard.RegisterListenerFloat(GetAllBlackboardDefs().Vehicle.BikeTilt, this, n"OnLeanAngleChanged");
//       this.m_isTargetingFriendlyConnectionId = this.m_vehicleBlackboard.RegisterListenerBool(GetAllBlackboardDefs().Vehicle.IsTargetingFriendly, this, n"OnIsTargetingFriendly");
//     };
//     this.currentTime = GameInstance.GetTimeSystem(ownerObject.GetGame()).GetGameTime();
//     inkTextRef.SetText(this.m_Timer, ToString(GameTime.Hours(this.currentTime)) + ":" + ToString(GameTime.Minutes(this.currentTime)) + ":" + ToString(GameTime.Seconds(this.currentTime)));
//     inkTextRef.SetText(this.m_Date, "05-13-2077");
//     this.SpawnTargetIndicators();
//     this.m_weaponBlackboard = bbSys.Get(GetAllBlackboardDefs().UI_ActiveWeaponData);
//     if IsDefined(this.m_weaponBlackboard) {
//       this.m_weaponParamsListenerId = this.m_weaponBlackboard.RegisterDelayedListenerVariant(GetAllBlackboardDefs().UI_ActiveWeaponData.SmartGunParams, this, n"OnSmartGunParams");
//     };
// }