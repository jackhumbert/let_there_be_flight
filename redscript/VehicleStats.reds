public class VehicleStats {
  public let vehicle: ref<VehicleObject>;
  public let s_record: ref<Vehicle_Record>;
  public let s_driveModelData: wref<VehicleDriveModelData_Record>;
  public let s_engineData: ref<VehicleEngineData_Record>;
  public let s_wheelDimensions: ref<VehicleWheelDimensionsSetup_Record>;
  public let s_wheelDriving: ref<VehicleWheelDrivingSetup_Record>;
  public let s_mass: Float;
  public let s_centerOfMassOffset: Vector4;
  public let s_momentOfInertia: Vector4;
  public let s_forwardWeightTransferFactor: Float;

  public let d_position: Vector4;
  public let d_orientation: Quaternion;
  public let d_forward: Vector4;
  public let d_right: Vector4;
  public let d_up: Vector4;
  public let d_velocity: Vector4;
  public let d_speed: Float;
  public let d_speedRatio: Float;
  public let d_speedRatioSquared: Float;
  public let d_speed2D: Float;
  public let d_direction: Vector4;
  public let d_direction2D: Vector4;

  public static func Create(vehicle: ref<VehicleObject>) -> ref<VehicleStats> {
    let instance: ref<VehicleStats> = new VehicleStats();
    instance.vehicle = vehicle;
    instance.UpdateStatic();
    return instance;
  }

  public final func UpdateStatic() -> Void {
    this.s_record = this.vehicle.GetRecord();
    this.s_driveModelData = this.s_record.VehDriveModelData();  
    // RollingResistanceFactor() -> Float;
    // HandbrakeBrakingTorque() -> Float;
    this.s_wheelDimensions = this.s_record.VehWheelDimensionsSetup();
    // .FrontPreset() & .BackPreset();
      // .TireRadius() -> Float;
      // .RimRadius() -> Float;
      // .TireWidth() -> Float;
      // .WheelOffset() -> Float;
    this.s_wheelDriving = this.s_driveModelData.WheelSetup();
    // .FrontPreset() & .BackPreset();
      // .MaxBrakingTorque() -> Float;
      // .Mass() -> Float;
    this.s_mass = this.vehicle.GetTotalMass(); // might be dynamic?
    this.s_centerOfMassOffset = Cast(this.s_driveModelData.Center_of_mass_offset());
    this.s_momentOfInertia = Cast(this.s_driveModelData.MomentOfInertia());
    // this isn't defined for all vehicles, so throw some numbers in for now
    // would be nice to compute from meshes outside this and import those values in
    if Vector4.IsXYZFloatZero(this.s_momentOfInertia) {
      this.s_momentOfInertia.X = this.s_mass * 1.75;
      this.s_momentOfInertia.Y = this.s_mass * 0.50;
      this.s_momentOfInertia.Z = this.s_mass * 2.00;
    }
    this.s_momentOfInertia /= 2000.0; // get to a workable range    
    // this torques the vehicile in some way upon acceleration - the details aren't currently known
    // it could also be tied to Vehicle.RPMValue - we could use vehicle.GetBlackboard().GetFloat(GetAllBlackboardDefs().Vehicle.RPMValue)
    this.s_forwardWeightTransferFactor = this.s_driveModelData.ForwardWeightTransferFactor();
  }
  
  public final func UpdateDynamic() -> Void {
    this.d_position = this.vehicle.GetWorldPosition() + this.s_centerOfMassOffset;
    this.d_orientation = this.vehicle.GetWorldOrientation();
    this.d_forward = Quaternion.GetForward(this.d_orientation);
    this.d_right = Quaternion.GetRight(this.d_orientation);
    this.d_up = Quaternion.GetUp(this.d_orientation);
    this.d_velocity = this.vehicle.GetLinearVelocity();
    this.d_speed = Vector4.Length(this.d_velocity);
    this.d_speedRatio = this.d_speed / 100.0;
    this.d_speedRatioSquared = this.d_speedRatio * this.d_speedRatio;
    this.d_speed2D = Vector4.Length2D(this.d_velocity);
    this.d_direction = Vector4.Normalize(this.d_velocity);
    this.d_direction2D = Vector4.Normalize2D(this.d_velocity);
  }
}