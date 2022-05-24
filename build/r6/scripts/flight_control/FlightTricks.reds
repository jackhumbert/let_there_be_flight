public abstract class FlightTrick
{
  public let suspendMode: Bool;
  public let force: Vector4;
  public let torque: Vector4;
  private let time: Float;
  private let component: ref<FlightComponent>;
  public func Initialize(component: ref<FlightComponent>, suspend: Bool) -> Void {
    this.component = component;
    this.suspendMode = suspend;
  }
  // returns true if finished
  public func Update(timeDelta: Float) -> Bool {
    this.time += timeDelta;
    return false;
  }
}

public class FlightTrickAileronRoll extends FlightTrick {
  private let direction: Float;
  public static func Create(component: ref<FlightComponent>, direction: Float) -> ref<FlightTrickAileronRoll> {
    let self = new FlightTrickAileronRoll();
    self.Initialize(component, true);
    self.direction = direction;
    return self;
  }
  public func Update(timeDelta: Float) -> Bool {
    if this.time < 0.3 {
      this.force = this.component.stats.d_localUp * 9.81000042;
      if this.time < 0.3 {
        this.torque.Y = 75.0 * this.direction; 
      }
      return super.Update(timeDelta);
    } else {
      return true;
    }
  }
}