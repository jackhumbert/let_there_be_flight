public class FlightStats {
  public let vehicle: wref<VehicleObject>;
  public let s_record: wref<Vehicle_Record>;
  // public let s_fcRecord: wref<FlightStatsData_Record>;
  public let s_fc_record: wref<FlightControl_Record>;
  public let s_driveModelData: wref<VehicleDriveModelData_Record>;
  public let s_engineData: wref<VehicleEngineData_Record>;
  public let s_wheelDimensions: wref<VehicleWheelDimensionsSetup_Record>;
  public let s_wheelDriving: wref<VehicleWheelDrivingSetup_Record>;
  public let s_mass: Float;
  public let s_centerOfMassOffset: Vector4;
  public let s_momentOfInertia: Vector4;
  public let s_forwardWeightTransferFactor: Float;

  public let d_position: Vector4;
  public let d_visualPosition: Vector4;
  public let d_orientation: Quaternion;
  public let d_lastOrientation: Quaternion;
  public let d_forward: Vector4;
  public let d_right: Vector4;
  public let d_up: Vector4;
  public let d_angularVelocity: Quaternion;
  public let d_velocity: Vector4;
  public let d_speed: Float;
  public let d_speedRatio: Float;
  public let d_speedRatioSquared: Float;
  public let d_speed2D: Float;
  public let d_direction: Vector4;
  public let d_direction2D: Vector4;

  private let reset: Bool;
  private let ipp: ref<IPositionProvider>;

  public static func Create(vehicle: wref<VehicleObject>) -> ref<FlightStats> {
    let self: ref<FlightStats> = new FlightStats();
    self.vehicle = vehicle;
    self.reset = false;
    self.UpdateStatic();
    // self.d_position = self.vehicle.GetWorldPosition() + self.s_centerOfMassOffset;
    self.ipp = IPositionProvider.CreateEntityPositionProvider(self.vehicle);
    self.ipp.CalculatePosition(self.d_position);
    self.d_position += self.s_centerOfMassOffset;
    self.d_orientation = self.vehicle.GetWorldOrientation();
    // self.d_position = Vector4.EmptyVector();
    return self;
  }

  public func Reset() -> Void {
    this.reset = true;
  }

  public final func UpdateStatic() -> Void {
    this.s_record = this.vehicle.GetRecord();
    // this.s_fcRecord = FlightStatsData.GetInstance(this.vehicle.GetGame()).Get(this.vehicle.GetRecordID());
    this.s_fc_record = TweakDBInterface.GetFlightRecord(this.vehicle.GetRecordID());
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
    // this.s_mass = this.s_fcRecord.mass;
    // this.s_mass = this.s_fc_record.mass;
    // if this.s_mass == 0.0 {
      this.s_mass = this.vehicle.GetTotalMass(); // might be dynamic? pulled from otherPhysicsData->totalMass
    // }
    // small sample size (2) for this value under limited circumstances
    this.s_mass *= 1.396;
    // need to just dynamically calculate this instead of messing around with random values
    this.s_centerOfMassOffset = Cast(this.s_driveModelData.Center_of_mass_offset());
    // this.s_centerOfMassOffset += this.s_fcRecord.comOffset_position;
    // this.s_momentOfInertia = Cast(this.s_driveModelData.MomentOfInertia());
    // this.s_momentOfInertia = Cast(this.s_fcRecord.inertia);
    // this isn't defined for all vehicles, so throw some numbers in for now
    // would be nice to compute from meshes outside this and import those values in
    // if Vector4.IsXYZFloatZero(this.s_momentOfInertia) {
      // LogChannel(n"DEBUG", "[FlightStats] no MOI, setting to bad defaults");
    let it = this.vehicle.GetInteriaTensor();
    this.s_momentOfInertia.X = it.X.X;
    this.s_momentOfInertia.Y = it.Y.Y;
    this.s_momentOfInertia.Z = it.Z.Z;
    // 
    // this is to try to make smaller vehicles more responsive than larger ones
    // maybe the same could be done for mass?
    // this.s_momentOfInertia.X = SqrtF(this.s_momentOfInertia.X);
    // this.s_momentOfInertia.Y = SqrtF(this.s_momentOfInertia.Y);
    // this.s_momentOfInertia.Z = SqrtF(this.s_momentOfInertia.Z);
    // this.s_momentOfInertia /= 2000.0; // get to a workable range    
    // this torques the vehicile in some way upon acceleration - the details aren't currently known
    // it could also be tied to Vehicle.RPMValue - we could use vehicle.GetBlackboard().GetFloat(GetAllBlackboardDefs().Vehicle.RPMValue)
    this.s_forwardWeightTransferFactor = this.s_driveModelData.ForwardWeightTransferFactor();
  }
  
  public final func UpdateDynamic(timeDelta: Float) -> Void {
    let orientation = this.vehicle.GetWorldOrientation();
    this.d_angularVelocity = (orientation - this.d_orientation) / timeDelta;
    this.d_lastOrientation = this.d_orientation;
    this.d_orientation = orientation;
    this.d_forward = Vector4.Normalize(Quaternion.GetForward(this.d_orientation));
    this.d_right = Vector4.Normalize(Quaternion.GetRight(this.d_orientation));
    this.d_up = Vector4.Normalize(Quaternion.GetUp(this.d_orientation));
    
    
    // GameInstance.GetSpatialQueriesSystem(FlightController.GetInstance().gameInstance).GetGeometryDescriptionSystem();
    let position: Vector4;
    if IsDefined(this.vehicle.chassis) {
      position = this.vehicle.GetWorldPosition() + this.vehicle.chassis.GetLocalPosition();// + this.vehicle.chassis.GetComOffset();
    } else {
      position = this.vehicle.GetWorldPosition() + this.s_centerOfMassOffset;
    }
    // this.ipp.CalculatePosition(position);
    // position += this.s_centerOfMassOffset;

    // if Vector4.Length(position - this.d_position) / timeDelta <= this.d_speed * 1.1 {
    // if this.reset {
    //   this.reset = false;
    //   this.d_position = position;
    // } else {
      // try to smooth out the position some
      // this.d_position = 0.99999 * this.d_position + 0.00001 * position;
      // this.d_position = Vector4.Interpolate(this.d_position, position, 0.4);
      // this.d_position = position;
    // }

    this.d_velocity = this.vehicle.GetLinearVelocity();


    this.d_speed = Vector4.Length(this.d_velocity);
    this.d_speedRatio = this.d_speed / 100.0;
    this.d_speedRatioSquared = this.d_speedRatio * this.d_speedRatio;
    this.d_speed2D = Vector4.Length2D(this.d_velocity);
    this.d_direction = Vector4.Normalize(this.d_velocity);
    this.d_direction2D = Vector4.Normalize2D(this.d_velocity);

    // let minS = 0.3;
    // let maxS = 0.7;
    // let factor = Vector4.Distance(position, this.d_position) / timeDelta / this.d_speed;
    // this.d_position = this.d_position * (minS + (maxS - minS) * factor) + position * (1.0 - minS - (maxS - minS) * factor);
    this.d_position =  position;
    this.d_visualPosition = this.d_position - this.d_velocity * timeDelta;
  }
}