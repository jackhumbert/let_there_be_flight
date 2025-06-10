public class FlightMalfunctionEffector extends Effector {
  public let m_owner: wref<VehicleObject>;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOn");
    this.m_owner = owner as VehicleObject;
    if IsDefined(this.m_owner) {
      this.m_owner.UnsetPhysicsStates();
      this.m_owner.EndActions();
      this.m_owner.m_flightComponent.Activate(true);
      this.m_owner.m_flightComponent.lift = 5.0;
    }
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] ActionOff");
    if IsDefined(this.m_owner) {
      this.m_owner.m_flightComponent.Deactivate(true);
    }
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    FlightLog.Info("[FlightMalfunctionEffector] Uninitialize");
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
    this.ActionOff(this.m_owner);
  }
}

public class DisableGravityEffector extends Effector {

}

public class FunhouseEffector extends Effector {
  public let m_owner: wref<GameObject>;

  protected func Initialize(record: TweakDBID, game: GameInstance, parentRecord: TweakDBID) -> Void {
    FlightLog.Info("[FunhouseEffector] Initialize");
    // this.m_quickhackLevel = TweakDBInterface.GetFloat(record + t".level", 1.00);
  }

  protected func ActionOn(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FunhouseEffector] ActionOn");
    this.m_owner = owner;
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.bouncy = true;
      vehicle.ignoreImpulses = false;
      vehicle.UnsetPhysicsStates();
      vehicle.EndActions();
      vehicle.m_flightComponent.FireVerticalImpulse(0);
    }
  }

  protected func ActionOff(owner: ref<GameObject>) -> Void {
    FlightLog.Info("[FunhouseEffector] ActionOff");
    let vehicle = owner as VehicleObject;
    if IsDefined(vehicle) {
      vehicle.bouncy = false;
    }
  }

  protected func Uninitialize(game: GameInstance) -> Void {
    if !IsDefined(this.m_owner) || !this.m_owner.IsAttached() {
      return;
    };
    this.ActionOff(this.m_owner);
  }
}