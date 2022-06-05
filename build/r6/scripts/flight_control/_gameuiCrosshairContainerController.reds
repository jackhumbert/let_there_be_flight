// @addField(gameuiCrosshairContainerController)
// private let m_flightControllerActiveCallbackId: ref<CallbackHandle>;

// @addField(gameuiCrosshairContainerController)
// private let m_isFlying: Bool;

// @addField(gameuiCrosshairContainerController)
// public let flightControllerUI: ref<FlightControllerUI>;

// @wrapMethod(gameuiCrosshairContainerController)
// protected cb func OnInitialize() -> Bool {
//     wrappedMethod();
//     this.m_isFlying = false;
//     this.flightControllerUI = FlightControllerUI.Create(this, this.m_rootWidget);
// }

// @wrapMethod(gameuiCrosshairContainerController)
// protected cb func OnUninitialize() -> Bool {
//     wrappedMethod();
// }

// @wrapMethod(gameuiCrosshairContainerController)
// protected cb func OnMountChanged(mounted: Bool) -> Bool {
//     wrappedMethod(mounted);
//     if IsDefined(this.m_Player) && this.m_Player.IsControlledByLocalPeer() {
//         if mounted {
//             this.m_flightControllerActiveCallbackId = FlightController.GetInstance().GetBlackboard().RegisterListenerBool(GetAllBlackboardDefs().FlightControllerBB.ShouldShowUI, this, n"OnFlightActiveChanged");
//         } else {
//             FlightController.GetInstance().GetBlackboard().UnregisterListenerBool(GetAllBlackboardDefs().FlightControllerBB.ShouldShowUI, this.m_flightControllerActiveCallbackId);
//         }
//     }
// }

// @wrapMethod(gameuiCrosshairContainerController)
// private final func UpdateRootVisibility() -> Void {
//     //wrappedMethod();
//     this.GetRootWidget().SetVisible(!this.m_isUnarmed || !this.m_isMounted || this.m_isFlying);
// }

// @addMethod(gameuiCrosshairContainerController)
// protected cb func OnFlightActiveChanged(active: Bool) -> Bool {
//     this.m_isFlying = active;
//     this.UpdateRootVisibility();
// }