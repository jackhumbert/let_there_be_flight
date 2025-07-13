// FRONT

public class FlightThrusterFront extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_front";

    let frontPreset = TweakDBInterface.GetVehicleRecord(vehicle.GetRecordID()).VehWheelDimensionsSetup().FrontPreset().GetID();
    FlightLog.Info("Looking for front preset " + TDBID.ToStringDEBUG(frontPreset));
    TDBID.Append(frontPreset, t".thrusterSlotOrientation");
    FlightLog.Info("Looking for thrusterSlotOrientation " + TDBID.ToStringDEBUG(frontPreset));
    let orientation = TweakDBInterface.GetEulerAngles(frontPreset, new EulerAngles());

    // orientation needs to be inverse of suspension_front_offset
    this.vehicle.AddSlot(n"suspension_front_offset", this.slotName, new Vector3(0.0, 0.0, -0.5), EulerAngles.ToQuat(orientation));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterF"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = true;
    this.parentSlotName = n"wheel_front_spring";
    this.radiusName = n"None";
    this.deviationName = n"None";
    return this;
  }
  
  public func SetOGComponents() {
    let comp: ref<IComponent>;
    ArrayClear(this.ogComponents);

    comp = this.flightComponent.FindComponentByName(n"axel_f_01");
    if IsDefined(comp) {
      ArrayPush(this.ogComponents, comp);
    }
    let comps = this.vehicle.GetComponentsUsingSlot(n"wheel_front_rot_set");
    for c in comps {
      if !ArrayContains(this.ogComponents, c) {
        ArrayPush(this.ogComponents, c);
      }
    }
    comp = this.flightComponent.FindComponentByName(n"wheel_f_01");
    if IsDefined(comp) {
      if !ArrayContains(this.ogComponents, comp) {
        ArrayPush(this.ogComponents, comp);
      }
    }
    comp = this.flightComponent.FindComponentByName(n"tire_f_01");
    if IsDefined(comp) {
      if !ArrayContains(this.ogComponents, comp) {
        ArrayPush(this.ogComponents, comp);
      }
    }
  }

  public func GetPitch() -> Float {
    let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Right());
    if angle < (-this.maxThrusterAnglePitch - this.thrusterAngleAllowance) || angle > (this.maxThrusterAnglePitch + this.thrusterAngleAllowance) {
      angle = 0.0;
    }
    angle *= (1.0 - AbsF(this.torque.Y) * 0.5);
    return -ClampF(angle, -this.maxThrusterAnglePitch, this.maxThrusterAnglePitch);
  }

  public func GetYaw() -> Float {
    if this.flightComponent.active {
      let outside: Float;
      let inside: Float;
      outside = this.maxThrusterAngleOutside;
      inside = this.maxThrusterAngleOutside;
      let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Forward());
      if angle < (-inside - this.thrusterAngleAllowance) || angle > (outside + this.thrusterAngleAllowance) {
        angle = 0.0;
      }
      return this.torque.Z * 30.0 + ClampF(angle, -inside, outside);
    } else {
      return 180.0;
    }
  }
}

// BACK

public class FlightThrusterBack extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back";

    let backPreset = TweakDBInterface.GetVehicleRecord(vehicle.GetRecordID()).VehWheelDimensionsSetup().BackPreset().GetID();
    FlightLog.Info("Looking for front preset " + TDBID.ToStringDEBUG(backPreset));
    TDBID.Append(backPreset, t".thrusterSlotOrientation");
    FlightLog.Info("Looking for thrusterSlotOrientation " + TDBID.ToStringDEBUG(backPreset));
    let orientation = TweakDBInterface.GetEulerAngles(backPreset, new EulerAngles());

    this.vehicle.AddSlot(n"suspension_back", this.slotName, new Vector3(0.0, 0.0, -0.5), EulerAngles.ToQuat(orientation));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterB"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.parentSlotName = n"axel_back";
    this.radiusName = n"None";
    this.deviationName = n"None";
    return this;
  }
  
  public func SetOGComponents() {
    let comp: ref<IComponent>;
    ArrayClear(this.ogComponents);

    comp = this.flightComponent.FindComponentByName(n"wheel_b_01");
    if IsDefined(comp) {
      ArrayPush(this.ogComponents, comp);
    }
    comp = this.flightComponent.FindComponentByName(n"tire_b_01");
    if IsDefined(comp) {
      if !ArrayContains(this.ogComponents, comp) {
        ArrayPush(this.ogComponents, comp);
      }
    }
    let comps = this.vehicle.GetComponentsUsingSlot(n"axel_back_wheel");
    for c in comps {
      if !ArrayContains(this.ogComponents, c) {
        ArrayPush(this.ogComponents, c);
      }
    }
    comps = this.vehicle.GetComponentsUsingSlot(n"wheel_back");
    for c in comps {
      if !ArrayContains(this.ogComponents, c) {
        ArrayPush(this.ogComponents, c);
      }
    }
  }

  public func GetPitch() -> Float {
    let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Right());
    if angle < (-this.maxThrusterAnglePitch - this.thrusterAngleAllowance) || angle > (this.maxThrusterAnglePitch + this.thrusterAngleAllowance) {
      angle = 0.0;
    }
    angle *= (1.0 - AbsF(this.torque.Y) * 0.5);
    return -ClampF(angle, -this.maxThrusterAnglePitch, this.maxThrusterAnglePitch);
  }

  public func GetYaw() -> Float {
    if this.flightComponent.active {
      let outside: Float;
      let inside: Float;
      outside = this.maxThrusterAngleOutside;
      inside = this.maxThrusterAngleOutside;
      let angle = Vector4.GetAngleDegAroundAxis(FlightUtils.Up(), this.force, FlightUtils.Forward());
      if angle < (-inside - this.thrusterAngleAllowance) || angle > (outside + this.thrusterAngleAllowance) {
        angle = 0.0;
      }
      return this.torque.Z * -30.0 - ClampF(angle, -inside, outside);
    } else {
      return 180.0;
    }
  }
}