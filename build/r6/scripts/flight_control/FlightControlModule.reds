// module FlightControl

// needs a ModuleManager

public class Force {
  public let position: Vector4;
  public let direction: Vector4;
  public static func Create(position: Vector4, direction: Vector4) -> ref<Force> {
    let instance = new Force();
    instance.position = position;
    instance.direction = direction;
    return instance;
  }
}

public class Torque extends Force {
  public let offset: Vector4;
  public static func Create(position: Vector4, offset: Vector4, direction: Vector4) -> ref<Torque> {
    let instance = new Torque();
    instance.position = position;
    instance.offset = offset;
    instance.direction = direction;
    return instance;
  }
} 

public abstract class BaseModule {
  protected let timeDelta: Float;
  protected let controller: ref<FlightController>;
  public func GetVehicle() -> ref<VehicleObject> {
    return this.controller.GetVehicle();
  } 
  public func SetController(controller: ref<FlightController>) {
    this.controller = controller;
  }
  protected func GetCenterOfMass() -> Vector4 {
    return this.GetVehicle().GetWorldPosition();
  }
  protected func GetForces() -> array<ref<Force>> {
    return [];
  }
  protected func GetTorques() -> array<ref<Torque>> {
    return [];
  }
  protected func ApplyForce(position: Vector4, direction: Vector4) {
    let impulseEvent: ref<PhysicalImpulseEvent> = new PhysicalImpulseEvent();
    impulseEvent.radius = 0.5;
    impulseEvent.worldPosition = Vector4.Vector4To3(position);
    impulseEvent.worldImpulse = Vector4.Vector4To3(direction * this.timeDelta);
    this.GetVehicle().QueueEvent(impulseEvent);
  }
  protected func ApplyTorque(torque: ref<Torque>) {
    this.ApplyForce(torque.position + torque.offset, torque.direction / 2.0);
    this.ApplyForce(torque.position - torque.offset, torque.direction / -2.0);
  }
  public func ApplyImpulses(timeDelta: Float) {
    this.timeDelta = timeDelta;
    for force in this.GetForces() {
      this.ApplyForce(force.position, force.direction);
    }
    for torque in this.GetTorques() {
      this.ApplyTorque(torque);
    }
  }
}

public class RollInputModule extends BaseModule {
  public static func Create(controller: ref<FlightController>) -> ref<RollInputModule> {
    let instance = new RollInputModule();
    instance.SetController(controller);
    return instance;
  }
  protected func GetTorques() -> array<ref<Torque>> {
    let items: array<ref<Torque>>;
    let i = new Torque();
    i.position = this.GetCenterOfMass();
    i.offset = Quaternion.GetRight(this.GetVehicle().GetWorldOrientation());
    i.direction = Quaternion.GetUp(this.GetVehicle().GetWorldOrientation());
    ArrayPush(items, i);
    return items;
  }
}

public class RollCorrectionModule extends BaseModule {
  public static func Create(controller: ref<FlightController>) -> ref<RollCorrectionModule> {
    let instance = new RollCorrectionModule();
    instance.SetController(controller);
    return instance;
  }
  protected func GetTorques() -> array<ref<Torque>> {
    let items: array<ref<Torque>>;
    let i = new Torque();
    i.position = this.GetCenterOfMass();
    i.offset = Quaternion.GetRight(this.GetVehicle().GetWorldOrientation());
    i.direction = Quaternion.GetUp(this.GetVehicle().GetWorldOrientation());
    ArrayPush(items, i);
    return items;
  }
}

public class LiftInputModule extends BaseModule {
  public static func Create(controller: ref<FlightController>) -> ref<LiftInputModule> {
    let instance = new LiftInputModule();
    instance.SetController(controller);
    return instance;
  }
  protected func GetForces() -> array<ref<Force>> {
    let items: array<ref<Force>>;
    let i = new Force();
    i.position = this.GetCenterOfMass();
    i.direction = new Vector4(0.0, 0.0, 1.0, 0.0);
    ArrayPush(items, i);
    return items;
  }
}

public class HoverModule extends BaseModule {
  public static func Create(controller: ref<FlightController>) -> ref<HoverModule> {
    let instance = new HoverModule();
    instance.SetController(controller);
    return instance;
  }
  protected func GetForces() -> array<ref<Force>> {
    let items: array<ref<Force>>;
    let i = new Force();
    i.position = this.GetCenterOfMass();
    i.direction = new Vector4(0.0, 0.0, 1.0, 0.0);
    ArrayPush(items, i);
    return items;
  }
}