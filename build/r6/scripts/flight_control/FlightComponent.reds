// public class FlightComponent extends VehicleComponent {

//   private final func OnGameAttach() -> Void {
//     super.OnGameAttach();
//     // LogChannel(n"DEBUG", "Game attached to FlightComponent for " + this.GetVehicle());
//   }

//   protected const func GetPS() -> ref<GameComponentPS> {
//     return this.GetBasePS();
//   }  

//   // private final func SetupThrusterFX() -> Void {
//     // let toggle: Bool = (this.GetPS() as FlightComponentPS).GetThrusterState();
//     // if toggle {
//     //   GameObjectEffectHelper.StartEffectEvent(this.GetVehicle(), n"thrusters", true);
//     // } else {
//     //   GameObjectEffectHelper.BreakEffectLoopEvent(this.GetVehicle(), n"thrusters");
//     // };
//   // }
// }

// public class FlightComponentPS extends VehicleComponentPS {
//   // public final func GetThrusterState() -> Bool {
//   //   return this.m_playerVehicle && FlightControl.GetInstance().GetThrusterState();
//   // }
// }

// // hook stuff up

// @addField(VehicleObject)
// private let m_flightComponent: wref<FlightComponent>;

// @wrapMethod(VehicleObject)
// protected cb func OnRequestComponents(ri: EntityRequestComponentsInterface) -> Bool {
//   EntityRequestComponentsInterface.RequestComponent(ri, n"flight", n"FlightComponent", true);
//   wrappedMethod(ri);
// }

// @wrapMethod(VehicleObject)
// protected cb func OnTakeControl(ri: EntityResolveComponentsInterface) -> Bool {
//   this.m_flightComponent = EntityResolveComponentsInterface.GetComponent(ri, n"flight") as FlightComponent;
//   wrappedMethod(ri);
// }

// @addMethod(VehicleObject)
// public const func GetFlightComponent() -> ref<FlightComponent> {
//   return this.m_flightComponent as FlightComponent;
// }

// @wrapMethod(VehicleObject)
// public const func GetVehicleComponent() -> ref<VehicleComponent> {
//   return this.m_flightComponent as VehicleComponent;
// }

// // other ideas

// // public native class FlyingObject extends VehicleObject {

// // }

// // public class AVComponent extends VehicleComponent {
// // }

// // public class CarComponent extends VehicleComponent {
// // }
