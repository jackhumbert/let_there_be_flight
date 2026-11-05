// Native so the plugin reads its fields (Flight/FlightStats.hpp); UpdateStatic stays script (TweakDB records at setup).
public native class FlightStats extends IScriptable {
  public let vehicle: wref<VehicleObject>;
  public let s_record: wref<Vehicle_Record>;
  public let s_driveModelData: wref<VehicleDriveModelData_Record>;
  public let s_engineData: wref<VehicleEngineData_Record>;
  public let s_wheelDimensions: wref<VehicleWheelDimensionsSetup_Record>;
  public let s_wheelDriving: wref<VehicleWheelDrivingSetup_Record>;
  public native let s_mass: Float;
  public native let s_centerOfMass: Vector3;
  public native let s_momentOfInertia: Vector4;
  public native let s_forwardWeightTransferFactor: Float;
  public native let s_brakingFrictionFactor: Float;
  public native let s_airResistanceFactor: Float;
  // measured by the configuration when flight starts (FlightStats.hpp): CdA per body axis, m2, and the thrusters
  public native let s_dragArea: Vector4;
  public native let s_thrusters: Float;

  public native let d_position: Vector4;
  public native let d_visualPosition: Vector4;
  public native let d_orientation: Quaternion;
  public native let d_lastOrientation: Quaternion;
  public native let d_forward: Vector4;
  public native let d_right: Vector4;
  public native let d_up: Vector4;
  public native let d_localUp: Vector4;
  public native let d_forward2D: Vector4;
  public native let d_orientationChange: Quaternion;
  public native let d_angularVelocity: Vector4;
  public native let d_angularAcceleration: Vector4;
  public native let d_velocity: Vector4;
  public native let d_localVelocity: Vector4;
  public native let d_velocity2D: Vector4;
  public native let d_speed: Float;
  public native let d_speedRatio: Float;
  public native let d_speedRatioSquared: Float;
  public native let d_speed2D: Float;
  public native let d_direction: Vector4;
  public native let d_localDirection: Vector4;
  public native let d_direction2D: Vector4;
  public native let d_localDirection2D: Vector4;
  public native let d_pitchDeg: Float;  // nose above the horizon, degrees (the HUD)
  public native let d_rollDeg: Float;  // roll about the forward axis, degrees (the HUD)

  // true when the plugin updates the dynamic half before FlightComponent.OnUpdate this frame
  public native let nativeUpdate: Bool;

  private let reset: Bool;
  private let ipp: ref<IPositionProvider>;
  private let iop: ref<IOrientationProvider>;

  public static func Create(vehicle: wref<VehicleObject>) -> ref<FlightStats> {
    let self: ref<FlightStats> = new FlightStats();
    self.vehicle = vehicle;
    self.reset = false;
    self.UpdateStatic();
    self.ipp = IPositionProvider.CreateEntityPositionProvider(self.vehicle, self.s_centerOfMass);
    self.ipp.CalculatePosition(self.d_position);
    self.d_orientation = self.vehicle.GetWorldOrientation();
    return self;
  }

  public func Reset() -> Void {
    this.reset = true;
  }

  public final func UpdateStatic() -> Void {
    this.s_record = this.vehicle.GetRecord();
    this.s_driveModelData = this.s_record.VehDriveModelData();
    this.s_brakingFrictionFactor = this.s_driveModelData.BrakingFrictionFactor();
    // bikes are 0.0
    if (this.s_brakingFrictionFactor < 0.001) {
      this.s_brakingFrictionFactor = 1.4;
    }
    this.s_airResistanceFactor = this.s_driveModelData.AirResistanceFactor();
    this.s_wheelDimensions = this.s_record.VehWheelDimensionsSetup();
    this.s_wheelDriving = this.s_driveModelData.WheelSetup();
    this.s_mass = this.vehicle.GetTotalMass(); // might be dynamic? pulled from otherPhysicsData->totalMass
    this.s_centerOfMass = this.vehicle.GetCenterOfMass();
    let it = this.vehicle.GetInertiaTensor();
    this.s_momentOfInertia.X = it.X.X;
    this.s_momentOfInertia.Y = it.Y.Y;
    this.s_momentOfInertia.Z = it.Z.Z;
    // this torques the vehicile in some way upon acceleration - the details aren't currently known
    // it could also be tied to Vehicle.RPMValue - we could use vehicle.GetBlackboard().GetFloat(GetAllBlackboardDefs().Vehicle.RPMValue)
    this.s_forwardWeightTransferFactor = this.s_driveModelData.ForwardWeightTransferFactor();
  }

  public final func UpdateDynamic() -> Void {
    this.d_lastOrientation = this.d_orientation;
    let orientation = this.vehicle.GetWorldOrientation();
    this.d_orientation = orientation;
    this.d_orientationChange = Quaternion.MulInverse(this.d_orientation, this.d_lastOrientation);

    let angularVelocity = Quaternion.Conjugate(this.d_orientation) * Vector4.Vector3To4(this.vehicle.GetAngularVelocity());
    this.d_angularAcceleration = (angularVelocity - this.d_angularVelocity);
    this.d_angularVelocity = angularVelocity;

    Quaternion.GetAxes(this.d_orientation, this.d_forward, this.d_right, this.d_up);
    this.d_forward2D = Vector4.Normalize2D(this.d_forward);
    let position: Vector4;
    this.ipp.CalculatePosition(position);
    this.d_velocity = this.vehicle.GetLinearVelocity();
    this.d_localVelocity = Quaternion.Conjugate(this.d_orientation) * this.d_velocity;
    this.d_localUp = Quaternion.Conjugate(this.d_orientation) * FlightUtils.Up();
    this.d_speed = Vector4.Length(this.d_velocity);
    this.d_speedRatio = this.d_speed / 100.0;
    this.d_speedRatioSquared = this.d_speedRatio * this.d_speedRatio;
    this.d_speed2D = Vector4.Length2D(this.d_velocity);
    this.d_direction = Vector4.Normalize(this.d_velocity);
    this.d_localDirection = Vector4.Normalize(this.d_localVelocity);
    this.d_direction2D = Vector4.Normalize2D(this.d_velocity);
    this.d_localDirection2D = Vector4.Normalize2D(this.d_localVelocity);
    this.d_velocity2D = this.d_direction2D * this.d_speed2D;
    this.d_visualPosition = this.d_position;
    this.d_position =  position;
    this.d_pitchDeg = 90.0 - Vector4.GetAngleBetween(this.d_forward, FlightUtils.Up());
    this.d_rollDeg = Vector4.GetAngleDegAroundAxis(this.d_localUp, FlightUtils.Up(), FlightUtils.Forward());
  }

  // what the script's own position provider gives, to compare with d_position (LtbfBridge.StatsDiff)
  public func ScriptPosition() -> Vector4 {
    let p: Vector4;
    this.ipp.CalculatePosition(p);
    return p;
  }

  // The local angular velocity as UpdateDynamic works it out from the vehicle now, without writing a field (the
  // bridge's StatsDiff compares d_angularVelocity with it). Read with the car parked or hovering still for an
  // exact comparison: in motion the values differ by up to a frame's motion, because the script values are read
  // later in the frame. With nativeStats off the d_ fields are the script's from earlier in the frame, so the
  // difference is near 0, not exactly 0.
  public func ScriptAngularVelocity() -> Vector4 {
    return Quaternion.Conjugate(this.vehicle.GetWorldOrientation()) * Vector4.Vector3To4(this.vehicle.GetAngularVelocity());
  }

  public func ScriptVelocity() -> Vector4 {
    return this.vehicle.GetLinearVelocity();
  }
}
