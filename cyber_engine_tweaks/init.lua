-- local tweakdbids = {
-- 		'Camera.VehicleTPP_2w_DefaultParams.autoCenterStartTimeGamepad',
-- 		'Camera.VehicleTPP_2w_DefaultParams.autoCenterStartTimeMouse',
-- 		'Camera.VehicleTPP_DefaultParams.autoCenterStartTimeGamepad',
-- 		'Camera.VehicleTPP_DefaultParams.autoCenterStartTimeMouse',
-- 		'RTDB.VehicleTPPCameraParams.autoCenterStartTimeGamepad',
-- 		'RTDB.VehicleTPPCameraParams.autoCenterStartTimeMouse',
-- 		'Vehicle.VehicleTPP_Params_v_militech_basilisk.autoCenterStartTimeGamepad',
-- 		'Vehicle.VehicleTPP_Params_v_militech_basilisk.autoCenterStartTimeMouse'
-- }

-- local GameHUD = require('GameHUD')

registerForEvent('onInit', function()

    -- for _, tdbid in pairs(tweakdbids) do
	-- 	TweakDB:SetFlat(TweakDBID.new(tdbid), 0.01000)
	-- 	TweakDB:Update(TweakDBID.new(tdbid))
	-- end

    -- TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.lockedCamera', true) 
    -- TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.lockedCamera', true) 

    TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirDampFactor', 0.1)
    TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirFallCoef', 0)
    TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirPitchMax', 30)
    TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirPitchMin', -30)
    TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirRaiseCoef', 0.0)
    TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirSpeedMax', 10)
    TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirStrength', 4)
    
    TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirDampFactor', 0.1)
    TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirFallCoef', 0)
    TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirPitchMax', 30)
    TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirPitchMin', -30)
    TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirRaiseCoef', 0.0)
    TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirSpeedMax', 10)
    TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirStrength', 4)

    -- why not
    TweakDB:SetFlat('vehicles.common.temp_hack_ignore_trunk_max_height', 200)

    TweakDB:SetFlat('Vehicle.VehicleDriveModelData_Muscle.momentOfInertia', Vector3.new(4500, 1300, 4800))
    TweakDB:SetFlat('Vehicle.v_sport2_mizutani_shion_nomad_inline0.momentOfInertia', Vector3.new(3820, 800, 4080))
    TweakDB:SetFlat('Vehicle.v_sport2_quadra_type66_nomad_inline0.momentOfInertia', Vector3.new(4500, 1210, 4500))
    TweakDB:SetFlat('Vehicle.v_standard25_thorton_colby_nomad_inline5.momentOfInertia', Vector3.new(3000, 700, 3790))
    TweakDB:SetFlat('Vehicle.v_standard2_thorton_galena_gt_camber_inline3.momentOfInertia', Vector3.new(1570, 460, 1780))

    TweakDB:SetFlat('vehicles.showDebugUi', true)

    TweakDB:SetFlat('Vehicle.Voight.enumComment', '')
    TweakDB:SetFlat('Vehicle.Voight.enumName', 'Voight')

	-- GameHUD.Initialize()

	Observe('FlightControl', 'Activate', function(self)
		Game.GetPlayer().FlightControlInstance:StartSnd()
	end)

	Observe('FlightControl', 'OnUpdate', function(self)
        local as = Game.GetPlayer().FlightControlInstance.audioStats
		Game.GetPlayer().FlightControlInstance:SetParams(as.volume, as.playerPosition, as.playerUp, as.playerForward, as.cameraPosition, as.cameraUp, as.cameraForward, as.speed, as.surge, as.yawDiff, as.lift, as.yaw, as.pitchDiff, as.brake)
	end)

	Observe('FlightControl', 'Deactivate', function(self)
		Game.GetPlayer().FlightControlInstance:StopSnd()
	end)

end)

-- Camera.VehicleTPP_DefaultParams.airFlowDistortion,Bool,true 
-- Camera.VehicleTPP_DefaultParams.autoCenterMaxSpeedThreshold,Float,20 
-- Camera.VehicleTPP_DefaultParams.autoCenterSpeed,Float,5 
-- Camera.VehicleTPP_DefaultParams.autoCenterStartTimeGamepad,Float,0.5 
-- Camera.VehicleTPP_DefaultParams.autoCenterStartTimeMouse,Float,2 
-- Camera.VehicleTPP_DefaultParams.cameraBoomExtensionSpeed,Float,3 
-- Camera.VehicleTPP_DefaultParams.cameraMaxPitch,Float,80 
-- Camera.VehicleTPP_DefaultParams.cameraMinPitch,Float,-35 
-- Camera.VehicleTPP_DefaultParams.cameraSphereRadius,Float,1 
-- Camera.VehicleTPP_DefaultParams.collisionDetection,Bool,true 
-- Camera.VehicleTPP_DefaultParams.drivingDirectionCompensation,Bool,true 
-- Camera.VehicleTPP_DefaultParams.drivingDirectionCompensationAngle,Float,100 
-- Camera.VehicleTPP_DefaultParams.drivingDirectionCompensationAngleSmooth,Float,70 
-- Camera.VehicleTPP_DefaultParams.drivingDirectionCompensationAngularVelocityMin,Float,150 
-- Camera.VehicleTPP_DefaultParams.drivingDirectionCompensationSpeedCoef,Float,1 
-- Camera.VehicleTPP_DefaultParams.drivingDirectionCompensationSpeedMax,Float,120 
-- Camera.VehicleTPP_DefaultParams.drivingDirectionCompensationSpeedMin,Float,4 
-- Camera.VehicleTPP_DefaultParams.elasticBoomAcceleration,Bool,true 
-- Camera.VehicleTPP_DefaultParams.elasticBoomAccelerationExpansionLength,Float,0.5 
-- Camera.VehicleTPP_DefaultParams.elasticBoomForwardAccelerationCoef,Float,10 
-- Camera.VehicleTPP_DefaultParams.elasticBoomSpeedExpansionLength,Float,0.5 
-- Camera.VehicleTPP_DefaultParams.elasticBoomSpeedExpansionSpeedMax,Float,20 
-- Camera.VehicleTPP_DefaultParams.elasticBoomSpeedExpansionSpeedMin,Float,10 
-- Camera.VehicleTPP_DefaultParams.elasticBoomVelocity,Bool,true 
-- Camera.VehicleTPP_DefaultParams.fov,Float,69 
-- Camera.VehicleTPP_DefaultParams.headLookAtCenterYawThreshold,Float,100 
-- Camera.VehicleTPP_DefaultParams.headLookAtMaxPitchDown,Float,40 
-- Camera.VehicleTPP_DefaultParams.headLookAtMaxPitchUp,Float,0 
-- Camera.VehicleTPP_DefaultParams.headLookAtMaxYaw,Float,70 
-- Camera.VehicleTPP_DefaultParams.headLookAtRotationSpeed,Float,0.800000012 
-- Camera.VehicleTPP_DefaultParams.lockedCamera,Bool,false 
-- Camera.VehicleTPP_DefaultParams.slopeAdjustement,Bool,true 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirDampFactor,Float,0.000000001 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirFallCoef,Float,2 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirPitchMax,Float,30 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirPitchMin,Float,-30 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirRaiseCoef,Float,0.0 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirSpeedMax,Float,10 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirStrength,Float,4
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionOnGroundPitchMax,Float,30 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionOnGroundPitchMin,Float,-30 
-- Camera.VehicleTPP_DefaultParams.slopeCorrectionOnGroundStrength,Float,4 