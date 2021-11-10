native class vehicleTPPCameraComponent extends CameraComponent {
    public func Peak() -> Void {   
        FlightLog.Info("TPP Camera! " + ToString(this));
    }
}

@addField(entCameraComponent)
native let fov: Float;

@addField(entCameraComponent)
native let zoom: Float;

@addField(entCameraComponent)
native let nearPlaneOverride: Float;

@addField(entCameraComponent)
native let farPlaneOverride: Float;

@addField(entCameraComponent)
native let motionBlurScale: Float;

//FindVehicleCameraManager