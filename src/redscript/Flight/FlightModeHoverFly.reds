public class FlightModeHoverFly extends FlightModeStandard {
  protected let hovering: Float;
  // world height held above the hover band while there is no lift input
  protected let referenceZ: Float;
  protected let holdingAltitude: Bool;
  // m/s^2 per metre off referenceZ
  public let altitudeHoldFactor: Float = 1.0;

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
    this.holdingAltitude = false;
    this.component.hoverGroundPID.Reset();
    this.component.FindGround(normal);
    this.component.hoverHeight = ClampF(this.component.distance, FlightSettings.GetFloat("hoverModeMinHoverHeight"), FlightSettings.GetFloat("hoverModeMaxHoverHeight"));
  }

  public func Deactivate() {}

  public func GetDescription() -> String = "Hover & Fly";

  public func Update(timeDelta: Float) -> Void {
    let lastHovering = this.hovering;
    let normal: Vector4;
    let foundGround = this.component.FindGround(normal);
    if foundGround {
      this.hovering = ClampF(1.0 - (this.component.distance - FlightSettings.GetFloatN(n"hoverModeMinHoverHeight")) / (FlightSettings.GetFloatN(n"hoverModeMaxHoverHeight") - FlightSettings.GetFloatN(n"hoverModeMinHoverHeight")), 0.0, 1.0);
    } else {
      this.hovering = 0.0;
    }

    if lastHovering == 0.0 && this.hovering > 0.0 {
      this.component.hoverHeight = this.component.distance;
      // the PID did not run above the band, start it fresh at the new target
      this.component.hoverGroundPID.Reset();
    }

    // heightCorrectionFactor was tuned as metres per frame at 60 fps; kept inside the band so the
    // ground PID never chases a target the hover weighting has already faded out
    this.component.hoverHeight = ClampF(this.component.hoverHeight + this.component.lift * this.heightCorrectionFactor * timeDelta * 60.0, FlightSettings.GetFloatN(n"hoverModeMinHoverHeight"), FlightSettings.GetFloatN(n"hoverModeMaxHoverHeight"));

    let heightDifference = this.component.hoverHeight - this.component.distance;
    let idealNormal = Vector4.Interpolate(FlightUtils.Up(), normal, this.hovering);

    let hoverCorrection = 0.0;
    if this.hovering > 0.0 {
      hoverCorrection = this.component.hoverGroundPID.GetCorrectionClamped(heightDifference, timeDelta, FlightSettings.GetFloatN(n"hoverClamp"));// / FlightSettings.GetFloat("hoverClamp");
    }

    // Above the band there is no ground to hold, and damping vertical speed alone turns any
    // leftover vertical force into a steady climb or sink. Hold the height where the vehicle
    // would stop under that damping once lift is released.
    let z = this.component.stats.d_position.Z;
    let vz = this.component.stats.d_velocity.Z;
    if AbsF(this.component.lift) > 0.01 || this.hovering > 0.0 {
      this.holdingAltitude = false;
    } else {
      if !this.holdingAltitude {
        this.holdingAltitude = true;
        this.referenceZ = z;
        if this.heightDampening > 0.0 {
          this.referenceZ += vz / this.heightDampening;
        }
      }
    }

    let flyCorrection = this.component.lift * this.standardModeLiftFactor - vz * this.heightDampening;
    if this.holdingAltitude {
      flyCorrection += ClampF((this.referenceZ - z) * this.altitudeHoldFactor, -this.standardModeLiftFactor, this.standardModeLiftFactor);
    }
    let liftFactor = LerpF(this.hovering, flyCorrection, hoverCorrection * this.standardModeHoverFactor);
    // liftFactor -= this.component.stats.d_velocity.Z * this.heightDampening;
    liftFactor *= (1.0 - this.component.linearBrake);

    this.UpdateWithNormalLift(timeDelta, idealNormal, liftFactor + (9.81000042) * this.gravityFactor);
  }
}
