// FRONT LEFT

public class FlightThrusterFL extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_front_left";
    this.vehicle.AddSlot(n"swingarm_front_left", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterFL"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = true;
    this.parentSlotName = n"wheel_front_left";
    this.radiusName = n"veh_rad_w_f_l";
    this.deviationName = n"veh_press_w_f_l";
    return this;
  }
}

// FRONT RIGHT

public class FlightThrusterFR extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_front_right";
    this.vehicle.AddSlot(n"swingarm_front_right", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterFR"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = true;
    this.isRight = true;
    this.parentSlotName = n"wheel_front_right";
    this.radiusName = n"veh_rad_w_f_r";
    this.deviationName = n"veh_press_w_f_r";
    return this;
  }
}

// BACK RIGHT

public class FlightThrusterBR extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back_right";
    this.vehicle.AddSlot(n"swingarm_back_right", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBR"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isRight = true;
    this.parentSlotName = n"wheel_back_right";
    this.radiusName = n"veh_rad_w_b_r";
    this.deviationName = n"veh_press_w_b_r";
    return this;
  }
}

// BACK LEFT

public class FlightThrusterBL extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back_left";
    this.vehicle.AddSlot(n"swingarm_back_left", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBL"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.parentSlotName = n"wheel_back_left";
    this.radiusName = n"veh_rad_w_b_l";
    this.deviationName = n"veh_press_w_b_l";
    return this;
  }
}

// FRONT LEFT B

public class FlightThrusterFLB extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_front_left_b";
    this.vehicle.AddSlot(n"swingarm_front_left_b", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterFLB"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = true;
    this.isB = true;
    this.parentSlotName = n"wheel_front_left_b";
    this.radiusName = n"veh_rad_w_1_l";
    this.deviationName = n"veh_press_w_1_l";
    return this;
  }
}

// FRONT RIGHT B

public class FlightThrusterFRB extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_front_right_b";
    this.vehicle.AddSlot(n"swingarm_front_right_b", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterFRB"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = true;
    this.isRight = true;
    this.isB = true;
    this.parentSlotName = n"wheel_front_right_b";
    this.radiusName = n"veh_rad_w_1_r";
    this.deviationName = n"veh_press_w_1_r";
    return this;
  }
}

// BACK LEFT B

public class FlightThrusterBLB extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back_left_b";
    this.vehicle.AddSlot(n"swingarm_back_left_b", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBLB"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = false;
    this.isB = true;
    this.parentSlotName = n"wheel_back_left_b";
    this.radiusName = n"veh_rad_w_1_l";
    this.deviationName = n"veh_press_w_1_l";
    return this;
  }
}

// BACK RIGHT B

public class FlightThrusterBRB extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back_right_b";
    this.vehicle.AddSlot(n"swingarm_back_right_b", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBRB"; 
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = false;
    this.isRight = true;
    this.isB = true;
    this.parentSlotName = n"wheel_back_right_b";
    this.radiusName = n"veh_rad_w_1_r";
    this.deviationName = n"veh_press_w_1_r";
    return this;
  }
}
// BACK LEFT C, BACK RIGHT C, BACK LEFT D, BACK RIGHT D: the third and fourth rear axles (the Bratsk U4020 extended);
// their suspension inputs are assumed to follow w_1 as w_2 and w_3 (an input the vehicle lacks does nothing)

public class FlightThrusterBLC extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back_left_c";
    this.vehicle.AddSlot(n"swingarm_back_left_c", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBLC";
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = false;
    this.isB = true;
    this.parentSlotName = n"wheel_back_left_c";
    this.radiusName = n"veh_rad_w_2_l";
    this.deviationName = n"veh_press_w_2_l";
    return this;
  }
}

public class FlightThrusterBRC extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back_right_c";
    this.vehicle.AddSlot(n"swingarm_back_right_c", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBRC";
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = false;
    this.isRight = true;
    this.isB = true;
    this.parentSlotName = n"wheel_back_right_c";
    this.radiusName = n"veh_rad_w_2_r";
    this.deviationName = n"veh_press_w_2_r";
    return this;
  }
}

public class FlightThrusterBLD extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back_left_d";
    this.vehicle.AddSlot(n"swingarm_back_left_d", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBLD";
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = false;
    this.isB = true;
    this.parentSlotName = n"wheel_back_left_d";
    this.radiusName = n"veh_rad_w_3_l";
    this.deviationName = n"veh_press_w_3_l";
    return this;
  }
}

public class FlightThrusterBRD extends IFlightThruster {
  public func Create(vehicle: ref<VehicleObject>, meshComponent: ref<MeshComponent>) -> ref<IFlightThruster> {
    this.vehicle = vehicle;
    this.slotName = n"thruster_back_right_d";
    this.vehicle.AddSlot(n"swingarm_back_right_d", this.slotName, new Vector3(0.0, 0.0, 0.0), new Quaternion(0.0, 0.0, 0.0, 1.0));

    this.meshComponent = meshComponent;
    this.meshComponent.name = n"ThrusterBRD";
    this.meshComponent.SetParentTransform(n"vehicle_slots", this.slotName);

    this.isFront = false;
    this.isRight = true;
    this.isB = true;
    this.parentSlotName = n"wheel_back_right_d";
    this.radiusName = n"veh_rad_w_3_r";
    this.deviationName = n"veh_press_w_3_r";
    return this;
  }
}
