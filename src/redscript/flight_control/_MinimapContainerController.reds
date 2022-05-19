@wrapMethod(MinimapContainerController)
protected final func InitializePlayer(playerPuppet: ref<GameObject>) -> Void {
  wrappedMethod(playerPuppet);
  FlightController.GetInstance().navPath.mmcc = this;
}

@addField(MinimapContainerController)
public native let questPoints: array<Vector4>;

@addField(MinimapContainerController)
public native let playerPoints: array<Vector4>;