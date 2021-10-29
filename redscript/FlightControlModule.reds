// public class FlightControlModule extends HUDModule {
//   private let m_activeFlightControlModule: ref<FlightControlInstance>;


//   public final static func RequestRefreshQuickhackMenu(context: GameInstance, requester: EntityID) -> Void {
//     let self: ref<HUDManager> = GameInstance.GetScriptableSystemsContainer(context).Get(n"HUDManager") as HUDManager;
//     if IsDefined(self) {
//       if self.IsQuickHackPanelOpened() && self.GetCurrentTargetID() == requester {
//         QuickhackModule.SendRevealQuickhackMenu(self, requester, true);
//       };
//     };
//   }

//   public final static func RequestCloseQuickhackMenu(context: GameInstance, requester: EntityID) -> Void {
//     let self: ref<HUDManager> = GameInstance.GetScriptableSystemsContainer(context).Get(n"HUDManager") as HUDManager;
//     if IsDefined(self) {
//       QuickhackModule.SendRevealQuickhackMenu(self, requester, false);
//     };
//   }

//   private final static func SendRevealQuickhackMenu(hudManager: ref<HUDManager>, requester: EntityID, shouldOpen: Bool) -> Void {
//     let request: ref<RevealQuickhackMenu> = new RevealQuickhackMenu();
//     request.shouldOpenWheel = shouldOpen;
//     request.ownerID = requester;
//     hudManager.QueueRequest(request);
//   }

// }

// public class RevealQuickhackMenu extends c {

//   public let shouldOpenWheel: Bool;
// }

// public class FlightControlInstance extends ModuleInstance {
//   private let open: Bool;

//   private let process: Bool;

//   public final func ShouldOpen() -> Bool {
//     return this.open;
//   }

//   public final func ShouldProcess() -> Bool {
//     return this.process;
//   }

//   public final func SetContext(_open: Bool) -> Void {
//     this.process = true;
//     this.open = _open;
//   }
// }

// // @addField(HUDInstruction)
// // public let flightControlInstruction: ref<FlightControlInstance>;

// // @wrapMethod(HUDInstruction)
// // public final static func Construct(self: ref<HUDInstruction>, id: EntityID) -> Void {
// //   if !EntityID.IsDefined(id) {
// //     return;
// //   };
// //   wrappedMethod(self, id);
// //   self.flightControlInstruction = new FlightControlInstance();
// //   ModuleInstance.Construct(self.flightControlInstruction, id);
// // }

// // public class FlightControlChangedRequest extends ScriptableSystemRequest {

// //   public let playerTarget: EntityID;
// // }

// @addField(HUDManager)
// private let m_uiFlightHUDVisibleCallbackID: ref<CallbackHandle>;

// @addField(HUDManager)
// private let m_uiFlightHUDVisible: Bool;

// @addField(HUDManager)
// private let m_flightControlModule: ref<FlightControlModule>;

// // @addField(HUDManager)
// // private let m_flightControl: wref<FlightControl>;

// // @addMethod(HUDManager)
// // protected final func RegisterFlightControlToggleCallback() -> Void {
// //   let blackboard: ref<IBlackboard> = GameInstance.GetBlackboardSystem(this.GetGameInstance()).Get(GetAllBlackboardDefs().UI_NameplateData);
// //   if IsDefined(blackboard) && !IsDefined(this.m_nameplateCallbackID) {
// //     this.m_nameplateCallbackID = blackboard.RegisterListenerVariant(GetAllBlackboardDefs().UI_NameplateData.EntityID, this, n"OnFlightControlHUDChanged");
// //   };
// // }

// // @addMethod(HUDManager)
// // protected cb func OnFlightControlHUDChanged(value: Variant) -> Bool {
// //   let request: ref<FlightControlChangedRequest> = new FlightControlChangedRequest();
// //   request.playerTarget = FromVariant(value);
// //   this.QueueRequest(request);
// // }

// @wrapMethod(HUDManager)
// private final func InitializeModules() -> Void {
//   wrappedMethod();
//   this.m_flightControlModule = new FlightControlModule();
//   this.m_flightControlModule.InitializeModule(this, ModuleState.DISABLED);
//   ArrayPush(this.m_modulesArray, this.m_flightControlModule);
// }

// @addMethod(HUDManager)
// private final func OnRevealFlightControl(request: ref<RevealQuickhackMenu>) -> Void {
//   if request.shouldOpenWheel {
//     this.GetCurrentTarget().SetShouldRefreshQHack(true);
//     this.RefreshHudForSingleActor(this.GetCurrentTarget());
//   } else {
//     this.CloseQHackMenu();
//   };
// }