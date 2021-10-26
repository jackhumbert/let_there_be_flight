  // @replaceMethod(VehicleComponent)
  // protected final func EvaluateDoorReaction(doorID: CName) -> Void {
  //   let animFeature: ref<AnimFeature_PartData>;
  //   let animFeatureName: CName;
  //   let door: EVehicleDoor;
  //   let doorState: VehicleDoorState;
  //   let vehDataPackage: wref<VehicleDataPackage_Record>;
  //   VehicleComponent.GetVehicleDataPackage(this.GetVehicle().GetGame(), this.GetVehicle(), vehDataPackage);
  //   animFeature = new AnimFeature_PartData();
  //   animFeatureName = doorID;
  //   if !this.GetVehicleDoorEnum(door, doorID) {
  //     return;
  //   };
  //   doorState = (this.GetPS() as VehicleComponentPS).GetDoorState(door);
  //   if Equals(doorState, VehicleDoorState.Open) {
  //     animFeature.state = 1;
  //     animFeature.duration = vehDataPackage.Open_close_duration();
  //     AnimationControllerComponent.ApplyFeatureToReplicate(this.GetVehicle(), animFeatureName, animFeature);
  //     AnimationControllerComponent.PushEvent(this.GetVehicle(), this.GetAnimEventName(doorState, door));
  //   };
  //   if Equals(doorState, VehicleDoorState.Closed) {
  //     animFeature.state = 3;
  //     animFeature.duration = vehDataPackage.Open_close_duration();
  //     AnimationControllerComponent.ApplyFeatureToReplicate(this.GetVehicle(), animFeatureName, animFeature);
  //     AnimationControllerComponent.PushEvent(this.GetVehicle(), this.GetAnimEventName(doorState, door));
  //   };
  // }



  // (this.GetPS() as VehicleComponentPS).GetDoorState(EVehicleDoor.seat_front_left, VehicleDoorState.Detached)