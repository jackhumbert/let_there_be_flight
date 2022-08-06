public class FlightModeHover extends FlightModeStandard {

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "Flight Mode Settings")
  @runtimeProperty("ModSettings.displayName", "Hover Mode Enabled")
  public let enabled: Bool = false;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeHover> {
    let self = new FlightModeHover();
    self.Initialize(component);
    return self;
  }

  public func GetDescription() -> String = "Hover";

  public func Activate() -> Void {
    let normal: Vector4;
    this.component.FindGround(normal);
    this.component.hoverHeight = MaxF(this.component.distance, FlightSettings.GetFloat("hoverModeMinHoverHeight"));
  }
  
  public func Update(timeDelta: Float) -> Void {
    this.component.hoverHeight = MaxF(FlightSettings.GetFloat("hoverModeMinHoverHeight"), this.component.hoverHeight);

    // let findWater: TraceResult;
    let heightDifference = 0.0;
    let normal: Vector4;
    let idealNormal = FlightUtils.Up();

    // this.component.sqs.SyncRaycastByCollisionGroup(this.component.stats.d_position, this.component.stats.d_position - FlightSettings.GetFloat("lookDown"), n"Water", findWater, true, false);
    // if !TraceResult.IsValid(findWater) {
      if (this.component.FindGround(normal)) {
          heightDifference = this.component.hoverHeight - this.component.distance;
          idealNormal = normal;
      }
    // }

    this.UpdateWithNormalDistance(timeDelta, idealNormal, heightDifference);
  }
}