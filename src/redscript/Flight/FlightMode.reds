// Native (Flight/Mode.hpp): the base of the C++ modes too. A C++ mode runs through Activate, Deactivate, Update and
// GetDescription below; a mod extends one by overriding the hooks (OnActivate, OnDeactivate, OnUpdate, Description),
// which the C++ calls after its own step. The redscript modes override Activate and the rest directly, as before.
public abstract native class FlightMode extends IScriptable {
  protected let sys: ref<FlightSystem>;
  protected let component: ref<FlightComponent>;

  public native let force: Vector4;
  public native let torque: Vector4;
  public let linearDamp: Float;
  public let angularDamp: Float;

  public let gravityFactor: Float;

  // the mode controllers' maths in C++ (FlightComponent.nativeModes), created on first use
  protected let control: ref<FlightModeControl>;

  public let usesRightStickInput: Bool;
  public let collisionPenalty: Float;

  public let dampAccVector: Vector3;
  // public let lastAngularDamp: Vector4;

  // public let enabled: Bool;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    this.component = component;
    this.sys = component.sys;
    // this.gravityFactor = 2.885;
    this.gravityFactor = 1.0;
  }

  public func Deinitialize() -> Void;

  // Whether the mode joins the car's mode list (its Mod Settings switch); the list is TweakDB's FlightModes.classes
  public func IsEnabled() -> Bool = true;

  public func Activate() -> Void {
    this.NativeActivate();
  }
  public func Deactivate() -> Void {
    this.NativeDeactivate();
  }
  // the car was teleported (FlightComponent.OnTeleported): start over where it is now (the heights
  // and positions a mode holds are the old place's)
  public func Teleported() -> Void {
    this.Activate();
  }
  public func GetDescription() -> String = this.NativeDescription();

  public func Update(timeDelta: Float) -> Void {
    this.NativeUpdate(timeDelta);
  }

  // A C++ mode's steps, each followed by the hook below when the class overrides it (Flight/Mode.hpp)
  public final native func NativeActivate() -> Void;
  public final native func NativeDeactivate() -> Void;
  public final native func NativeUpdate(timeDelta: Float) -> Void;
  public final native func NativeDescription() -> String;

  // The mode list: TweakDB's FlightModes.classes, and a new instance of a FlightMode class by name (C++ or redscript;
  // null for a name that is not a concrete FlightMode class)
  public static native func ModeClasses() -> array<CName>;
  public static native func Instantiate(className: CName) -> ref<FlightMode>;

  // Hooks for a mod extending a C++ mode: called after its own step (force and torque already set by it), or in
  // place of its name (Description).
  protected func OnActivate() -> Void;
  protected func OnDeactivate() -> Void;
  protected func OnUpdate(timeDelta: Float) -> Void;
  protected func Description() -> String = "";

  // The native mode controller with this frame's controls, the brake factors and gravityFactor filled in; the mode adds
  // its own params and calls the maths.
  protected final func Control(timeDelta: Float) -> ref<FlightModeControl> {
    if !IsDefined(this.control) {
      this.control = new FlightModeControl();
    }
    let k = this.control;
    k.dt = timeDelta;
    k.surge = this.component.surge;
    k.lift = this.component.lift;
    k.sway = this.component.sway;
    k.yaw = this.component.yaw;
    k.roll = this.component.roll;
    k.pitch = this.component.pitch;
    k.linearBrake = this.component.linearBrake;
    k.angularBrake = this.component.angularBrake;
    k.brakeFactorLinear = FlightSettings.GetInstance().brakeFactorLinear;
    k.brakeFactorAngular = FlightSettings.GetInstance().brakeFactorAngular;
    k.gravityFactor = this.gravityFactor;
    return k;
  }

  // Limits one axis of the angular damping torque so it cannot remove more than
  // dampMaxPerFrame of that axis' angular velocity in a single frame.
  protected final func CapDamping(damp: Float, angularVelocity: Float, tensor: Float, inertia: Float) -> Float {
    if AbsF(angularVelocity) < 0.0001 || inertia <= 0.0 || tensor <= 0.0 {
      return damp;
    }
    let response = FlightSettings.GetFloatN(n"dampStabilityK") * this.component.stats.s_mass * tensor / inertia;
    if response <= 0.0 {
      return damp;
    }
    let maxGain = FlightSettings.GetFloatN(n"dampMaxPerFrame") / response;
    let gain = damp / angularVelocity;
    if gain > maxGain {
      return angularVelocity * maxGain;
    }
    return damp;
  }

  public func ApplyPhysics(timeDelta: Float) -> Void {
    let fs =  FlightSettings.GetInstance();
    
    let velocityDamp: Vector4; // = this.component.stats.d_speed * this.component.stats.d_localVelocity * fs.generalDampFactorLinear * this.component.stats.s_airResistanceFactor;
    // With PhysX flight physics the game's integrator is skipped (RigidBody_IntegrateMotion in
    // ChassisContacts.cpp), and with it the vanilla air resistance that gave every mode a top
    // speed; without it full surge accelerates forever. Same quadratic drag as
    // vehicle::WheeledPhysics::ApplyAirResistance (1.2 * factor * v^2), as an acceleration.
    if fs.physxFlightPhysics && this.component.stats.s_mass > 0.0 {
      velocityDamp = this.component.stats.d_localVelocity * this.component.stats.d_speed * 1.2 * this.component.stats.s_airResistanceFactor / this.component.stats.s_mass;
    }
    // let angularDamp: Vector4 = this.component.stats.d_angularVelocity * fs.generalDampFactorAngular;
    // Damp only the rotation that did not come from the world: with PhysX flight physics the
    // plugin measures how much spin contacts (ground, walls, impacts) gave the vehicle, so a
    // tip-over or crash tumbles physically while inputs and free flight stay damped.
    let spin = this.component.stats.d_angularVelocity - Quaternion.Conjugate(this.component.stats.d_orientation) * this.component.GetContactSpin();
    let angularDamp: Vector4 = spin * (fs.generalDampFactorAngular + this.component.angularBrake * fs.brakeFactorAngular * this.component.stats.s_brakingFrictionFactor);

    // angularDamp += this.component.stats.d_angularAcceleration;
    
    // let angularDamp = 0.0;

    // only damp if no input is being received on that axis
    // angularDamp += (1.0 - SqrtF(AbsF(this.component.pitch)));
    // angularDamp += (1.0 - SqrtF(AbsF(this.component.roll)));
    // angularDamp += (1.0 - SqrtF(AbsF(this.component.yaw)));
    angularDamp.X *= (1.0 - AbsF(this.component.pitch));
    angularDamp.Y *= (1.0 - AbsF(this.component.roll));
    angularDamp.Z *= (1.0 - AbsF(this.component.yaw));

    // extra resistance on yaw only, so turning can be settled without making pitch/roll stiff
    angularDamp.Z *= fs.generalYawDampFactor;

    // detect when we hit stuff and delay the damping
    // this.dampAccVector.X = ClampF(MaxF(this.dampAccVector.X, AbsF(this.component.stats.d_angularAcceleration.X) / timeDelta / 10.0), 0.0, 1.0);
    // this.dampAccVector.Y = ClampF(MaxF(this.dampAccVector.Y, AbsF(this.component.stats.d_angularAcceleration.Y) / timeDelta / 10.0), 0.0, 1.0);
    // this.dampAccVector.Z = ClampF(MaxF(this.dampAccVector.Z, AbsF(this.component.stats.d_angularAcceleration.Z) / timeDelta / 10.0), 0.0, 1.0);

    // angularDamp.X *= (1.0 - this.dampAccVector.X);
    // angularDamp.Y *= (1.0 - this.dampAccVector.Y);
    // angularDamp.Z *= (1.0 - this.dampAccVector.Z);

    // decay over 200 ms
    // this.dampAccVector.X -= timeDelta / 0.200;
    // this.dampAccVector.Y -= timeDelta / 0.200;
    // this.dampAccVector.Z -= timeDelta / 0.200;

    // Damping is a torque proportional to angular velocity, applied once per frame. The
    // engine removes roughly k = dampStabilityK * gain * mass * thrusterTensor / inertia of
    // the angular velocity per frame (calibrated 2026-09-30 on a Shion MZ1 from brake
    // telemetry; independent of frame time). k > 1 overshoots and k >= 2 diverges, which
    // was the front-to-back twitch when braking. Cap each axis at k = dampMaxPerFrame.
    angularDamp.X = this.CapDamping(angularDamp.X, spin.X, this.component.thrusterTensor.X, this.component.stats.s_momentOfInertia.X);
    angularDamp.Y = this.CapDamping(angularDamp.Y, spin.Y, MaxF(this.component.thrusterTensor.Y, 1.0), this.component.stats.s_momentOfInertia.Y);
    angularDamp.Z = this.CapDamping(angularDamp.Z, spin.Z, this.component.thrusterTensor.Z, this.component.stats.s_momentOfInertia.Z);

    // clamp the dampening
    if Vector4.Length(angularDamp) > fs.generalDampFactorAngularMax {
      angularDamp = Vector4.Normalize(angularDamp) * fs.generalDampFactorAngularMax;
    }

    // this.angularDamp = fs.generalDampFactorAngular * ClampF(angularDamp, 0.0, 1.0) + this.component.angularBrake * fs.brakeFactorAngular;
    this.linearDamp = fs.generalDampFactorLinear + this.component.linearBrake * fs.brakeFactorLinear;

    // this.lastAngularDamp = angularDamp;

    // let the world throw us around on collisions
    // angularDamp *= this.timeSinceLastCollision;
    // this.timeSinceLastCollision = MinF(this.timeSinceLastCollision + timeDelta, 1.0);

    let direction = this.component.stats.d_direction;
    if Vector4.Dot(this.component.stats.d_direction, this.component.stats.d_forward) < 0.0 {
      direction = -this.component.stats.d_direction;
    }
    let yawDirectionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_up);
    let pitchDirectionAngle: Float = Vector4.GetAngleDegAroundAxis(direction, this.component.stats.d_forward, this.component.stats.d_right);

    // let aeroDynamicYaw = this.component.aeroYawPID.GetCorrectionClamped(yawDirectionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;
    // let aeroDynamicPitch = this.component.pitchAeroPID.GetCorrectionClamped(pitchDirectionAngle, timeDelta, 10.0) * this.component.stats.d_speedRatio;// / 10.0;
    let aeroDynamicYaw = yawDirectionAngle * this.component.stats.d_speedRatio;// / 10.0;
    let aeroDynamicPitch = pitchDirectionAngle * this.component.stats.d_speedRatio;// / 10.0;

    let yawDirectionality: Float = (this.component.stats.d_speedRatio + AbsF(this.component.yaw) * fs.generalSwayWithYaw) * fs.generalYawDirectionalityFactor;
    let pitchDirectionality: Float = this.component.stats.d_speedRatio * fs.generalPitchDirectionalityFactor;
    let aeroFactor = Vector4.Dot(this.component.stats.d_forward, this.component.stats.d_direction);
    // yawDirectionality - redirect non-directional velocity to vehicle forward

    this.force = -velocityDamp;
    
    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_right)) * yawDirectionality * AbsF(aeroFactor);

    this.force += FlightUtils.Forward() * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_up)) * pitchDirectionality * aeroFactor;
    this.force += -this.component.stats.d_localDirection * AbsF(Vector4.Dot(this.component.stats.d_forward - this.component.stats.d_direction, this.component.stats.d_up)) * pitchDirectionality * AbsF(aeroFactor);

    this.torque = -angularDamp;
    this.torque.Z -= aeroDynamicYaw * fs.generalYawAeroFactor;
    this.torque.X -= aeroDynamicPitch * fs.generalPitchAeroFactor;
  }
}