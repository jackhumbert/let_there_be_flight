public class VehicleStats {
  public let vehicle: wref<VehicleObject>;
  public let s_record: wref<Vehicle_Record>;
  public let s_fcRecord: wref<VehicleStatsData_Record>;
  public let s_driveModelData: wref<VehicleDriveModelData_Record>;
  public let s_engineData: wref<VehicleEngineData_Record>;
  public let s_wheelDimensions: wref<VehicleWheelDimensionsSetup_Record>;
  public let s_wheelDriving: wref<VehicleWheelDrivingSetup_Record>;
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

  private let reset: Bool;

  public static func Create(vehicle: wref<VehicleObject>) -> ref<VehicleStats> {
    let instance: ref<VehicleStats> = new VehicleStats();
    instance.vehicle = vehicle;
    instance.reset = false;
    instance.UpdateStatic();
    instance.d_position = instance.vehicle.GetWorldPosition() + instance.s_centerOfMassOffset;
    // instance.d_position = Vector4.EmptyVector();
    return instance;
  }

  public func Reset() -> Void {
    this.reset = true;
  }

  public final func UpdateStatic() -> Void {
    this.s_record = this.vehicle.GetRecord();
    this.s_fcRecord = VehicleStatsData.GetInstance(this.vehicle.GetGame()).Get(this.vehicle.GetRecordID());
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
    // this.s_mass = this.vehicle.GetTotalMass(); // might be dynamic?
    this.s_mass = this.s_fcRecord.mass;
    this.s_centerOfMassOffset = Cast(this.s_driveModelData.Center_of_mass_offset());
    this.s_centerOfMassOffset += this.s_fcRecord.comOffset_position;
    // this.s_momentOfInertia = Cast(this.s_driveModelData.MomentOfInertia());
    this.s_momentOfInertia = Cast(this.s_fcRecord.inertia);
    // this isn't defined for all vehicles, so throw some numbers in for now
    // would be nice to compute from meshes outside this and import those values in
    if Vector4.IsXYZFloatZero(this.s_momentOfInertia) {
      LogChannel(n"DEBUG", "[VehicleStats] no MOI, setting to bad defaults");
      this.s_momentOfInertia.X = this.s_mass * 1.75;
      this.s_momentOfInertia.Y = this.s_mass * 0.50;
      this.s_momentOfInertia.Z = this.s_mass * 2.00;
    }
    // this is to try to make smaller vehicles more responsive than larger ones
    // maybe the same could be done for mass?
    this.s_momentOfInertia.X = SqrtF(this.s_momentOfInertia.X);
    this.s_momentOfInertia.Y = SqrtF(this.s_momentOfInertia.Y);
    this.s_momentOfInertia.Z = SqrtF(this.s_momentOfInertia.Z);
    // this.s_momentOfInertia /= 2000.0; // get to a workable range    
    // this torques the vehicile in some way upon acceleration - the details aren't currently known
    // it could also be tied to Vehicle.RPMValue - we could use vehicle.GetBlackboard().GetFloat(GetAllBlackboardDefs().Vehicle.RPMValue)
    this.s_forwardWeightTransferFactor = this.s_driveModelData.ForwardWeightTransferFactor();
  }
  
  public final func UpdateDynamic(timeDelta: Float) -> Void {
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

    let position = this.vehicle.GetWorldPosition() + this.s_centerOfMassOffset;
    // if this.reset {
    //   this.reset = false;
    //   this.d_position = position;
    // } else {
      // try to smooth out the position some
      // this.d_position = 0.99999 * this.d_position + 0.00001 * position;
      // this.d_position = Vector4.Interpolate(this.d_position, position, 0.4);
      this.d_position = position;
    // }
  }
}