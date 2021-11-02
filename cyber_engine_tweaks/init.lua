registerForEvent('onInit', function()

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

	Observe('FlightController', 'Activate', function(self)
		Game.GetPlayer().flightController:StartSnd()
	end)

	Observe('FlightController', 'OnUpdate', function(self)
		Game.GetPlayer().flightController:SetParams()
	end)

	Observe('FlightController', 'Deactivate', function(self)
		Game.GetPlayer().flightController:StopSnd()
	end)

end)