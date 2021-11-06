public native class vehicleTPPCameraComponent extends gameCameraComponent {

}

public native class gameCameraComponent extends BaseCameraComponent {

}

public native class BaseCameraComponent extends IPlacedComponent {
    native let fov: Float;
    native let zoom: Float;
    native let nearPlaneOverride: Float;
    native let farPlaneOverride: Float;
    native let motionBlurScale: Float;
}