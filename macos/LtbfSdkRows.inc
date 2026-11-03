        // Let There Be Flight (R41). Mac RVAs for the address hashes this plugin uses, from macos/ADDRESS_SURVEY.md (R40) and macos/ADDRESS_LEFTOVERS.md: every
        // row there marked MAPPED, plus two rows Codeware already has in its own resolver (cited). Format: { hash, RVA },  // name.
        // Rows the survey left INLINED or UNSURE are not here: they resolve to the fail-safe trap, and the code that needs them is
        // skipped with a log line (macos/NOT_YET.md, the LTBF_MAC_TODO_R41b_* placeholders).
        // Hooks (27 in LTBF; GetFxForMaterial 2081691750 is INLINED on Mac and has no row).
        { 2879787320, 0x2B3572C },  // CarSuspension_AnimationUpdate. Car physics vslot 0x48 caller, 6th call. OK
        { 501486464,  0x2B073E4 },  // UpdateVehicleCameraInput. Caller 0x12B163C. STRONG
        { 3303544265, 0x2BAA7CC },  // vehiclePhysicsData_ApplyTorqueAtPosition. Also inlined in 0x13593AC (see NOT_YET). OK
        { 898639042,  0x11A7E28 },  // ProcessScriptTypes = Codeware MacAddressResolver.cpp ScriptValidator_Validate (Codeware hooks it: chained)
        { 3191280029, 0x2B29100 },  // BikeSuspension_AnimationUpdate. Bike physics vslot 0x48 caller, 6th call. OK
        { 2531201123, 0x1E7D44C },  // FPPCameraUpdate (FPPCameraComponent::Rotate). x0, s0..s4, w1. OK
        { 2932936864, 0x2B8D1BC },  // WheelEffectUpdate. STRONG
        { 3590925758, 0x2B92E30 },  // TireTrackEffectStart. 8 arguments x0..x7. OK
        { 4067628720, 0x2BAADB4 },  // PhysicsUnkStructVelocityUpdate (RigidBody::ForceSetLinearVelocity). void on Mac. OK
        { 593628708,  0x2baaf34 },  // PhysicsStructUpdate (RigidBody::Recalculate). void on Mac; also called directly. OK
        { 611586815,  0x2baa740 },  // ApplyForceAtPosition. Torque part inlined inside it on Mac. REVIEWED (Windows leaf)
        { 1989218322, 0x2B65468 },  // RollFactorTorqueThing (DrivingForcesHelper::Apply). OK
        { 3281786499, 0x2be1d10 },  // VehicleHelperUpdate. WheeledPhysics vslot 0x60. OK
        { 283779224,  0x2A5D238 },  // GetLocationFromOffset. Result through x8 on Mac (Apple prototype in the hook). OK
        { 2719293980, 0x2A60BA4 },  // UpdatePitch. STRONG
        { 3490519617, 0x19AA7D8 },   // Entity_InitializeComponents = Codeware MacAddressResolver.cpp Entity_Initialize (Codeware hooks it: chained)
        { 2526549425, 0x2be2ff4 },  // ProcessAirResistance. ApplyAirResistance is inlined into it. REVIEWED
        { 2150896908, 0x2B5D484 },  // VehicleDetachPart. OK
        { 2268865059, 0x2BE9170 },  // FourWheelTorque (ApplyAntiSwaybarForce). OK
        { 1554324956, 0x2AE37FC },  // UpdateTireParameters. STRONG
        { 2693082452, 0x2B92418 },  // SkidMarkEffectStart. 8 arguments x0..x7. OK
        { 2463305925, 0x1AEBC3C },  // GetGameSystemsData. void on Mac. OK
        { 4125170300, 0x2A614BC },  // TPPCameraStats_Update (TPPCameraComponent::SetVehicleData). void on Mac. OK
        { 1414536155, 0x2B33030 },  // VehicleUpdateOrientationWithPID. Vehicle object vslot 0x2B0. OK
        { 1836326624, 0x2A9B43C },  // AreaSpeedLimiter_Update. STRONG
        { 4124321726, 0x2A5FE58 },  // GetYaw. STRONG
        // Direct calls the survey MAPPED.
        { 1695226878, 0x2BE80E0 },  // WheeledPhysics::ApplyLowSpeedResistances. OK
        { 1210388152, 0x458697C },  // physics::ProxyHelper::Create. OK
        { 200045,     0x45868D8 },  // physics::ProxyHelper::UpdateProxyCache. REVIEWED (tiny)
        { 3124696465, 0x4585F28 },  // physics::ProxyHelper::SetSimulationShape. OK
        { 811668261,  0x4585ed8 },  // physics::ProxyHelper::SetIsQueryable. OK
        { 3094614336, 0x44EF274 },  // physics::FilterData::LoadPreset. OK
        { 2793083181, 0x2B04B68 },  // vehicle::BaseObject::ForceEnablePhysics. STRONG
        // Direct calls R41b MAPPED (macos/ADDRESS_LEFTOVERS.md section 3).
        { 1189351231, 0x4586b94 },  // physics::ProxyHelper::Unlock, void(ProxyHelper*). OK
        { 1570510966, 0x454A3D0 },  // physics::ProxyID::GetProxy, void*(uint32_t). WEAK (tiny), reviewed
        { 688524786,  0x1959790 },   // ent::SlotComponent::GetSlotIndex, int32_t(SlotComponent*, CName), -1 if missing. WEAK (wrapper), reviewed
        { 61607255,   0x1D4E758 },  // action::ActionInterface::EndActions, void(ActionInterface*), this = BaseObject + 0x3B8. OK
