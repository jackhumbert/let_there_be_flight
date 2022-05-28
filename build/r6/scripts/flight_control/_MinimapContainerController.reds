@wrapMethod(MinimapContainerController)
protected final func InitializePlayer(playerPuppet: ref<GameObject>) -> Void {
  wrappedMethod(playerPuppet);
  FlightController.GetInstance().navPath.mmcc = this;
}

@addMethod(MinimapContainerController)
public native func GetQuestMappin() -> ref<QuestMappin>;

@addField(MinimapContainerController)
public native let questPoints: array<Vector4>;

@addMethod(MinimapContainerController)
public native func GetPOIMappin() -> ref<PointOfInterestMappin>;

@addField(MinimapContainerController)
public native let poiPoints: array<Vector4>;
