// Native (Flight/Mode.hpp): the base of the C++ modes too. A C++ mode runs through Initialize, IsEnabled, Activate,
// Deactivate, Teleported, Update and GetDescription below; a mod extends one by overriding the hooks (OnInitialize,
// Enabled, OnActivate, OnDeactivate, OnTeleported, OnUpdate, Description), which the C++ calls after its own step (or in
// place of its answer: Enabled, Description). The redscript modes override Activate and the rest directly, as before.
public abstract native class FlightMode extends IScriptable {
  protected let sys: ref<FlightSystem>;
  protected let component: ref<FlightComponent>;

  public native let force: Vector4;
  public native let torque: Vector4;
  public let linearDamp: Float; // written by the force tail (FlightComponent.NativeForceTail)

  public let gravityFactor: Float;

  // the mode controllers' maths in C++ (FlightModeControl, Drone's), created on first use
  protected let control: ref<FlightModeControl>;

  public let usesRightStickInput: Bool;
  public let collisionPenalty: Float;

  public func Initialize(component: ref<FlightComponent>) -> Void {
    this.component = component;
    this.sys = component.sys;
    this.gravityFactor = 1.0;
    this.NativeInitialize();
  }

  public func Deinitialize() -> Void;

  // Whether the mode joins the car's mode list (its Mod Settings switch); the list is TweakDB's FlightModes.classes
  public func IsEnabled() -> Bool = this.NativeIsEnabled();

  public func Activate() -> Void {
    this.NativeActivate();
  }
  public func Deactivate() -> Void {
    this.NativeDeactivate();
  }
  // the car was teleported (FlightComponent.OnTeleported): start over where it is now (the heights
  // and positions a mode holds are the old place's). By default the C++ calls this object's Activate (Flight/Mode.hpp).
  public func Teleported() -> Void {
    this.NativeTeleported();
  }
  public func GetDescription() -> String = this.NativeDescription();

  public func Update(timeDelta: Float) -> Void {
    this.NativeUpdate(timeDelta);
  }

  // A C++ mode's steps, each followed by the hook below when the class overrides it (Flight/Mode.hpp)
  public final native func NativeInitialize() -> Void;
  public final native func NativeIsEnabled() -> Bool;
  public final native func NativeActivate() -> Void;
  public final native func NativeDeactivate() -> Void;
  public final native func NativeTeleported() -> Void;
  public final native func NativeUpdate(timeDelta: Float) -> Void;
  public final native func NativeDescription() -> String;

  // The mode list: TweakDB's FlightModes.classes, and a new instance of a FlightMode class by name (C++ or redscript;
  // null for a name that is not a concrete FlightMode class)
  public static native func ModeClasses() -> array<CName>;
  public static native func Instantiate(className: CName) -> ref<FlightMode>;

  // Hooks for a mod extending a C++ mode: called after its own step (force and torque already set by it), or in
  // place of its answer (Enabled, Description).
  protected func OnInitialize() -> Void;
  protected func Enabled() -> Bool = true;
  protected func OnActivate() -> Void;
  protected func OnDeactivate() -> Void;
  protected func OnTeleported() -> Void;
  protected func OnUpdate(timeDelta: Float) -> Void;
  protected func Description() -> String = "";

  // The native mode controller with this frame's controls and the brake factors (Brake Strength applied) filled in; the
  // mode adds its own params and calls the maths.
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
    k.brakeFactorLinear = FlightSettings.GetInstance().LinearBrake();
    k.brakeFactorAngular = FlightSettings.GetInstance().AngularBrake();
    return k;
  }

}