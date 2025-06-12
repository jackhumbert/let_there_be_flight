

// public class DriverCombatFirearmsEvents extends DriverCombatEvents {

//   protected let m_attachmentSlotListener: ref<AttachmentSlotsScriptListener>;

//   protected let m_posAnimFeature: ref<AnimFeature_ProceduralDriverCombatData>;

//   protected let m_vehicleRecord: ref<Vehicle_Record>;

//   protected let m_angleDelta: EulerAngles;

//   protected let m_localOrientation: EulerAngles;

//   protected let m_updateItemType: gamedataItemType;

//   protected let m_photoModeActiveListener: ref<CallbackHandle>;

//   protected let m_isPhotoModeActive: Bool;

//   @default(DriverCombatFirearmsEvents, 0.1f)
//   protected const let m_minSwaySpeed: Float;

//   protected let m_prevSpeed: Float;

//   private final func UpdateOrientations(scriptInterface: ref<StateGameScriptInterface>, playerOwner: ref<PlayerPuppet>) -> Void {
//     let blackboard: ref<IBlackboard>;
//     let newForward: Vector4;
//     let newForwardAngles: EulerAngles;
//     let oldForward: Vector4;
//     let oldForwardAngles: EulerAngles;
//     let playerPosition: Vector4;
//     let targetPosition: Vector4;
//     let playerForward: Vector4 = playerOwner.GetWorldForward();
//     let playerForwardAngle: EulerAngles = Vector4.ToRotation(playerForward);
//     if this.m_vehicleInTPP {
//       playerPosition = playerOwner.GetWorldPosition();
//       playerPosition.Z += 0.75;
//       newForward = Vector4.Normalize(targetPosition - playerPosition);
//       oldForward = playerOwner.GetWorldForward();
//     } else {
//       playerPosition = Matrix.GetTranslation(playerOwner.GetFPPCameraComponent().GetLocalToWorld());
//       oldForward = Vector4.Normalize(Matrix.GetDirectionVector(playerOwner.GetFPPCameraComponent().GetLocalToWorld()));
//     };
//     if IsDefined(this.m_targetComponent) {
//       targetPosition = Matrix.GetTranslation(this.m_targetComponent.GetLocalToWorld());
//       newForward = Vector4.Normalize(targetPosition - playerPosition);
//     } else {
//       blackboard = GameInstance.GetBlackboardSystem(playerOwner.GetGame()).GetLocalInstanced(playerOwner.GetEntityID(), GetAllBlackboardDefs().PlayerStateMachine);
//       targetPosition = blackboard.GetVector4(GetAllBlackboardDefs().PlayerStateMachine.TPPAimPosition);
//       newForward = Vector4.Normalize(targetPosition - playerPosition);
//     };
//     newForward = Vector4.Normalize(newForward);
//     oldForward = Vector4.Normalize(oldForward);
//     GameInstance.GetDebugDrawHistorySystem(playerOwner.GetGame()).DrawArrow(playerPosition, newForward, new Color(0u, 0u, 255u, 255u), "TPPCarOrientationNewForward");
//     GameInstance.GetDebugDrawHistorySystem(playerOwner.GetGame()).DrawWireSphere(targetPosition, 0.25, new Color(255u, 0u, 0u, 255u), "TPPCarOrientationTargetPos");
//     GameInstance.GetDebugDrawHistorySystem(playerOwner.GetGame()).DrawArrow(playerPosition, oldForward, new Color(0u, 255u, 0u, 255u), "TPPCarOrientationOldForward");
//     newForwardAngles = Vector4.ToRotation(newForward);
//     oldForwardAngles = Vector4.ToRotation(oldForward);
//     this.m_angleDelta.Yaw = -AngleDistance(oldForwardAngles.Yaw, newForwardAngles.Yaw);
//     this.m_angleDelta.Pitch = AngleDistance(oldForwardAngles.Pitch, newForwardAngles.Pitch);
//     this.m_angleDelta.Roll = AngleDistance(oldForwardAngles.Roll, newForwardAngles.Roll);
//     this.m_localOrientation.Yaw = AngleDistance(newForwardAngles.Yaw, playerForwardAngle.Yaw);
//     this.m_localOrientation.Pitch = AngleDistance(newForwardAngles.Pitch, playerForwardAngle.Pitch);
//   }

//   private final func UpdateAimingDirectionAnimFeature(playerOwner: ref<PlayerPuppet>) -> Void {
//     let animFeatureYawDelta: Float;
//     if !this.m_vehicleInTPP && !IsDefined(this.m_targetComponent) {
//       this.m_posAnimFeature.isEnabled = false;
//       this.m_posAnimFeature.yaw = 0.00;
//       this.m_posAnimFeature.pitch = 0.00;
//       this.m_posAnimFeature.roll = 0.00;
//       return;
//     };
//     this.m_posAnimFeature.isEnabled = true;
//     if AbsF(this.m_angleDelta.Yaw) > 167.00 && SgnF(this.m_angleDelta.Yaw) != SgnF(this.m_posAnimFeature.yaw) {
//       animFeatureYawDelta = 180.00 * SgnF(this.m_posAnimFeature.yaw);
//     } else {
//       animFeatureYawDelta = this.m_angleDelta.Yaw;
//     };
//     animFeatureYawDelta = ClampF(animFeatureYawDelta, -167.00, 167.00);
//     if SgnF(this.m_posAnimFeature.yaw) != SgnF(animFeatureYawDelta) && AbsF(animFeatureYawDelta) > 90.00 && AbsF(this.m_posAnimFeature.yaw) > 90.00 {
//       this.m_posAnimFeature.yawDirectionFlipped = true;
//     } else {
//       this.m_posAnimFeature.yawDirectionFlipped = false;
//     };
//     this.m_posAnimFeature.yaw = animFeatureYawDelta;
//     this.m_posAnimFeature.pitch = this.m_angleDelta.Pitch;
//     this.m_posAnimFeature.roll = this.m_angleDelta.Roll;
//   }

//   private final func UpdateSafeMode(scriptInterface: ref<StateGameScriptInterface>, stateContext: ref<StateContext>, yaw: Float) -> Void {
//     let backBound: wref<WeaponSafeModeBound_Record>;
//     let forceSafeState: StateResultBool;
//     let lookingAtWindshield: Bool;
//     let lookingBehind: Bool;
//     let windshieldBound: wref<WeaponSafeModeBound_Record>;
//     let weaponBounds: wref<WeaponSafeModeBounds_Record> = this.m_vehicleRecord.WeaponSafeModeBounds();
//     if !weaponBounds.EnableSafeModeBounds() {
//       return;
//     };
//     windshieldBound = weaponBounds.WindshieldBound();
//     backBound = weaponBounds.BackBound();
//     lookingAtWindshield = yaw < windshieldBound.YawMax() && yaw > windshieldBound.YawMin();
//     lookingBehind = yaw > backBound.YawMax() || yaw < backBound.YawMin();
//     forceSafeState = stateContext.GetTemporaryBoolParameter(n"ForceWeaponSafeState");
//     if lookingAtWindshield || lookingBehind && !this.m_vehicleInTPP {
//       if !forceSafeState.value {
//         stateContext.SetTemporaryBoolParameter(n"ForceWeaponSafeState", true, true);
//       };
//     } else {
//       if forceSafeState.value {
//         stateContext.SetTemporaryBoolParameter(n"ForceWeaponSafeState", false);
//       };
//     };
//   }

//   protected final func ApplyWeaponFxScalings(itemType: gamedataItemType) -> Void {
//     let statusEffect: TweakDBID;
//     if Equals(itemType, gamedataItemType.Wea_Handgun) {
//       statusEffect = this.m_driverCombatInTPP ? t"BaseStatusEffect.DriverCombatHandgunVFXScale" : t"BaseStatusEffect.DriverCombatHandgunFPPVFXScale";
//     } else {
//       if Equals(itemType, gamedataItemType.Wea_Revolver) {
//         if this.m_driverCombatInTPP {
//           statusEffect = t"BaseStatusEffect.DriverCombatRevolverVFXScale";
//         } else {
//           statusEffect = t"BaseStatusEffect.DriverCombatRevolverFPPVFXScale";
//         };
//       } else {
//         if Equals(itemType, gamedataItemType.Wea_SubmachineGun) {
//           if this.m_driverCombatInTPP {
//             statusEffect = t"BaseStatusEffect.DriverCombatSMGVFXScale";
//           } else {
//             statusEffect = t"BaseStatusEffect.DriverCombatSMGFPPVFXScale";
//           };
//         };
//       };
//     };
//     StatusEffectHelper.RemoveStatusEffectsWithTag(this.m_executionOwner, n"DriverCombatWeaponVFXScaling");
//     if TDBID.IsValid(statusEffect) {
//       StatusEffectHelper.ApplyStatusEffect(this.m_executionOwner, statusEffect);
//     };
//   }

//   protected func OnPerspectiveUpdate(scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     this.ApplyWeaponFxScalings(WeaponObject.GetWeaponType(scriptInterface.GetTransactionSystem().GetItemInSlot(scriptInterface.executionOwner, t"AttachmentSlots.WeaponRight").GetItemID()));
//     this.UpdateWeaponSwayRemoval(this.m_vehicleInTPP);
//     this.UpdateWeaponSwayPause(!this.m_vehicleInTPP && AbsF((this.m_owner as VehicleObject).GetCurrentSpeed()) > this.m_minSwaySpeed);
//     if this.m_vehicleInTPP {
//       StatusEffectHelper.RemoveStatusEffect(scriptInterface.executionOwner, t"BaseStatusEffect.PhotoModeForceFPPCamera");
//     } else {
//       StatusEffectHelper.ApplyStatusEffect(scriptInterface.executionOwner, t"BaseStatusEffect.PhotoModeForceFPPCamera");
//     };
//   }

//   private func OnItemEquipped(slot: TweakDBID, item: ItemID) -> Void {
//     if slot == t"AttachmentSlots.WeaponRight" {
//       this.m_updateItemType = WeaponObject.GetWeaponType(item);
//     };
//   }

//   private final func UpdateItemEquipped(scriptInterface: ref<StateGameScriptInterface>, itemType: gamedataItemType) -> Void {
//     this.UpdateWeaponData(scriptInterface, itemType);
//     this.ApplyWeaponFxScalings(itemType);
//     this.EnableSmartGunHandler(false);
//     this.UpdatePistolADSSpread(Equals(itemType, gamedataItemType.Wea_Handgun) || Equals(itemType, gamedataItemType.Wea_Revolver));
//   }

//   private final func EnableSmartGunHandler(enable: Bool) -> Void {
//     let evt: ref<EnableSmartGunHandlerEvent>;
//     let transactionSystem: ref<TransactionSystem> = GameInstance.GetTransactionSystem(this.m_executionOwner.GetGame());
//     let weapon: ref<WeaponObject> = transactionSystem.GetItemInSlot(this.m_executionOwner, t"AttachmentSlots.WeaponRight") as WeaponObject;
//     let weaponRecData: ref<WeaponItem_Record> = weapon.GetWeaponRecord();
//     if Equals(weaponRecData.Evolution().Type(), gamedataWeaponEvolution.Smart) {
//       evt = new EnableSmartGunHandlerEvent();
//       evt.owner = this.m_executionOwner;
//       evt.enable = enable;
//       weapon.QueueEvent(evt);
//     };
//   }

//   protected func OnAimChange() -> Void {
//     super.OnAimChange();
//     this.EnableSmartGunHandler(this.m_aimPressed);
//   }

//   private final func RollDownWindowsForCombat(scriptInterface: ref<StateGameScriptInterface>, value: Bool) -> Void {
//     this.ToggleWindowForOccupiedSeat(scriptInterface, n"seat_front_right", value);
//     this.ToggleWindowForOccupiedSeat(scriptInterface, n"seat_front_left", value);
//   }

//   private final func UpdatePistolADSSpread(applyEffect: Bool) -> Void {
//     if applyEffect {
//       StatusEffectHelper.ApplyStatusEffect(this.m_executionOwner, t"BaseStatusEffect.DriverCombatPistol");
//     } else {
//       StatusEffectHelper.RemoveStatusEffect(this.m_executionOwner, t"BaseStatusEffect.DriverCombatPistol");
//     };
//   }

//   private final func UpdateWeaponSwayRemoval(applyEffect: Bool) -> Void {
//     if applyEffect {
//       StatusEffectHelper.ApplyStatusEffect(this.m_executionOwner, t"BaseStatusEffect.DriverCombatSwayRemoval");
//     } else {
//       StatusEffectHelper.RemoveStatusEffect(this.m_executionOwner, t"BaseStatusEffect.DriverCombatSwayRemoval");
//     };
//   }

//   private final func UpdateWeaponSwayPause(applyEffect: Bool) -> Void {
//     if applyEffect {
//       StatusEffectHelper.ApplyStatusEffect(this.m_executionOwner, t"BaseStatusEffect.DriverCombatSwayPause");
//     } else {
//       StatusEffectHelper.RemoveStatusEffect(this.m_executionOwner, t"BaseStatusEffect.DriverCombatSwayPause");
//     };
//   }

//   protected cb func OnPhotomodeUpdate(isInPhotoMode: Bool) -> Bool {
//     this.m_isPhotoModeActive = isInPhotoMode;
//   }

//   protected final func OnAttach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     this.m_photoModeActiveListener = GameInstance.GetBlackboardSystem(scriptInterface.executionOwner.GetGame()).Get(GetAllBlackboardDefs().PhotoMode).RegisterListenerBool(GetAllBlackboardDefs().PhotoMode.IsActive, this, n"OnPhotomodeUpdate");
//     this.m_isPhotoModeActive = false;
//   }

//   protected final func OnDetach(const stateContext: ref<StateContext>, const scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     GameInstance.GetBlackboardSystem(scriptInterface.executionOwner.GetGame()).Get(GetAllBlackboardDefs().PhotoMode).UnregisterListenerBool(GetAllBlackboardDefs().PhotoMode.IsActive, this.m_photoModeActiveListener);
//     this.m_photoModeActiveListener = null;
//   }

//   protected func OnEnter(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     let attachmentSlotCallback: ref<DefaultTransitionAttachmentSlotsCallback>;
//     let drawItemRequest: ref<DrawItemRequest>;
//     super.OnEnter(stateContext, scriptInterface);
//     this.m_prevSpeed = (this.m_owner as VehicleObject).GetCurrentSpeed();
//     this.m_posAnimFeature = new AnimFeature_ProceduralDriverCombatData();
//     attachmentSlotCallback = new DefaultTransitionAttachmentSlotsCallback();
//     attachmentSlotCallback.m_transitionOwner = this;
//     attachmentSlotCallback.slotID = t"AttachmentSlots.WeaponRight";
//     this.m_attachmentSlotListener = scriptInterface.GetTransactionSystem().RegisterAttachmentSlotListener(scriptInterface.executionOwner, attachmentSlotCallback);
//     this.RollDownWindowsForCombat(scriptInterface, true);
//     this.m_vehicleRecord = TweakDBInterface.GetVehicleRecord((scriptInterface.owner as VehicleObject).GetRecordID());
//     if !UpperBodyTransition.HasAnyWeaponEquipped(scriptInterface) {
//       drawItemRequest = new DrawItemRequest();
//       drawItemRequest.owner = scriptInterface.executionOwner;
//       drawItemRequest.itemID = ItemID.CreateQuery(t"Items.Preset_V_Unity_Cutscene");
//       (scriptInterface.GetScriptableSystem(n"EquipmentSystem") as EquipmentSystem).QueueRequest(drawItemRequest);
//     } else {
//       this.OnItemEquipped(t"AttachmentSlots.WeaponRight", scriptInterface.GetTransactionSystem().GetItemInSlot(scriptInterface.executionOwner, t"AttachmentSlots.WeaponRight").GetItemID());
//     };
//     SetFactValue(scriptInterface.executionOwner.GetGame(), n"player_tried_veh_combat_firearms", 1);
//     if !this.m_vehicleInTPP {
//       StatusEffectHelper.ApplyStatusEffect(scriptInterface.executionOwner, t"BaseStatusEffect.PhotoModeForceFPPCamera");
//     };
//   }

//   protected func OnUpdate(timeDelta: Float, stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     let ammoCount: Int32;
//     let ammoDiff: Int32;
//     let ammoType: ItemID;
//     let driverCombatForbiddenZone: StateResultBool;
//     let magazineCapacity: Uint32;
//     let questForceEnableCombat: StateResultBool;
//     let vehicleSpeed: Float;
//     let weaponObject: ref<WeaponObject>;
//     super.OnUpdate(timeDelta, stateContext, scriptInterface);
//     if !this.m_isPhotoModeActive {
//       this.UpdateOrientations(scriptInterface, this.m_executionOwner as PlayerPuppet);
//       this.UpdateAimingDirectionAnimFeature(this.m_executionOwner as PlayerPuppet);
//       this.UpdateSafeMode(scriptInterface, stateContext, this.m_localOrientation.Yaw);
//       stateContext.SetPermanentFloatParameter(n"TPPVehiclePlayerYaw", this.m_posAnimFeature.yaw, true);
//     };
//     if NotEquals(this.m_updateItemType, gamedataItemType.Invalid) {
//       this.UpdateItemEquipped(scriptInterface, this.m_updateItemType);
//       this.m_updateItemType = gamedataItemType.Invalid;
//       weaponObject = DefaultTransition.GetActiveWeapon(scriptInterface);
//       if IsDefined(weaponObject) {
//         ammoType = WeaponObject.GetAmmoType(weaponObject);
//         ammoCount = scriptInterface.GetTransactionSystem().GetItemQuantity(scriptInterface.executionOwner, ammoType);
//         magazineCapacity = WeaponObject.GetMagazineCapacity(weaponObject);
//         ammoDiff = Cast<Int32>(magazineCapacity * 2u) - ammoCount;
//         if ammoDiff > 0 {
//           scriptInterface.GetTransactionSystem().GiveItem(scriptInterface.executionOwner, ammoType, ammoDiff);
//         };
//       };
//     };
//     questForceEnableCombat = stateContext.GetTemporaryBoolParameter(n"stopVehicleCombat");
//     driverCombatForbiddenZone = stateContext.GetPermanentBoolParameter(n"driverCombatForbiddenZone");
//     if questForceEnableCombat.valid && questForceEnableCombat.value || driverCombatForbiddenZone.valid && driverCombatForbiddenZone.value {
//       this.SendEquipmentSystemWeaponManipulationRequest(scriptInterface, EquipmentManipulationAction.UnequipAll);
//     };
//     vehicleSpeed = (this.m_owner as VehicleObject).GetCurrentSpeed();
//     if !this.m_vehicleInTPP && NotEquals(AbsF(vehicleSpeed) > this.m_minSwaySpeed, AbsF(this.m_prevSpeed) > this.m_minSwaySpeed) {
//       this.UpdateWeaponSwayPause(AbsF(vehicleSpeed) > this.m_minSwaySpeed);
//     };
//     this.m_prevSpeed = vehicleSpeed;
//     if !this.m_isPhotoModeActive {
//       scriptInterface.SetAnimationParameterFeature(n"ProceduralDriverCombatData", this.m_posAnimFeature, scriptInterface.executionOwner);
//     };
//   }

//   protected func OnForcedExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     super.OnForcedExit(stateContext, scriptInterface);
//     StatusEffectHelper.RemoveStatusEffect(scriptInterface.executionOwner, t"BaseStatusEffect.PhotoModeForceFPPCamera");
//   }

//   protected func OnExit(stateContext: ref<StateContext>, scriptInterface: ref<StateGameScriptInterface>) -> Void {
//     super.OnExit(stateContext, scriptInterface);
//     scriptInterface.GetTransactionSystem().UnregisterAttachmentSlotListener(scriptInterface.executionOwner, this.m_attachmentSlotListener);
//     this.UpdateWeaponSwayRemoval(false);
//     this.UpdateWeaponSwayPause(false);
//     this.UpdatePistolADSSpread(false);
//     this.m_posAnimFeature.yaw = 0.00;
//     this.m_posAnimFeature.pitch = 0.00;
//     this.m_posAnimFeature.roll = 0.00;
//     scriptInterface.SetAnimationParameterFeature(n"ProceduralDriverCombatData", this.m_posAnimFeature, scriptInterface.executionOwner);
//     stateContext.SetTemporaryBoolParameter(n"ForceWeaponSafeState", false);
//     this.RollDownWindowsForCombat(scriptInterface, false);
//     this.SendEquipmentSystemWeaponManipulationRequest(scriptInterface, EquipmentManipulationAction.UnequipAll);
//     StatusEffectHelper.RemoveStatusEffect(scriptInterface.executionOwner, t"BaseStatusEffect.PhotoModeForceFPPCamera");
//   }
// }