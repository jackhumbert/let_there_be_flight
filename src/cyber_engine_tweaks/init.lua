registerForEvent('onInit', function()

    -- TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirDampFactor', 0.1)
    -- TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirFallCoef', 0)
    -- TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirPitchMax', 30)
    -- TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirPitchMin', -30)
    -- TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirRaiseCoef', 0.0)
    -- TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirSpeedMax', 10)
    -- TweakDB:SetFlat('Camera.VehicleTPP_DefaultParams.slopeCorrectionInAirStrength', 4)
    
    -- TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirDampFactor', 0.1)
    -- TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirFallCoef', 0)
    -- TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirPitchMax', 30)
    -- TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirPitchMin', -30)
    -- TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirRaiseCoef', 0.0)
    -- TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirSpeedMax', 10)
    -- TweakDB:SetFlat('Camera.VehicleTPP_2w_DefaultParams.slopeCorrectionInAirStrength', 4)

    -- Observe("inkWidget", "CreateEffect", function(self)
    --     -- Create effect instance (it's Handle<ink::BoxBlurEffect>)
    --     local mask = inkMaskEffect.new()

    --     -- Set a unique name (used for control and animations)
    --     mask.effectName = CName('Mask_0')

    --     -- Fill effect properties (btw all prop can be animated)
    --     -- blur.samples = 3
    --     -- blur.intensity = 0.05
    --     -- blur.blurDimension = inkEBlurDimension.Horizontal

    --     -- Add effect to effects array
    --     self.effects = { mask }

    --     -- Activate effect
    --     self:SetEffectEnabled(inkEffectType.Mask, mask.effectName, true)
        
    -- end)

end)