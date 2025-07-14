public class FlightModeHoverFly extends FlightModeStandard {
  protected let hovering: Float;
  protected let referenceZ: Float;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Flight-Mode-Settings")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Hover-And-Fly-Enabled")
  public let enabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Hover-And-Fly-Height-Dampening")
  @runtimeProperty("ModSettings.step", "0.25")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "100.0")
  public let heightDampening: Float = 1.0;

  @runtimeProperty("ModSettings.mod", "Let There Be Flight")
  @runtimeProperty("ModSettings.category", "UI-Settings-Standard-Mode")
  @runtimeProperty("ModSettings.displayName", "UI-Settings-Hover-And-Fly-Height-Correction")
  @runtimeProperty("ModSettings.step", "0.01")
  @runtimeProperty("ModSettings.min", "0.0")
  @runtimeProperty("ModSettings.max", "5.0")
  public let heightCorrectionFactor: Float = 0.5;

  public static func Create(component: ref<FlightComponent>) -> ref<FlightModeHoverFly> {
    let self = new FlightModeHoverFly();
    self.Initialize(component);
    self.hovering = 1.0;
    return self;
  }

  public func Activate() -> Void {
    let normal: Vector4;
    this.referenceZ = this.component.stats.d_position.Z;
    this.component.FindGround(normal);
    this.component.hoverHeight = MaxF(this.component.distance, FlightSettings.GetFloat("hoverModeMinHoverHeight"));
  }
  
  public func GetDescription() -> String = "Hover & Fly";

  public func Update(timeDelta: Float) -> Void {
    let lastHovering = this.hovering;
    let normal: Vector4;
    let foundGround = this.component.FindGround(normal);
    if foundGround {
      this.hovering = ClampF(1.0 - (this.component.distance - FlightSettings.GetFloat("hoverModeMinHoverHeight")) / (FlightSettings.GetFloat("hoverModeMaxHoverHeight") - FlightSettings.GetFloat("hoverModeMinHoverHeight")), 0.0, 1.0);
    } else {
      this.hovering = 0.0;
    }

    if lastHovering == 0.0 && this.hovering > 0.0 {
      this.component.hoverHeight = this.component.distance;
    }

    // this.component.hoverHeight = MaxF(this.component.hoverHeight + this.component.lift * timeDelta * this.standardModeHoverFactor, FlightSettings.GetFloat("hoverModeMinHoverHeight"));
    this.component.hoverHeight = MaxF(this.component.hoverHeight + this.component.lift * this.heightCorrectionFactor, FlightSettings.GetFloat("hoverModeMinHoverHeight"));
    // this.component.hoverHeight = MaxF(this.component.hoverHeight + this.component.lift, FlightSettings.GetFloat("hoverModeMinHoverHeight"));

    let heightDifference = this.component.hoverHeight - this.component.distance;
    let idealNormal = Vector4.Interpolate(FlightUtils.Up(), normal, this.hovering);

    let hoverCorrection = this.component.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, FlightSettings.GetFloat("hoverClamp"));// / FlightSettings.GetFloat("hoverClamp");
    let flyCorrection = this.component.lift;// * timeDelta;
    let liftFactor = LerpF(this.hovering, flyCorrection * this.standardModeLiftFactor - this.component.stats.d_velocity.Z * this.heightDampening, hoverCorrection * this.standardModeHoverFactor);
    // liftFactor -= this.component.stats.d_velocity.Z * this.heightDampening;
    liftFactor *= (1.0 - this.component.linearBrake);

    this.UpdateWithNormalLift(timeDelta, idealNormal, liftFactor + (9.81000042) * this.gravityFactor);
  }
}