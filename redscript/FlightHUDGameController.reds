// public class FlightHUDGameController extends inkProjectedHUDGameController {

//   private let m_OnTargetHitCallback: ref<CallbackHandle>;

//   protected cb func OnInitialize() -> Bool {
//     LogChannel(n"DEBUG", "FlightHUDGameController init");
//     let blackboardSystem: ref<BlackboardSystem> = this.GetBlackboardSystem();
//     let blackboard: ref<IBlackboard> = blackboardSystem.Get(GetAllBlackboardDefs().Vehicle);
//     this.m_OnTargetHitCallback = blackboard.RegisterListenerBool(GetAllBlackboardDefs().Vehicle.FlightActive, this, n"OnFlightActivate");
//   }

//   protected cb func OnFlightActivate(active: Bool) -> Bool {
//     if active {
//         let screenProjectionData: inkScreenProjectionData;
//         let targetController: wref<FlightHUDLogicController> = this.SpawnFromLocal(this.GetRootWidget(), n"Target").GetController() as FlightHUDLogicController;
//         screenProjectionData.entity = FlightControl.GetInstance().stats.vehicle;
//         screenProjectionData.slotComponentName = n"UI_Slots";
//         screenProjectionData.slotName = n"FlightControl";
//         screenProjectionData.fixedWorldOffset = new Vector4(0.00, 0.00, 0.00, 0.00);
//         screenProjectionData.userData = targetController;
//         let projection: ref<inkScreenProjection> = this.RegisterScreenProjection(screenProjectionData);
//         targetController.SetProjection(projection);
//         targetController.RegisterToCallback(n"OnReadyToRemove", this, n"OnDeactivate");
//     }
//   }

//   protected cb func OnScreenProjectionUpdate(projections: ref<gameuiScreenProjectionsData>) -> Bool {
//     let controller: ref<FlightHUDLogicController>;
//     let projection: ref<inkScreenProjection>;
//     let count: Int32 = ArraySize(projections.data);
//     let i: Int32 = 0;
//     while i < count {
//       projection = projections.data[i];
//       controller = projection.GetUserData() as FlightHUDLogicController;
//       controller.UpdatewidgetPosition(projection);
//       i += 1;
//     };
//   }

//   protected cb func OnDeactivate(targetWidget: wref<inkWidget>) -> Bool {
//     let rootWidget: wref<inkCompoundWidget>;
//     let targetController: ref<FlightHUDLogicController> = targetWidget.GetController() as FlightHUDLogicController;
//     this.UnregisterScreenProjection(targetController.GetProjection());
//     rootWidget = this.GetRootWidget() as inkCompoundWidget;
//     rootWidget.RemoveChild(targetWidget);
//   }
// }

// // public abstract class FlightHUDEvent extends inkEvent {
// // 	protected let controller: ref<FlightHUDLogicController>;
// // 	public func GetController() -> ref<FlightHUDLogicController> {
// // 		return this.controller as FlightHUDLogicController;
// // 	}
// // }

// // public class ShowFlightHUDEvent extends FlightHUDEvent {
// // 	public static func Create(controller: ref<FlightHUDLogicController>) -> ref<ShowFlightHUDEvent> {
// // 		let event: ref<ShowFlightHUDEvent> = new ShowFlightHUDEvent();
// //         LogChannel(n"DEBUG", "creating ShowFlightHUDEvent");
// // 		event.controller = controller;

// // 		return event;
// // 	}
// // }

// // public class HideFlightHUDEvent extends FlightHUDEvent {
// // 	public static func Create(controller: ref<FlightHUDLogicController>) -> ref<HideFlightHUDEvent> {
// // 		let event: ref<HideFlightHUDEvent> = new HideFlightHUDEvent();
// // 		event.controller = controller;

// // 		return event;
// // 	}
// // }

// // public class FlightHUDNotificationData extends inkGameNotificationData {
// // 	public let controller: ref<FlightHUDLogicController>;
// // }

// // @wrapMethod(PopupsManager)
// // protected cb func OnPlayerAttach(playerPuppet: ref<GameObject>) -> Bool {
// // 	wrappedMethod(playerPuppet);
// //     FlightControl.GetInstancer().popupsManager = this;
// // }

// // @addMethod(PopupsManager)
// // protected cb func OnShowFlightHUD(evt: ref<ShowFlightHUDEvent>) -> Bool {
// //     let notificationData: ref<FlightHUDNotificationData> = new FlightHUDNotificationData();
// //     notificationData.controller = evt.GetController();
// //     notificationData.notificationName = n"base\\gameplay\\gui\\widgets\\notifications\\generic_fullscreen_message_notification.inkwidget";
// //     notificationData.queueName = n"modal_popup";
// //     notificationData.isBlocking = false;
// //     notificationData.useCursor = false;
// //     let notificationToken: ref<inkGameNotificationToken> = this.ShowGameNotification(notificationData);
// //     LogChannel(n"DEBUG", "OnShowFlightHUD");
// // }

// // @addMethod(PopupsManager)
// // protected cb func OnHideFlightHUD(evt: ref<HideFlightHUDEvent>) -> Bool {
// // 	evt.GetController().Detach();
// // }

