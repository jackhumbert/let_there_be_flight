-- registerForEvent('onInit', function()

--     TweakDB:SetFlat('Vehicle.Peugeot.enumComment', '')
--     TweakDB:SetFlat('Vehicle.Peugeot.enumName', 'Peugeot')
    
--     TweakDB:SetFlat('Vehicle.Spinner.enumComment','') 
--     TweakDB:SetFlat('Vehicle.Spinner.enumName','Spinner') 

--     TweakDB:SetFlat('UIIcon.Peugeot.atlasPartName','peugeot')
--     TweakDB:SetFlat('UIIcon.Peugeot.atlasResourcePath',RaRefCResource.new(11880208925820980408)) 
    
--     TweakDB:SetFlat('UIIcon.peugeot_spinner__basic.atlasPartName','peugeot_spinner__basic')
--     TweakDB:SetFlat('UIIcon.peugeot_spinner__basic.atlasResourcePath',RaRefCResource.new(16104769032458085763))

--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.affiliation', TweakDBID.new("Factions.Unaffiliated"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.appearanceName',"None")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.archetypeName', "vehicle")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.attachmentSlots', ArrayTweakDBID.new("AttachmentSlots.Engine1", "AttachmentSlots.Engine2", "AttachmentSlots.Engine3", "AttachmentSlots.Engine4"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.audioResourceName','')
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.brakelightColor', ArrayInt32.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.cameraManagerParams', TweakDBID.new('Camera.VehicleCameraManager_Default'))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.crackLockDifficulty', "HARD")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.crowdMemberSettings', TweakDBID.new("Crowds.Sport_DrivingPackage"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.curvesPath',RaRefCResource.new(14783903123043989255))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.destroyedAppearance',"quadra_type66__basic_burnt_01")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.destruction', TweakDBID.new("Vehicle.VehicleDestructionParamsQuadraType66"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.displayName,gamedataLocKeyWrapper,1618 ')
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.driving', TweakDBID.new("Driving.Default_4w"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.effectors', ArrayTweakDBID.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.enableDestruction',true)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.entityTemplatePath', RaRefCResource.new(13101370074149372271))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.fxCollision', TweakDBID.new("Vehicle.FxCollision_Default"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.fxWheelsDecals', TweakDBID.new("Vehicle.FxWheels_Sport_XL"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.fxWheelsParticles', TweakDBID.new("Vehicle.FxWheelsParticles_Default"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.headlightColor', ArrayInt32.new(255, 255, 255, 255 ))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.hijackDifficulty', "HARD")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.icon', TweakDBID.new("UIIcon.peugeot_spinner__basic"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.interiorColor',ArrayInt32.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.interiorDamageColor',ArrayInt32.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.leftBackCamber', 3)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.leftBackCamberOffset',Vector3( 0.0199999996, 0, 0 ))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.leftBlinkerlightColor',ArrayInt32.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.leftFrontCamber', 3)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.leftFrontCamberOffset',Vector3( 0.0199999996, 0, 0 ))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.manufacturer', TweakDBID.new("Vehicle.Quadra"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.model', TweakDBID.new("Vehicle.Type66"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.multiplayerTemplatePaths', ArrayRaRefCResource( ))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.objectActions', ArrayTweakDBID.new( "VehicleActions.VehicleHijackFrontLeft", "VehicleActions.VehicleHijackFrontRight", "VehicleActions.VehicleMountFrontLeft", "VehicleActions.VehicleMountFrontRight", "VehicleActions.VehicleMountBackLeft", "VehicleActions.VehicleMountBackRight", "VehicleActions.VehicleCrackLockFrontLeft", "VehicleActions.VehicleCrackLockFrontRight"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.persistentName',"None")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.player_audio_resource', "v_car_quadra_type_66")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.priority', TweakDBID.new("SpawnableObjectPriority.Regular"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.queryOnlyExceptions', ArrayCName.new("trunk_a", "trunk_b", "hood_a"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.randomPassengers', ArrayTweakDBID.new( "Passengers.NightLifeDriverEntry", "Passengers.NightLifePassengerFrontEntry"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.reverselightColor',ArrayInt32.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.rightBLinkerlightColor',ArrayInt32.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.rightBackCamber',-3)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.rightBackCamberOffset',Vector3( -0.0199999996, 0, 0 ))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.rightFrontCamber',-3)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.rightFrontCamberOffset',Vector3( -0.0199999996, 0, 0 ))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.savable',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.statModifierGroups', ArrayTweakDBID.new( "VehicleStatPreset.BaseCar"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.statModifiers', ArrayTweakDBID.new( "Vehicle.v_sport2_peugeot_spinner_inline1"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.statPools', ArrayTweakDBID.new( "BaseStatPools.VehicleHealth"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.tags', ArrayCName.new("InteractiveTrunk", "Sport"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.tppCameraParams', TweakDBID.new("Camera.VehicleTPP_DefaultParams"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.tppCameraPresets', ArrayTweakDBID.new( "Camera.VehicleTPP_4w_Quadra66_Low_Close", "Camera.VehicleTPP_4w_Quadra66_High_Close", "Camera.VehicleTPP_4w_Quadra66_Low_Far", "Camera.VehicleTPP_4w_Quadra66_High_Far"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.traffic_audio_resource', "v_car_quadra_type_66_traffic")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.type', TweakDBID.new("Vehicle.Car"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.unmountOffsetPosition',Vector3( 1.64999998, 5, 2.5 ))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehAirControl', TweakDBID.new("Vehicle.VehicleAirControlCar"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehAirControlAI', TweakDBID.new("Vehicle.VehicleAirControlCarAI"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehBehaviorData', TweakDBID.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehDataPackage', TweakDBID.new("Vehicle.v_sport2_peugeot_spinner_inline0"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehDefaultState', TweakDBID.new("Vehicle.Veh4WDefaultState"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehDriveModelData', TweakDBID.new("Vehicle.VehicleDriveModelData_Type66"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehDriveModelDataAI', TweakDBID.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehDriverCombat_FPPCameraParams', TweakDBID.new("Vehicle.VehicleDriverCombatFPPCameraParamsDefault"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehDriverCombat_ProceduralFPPCameraParams', TweakDBID.new("Camera.VehicleProceduralFPPCamera_DefaultCombatParams"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehDriver_FPPCameraParams', TweakDBID.new("Vehicle.v_sport2_peugeot_spinner_inline5"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehDriver_ProceduralFPPCameraParams', TweakDBID.new("Camera.VehicleProceduralFPPCamera_DefaultParams"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehEngineData', TweakDBID.new("Vehicle.VehicleEngineData_4_Sport"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehImpactTraffic', TweakDBID.new("Driving.VehicleImpactTraffic_DefaultParams"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehPassCombatL_FPPCameraParams', TweakDBID.new("Vehicle.VehiclePassengerLCombatFPPCameraParamsDefault"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehPassCombatL_ProceduralFPPCameraParams', TweakDBID.new("Camera.VehicleProceduralFPPCamera_DefaultCombatParams"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehPassCombatR_FPPCameraParams', TweakDBID.new("Vehicle.VehiclePassengerRCombatFPPCameraParamsDefault"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehPassCombatR_ProceduralFPPCameraParams', TweakDBID.new("Camera.VehicleProceduralFPPCamera_DefaultCombatParams"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehPassL_FPPCameraParams', TweakDBID.new("Vehicle.VehiclePassengerLFPPCameraParamsDefault"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehPassL_ProceduralFPPCameraParams', TweakDBID.new("Camera.VehicleProceduralFPPCamera_DefaultParams"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehPassR_FPPCameraParams', TweakDBID.new("Vehicle.VehiclePassengerRFPPCameraParamsDefault"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehPassR_ProceduralFPPCameraParams', TweakDBID.new("Camera.VehicleProceduralFPPCamera_DefaultParams"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehWheelDimensionsSetup', TweakDBID.new("Vehicle.v_sport2_peugeot_spinner_inline2"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.vehicleUIData', TweakDBID.new("Vehicle.VehicleQuadraType66UIData"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.visualDestruction', TweakDBID.new("Vehicle.VehicleVisualDestructionParamsDefault"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.visualTags', ArrayCName.new("Standard"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.weakspots', ArrayTweakDBID.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.weapons', ArrayTweakDBID.new())
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner.widgetStyleSheetPath', RaRefCResource.new(0))

--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.animVars', ArrayCName.new("sport2_vehicle_seat_front_left", "sport2_vehicle_seat_front_right", "sport2_vehicle_seat_back_left", "sport2_vehicle_seat_back_right"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.barnDoorsTailgate',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.boneNames', ArrayCName.new("seat_front_left", "seat_front_right", "seat_back_left", "seat_back_right"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.canStoreBody',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.disableSwitchSeats',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.entering',2.4000001)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.exitDelay',0.400000006)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.fppCameraOverride',"None")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.fromCombat',0.800000012)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.hasSiren',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.hasSpoiler',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.hasTurboCharger',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.interactiveHood',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.interactiveTrunk',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.knockOffForce',13)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.normal_open',1.70000005)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.open_close_duration',1.20000005)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.parkingAngle',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.seatingTemplateOverride',"sport2_vehicle")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.slideDuration',0.800000012)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.slidingRearDoors',false)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.speedToClose',2)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.spoilerSpeedToDeploy',20)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.spoilerSpeedToRetract',10)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.stealing',3.70000005)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.stealing_open',3.29999995)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.supportsCombat',true)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.switchSeats',1.54999995)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.toCombat',0.699999988)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline0.vehSeatSet', TweakDBID.new("Vehicle.Vehicle2SeatSetDefault"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline1.modifierType',"Additive")
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline1.statType', TweakDBID.new("BaseStats.PowerLevel"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline1.value',5)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline2.backPreset', TweakDBID.new("Vehicle.v_sport2_peugeot_spinner_inline4"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline2.frontPreset', TweakDBID.new("Vehicle.v_sport2_peugeot_spinner_inline3"))
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline3.rimRadius',0.239999995)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline3.tireRadius',0.400000006)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline3.tireWidth',0.300000012)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline3.wheelOffset',0.0500000007)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline4.rimRadius',0.239999995)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline4.tireRadius',0.409999996)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline4.tireWidth',0.439999998)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline4.wheelOffset',0.0500000007)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.forward_offset_value',0.100000001)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.is_forward_offset',1)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.is_paralax',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.is_pitch_off',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.is_yaw_off',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.lookat_offset_vertical',0.0500000007)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.lookat_pitch_forward_down_ratio',1)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.lookat_pitch_forward_offset',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.lookat_yaw_left_offset',0.150000006)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.lookat_yaw_left_up_offset',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.lookat_yaw_offset_active_angle',30)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.lookat_yaw_right_offset',0.150000006)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.lookat_yaw_right_up_offset',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.paralax_forward_offset',0.100000001)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.paralax_radius',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.upperbody_pitch_weight',0)
--     TweakDB:SetFlat('Vehicle.v_sport2_peugeot_spinner_inline5.upperbody_yaw_weight',0)

-- end)