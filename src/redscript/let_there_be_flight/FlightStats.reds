public class FlightStats {
  public let vehicle: wref<VehicleObject>;
  public let s_record: wref<Vehicle_Record>;
  // public let s_fc_record: wref<FlightControl_Record>;
  public let s_driveModelData: wref<VehicleDriveModelData_Record>;
  public let s_engineData: wref<VehicleEngineData_Record>;
  public let s_wheelDimensions: wref<VehicleWheelDimensionsSetup_Record>;
  public let s_wheelDriving: wref<VehicleWheelDrivingSetup_Record>;
  public let s_mass: Float;
  public let s_centerOfMass: Vector3;
  public let s_momentOfInertia: Vector4;
  public let s_forwardWeightTransferFactor: Float;
  public let s_brakingFrictionFactor: Float;
  public let s_airResistanceFactor: Float;

  public let d_position: Vector4;
  public let d_visualPosition: Vector4;
  public let d_orientation: Quaternion;
  public let d_lastOrientation: Quaternion;
  public let d_forward: Vector4;
  public let d_right: Vector4;
  public let d_up: Vector4;
  public let d_localUp: Vector4;
  public let d_forward2D: Vector4;
  public let d_orientationChange: Quaternion;
  public let d_angularVelocity: Vector4;
  public let d_velocity: Vector4;
  public let d_localVelocity: Vector4;
  public let d_velocity2D: Vector4;
  public let d_speed: Float;
  public let d_speedRatio: Float;
  public let d_speedRatioSquared: Float;
  public let d_speed2D: Float;
  public let d_direction: Vector4;
  public let d_localDirection: Vector4;
  public let d_direction2D: Vector4;
  public let d_localDirection2D: Vector4;

  private let reset: Bool;
  private let ipp: ref<IPositionProvider>;
  private let iop: ref<IOrientationProvider>;

  public static func Create(vehicle: wref<VehicleObject>) -> ref<FlightStats> {
    let self: ref<FlightStats> = new FlightStats();
    self.vehicle = vehicle;
    self.vehicle.chassis = self.vehicle.FindComponentByName(n"Chassis") as vehicleChassisComponent;
    self.reset = false;
    self.UpdateStatic();
    // self.d_position = self.vehicle.GetWorldPosition() + self.s_centerOfMass;
    self.ipp = IPositionProvider.CreateEntityPositionProvider(self.vehicle, self.s_centerOfMass);
    // self.ipp = IPositionProvider.CreatePlacedComponentPositionProvider(self.vehicle.chassis);//, Vector4.Vector4To3(-self.vehicle.chassis.GetInitialPosition()));
    self.ipp.CalculatePosition(self.d_position);
    // CreateMoveComponentVelocityProvider(this.player);
    // self.d_position = self.vehicle.chassis.GetLocalToWorld(); * self.s_centerOfMass;
    // self.d_position = self.vehicle.chassis.GetLocalToWorld() * new Vector4(0.0, 0.0, 0.0, 0.0);
    // self.d_position += self.s_centerOfMass;
    // self.d_orientation = Matrix.ToQuat(self.vehicle.chassis.GetLocalToWorld());
    self.d_orientation = self.vehicle.GetWorldOrientation();
    // self.d_orientation = self.vehicle.chassis.GetWorldOrientation();
    // self.d_position = Vector4.EmptyVector();
    return self;
  }

  public func Reset() -> Void {
    this.reset = true;
  }

  public final func UpdateStatic() -> Void {
    this.s_record = this.vehicle.GetRecord();
    // this.s_fcRecord = FlightStatsData.GetInstance(this.vehicle.GetGame()).Get(this.vehicle.GetRecordID());
    // this.s_fc_record = TweakDBInterface.GetFlightRecord(this.vehicle.GetRecordID());
    this.s_driveModelData = this.s_record.VehDriveModelData();  
    this.s_brakingFrictionFactor = this.s_driveModelData.BrakingFrictionFactor();
    this.s_airResistanceFactor = this.s_driveModelData.AirResistanceFactor();
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
      // this.s_playerMass = 60.0;
    // }
    // small sample size (2) for this value under limited circumstances
    // this.s_mass *= 1.396;
    // need to just dynamically calculate this instead of messing around with random values
    // this.s_centerOfMass = Cast(this.s_driveModelData.Center_of_mass_offset());
    this.s_centerOfMass = this.vehicle.GetCenterOfMass();

    // let weightedVector = (this.vehicle.chassis.GetLocalToWorld() * this.s_centerOfMass.position) * this.s_mass;

    // let cbl = this.vehicle.FindComponentByName(n"ColliderBL") as ColliderComponent;
    // if IsDefined(cbl) {
    //   this.s_mass += cbl.mass;
    //   weightedVector += (cbl.GetLocalToWorld() * cbl.comOffset.position) * cbl.mass;
    // }
    // let cbr = this.vehicle.FindComponentByName(n"ColliderBR") as ColliderComponent;
    // if IsDefined(cbr) {
    //   this.s_mass += cbr.mass;
    //   weightedVector += (cbr.GetLocalToWorld() * cbr.comOffset.position) * cbr.mass;
    // }
    // let cfl = this.vehicle.FindComponentByName(n"ColliderFL") as ColliderComponent;
    // if IsDefined(cfl) {
    //   this.s_mass += cfl.mass;
    //   weightedVector += (cfl.GetLocalToWorld() * cfl.comOffset.position) * cfl.mass;
    // }
    // let cfr = this.vehicle.FindComponentByName(n"ColliderFR") as ColliderComponent;
    // if IsDefined(cfr) {
    //   this.s_mass += cfr.mass;
    //   weightedVector += (cfr.GetLocalToWorld() * cfr.comOffset.position) * cfr.mass;
    // }
    // let cc = this.vehicle.FindComponentByName(n"ColliderC") as ColliderComponent;
    // if IsDefined(cc) {
    //   this.s_mass += cc.mass;
    //   weightedVector += (cc.GetLocalToWorld() * cc.comOffset.position) * cc.mass;
    // }
    // weightedVector /= this.s_mass;

    // this.s_centerOfMass.position = Matrix.GetInverted(this.vehicle.chassis.GetLocalToWorld()) * weightedVector;


    // let playerMass = 80.0;
    // this.s_centerOfMass = playerMass * (Matrix.GetInverted(this.vehicle.chassis.GetLocalToWorld()) * this.player.GetWorldPosition()) / (playerMass + this.s_mass);
    // this.s_centerOfMass += this.s_fcRecord.comOffset_position;
    // this.s_momentOfInertia = Cast(this.s_driveModelData.MomentOfInertia());
    // this.s_momentOfInertia = Cast(this.s_fcRecord.inertia);
    // this isn't defined for all vehicles, so throw some numbers in for now
    // would be nice to compute from meshes outside this and import those values in
    // if Vector4.IsXYZFloatZero(this.s_momentOfInertia) {
      // LogChannel(n"DEBUG", "[FlightStats] no MOI, setting to bad defaults");
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
    // let orientation = Matrix.ToQuat(this.vehicle.chassis.GetLocalToWorld());
    this.d_orientation = orientation;
    this.d_orientationChange = Quaternion.MulInverse(this.d_orientation, this.d_lastOrientation);
    this.d_angularVelocity = Quaternion.Conjugate(this.d_orientation) * Vector4.Vector3To4(this.vehicle.GetAngularVelocity());
    Quaternion.GetAxes(this.d_orientation, this.d_forward, this.d_right, this.d_up);
    this.d_forward2D = Vector4.Normalize2D(this.d_forward);
    // this.d_forward = Vector4.Normalize(Quaternion.GetForward(this.d_orientation));
    // this.d_right = Vector4.Normalize(Quaternion.GetRight(this.d_orientation));
    // this.d_up = Vector4.Normalize(Quaternion.GetUp(this.d_orientation));
    // this.d_forward = Quaternion.Transform(this.s_centerOfMass.orientation, this.d_forward);
    // this.d_right = Quaternion.Transform(this.s_centerOfMass.orientation, this.d_right);
    // this.d_up = Quaternion.Transform(this.s_centerOfMass.orientation, this.d_up);
    
    // let playerMass = 80.0;

    // this.s_centerOfMass.position = this.s_playerMass * (Matrix.GetInverted(this.vehicle.GetLocalToWorld()) * this.player.GetWorldPosition()) / (this.s_playerMass + this.s_mass);
    // this.s_centerOfMass.position = this.s_playerMass * (Matrix.GetInverted(this.vehicle.chassis.GetLocalToWorld()) * this.player.GetWorldPosition()) / (this.s_playerMass + this.s_mass);
    // this.s_centerOfMass /= this.s_centerOfMass.W;
    
    // GameInstance.GetSpatialQueriesSystem(FlightController.GetInstance().gameInstance).GetGeometryDescriptionSystem();
    let position: Vector4;
    // if IsDefined(this.vehicle.chassis) {
      // position = this.vehicle.GetWorldPosition() * this.s_mass;
      // position += this.player.GetWorldPosition() * this.s_playerMass;
      // position /= (this.s_mass + this.s_playerMass);
      // position /= position.W;
      // position = this.vehicle.chassis.GetLocalToWorld() * -this.vehicle.chassis.GetComOffset();
      // position = this.vehicle.chassis.GetLocalToWorld() * this.s_centerOfMass;
      // position = this.vehicle.GetLocalToWorld() * this.s_centerOfMass;
    // } else {
      // position = this.vehicle.GetWorldPosition() + this.s_centerOfMass;
    // }
    this.ipp.CalculatePosition(position);
    // position += Quaternion.Transform(this.d_orientation, Vector4.Vector3To4(this.s_centerOfMass));

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
    this.d_localVelocity = Quaternion.Conjugate(this.d_orientation) * this.d_velocity;
    this.d_localUp = Quaternion.Conjugate(this.d_orientation) * FlightUtils.Up();
    // this.d_localUp = this.d_orientation * this.d_up;

    this.d_speed = Vector4.Length(this.d_velocity);
    this.d_speedRatio = this.d_speed / 100.0;
    this.d_speedRatioSquared = this.d_speedRatio * this.d_speedRatio;
    this.d_speed2D = Vector4.Length2D(this.d_velocity);
    this.d_direction = Vector4.Normalize(this.d_velocity);
    this.d_localDirection = Vector4.Normalize(this.d_localVelocity);
    this.d_direction2D = Vector4.Normalize2D(this.d_velocity);
    this.d_localDirection2D = Vector4.Normalize2D(this.d_localVelocity);
    this.d_velocity2D = this.d_direction2D * this.d_speed2D;

    // let minS = 0.3;
    // let maxS = 0.7;
    // let factor = Vector4.Distance(position, this.d_position) / timeDelta / this.d_speed;
    // this.d_position = this.d_position * (minS + (maxS - minS) * factor) + position * (1.0 - minS - (maxS - minS) * factor);
    this.d_visualPosition = this.d_position;
    this.d_position =  position;
    // this.d_visualPosition = this.d_position - this.d_velocity * timeDelta;
  }
}